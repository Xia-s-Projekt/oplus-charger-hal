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
use std::sync::OnceLock;

use oplus_charger::binder;
use oplus_charger::charger::Charger;
use oplus_charger::constants::SERVICE_NAME;
use oplus_charger::log;
use oplus_charger::txn;

static CHARGER: OnceLock<&'static Charger> = OnceLock::new();

fn charger() -> &'static Charger {
    CHARGER
        .get()
        .expect("charger instance created before the binder is registered")
}

/// Binder transaction callback.  Registered through AIBinder_Class_define, so
/// the NDK calls this with a valid binder pointer and driver-owned parcels.
extern "C" fn on_transact(
    _binder: *mut binder::AIBinder,
    code: u32,
    in_parcel: *const binder::AParcel,
    reply: *mut binder::AParcel,
) -> binder::BinderStatus {
    // SAFETY: the driver keeps both parcels alive for the whole call, which is
    // the lifetime Parcel::borrow requires; the wrappers are dropped without
    // freeing because the driver owns them.
    let inp = unsafe { binder::Parcel::borrow(in_parcel) };
    let mut out = unsafe { binder::Parcel::borrow(reply) };
    let c = charger();
    txn::dispatch(c, code, &inp, &mut out)
}

/// The binder class needs a context pointer; the service itself is reachable
/// through the static, so the context is unused.  No unsafe operation here,
/// the ABI just wants a plain function pointer.
extern "C" fn on_create(_args: *mut std::ffi::c_void) -> *mut std::ffi::c_void {
    std::ptr::null_mut()
}

// The platform's AIBinder_Class_define rejects a null onDestroy (its
// disassembly returns null for every null argument), so the class must
// carry one even though the service owns nothing in the context.
extern "C" fn on_destroy(_context: *mut std::ffi::c_void) {}

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
    // SAFETY: descriptor is a NUL-terminated CString alive for the call, and
    // the three callbacks are plain extern "C" fns with matching signatures.
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

    // SAFETY: class was just created and the call is single-threaded, so no
    // other thread can race the construction; the context is unused.
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
    // SAFETY: binder_obj is owned by this thread until the call registers it,
    // and cname is a NUL-terminated CString alive for the call.
    let status = unsafe { binder::AServiceManager_addService(binder_obj, cname.as_ptr()) };
    if status != binder::STATUS_OK {
        log::error(&format!("addService({name}) failed: {status}"));
        return -1;
    }
    // The shipped code logs the registration with the result appended, so a
    // failure reads "registerChargerService-1".
    log::info("registerChargerService0");

    // The monitor loop is the last thing service.cpp starts.
    std::thread::spawn(move || {
        oplus_charger::monitor::run_monitor_loop(c);
    });
    0
}

fn main() {
    // SAFETY: Called once before the binder thread pool starts and takes only an integer count.
    unsafe { binder::ABinderProcess_setThreadPoolMaxThreadCount(0) };

    let status = register_charger_service();
    if status != 0 {
        log::error(&format!("registerChargerService{status}"));
        return;
    }

    log::info("oplus charger service start");
    // SAFETY: Called from the main thread after the service is registered; it only returns on process shutdown.
    unsafe { binder::ABinderProcess_joinThreadPool() };
}
