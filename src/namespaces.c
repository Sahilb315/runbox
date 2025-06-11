#define _GNU_SOURCE

#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include "namespaces.h"

int setup_user_namespace(void) {
    uid_t uid = getuid();
    gid_t gid = getgid();

    if (unshare(CLONE_NEWUSER) == -1) {
        printf("unshare failed: %s\n", strerror(errno));
        return 1;
    }

    // Check if the kernel actually allows user namespaces
    if (access("/proc/self/uid_map", W_OK) == -1) {
        perror("User namespaces may be disabled by sysctl");
        return 1;
    }

    int uid_fd = open("/proc/self/uid_map", O_WRONLY);
    if (uid_fd == -1) {
        printf("uid_map open failed: %s\n", strerror(errno));
        return 1;
    }

    int gid_fd = open("/proc/self/gid_map", O_WRONLY);
    if (gid_fd == -1) {
        printf("gid_map open failed: %s\n", strerror(errno));
        close(uid_fd);
        return 1;
    }

    // Write the mappings
    if (dprintf(uid_fd, "0 %d 1", uid) < 0) {
        printf("uid_map write failed: %s\n", strerror(errno));
        close(uid_fd);
        close(gid_fd);
        return 1;
    }

    int setgroups_fd = open("/proc/self/setgroups", O_WRONLY);
    if (setgroups_fd == -1) {
        printf("setgroups open failed: %s\n", strerror(errno));
        close(uid_fd);
        close(gid_fd);
        return 1;
    }

    if (write(setgroups_fd, "deny", 4) != 4) {
        printf("setgroups write failed: %s\n", strerror(errno));
        close(setgroups_fd);
        return 1;
    }

    close(setgroups_fd);

    if (dprintf(gid_fd, "0 %d 1", gid) < 0) {
        printf("gid_map write failed: %s\n", strerror(errno));
        close(uid_fd);
        close(gid_fd);
        return 1;
    }

    close(gid_fd);
    close(uid_fd);

    printf("User Namespace created successfully!!");
    return 0;
}
