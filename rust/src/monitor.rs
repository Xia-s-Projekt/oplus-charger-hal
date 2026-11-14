// monitor.rs - the monitor loop and the battery-property trigger.
//
// A port of ChargerManual.cpp's runMonitorLoop() and triggerBatteryUpdate().
//
// The loop watches the bypass prop, the charger online state, the battery
// status and the adapter wattage, and re-derives the bypass node whenever
// the prop changes.  It also pulses the battery-property push after a state
// change so SystemUI repaints the charging indicator.

use std::ffi::CString;
use std::sync::Mutex;
use std::time::Duration;

use crate::charger::Charger;
use crate::constants::{K_IO_ERROR, K_MIN_REPORT_WATTS, K_READ_ERROR, K_SUPER_VOOC_WATTS};
use crate::decimal::Throttle;
use crate::nodes;
use crate::props;

const TRIGGER_MIN_GAP_MS: u64 = 75;
const PULSE_INTERVAL_MS: u128 = 150;
const PULSE_COUNT: i32 = 4;
const FAST_POLL_MS: i64 = 20;
const SLOW_POLL_MS: i64 = 100;
const FAST_POLL_WINDOW_MS: u128 = 15000;
const UEVENT_BUFFER_SIZE: usize = 2048;

static TRIGGER: Mutex<Option<Throttle>> = Mutex::new(None);

/// Double-fork `/system/bin/service call batteryproperties 2`, exactly as
/// the shipped triggerBatteryUpdate does.
pub fn trigger_battery_update(force: bool) {
    {
        let mut slot = TRIGGER.lock().unwrap_or_else(|e| e.into_inner());
        let t = slot.get_or_insert_with(Throttle::new);
        if !t.allow(force, Duration::from_millis(TRIGGER_MIN_GAP_MS)) {
            return;
        }
    }

    // SAFETY: fork() in a single-threaded binder service is safe from lock deadlocks only because the parent does not hold any Rust mutex across the call; the child never returns (execv or _exit) so it cannot run any Drop.
    unsafe {
        let pid = libc::fork();
        if pid == 0 {
            let p2 = libc::fork();
            if p2 == 0 {
                let devnull = libc_open("/dev/null", libc::O_RDWR);
                if devnull >= 0 {
                    libc::dup2(devnull, 0);
                    libc::dup2(devnull, 1);
                    libc::dup2(devnull, 2);
                    libc::close(devnull);
                }
                // These are all static literals with no interior NUL, so the
                // construction cannot fail; expect() documents the invariant
                // instead of silently unwrapping.
                let path = CString::new("/system/bin/service").expect("static path has no interior NUL");
                let a0 = CString::new("service").expect("static arg has no interior NUL");
                let a1 = CString::new("call").expect("static arg has no interior NUL");
                let a2 = CString::new("batteryproperties").expect("static arg has no interior NUL");
                let a3 = CString::new("2").expect("static arg has no interior NUL");
                let argv = [
                    a0.as_ptr(),
                    a1.as_ptr(),
                    a2.as_ptr(),
                    a3.as_ptr(),
                    std::ptr::null(),
                ];
                libc::execv(path.as_ptr(), argv.as_ptr());
                libc::_exit(127);
            }
            libc::_exit(0);
        }
        if pid > 0 {
            libc::waitpid(pid, std::ptr::null_mut(), 0);
        }
    }
}

fn libc_open(path: &str, flags: i32) -> i32 {
    // A path with an interior NUL cannot be opened at all; returning -1 keeps
    // the monitor loop alive instead of panicking on an unopenable path.
    let c = match CString::new(path) {
        Ok(c) => c,
        Err(_) => return -1,
    };
    // SAFETY: c is a NUL-terminated CString alive for the call and flags is an
    // integer, so no aliasing obligation crosses the boundary.
    unsafe { libc::open(c.as_ptr(), flags) }
}

/// The monitor loop body.  Runs until the charger's running flag clears,
/// mirroring the C++ `while (mRunning.load(std::memory_order_relaxed))` in
/// Charger::runMonitorLoop (ChargerManual.cpp:339).
pub fn run_monitor_loop(charger: &Charger) {
    let mut last_watts: i32 = -1;
    let mut last_online: i32 = -1;
    let mut last_status = String::new();
    let mut pulse_countdown: i32 = 0;
    let mut online_since = std::time::Instant::now();
    let mut last_pulse = std::time::Instant::now();
    let mut last_bypass_prop: i32 = -1;
    let mut last_bypass_node: i32 = -1;

    let adapter_capacity_path = std::env::var("OPLUS_CHARGER_CAPACITY_NODE")
        .ok()
        .filter(|v| !v.is_empty())
        .unwrap_or_else(|| "/sys/devices/platform/charger/adapter_capacity".to_string());

    let mut uevent_fd = -1i32;
    // SAFETY: These are plain integer socket arguments with no pointers, so there is no aliasing to uphold; the returned fd is stored and only used from the monitor thread.
    unsafe {
        let fd = libc::socket(
            libc::AF_NETLINK,
            libc::SOCK_DGRAM | libc::SOCK_CLOEXEC | libc::SOCK_NONBLOCK,
            libc::NETLINK_KOBJECT_UEVENT,
        );
        if fd >= 0 {
            let mut sa: libc::sockaddr_nl = std::mem::zeroed();
            sa.nl_family = libc::AF_NETLINK as u16;
            sa.nl_groups = 1;
            if libc::bind(
                fd,
                &sa as *const libc::sockaddr_nl as *const libc::sockaddr,
                std::mem::size_of::<libc::sockaddr_nl>() as libc::socklen_t,
            ) < 0
            {
                libc::close(fd);
                uevent_fd = -1;
            } else {
                uevent_fd = fd;
            }
        }
    }

    while charger.running() {
        let now = std::time::Instant::now();
        let online_elapsed = now.duration_since(online_since).as_millis();
        let poll_timeout_ms = if last_online == 1
            && last_watts < K_SUPER_VOOC_WATTS
            && online_elapsed < FAST_POLL_WINDOW_MS
        {
            FAST_POLL_MS
        } else {
            SLOW_POLL_MS
        };

        if uevent_fd >= 0 {
            let mut pfd = libc::pollfd {
                fd: uevent_fd,
                events: libc::POLLIN as i16,
                revents: 0,
            };
            let ret = unsafe { libc::poll(&mut pfd, 1, poll_timeout_ms as i32) };
            if ret > 0 && (pfd.revents & libc::POLLIN as i16) != 0 {
                let mut buf = [0u8; UEVENT_BUFFER_SIZE];
                loop {
                    let n = unsafe {
                        libc::recv(
                            uevent_fd,
                            buf.as_mut_ptr() as *mut libc::c_void,
                            buf.len(),
                            libc::MSG_DONTWAIT,
                        )
                    };
                    if n <= 0 {
                        break;
                    }
                }
            }
        } else {
            std::thread::sleep(Duration::from_millis(poll_timeout_ms as u64));
        }

        let bypass_prop = props::is_bypass_prop_enabled() as i32;
        if bypass_prop != last_bypass_prop {
            last_bypass_prop = bypass_prop;
            // The prop is the user's intent; when it is off the kernel owns
            // the node, so the policy only runs when something is actually
            // requesting bypass.
            {
                let mut st = charger.lock_state();
                if bypass_prop == 1 {
                    st.bypass.game_switch = 1;
                    st.bypass.up_limit_enabled = 1;
                } else {
                    st.bypass.game_switch = 0;
                    st.bypass.up_limit_enabled = 0;
                }
            }
            let applied = charger.apply_bypass_policy(true);
            if applied >= 0 {
                last_bypass_node = applied;
                charger.trigger_battery_update(true);
            }
        }

        // Reconcile: the node can be left at 1 with nothing requesting it -
        // an external writer, a transient decision from before the prop
        // writeback was removed, or the charge limit that has since been
        // crossed back below its target.  Re-derive it whenever the live
        // node disagrees with the last decision, so the bypass can never
        // stick on by itself.
        let live_node = nodes::read_int_path("/sys/devices/platform/charger/bypass_charger");
        if !nodes::is_error(live_node) && live_node != last_bypass_node {
            let applied = charger.apply_bypass_policy(true);
            if applied >= 0 {
                last_bypass_node = applied;
                if applied != live_node {
                    charger.trigger_battery_update(true);
                }
            }
        }

        let mut online = nodes::read_int_path("/sys/class/power_supply/charger/online");
        if online <= 0 || online == K_READ_ERROR || online == K_IO_ERROR {
            let usb_online = nodes::read_int_path("/sys/class/power_supply/usb/online");
            if usb_online == 1 {
                online = 1;
            } else if online == K_READ_ERROR || online == K_IO_ERROR {
                online = 0;
            }
        }

        let mut status = nodes::read_string_path("/sys/class/power_supply/battery/status");
        while status.ends_with('\n') || status.ends_with('\r') || status.ends_with(' ') {
            status.pop();
        }

        let mut watts = 0;
        if online == 1 || status == "Charging" {
            watts = nodes::read_int_path(&adapter_capacity_path);
            if watts <= 0 || watts == K_READ_ERROR || watts == K_IO_ERROR {
                watts = nodes::read_int_path("/sys/class/power_supply/tran-auto-test/ta_power");
                if watts == K_READ_ERROR || watts == K_IO_ERROR {
                    watts = 0;
                }
            }
        }
        if online == 0 && (status == "Discharging" || status == "Not charging") {
            watts = 0;
        }
        if watts < K_MIN_REPORT_WATTS {
            watts = 0;
        }

        let now = std::time::Instant::now();
        let state_changed = watts != last_watts || online != last_online || status != last_status;
        if state_changed {
            if online == 1 && last_online != 1 {
                online_since = now;
                charger.decimal_reset();
            } else if online == 0
                || (watts >= K_SUPER_VOOC_WATTS && last_watts < K_SUPER_VOOC_WATTS)
            {
                charger.decimal_reset();
            }

            crate::log::info(&format!(
                "Power change detected: watts={last_watts}->{watts} online={last_online}->{online} status={last_status}->{status}"
            ));
            let plug_changed = online != last_online;
            last_watts = watts;
            last_online = online;
            last_status = status;

            charger.fast_charge_refresh_pub();
            charger.trigger_battery_update(plug_changed);
            pulse_countdown = if online == 1 { PULSE_COUNT } else { 1 };
            last_pulse = now;
        } else if pulse_countdown > 0 {
            let pulse_elapsed = now.duration_since(last_pulse).as_millis();
            if pulse_elapsed >= PULSE_INTERVAL_MS {
                pulse_countdown -= 1;
                last_pulse = now;
                charger.fast_charge_refresh_pub();
                charger.trigger_battery_update(true);
            }
        }
    }

    if uevent_fd >= 0 {
        // SAFETY: The fd was obtained from socket()/open() and is closed exactly once.
        unsafe { libc::close(uevent_fd) };
    }
}

pub mod libc {
    // The type names are the libc spellings, kept verbatim on purpose.
    #![allow(non_camel_case_types)]

    pub type c_int = i32;
    pub type c_char = std::os::raw::c_char;
    pub type c_void = core::ffi::c_void;
    pub type socklen_t = u32;
    pub type pid_t = i32;

    pub const AF_NETLINK: c_int = 16;
    pub const SOCK_DGRAM: c_int = 2;
    pub const SOCK_CLOEXEC: c_int = 0o2000000;
    pub const SOCK_NONBLOCK: c_int = 0o4000;
    pub const NETLINK_KOBJECT_UEVENT: c_int = 15;
    pub const POLLIN: c_int = 1;
    pub const MSG_DONTWAIT: c_int = 0x40;
    pub const O_RDWR: c_int = 2;

    #[repr(C)]
    pub struct sockaddr_nl {
        pub nl_family: u16,
        pub nl_pad: u16,
        pub nl_pid: u32,
        pub nl_groups: u32,
    }

    #[repr(C)]
    pub struct sockaddr {
        pub sa_family: u16,
        pub sa_data: [u8; 14],
    }

    #[repr(C)]
    pub struct pollfd {
        pub fd: c_int,
        pub events: i16,
        pub revents: i16,
    }

    // SAFETY: bionic libc declarations used only through the unsafe blocks in
    // trigger_battery_update and run_monitor_loop, whose comments record the
    // pointer/lifetime obligations for each call.
    unsafe extern "C" {
        pub fn socket(domain: c_int, ty: c_int, protocol: c_int) -> c_int;
        pub fn close(fd: c_int) -> c_int;
        pub fn bind(fd: c_int, addr: *const sockaddr, len: socklen_t) -> c_int;
        pub fn poll(fds: *mut pollfd, nfds: u64, timeout: c_int) -> c_int;
        pub fn recv(fd: c_int, buf: *mut c_void, len: usize, flags: c_int) -> isize;
        pub fn fork() -> pid_t;
        pub fn execv(path: *const c_char, argv: *const *const c_char) -> c_int;
        pub fn dup2(oldfd: c_int, newfd: c_int) -> c_int;
        pub fn waitpid(pid: pid_t, status: *mut c_int, options: c_int) -> pid_t;
        pub fn _exit(status: c_int) -> !;
        pub fn open(path: *const c_char, flags: c_int, ...) -> c_int;
    }
}
