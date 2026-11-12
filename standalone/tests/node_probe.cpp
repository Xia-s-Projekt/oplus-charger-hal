// node_probe.cpp - report whether the HAL's own security context can read
// each candidate node.
//
// The rebuilt HAL runs as uid system in the hal_allocator_default domain
// (set by the file label hal_allocator_default_exec).  Whether a node is
// readable depends on BOTH the file label and the domain's allow rules, so
// the only trustworthy answer is to open the node in that exact context.
//
// Usage: node_probe <path> [<path> ...]
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        const char* path = argv[i];
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            printf("DENIED  %-62s errno=%d (%s)\n", path, errno, strerror(errno));
            continue;
        }
        char buf[128] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n < 0) {
            printf("READFAIL %-61s errno=%d (%s)\n", path, errno, strerror(errno));
            continue;
        }
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
            buf[--n] = '\0';
        }
        printf("OK      %-62s value=\"%s\"\n", path, buf);
    }
    return 0;
}
