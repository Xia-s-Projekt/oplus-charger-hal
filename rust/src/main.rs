// main.rs - the HAL entry point.
//
// A port of src/service.cpp.  The shipped main() does exactly this:
//   ABinderProcess_setThreadPoolMaxThreadCount(0)
//   registerChargerService()
//   ABinderProcess_joinThreadPool()
//
// and registerChargerService() builds the instance name by appending
// "/default" to the interface descriptor
// "vendor.oplus.hardware.charger.ICharger", then calls
// AServiceManager_addService() with the BnCharger binder.

use std::ffi::CString;
use std::sync::atomic::AtomicBool;
use std::sync::Arc;
use std::sync::OnceLock;

use oplus_charger::binder;
use oplus_charger::charger::Charger;
use oplus_charger::constants::SERVICE_NAME;
use oplus_charger::log;
use oplus_charger::txn;

static CHARGER: OnceLock<&'static Charger> = OnceLock::new();

fn charger() -> &'static Charger {
    *CHARGER
        .get()
        .expect("charger instance created before the binder is registered")
}

unsafe extern "C" fn on_transact(
    _binder: *mut binder::AIBinder,
    code: u32,
    in_parcel: *const binder::AParcel,
    reply: *mut binder::AParcel,
) -> binder::BinderStatus {
    // Both parcels are owned by the binder driver for the duration of the
    // call, so neither wrapper frees them on drop.
    let inp = unsafe { binder::Parcel::borrow(in_parcel) };
    let mut out = unsafe { binder::Parcel::borrow(reply) };
    let c = charger();
    txn::dispatch(c, code, &inp, &mut out)
}

unsafe extern "C" fn on_create(_args: *mut std::ffi::c_void) -> *mut std::ffi::c_void {
    // The binder class needs a context pointer; the service itself is
    // reachable through the static, so the context is unused.
    std::ptr::null_mut()
}

// The platform's AIBinder_Class_define rejects a null onDestroy (its
// disassembly returns null for every null argument), so the class must
// carry one even though the service owns nothing in the context.
unsafe extern "C" fn on_destroy(_context: *mut std::ffi::c_void) {}

/// The instance suffix can be overridden so a test build can register next
/// to the HAL that is already running on the device.  Unset - the shipped
/// behaviour - means "/default".
fn service_instance() -> String {
    match std::env::var("OPLUS_CHARGER_INSTANCE") {
        Ok(v) if !v.is_empty() => format!("/{v}"),
        _ => "/default".to_string(),
    }
}

fn register_charger_service() -> i32 {
    let c: &'static Charger = Box::leak(Box::new(Charger::new()));
    let _ = CHARGER.set(c);
    c.set_running(true);

    let descriptor = match CString::new(SERVICE_NAME) {
        Ok(d) => d,
        Err(_) => return -1,
    };
    let class = unsafe {
        binder::AIBinder_Class_define(
            descriptor.as_ptr(),
            Some(on_create),
            Some(on_destroy),
            Some(on_transact),
        )
    };
    if class.is_null() {
        log::error("AIBinder_Class_define failed");
        return -1;
    }

    let binder_obj = unsafe { binder::AIBinder_new(class, std::ptr::null_mut()) };
    if binder_obj.is_null() {
        log::error("AIBinder_new failed");
        return -1;
    }

    let name = format!("{SERVICE_NAME}{}", service_instance());
    let cname = match CString::new(name.clone()) {
        Ok(n) => n,
        Err(_) => return -1,
    };
    let status = unsafe { binder::AServiceManager_addService(binder_obj, cname.as_ptr()) };
    if status != binder::STATUS_OK {
        log::error(&format!("addService({name}) failed: {status}"));
        return -1;
    }
    // The shipped code logs the registration with the result appended, so a
    // failure reads "registerChargerService-1".
    log::info("registerChargerService0");

    // The monitor loop is the last thing service.cpp starts.
    let stop = Arc::new(AtomicBool::new(false));
    let stop2 = Arc::clone(&stop);
    std::thread::spawn(move || {
        oplus_charger::monitor::run_monitor_loop(c, &stop2);
    });
    0
}

fn main() {
    unsafe { binder::ABinderProcess_setThreadPoolMaxThreadCount(0) };

    let status = register_charger_service();
    if status != 0 {
        log::error(&format!("registerChargerService{status}"));
        return;
    }

    log::info("oplus charger service start");
    unsafe { binder::ABinderProcess_joinThreadPool() };
}
