// props.rs - the Android system-property surface the HAL touches.
//
// There is exactly one: the bypass-charge state.  The node is the source of
// truth; the prop exists because init re-applies it to the node on every boot
// (erofs/vendor/etc/init/tran_optimize.rc).

use std::ffi::CString;
use std::os::raw::c_char;

use crate::constants::tran;

const PROP_VALUE_MAX: usize = 92;

#[cfg(target_os = "android")]
unsafe extern "C" {
    fn __system_property_get(name: *const c_char, value: *mut c_char) -> i32;
    fn __system_property_set(name: *const c_char, value: *const c_char) -> i32;
}

#[cfg(not(target_os = "android"))]
unsafe fn __system_property_get(_name: *const c_char, _value: *mut c_char) -> i32 {
    0
}

#[cfg(not(target_os = "android"))]
unsafe fn __system_property_set(_name: *const c_char, _value: *const c_char) -> i32 {
    0
}

fn read_prop(key: &str) -> Option<String> {
    let c = CString::new(key).ok()?;
    let mut buf = vec![0u8; PROP_VALUE_MAX];
    let n = unsafe { __system_property_get(c.as_ptr(), buf.as_mut_ptr() as *mut c_char) };
    if n <= 0 {
        return None;
    }
    buf.truncate(n as usize);
    String::from_utf8(buf).ok()
}

pub fn is_bypass_prop_enabled() -> bool {
    read_prop(tran::BYPASS_PROP).map(|v| v == "1").unwrap_or(false)
}

pub fn set_bypass_prop(value: i32) {
    let key = match CString::new(tran::BYPASS_PROP) {
        Ok(k) => k,
        Err(_) => return,
    };
    let val = match CString::new(if value == 1 { "1" } else { "0" }) {
        Ok(v) => v,
        Err(_) => return,
    };
    unsafe {
        __system_property_set(key.as_ptr(), val.as_ptr());
    }
}
