// testkit.rs - the shipped test-kit client.
//
// A port of TestKit::KernelTest in ChargerManual.cpp.  The ioctl numbers are
// _IOR('f', n, ...) / _IOWR('f', n, ...) with type 'f'.

use std::fs::{File, OpenOptions};
use std::os::unix::io::AsRawFd;

use crate::constants;

pub struct KernelTest {
    fd: Option<File>,
}

impl KernelTest {
    pub fn new() -> Self {
        Self {
            fd: OpenOptions::new()
                .read(true)
                .write(true)
                .open(constants::TEST_KIT_DEVICE)
                .ok(),
        }
    }

    fn fd(&self) -> Option<i32> {
        self.fd.as_ref().map(|f| f.as_raw_fd())
    }

    pub fn feature_num(&self) -> Option<u32> {
        let fd = self.fd()?;
        let mut num: u32 = 0;
        let rc = unsafe {
            ioctl(
                fd,
                constants::IOCTL_GET_FEATURE_NUM,
                &mut num as *mut u32 as *mut core::ffi::c_void,
            )
        };
        if rc < 0 {
            return None;
        }
        Some(num)
    }

    pub fn feature_list(&self) -> Option<String> {
        let fd = self.fd()?;
        let mut buf = [0u8; 512];
        let rc = unsafe {
            ioctl(
                fd,
                constants::IOCTL_GET_FEATURE_LIST,
                buf.as_mut_ptr() as *mut core::ffi::c_void,
            )
        };
        if rc < 0 {
            return None;
        }
        buf[511] = 0;
        let end = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
        Some(String::from_utf8_lossy(&buf[..end]).into_owned())
    }

    pub fn name(&self, index: u32) -> Option<String> {
        let fd = self.fd()?;
        let mut idx = index;
        let rc = unsafe {
            ioctl(
                fd,
                constants::IOCTL_GET_NAME,
                &mut idx as *mut u32 as *mut core::ffi::c_void,
            )
        };
        if rc < 0 {
            return None;
        }
        Some(idx.to_string())
    }

    /// Returns (rc, result).  rc is 0 on success, the negative errno
    /// otherwise; the C++ code stores errno (positive) in the same slot.
    pub fn run_test(&self, index: u32) -> (i32, Option<String>) {
        let fd = match self.fd() {
            Some(f) => f,
            None => return (-libc::ENODEV, None),
        };
        // struct { unsigned int index; char text[504]; }
        #[repr(C)]
        struct Payload {
            index: u32,
            text: [u8; 504],
        }
        let mut payload: Payload = unsafe { std::mem::zeroed() };
        payload.index = index;
        let rc = unsafe {
            ioctl(
                fd,
                constants::IOCTL_RUN_TEST,
                &mut payload as *mut Payload as *mut core::ffi::c_void,
            )
        };
        if rc < 0 {
            return (errno(), None);
        }
        let end = payload.text.iter().position(|&b| b == 0).unwrap_or(504);
        (
            0,
            Some(String::from_utf8_lossy(&payload.text[..end]).into_owned()),
        )
    }
}

impl Default for KernelTest {
    fn default() -> Self {
        Self::new()
    }
}

unsafe fn ioctl(fd: i32, request: u32, arg: *mut core::ffi::c_void) -> i32 {
    unsafe extern "C" {
        fn ioctl(fd: i32, request: u64, arg: *mut core::ffi::c_void) -> i32;
    }
    unsafe { ioctl(fd, request as u64, arg) }
}

fn errno() -> i32 {
    unsafe extern "C" {
        fn __errno() -> *mut i32;
    }
    unsafe { *__errno() }
}

mod libc {
    pub const ENODEV: i32 = 19;
}
