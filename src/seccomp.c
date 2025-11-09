#define _GNU_SOURCE
#include "seccomp.h"
#include <sys/syscall.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <linux/audit.h>
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <linux/unistd.h>
#include <errno.h>

#define ALLOW_SYSCALL(name) \
    BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, __NR_##name, 0, 1), \
    BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW)

int setup_seccomp(int enable_strict_mode) {
    if (enable_strict_mode)
        return syscall(SYS_seccomp, SECCOMP_SET_MODE_STRICT, 0, NULL);

    struct sock_filter filters[] = {
        BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, nr)),
        #include "seccomp_allowlist.h"
        BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS),
    };

    struct sock_fprog fprog = {
        .len = (unsigned short)(sizeof(filters) / sizeof(filters[0])),
        .filter = filters,
    };

    if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &fprog) != 0) {
        perror("seccomp");
        return -1;
    }

    return 0;
}
