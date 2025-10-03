#include "rtt_utils.h"

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int _write(int file, char *ptr, int len)
{
    (void)file;
    return SEGGER_RTT_Write(RTT_BUFFER_DEFAULT, ptr, len);
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    return SEGGER_RTT_Read(RTT_BUFFER_DEFAULT, ptr, len);
}

int _isatty(int fd)
{
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
        return 1;
    }

    errno = EBADF;
    return 0;
}

int _close(int fd)
{
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
        return 0;
    }

    errno = EBADF;
    return -1;
}

int _lseek(int fd, int ptr, int dir)
{
    (void)fd;
    (void)ptr;
    (void)dir;

    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
        st->st_mode = S_IFCHR;
        return 0;
    }

    errno = EBADF;
    return 0;
}

int _getpid(void)
{
    errno = ENOSYS;
    return -1;
}

int _kill(pid_t pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}
