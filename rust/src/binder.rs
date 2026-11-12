// binder.rs - the NDK binder surface the service needs, declared by hand.
//
// This is deliberately a thin, literal binding of libbinder_ndk.  No AIDL
// codegen crate is involved: the transaction wire format is reproduced by
// txn.rs, which is what the generated C++ ICharger.cpp does too.

use std::os::raw::{c_char, c_int, c_uint, c_void};

pub type BinderStatus = c_int;
pub type TransactionCode = u32;

pub const STATUS_OK: BinderStatus = 0;
pub const FIRST_CALL_TRANSACTION: TransactionCode = 1;

pub const STATUS_UNKNOWN_TRANSACTION: BinderStatus = -2147483645;

#[repr(C)]
pub struct AIBinder {
    _private: [u8; 0],
}

#[repr(C)]
pub struct AParcel {
    _private: [u8; 0],
}

#[repr(C)]
pub struct AStatus {
    _private: [u8; 0],
}

/// AIBinder_Class_onTransact.
pub type OnTransact = unsafe extern "C" fn(
    binder: *mut AIBinder,
    code: TransactionCode,
    in_parcel: *const AParcel,
    reply: *mut AParcel,
) -> BinderStatus;

/// AIBinder_Class_onCreate.
pub type OnCreate = unsafe extern "C" fn(args: *mut c_void) -> *mut c_void;

/// AIBinder_Class_onDestroy.
pub type OnDestroy = unsafe extern "C" fn(context: *mut c_void);

/// AIBinder_Class_onDump.
pub type OnDump = unsafe extern "C" fn(
    binder: *mut AIBinder,
    fd: c_int,
    args: *mut *const c_char,
    num_args: u32,
) -> BinderStatus;

/// AParcel_stringAllocator: allocate a buffer of `length` bytes (which
/// includes the NUL terminator) and return it.  length == -1 means null.
pub type StringAllocator = unsafe extern "C" fn(
    string_data: *mut c_void,
    length: i32,
    buffer: *mut *mut c_char,
) -> bool;

#[repr(C)]
pub struct AIBinder_Class {
    _private: [u8; 0],
}

unsafe extern "C" {
    pub fn AIBinder_Class_define(
        interface_descriptor: *const c_char,
        on_create: Option<OnCreate>,
        on_destroy: Option<OnDestroy>,
        on_transact: Option<OnTransact>,
    ) -> *mut AIBinder_Class;

    pub fn AIBinder_Class_setOnDump(class: *mut AIBinder_Class, on_dump: Option<OnDump>);

    pub fn AIBinder_new(class: *const AIBinder_Class, args: *mut c_void) -> *mut AIBinder;
    pub fn AIBinder_incStrong(binder: *mut AIBinder);
    pub fn AIBinder_decStrong(binder: *mut AIBinder);
    pub fn AIBinder_associateClass(binder: *mut AIBinder, class: *const AIBinder_Class) -> bool;

    pub fn AServiceManager_addService(binder: *mut AIBinder, name: *const c_char) -> BinderStatus;
    pub fn AServiceManager_checkService(name: *const c_char) -> *mut AIBinder;
    pub fn AServiceManager_waitForService(name: *const c_char) -> *mut AIBinder;

    pub fn ABinderProcess_setThreadPoolMaxThreadCount(num_threads: c_uint);
    pub fn ABinderProcess_startThreadPool() -> bool;
    pub fn ABinderProcess_joinThreadPool();

    pub fn AParcel_create() -> *mut AParcel;
    pub fn AParcel_delete(parcel: *mut AParcel);

    pub fn AParcel_writeInt32(parcel: *mut AParcel, value: i32) -> BinderStatus;
    pub fn AParcel_readInt32(parcel: *const AParcel, value: *mut i32) -> BinderStatus;
    pub fn AParcel_writeBool(parcel: *mut AParcel, value: bool) -> BinderStatus;
    pub fn AParcel_readBool(parcel: *const AParcel, value: *mut bool) -> BinderStatus;

    pub fn AParcel_writeString(
        parcel: *mut AParcel,
        string: *const c_char,
        length: i32,
    ) -> BinderStatus;
    pub fn AParcel_readString(
        parcel: *const AParcel,
        string_data: *mut c_void,
        allocator: Option<StringAllocator>,
    ) -> BinderStatus;

    pub fn AParcel_writeStatusHeader(parcel: *mut AParcel, status: *const AStatus) -> BinderStatus;

    pub fn AParcel_getDataPosition(parcel: *const AParcel) -> c_int;
    pub fn AParcel_setDataPosition(parcel: *mut AParcel, position: c_int) -> BinderStatus;

    pub fn AStatus_newOk() -> *mut AStatus;
    pub fn AStatus_fromStatus(status: BinderStatus) -> *mut AStatus;
    pub fn AStatus_isOk(status: *const AStatus) -> bool;
    pub fn AStatus_delete(status: *mut AStatus);
}

/// Owns an AIBinder strong reference taken from the NDK.
pub struct BinderHandle {
    ptr: *mut AIBinder,
}

impl BinderHandle {
    /// # Safety
    /// `ptr` must be a strong reference, or null.
    pub unsafe fn from_raw(ptr: *mut AIBinder) -> Option<Self> {
        if ptr.is_null() {
            None
        } else {
            Some(Self { ptr })
        }
    }

    pub fn as_ptr(&self) -> *mut AIBinder {
        self.ptr
    }
}

impl Drop for BinderHandle {
    fn drop(&mut self) {
        unsafe { AIBinder_decStrong(self.ptr) }
    }
}

/// A parcel handle.  `owned` is false when the binder driver owns the
/// pointer, in which case Drop must not free it.
pub struct Parcel {
    ptr: *mut AParcel,
    owned: bool,
}

impl Parcel {
    /// Allocate a new parcel.  Freed on drop.
    pub fn new() -> Option<Self> {
        let ptr = unsafe { AParcel_create() };
        if ptr.is_null() {
            None
        } else {
            Some(Self { ptr, owned: true })
        }
    }

    /// Wrap a parcel the binder driver owns.  Not freed on drop.
    ///
    /// # Safety
    /// `ptr` must be a valid parcel for the duration of the call.
    pub unsafe fn borrow(ptr: *const AParcel) -> Self {
        Self {
            ptr: ptr as *mut AParcel,
            owned: false,
        }
    }

    pub fn as_ptr(&self) -> *mut AParcel {
        self.ptr
    }

    pub fn write_i32(&mut self, v: i32) -> bool {
        unsafe { AParcel_writeInt32(self.ptr, v) == STATUS_OK }
    }

    pub fn read_i32(&self) -> Result<i32, ()> {
        let mut v: i32 = 0;
        let rc = unsafe { AParcel_readInt32(self.ptr, &mut v) };
        if rc == STATUS_OK {
            Ok(v)
        } else {
            Err(())
        }
    }

    pub fn write_bool(&mut self, v: bool) -> bool {
        unsafe { AParcel_writeBool(self.ptr, v) == STATUS_OK }
    }

    pub fn read_bool(&self) -> Result<bool, ()> {
        let mut v = false;
        let rc = unsafe { AParcel_readBool(self.ptr, &mut v) };
        if rc == STATUS_OK {
            Ok(v)
        } else {
            Err(())
        }
    }

    pub fn write_string(&mut self, s: &str) -> bool {
        let c = match std::ffi::CString::new(s) {
            Ok(c) => c,
            Err(_) => return false,
        };
        unsafe { AParcel_writeString(self.ptr, c.as_ptr(), s.len() as i32) == STATUS_OK }
    }

    pub fn read_string(&self) -> Result<String, ()> {
        let mut out: Vec<u8> = Vec::new();
        let holder = &mut out as *mut Vec<u8> as *mut c_void;
        let rc = unsafe { AParcel_readString(self.ptr, holder, Some(alloc_vec)) };
        if rc != STATUS_OK {
            return Err(());
        }
        // A null string comes back as an empty vec.
        String::from_utf8(out).map_err(|_| ())
    }

    pub fn write_status_ok(&mut self) -> bool {
        unsafe {
            let status = AStatus_newOk();
            let rc = AParcel_writeStatusHeader(self.ptr, status);
            AStatus_delete(status);
            rc == STATUS_OK
        }
    }

    pub fn write_status(&mut self, status: BinderStatus) -> bool {
        unsafe {
            let s = AStatus_fromStatus(status);
            let rc = AParcel_writeStatusHeader(self.ptr, s);
            AStatus_delete(s);
            rc == STATUS_OK
        }
    }

    pub fn data_position(&self) -> i32 {
        unsafe { AParcel_getDataPosition(self.ptr) }
    }

    pub fn set_data_position(&mut self, pos: i32) -> bool {
        unsafe { AParcel_setDataPosition(self.ptr, pos) == STATUS_OK }
    }
}

impl Drop for Parcel {
    fn drop(&mut self) {
        if self.owned {
            unsafe { AParcel_delete(self.ptr) }
        }
    }
}

/// Allocator handed to AParcel_readString.  length includes the NUL
/// terminator; -1 means the parcel carried a null string.
unsafe extern "C" fn alloc_vec(
    string_data: *mut c_void,
    length: i32,
    buffer: *mut *mut c_char,
) -> bool {
    if string_data.is_null() {
        return false;
    }
    let out = unsafe { &mut *(string_data as *mut Vec<u8>) };
    if length < 0 {
        out.clear();
        *buffer = std::ptr::null_mut();
        return true;
    }
    out.clear();
    out.resize(length as usize, 0);
    *buffer = out.as_mut_ptr() as *mut c_char;
    true
}
