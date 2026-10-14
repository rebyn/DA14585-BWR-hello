/* Minimal newlib syscall stubs for the DA14585 (no OS, no heap via libc).
 * The SDK/BLE stack use their own retained heaps; libc malloc must never be
 * reached — _sbrk fails loudly (ENOMEM) instead of corrupting RAM. _exit/
 * abort spin; on this chip a Main-loop return is a firmware defect anyway.
 */
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

void *_sbrk(ptrdiff_t incr) {
    (void)incr;
    errno = ENOMEM;
    return (void *)-1;
}
int _fstat(int fd, struct stat *st) {
    (void)fd; (void)st;
    errno = EBADF;
    return -1;
}
int _isatty(int fd) { (void)fd; errno = EBADF; return 0; }
int _kill(pid_t pid, int sig) {
    (void)pid; (void)sig;
    errno = EPERM;
    return -1;
}
pid_t _getpid(void) { return 1; }
void _exit(int status) { (void)status; for (;;) { } }
void abort(void) { for (;;) { } }
/* NOTE: _write/_read/_close/_lseek already exist in arch_main.c (SDK) —
 * do not redefine them here (multiple-definition at link). */
