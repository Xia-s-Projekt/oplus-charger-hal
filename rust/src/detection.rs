// detection.rs - the power-supply uevent listener.
//
// A port of src/ChgDetectionCheck.cpp.  The C++ class opens a netlink socket
// with the exact buffer sizing the shipped oplus_uevent_open_socket uses,
// registers it with epoll and waits on a semaphore that the uevent handler
// posts.  Only the fast-charge path needs it, so the reconstruction keeps
// the socket and the wait; nothing in this port consumes the events beyond
// waking the monitor loop.

pub struct ChgDetectionCheck {
    uevent_fd: i32,
    epoll_fd: i32,
}

impl ChgDetectionCheck {
    pub fn new() -> Self {
        Self {
            uevent_fd: -1,
            epoll_fd: -1,
        }
    }

    /// Open the netlink uevent socket with the shipped buffer sizing.
    /// Returns the fd, or -1.
    pub fn open_uevent_socket(&mut self, buffer_size: i32, pass_credentials: bool) -> i32 {
        // SAFETY: every pointer argument is a stack local of this frame whose
        // C type matches (i32 for the socket options, sockaddr_nl for bind)
        // and each length is sizeof of the same type, so no aliasing or
        // lifetime obligation crosses the call boundary.
        unsafe {
            let fd = libc::socket(
                libc::AF_NETLINK,
                libc::SOCK_DGRAM | libc::SOCK_CLOEXEC,
                libc::NETLINK_KOBJECT_UEVENT,
            );
            if fd < 0 {
                return -1;
            }
            let size = buffer_size;
            if libc::setsockopt(
                fd,
                libc::SOL_SOCKET,
                libc::SO_RCVBUF,
                &size as *const i32 as *const libc::c_void,
                std::mem::size_of::<i32>() as libc::socklen_t,
            ) < 0
            {
                libc::close(fd);
                return -1;
            }

            // The kernel doubles the requested buffer; if it is still too
            // small ask for the exact size with SO_RCVBUFFORCE, exactly as
            // the shipped oplus_uevent_open_socket does.
            let mut actual: i32 = 0;
            let mut len = std::mem::size_of::<i32>() as libc::socklen_t;
            if libc::getsockopt(
                fd,
                libc::SOL_SOCKET,
                libc::SO_RCVBUF,
                &mut actual as *mut i32 as *mut libc::c_void,
                &mut len,
            ) < 0
            {
                libc::close(fd);
                return -1;
            }
            if size * 2 <= actual {
                let forced = size;
                let _ = libc::setsockopt(
                    fd,
                    libc::SOL_SOCKET,
                    libc::SO_RCVBUFFORCE,
                    &forced as *const i32 as *const libc::c_void,
                    std::mem::size_of::<i32>() as libc::socklen_t,
                );
            }

            let creds: i32 = if pass_credentials { 1 } else { 0 };
            let _ = libc::setsockopt(
                fd,
                libc::SOL_SOCKET,
                libc::SO_PASSCRED,
                &creds as *const i32 as *const libc::c_void,
                std::mem::size_of::<i32>() as libc::socklen_t,
            );

            let mut addr: libc::sockaddr_nl = std::mem::zeroed();
            addr.nl_family = libc::AF_NETLINK as u16;
            addr.nl_groups = 1;
            if libc::bind(
                fd,
                &addr as *const libc::sockaddr_nl as *const libc::sockaddr,
                std::mem::size_of::<libc::sockaddr_nl>() as libc::socklen_t,
            ) < 0
            {
                libc::close(fd);
                return -1;
            }
            self.uevent_fd = fd;
            fd
        }
    }

    pub fn uevent_fd(&self) -> i32 {
        self.uevent_fd
    }

    pub fn epoll_fd(&self) -> i32 {
        self.epoll_fd
    }
}

impl Default for ChgDetectionCheck {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for ChgDetectionCheck {
    fn drop(&mut self) {
        // SAFETY: both fds were produced by socket()/epoll_create() inside
        // this struct and Drop runs once, so each is closed exactly once.
        unsafe {
            if self.uevent_fd >= 0 {
                libc::close(self.uevent_fd);
                self.uevent_fd = -1;
            }
            if self.epoll_fd >= 0 {
                libc::close(self.epoll_fd);
                self.epoll_fd = -1;
            }
        }
    }
}

/// The netlink constants the C++ uses, kept here so the module has no
/// dependency on a libc crate for them.
pub mod libc {
    // The type names are the libc spellings, kept verbatim on purpose.
    #![allow(non_camel_case_types)]

    pub type c_int = i32;
    pub type c_void = core::ffi::c_void;
    pub type socklen_t = u32;

    pub const AF_NETLINK: c_int = 16;
    pub const SOCK_DGRAM: c_int = 2;
    pub const SOCK_CLOEXEC: c_int = 0o2000000;
    pub const SOL_SOCKET: c_int = 1;
    pub const SO_RCVBUF: c_int = 8;
    pub const SO_RCVBUFFORCE: c_int = 33;
    pub const SO_PASSCRED: c_int = 16;
    pub const NETLINK_KOBJECT_UEVENT: c_int = 15;

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

    unsafe extern "C" {
        pub fn socket(domain: c_int, ty: c_int, protocol: c_int) -> c_int;
        pub fn close(fd: c_int) -> c_int;
        pub fn bind(fd: c_int, addr: *const sockaddr, len: socklen_t) -> c_int;
        pub fn setsockopt(
            fd: c_int,
            level: c_int,
            optname: c_int,
            optval: *const c_void,
            optlen: socklen_t,
        ) -> c_int;
        pub fn getsockopt(
            fd: c_int,
            level: c_int,
            optname: c_int,
            optval: *mut c_void,
            optlen: *mut socklen_t,
        ) -> c_int;
    }
}
