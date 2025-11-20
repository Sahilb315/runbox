#define _GNU_SOURCE

#include <sys/wait.h>
#include <pty.h>
#include <utmp.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/sysmacros.h>
#include <linux/capability.h>
#include "namespaces.h"
#include "seccomp.h"

int setup_all_namespaces(int enable_network) {
    // First fork: isolate namespace setup from main process
    pid_t pid = fork();
    if (pid == 0) {
        if (setup_user_namespace() != 0) {
            return 1;
        }

        if (setup_mount_namespace() != 0) {
            return 1;
        }

        if (setup_pid_namespace() != 0) {
            return 1;
        }

        // Second fork: actually enter the PID namespace (becomes PID 1)
        pid_t child_pid = fork();
        if (child_pid == 0) {
            // Mount /proc to show processes from the new PID namespace
            if (mount("proc", "/tmp/runbox/proc", "proc", 0, NULL) == -1) {
                printf("failed mounting proc: %s\n", strerror(errno));
                return 1;
            }

            if (setup_ipc_and_uts_namespace() != 0) {
                return 1;
            }

            // Currently there is no functionality to forward ports or create a tunnel for 
            // getting network connection, so network is fully isolated
            setup_network_namespace(0);
            setup_pivot_root();

            //  To use the `SECCOMP_SET_MODE_FILTER` operation, either the calling thread must have the CAP_SYS_ADMIN
            //  capability in its user namespace, or the thread must
            //  already have the no_new_privs bit set
            if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
                perror("prctl(PR_SET_NO_NEW_PRIVS)");
                return -1;
            }

            // Drop privileged caps
            drop_bounding_caps();

            // Reset to minimal default capabilities
            apply_default_capabilities();

            setup_seccomp();

            exec_shell();
        } else if (child_pid > 0) {
            int status;
            waitpid(child_pid, &status, 0);
            return WEXITSTATUS(status);
        } else {
            perror("fork failed");
            return 1;
        }

    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        perror("fork failed");
        return 1;
    }

    return 0;
}

int setup_user_namespace(void) {
    uid_t uid = getuid();
    gid_t gid = getgid();

    if (unshare(CLONE_NEWUSER) == -1) {
        printf("unshare failed while creating user namespace: %s\n", strerror(errno));
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

    // Map current user to root (uid 0) in the namespace
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

    // Disable setgroups - required before mapping groups in user namespace
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

    return 0;
}

int setup_mount_namespace(void) {
    if (unshare(CLONE_NEWNS) == -1) {
        printf("unshare failed while creating mount namespace: %s\n", strerror(errno));
        return 1;
    }

    // Runbox root
    if (mkdir("/tmp/runbox", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating sandbox root: %s\n", strerror(errno));
            return 1;
        }
    }

    // Mount new tmpfs - creates isolated filesystem for sandbox
    if (mount("tmpfs", "/tmp/runbox", "tmpfs", 0, NULL) == -1) {
        printf("failed mounting tmpfs: %s\n", strerror(errno));
        return 1;
    }

    // Create necessary directories
    if (mkdir("/tmp/runbox/bin", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating sandbox bin: %s\n", strerror(errno));
            return 1;
        }
    }
    if (mkdir("/tmp/runbox/lib", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating sandbox lib: %s\n", strerror(errno));
            return 1;
        }
    }
    if (mkdir("/tmp/runbox/usr", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating sandbox usr: %s\n", strerror(errno));
            return 1;
        }
    }
    if (mkdir("/tmp/runbox/tmp", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating sandbox tmp: %s\n", strerror(errno));
            return 1;
        }
    }
    if (mkdir("/tmp/runbox/proc", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating sandbox proc: %s\n", strerror(errno));
            return 1;
        }
    }

    // Bind mount essential directories from host, then make read-only
    if (mount("/bin", "/tmp/runbox/bin", NULL, MS_BIND, NULL) == -1) {
        printf("failed mounting /bin: %s\n", strerror(errno));
        return 1;
    }
    mount(NULL, "/tmp/runbox/bin", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY, NULL);

    // Mount /lib
    if (mount("/lib", "/tmp/runbox/lib", NULL, MS_BIND, NULL) == -1) {
        printf("failed mounting /lib: %s\n", strerror(errno));
        return 1;
    }
    mount(NULL, "/tmp/runbox/lib", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY, NULL);

    // Mount /lib64 if it exists
    if (access("/lib64", F_OK) == 0) {
        if (mkdir("/tmp/runbox/lib64", 0755) == -1) {
            if (errno != EEXIST) {
                printf("failed creating sandbox lib64: %s\n", strerror(errno));
                return 1;
            }
        }
        if (mount("/lib64", "/tmp/runbox/lib64", NULL, MS_BIND, NULL) == -1) {
            printf("failed mounting /lib64: %s\n", strerror(errno));
        } else {
            mount(NULL, "/tmp/runbox/lib64", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY, NULL);
        }
    }

    // Mount /usr/bin if it exists
    if (access("/usr/bin", F_OK) == 0) {
        if (mkdir("/tmp/runbox/usr/bin", 0755) == -1) {
            if (errno != EEXIST) {
                printf("failed creating sandbox usr/bin: %s\n", strerror(errno));
                return 1;
            }
        }
        if (mount("/usr/bin", "/tmp/runbox/usr/bin", NULL, MS_BIND, NULL) == -1) {
            printf("failed mounting /usr/bin: %s\n", strerror(errno));
        } else {
            mount(NULL, "/tmp/runbox/usr/bin", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY, NULL);
        }
    }

    // Mount /usr/lib if it exists
    if (access("/usr/lib", F_OK) == 0) {
        if (mkdir("/tmp/runbox/usr/lib", 0755) == -1) {
            if (errno != EEXIST) {
                printf("failed creating sandbox usr/lib: %s\n", strerror(errno));
                return 1;
            }
        }
        if (mount("/usr/lib", "/tmp/runbox/usr/lib", NULL, MS_BIND, NULL) == -1) {
            printf("failed mounting /usr/lib: %s\n", strerror(errno));
        } else {
            mount(NULL, "/tmp/runbox/usr/lib", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY, NULL);
        }
    }

    // Create a writable tmp
    if (mount("tmpfs", "/tmp/runbox/tmp", "tmpfs", 0, NULL) == -1) {
        printf("failed mounting tmp tmpfs: %s\n", strerror(errno));
    }

    return 0;
}

int setup_pid_namespace(void) {
    if (unshare(CLONE_NEWPID) == -1) {
        printf("unshare failed while creating pid namespace: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int setup_network_namespace(int enable_network) {
    if (unshare(CLONE_NEWNET) == -1) {
        printf("unshare failed while creating net namespace: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int setup_ipc_and_uts_namespace(void) {
    if (unshare(CLONE_NEWUTS | CLONE_NEWIPC) == -1) {
        printf("unshare failed while creating uts & ipc namespace: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int setup_pivot_root(void) {
    // Create the old_root directory for pivot_root
    if (mkdir("/tmp/runbox/old_root", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating old_root directory: %s\n", strerror(errno));
            return 1;
        }
    }

    // Atomically switch to new root and store old root in old_root
    if (syscall(SYS_pivot_root, "/tmp/runbox", "/tmp/runbox/old_root") == -1) {
        printf("failed to pivot root: %s\n", strerror(errno));
        return 1;
    }

    if (chdir("/") == -1) {
        printf("failed to chdir to /: %s\n", strerror(errno));
        return 1;
    }

    // Clean up old root for security - remove access to host filesystem
    if (umount2("/old_root", MNT_DETACH) == -1) {
        printf("failed to unmount old root: %s\n", strerror(errno));
    }

    if (rmdir("/old_root") == -1) {
        printf("failed to remove old_root dir: %s\n", strerror(errno));
    }

    return 0;
}

int drop_bounding_caps() {
    int caps_to_drop[] = { CAP_SYS_ADMIN, CAP_NET_ADMIN, CAP_SYS_PTRACE, CAP_SYS_MODULE };
    for (size_t i = 0; i < sizeof(caps_to_drop)/sizeof(caps_to_drop[0]); i++) {
        prctl(PR_CAPBSET_DROP, caps_to_drop[i], 0, 0, 0);
    }
    return 0;
}

int apply_default_capabilities() {
    struct __user_cap_header_struct cap_header;
    struct __user_cap_data_struct cap_data[2];

    // Clear out any garage values if present (just a safety step)
    memset(&cap_header, 0, sizeof(cap_header));
    memset(&cap_data, 0, sizeof(cap_data));

    cap_header.version = _LINUX_CAPABILITY_VERSION_3;
    cap_header.pid = 0; // set caps for current process (0)

    unsigned long default_caps[] = {CAP_CHOWN, CAP_DAC_OVERRIDE, CAP_FSETID, CAP_FOWNER, 
         CAP_NET_RAW, CAP_SETGID, CAP_SETUID, CAP_SETFCAP, CAP_SETPCAP, CAP_NET_BIND_SERVICE, CAP_SYS_CHROOT,
         CAP_KILL, CAP_AUDIT_WRITE};

    for (size_t i = 0; i < sizeof(default_caps) / sizeof(default_caps[0]); i++) {
        int cap = default_caps[i];

        if (cap < 32) {
            cap_data[0].effective |= (1U << cap);
            cap_data[0].permitted |= (1U << cap);
        } else {
            cap_data[1].effective |= (1U << (cap - 32));
            cap_data[1].permitted |= (1U << (cap - 32));
        }
    }

    if (syscall(SYS_capset, &cap_header, &cap_data) < 0) {
        perror("capset failed");
        return -1;
    }

    return 0;
}

int exec_shell(void) {
    // Set up basic environment
    setenv("PATH", "/bin:/usr/bin", 1);
    setenv("HOME", "/tmp", 1);
    setenv("PS1", "runbox@root:\\w# ", 1);

    // Try different shells in order of preference
    if (access("/bin/bash", X_OK) == 0) {
        execl("/bin/bash", "bash", NULL);
    } else if (access("/bin/sh", X_OK) == 0) {
        execl("/bin/sh", "sh", NULL);
    } else if (access("/usr/bin/bash", X_OK) == 0) {
        execl("/usr/bin/bash", "bash", NULL);
    } else if (access("/usr/bin/sh", X_OK) == 0) {
        execl("/usr/bin/sh", "sh", NULL);
    } else {
        printf("No suitable shell found in sandbox\n");
        return 1;
    }

    printf("Failed to exec shell: %s\n", strerror(errno));
    return 1;
}
