// nodes.rs - the node I/O helpers.  Every one of these mirrors the shipped
// open()/read()/atoi() sequence exactly, including the failure sentinels and
// the retry-the-secondary-node-on-parse-failure quirk.

use std::fs;
use std::io::{Read, Write};

use crate::constants::{K_IO_ERROR, K_READ_ERROR, K_WRITE_ERROR};

// atoi() semantics: leading whitespace, optional sign, digits, stop at the
// first non-digit.  An unparseable buffer yields 0, which is what C's atoi
// returns; the shipped code compares that against kParseError (INT32_MIN) and
// so never takes the parse-failure branch.
pub fn c_atoi(buf: &[u8]) -> i32 {
    let mut i = 0;
    while i < buf.len()
        && (buf[i] == b' ' || buf[i] == b'\t' || buf[i] == b'\n' || buf[i] == b'\r')
    {
        i += 1;
    }
    let mut neg = false;
    if i < buf.len() && (buf[i] == b'+' || buf[i] == b'-') {
        neg = buf[i] == b'-';
        i += 1;
    }
    let mut v: i64 = 0;
    while i < buf.len() && buf[i].is_ascii_digit() {
        v = v * 10 + (buf[i] - b'0') as i64;
        i += 1;
    }
    let v = if neg { -v } else { v };
    v.clamp(i32::MIN as i64, i32::MAX as i64) as i32
}

/// One open/read/atoi.  Err carries the shipped sentinel.
fn read_int_once(path: &str) -> Result<i32, i32> {
    let mut file = match fs::File::open(path) {
        Ok(f) => f,
        Err(_) => return Err(K_READ_ERROR),
    };
    let mut buf = [0u8; 16];
    let n = match file.read(&mut buf) {
        Ok(n) => n,
        Err(_) => return Err(K_IO_ERROR),
    };
    if n < 0 {
        return Err(K_IO_ERROR);
    }
    Ok(c_atoi(&buf[..n]))
}

/// readNodeInt(primary, secondary): open primary, fall back to secondary on
/// open failure, and on a parse failure retry the secondary once.
pub fn read_node_int(primary: &str, secondary: Option<&str>) -> i32 {
    let fd_primary = fs::File::open(primary).ok();
    let mut file = match fd_primary {
        Some(f) => f,
        None => match secondary.and_then(|s| fs::File::open(s).ok()) {
            Some(f) => f,
            None => return K_READ_ERROR,
        },
    };
    let mut buf = [0u8; 16];
    let n = match file.read(&mut buf) {
        Ok(n) => n,
        Err(_) => return K_IO_ERROR,
    };
    let value = c_atoi(&buf[..n]);
    if value != K_READ_ERROR {
        return value;
    }
    // atoi() returned INT32_MIN: the shipped code retries the secondary node.
    let sec = match secondary {
        Some(s) => s,
        None => return K_READ_ERROR,
    };
    match read_int_once(sec) {
        Ok(v) => {
            if v == K_READ_ERROR {
                K_READ_ERROR
            } else {
                v
            }
        }
        Err(e) => e,
    }
}

pub fn read_node_int_single(path: &str) -> i32 {
    match read_int_once(path) {
        Ok(v) if v != K_READ_ERROR => v,
        Ok(_) => K_READ_ERROR,
        Err(e) => e,
    }
}

/// readNodeStr: the first path that opens and reads wins.  A read() failure
/// moves on to the next path, unlike readNodeStrSingle which returns "".
pub fn read_node_str(primary: &str, secondary: Option<&str>) -> String {
    for path in [Some(primary), secondary].into_iter().flatten() {
        if let Some(s) = read_str_raw(path) {
            return s;
        }
    }
    String::new()
}

pub fn read_node_str_single(path: &str) -> String {
    read_str_raw(path).unwrap_or_default()
}

/// None = the open or the read failed; Some("") = the node read back empty.
fn read_str_raw(path: &str) -> Option<String> {
    let mut file = fs::File::open(path).ok()?;
    let mut buf = [0u8; 256];
    let n = file.read(&mut buf).ok()?;
    let end = n.min(255);
    Some(String::from_utf8_lossy(&buf[..end]).into_owned())
}

/// writeNode: length must be 1..16, else -1.
pub fn write_node(primary: &str, secondary: Option<&str>, value: &str) -> i32 {
    let bytes = value.as_bytes();
    if bytes.is_empty() || bytes.len() > 16 {
        return -1;
    }
    let file = fs::OpenOptions::new()
        .write(true)
        .open(primary)
        .ok()
        .or_else(|| secondary.and_then(|s| fs::OpenOptions::new().write(true).open(s).ok()));
    let mut file = match file {
        Some(f) => f,
        None => return K_READ_ERROR,
    };
    match file.write(bytes) {
        Ok(n) => n as i32,
        Err(_) => K_WRITE_ERROR,
    }
}

/// readIntPath: the plain single-node read the hand-written methods use.
pub fn read_int_path(path: &str) -> i32 {
    match read_int_once(path) {
        Ok(v) if v != K_READ_ERROR => v,
        Ok(_) => K_READ_ERROR,
        Err(e) => e,
    }
}

pub fn read_string_path(path: &str) -> String {
    read_node_str_single(path)
}

pub fn write_string_path(path: &str, value: &str) -> bool {
    let mut file = match fs::OpenOptions::new().write(true).open(path) {
        Ok(f) => f,
        Err(_) => return false,
    };
    match file.write(value.as_bytes()) {
        Ok(n) => n == value.len(),
        Err(_) => false,
    }
}

pub fn node_exists(path: &str) -> bool {
    fs::metadata(path).is_ok()
}

/// The shipped getters clamp the sentinels to 0 before answering.
pub fn clamp0(v: i32) -> i32 {
    if is_error(v) {
        0
    } else {
        v
    }
}

pub fn is_error(v: i32) -> bool {
    v == K_READ_ERROR || v == K_IO_ERROR
}
