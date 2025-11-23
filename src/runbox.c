#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "namespaces.h"
#include "seccomp.h"
#include "cgroup.h"

int setup_sandbox(int enable_network, struct CgroupLimits *limits) {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    // First fork: isolate namespace setup from main process
    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);

        if (setup_user_namespace() != 0) {
            return 1;
        }

        if (setup_mount_namespace() != 0) {
            return 1;
        }

        if (setup_pid_namespace() != 0) {
            return 1;
        }

        // Second fork: actually enter the PID namespace (becomes PID 1 (the init process))
        pid_t child_pid = fork();

        if (child_pid == 0) {
            close(pipefd[1]);
            close(pipefd[0]);

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
            setup_network_namespace(enable_network);
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
            if (write(pipefd[1], &child_pid, sizeof(child_pid)) != sizeof(child_pid)) {
                perror("write pid to parent");
            }

            close(pipefd[1]);

            int status;
            waitpid(child_pid, &status, 0);
            return WEXITSTATUS(status);
        } else {
            perror("fork failed");
            return 1;
        }

    } else if (pid > 0) {
        close(pipefd[1]); // parent doesn't write
        pid_t gpid;
        ssize_t n = read(pipefd[0], &gpid, sizeof(gpid));
        if (n != sizeof(gpid)) {
            printf("error while reading grandchild pid");
        }

        close(pipefd[0]);
        if (gpid > 0) {
            setup_cgroup(limits, gpid);
        }

        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        perror("fork failed");
        return 1;
    }

    return 0;
}
