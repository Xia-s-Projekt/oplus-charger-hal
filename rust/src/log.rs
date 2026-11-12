// log.rs - the Android log surface the HAL uses.
//
// The shipped service logs under the tag
// "vendor.oplus.hardware.charger-V1-service" (service.cpp kLogTag), not the
// V3/V6 filename the binary ships under.

use std::os::raw::c_char;

use crate::constants::LOG_TAG;

const ANDROID_LOG_INFO: i32 = 4;
const ANDROID_LOG_ERROR: i32 = 6;

const FMT_PERCENT_S: &[u8] = b"%s\0";

pub fn info(msg: &str) {
    log(ANDROID_LOG_INFO, LOG_TAG, msg);
}

pub fn error(msg: &str) {
    log(ANDROID_LOG_ERROR, LOG_TAG, msg);
}

fn log(prio: i32, tag: &str, msg: &str) {
    let tag = match std::ffi::CString::new(tag) {
        Ok(t) => t,
        Err(_) => return,
    };
    let msg = match std::ffi::CString::new(msg) {
        Ok(m) => m,
        Err(_) => return,
    };
    unsafe {
        log_print(
            prio,
            tag.as_ptr() as *const c_char,
            FMT_PERCENT_S.as_ptr() as *const c_char,
            msg.as_ptr(),
        );
    }
}

unsafe fn log_print(prio: i32, tag: *const c_char, fmt: *const c_char, arg: *const c_char) {
    unsafe extern "C" {
        fn __android_log_print(prio: i32, tag: *const c_char, fmt: *const c_char, ...) -> i32;
    }
    unsafe {
        __android_log_print(prio, tag, fmt, arg);
    }
}
