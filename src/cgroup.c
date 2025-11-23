#include "cgroup.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Steps to implement cgroup: 
// Validate the limits 
// Validate if the controllers are allowed/available in cgroup.controllers
// Enable the available controllers in cgroup.subtree_control
// Create group for the sandbox
// Apply the limits for the specified controllers
// Make the sandbox's process to use the group we created

/* 
Mount cgroup v2 (/sys/fs/cgroup)

mkdir /sys/fs/cgroup/runbox

echo "+cpu +memory +pids" > /sys/fs/cgroup/cgroup.subtree_control

mkdir /sys/fs/cgroup/runbox/<sandbox-pid>

Now the <sandbox-pid> dir will have controller files.

Write the limits to /sys/fs/cgroup/runbox/<sandbox-pid>/<controller-file>

Write the sandbox-pid to the /sys/fs/cgroup/runbox/<sandbox-pid>/cgroup.procs
*/
int setup_cgroup(struct CgroupLimits *limits, pid_t child_pid) {

}

int create_and_apply_limits(struct CgroupLimits *limits, pid_t child_pid) {
    char path[256];
    snprintf(path, sizeof(path), "/sys/fs/cgroup/runbox/%d", child_pid);

    if (mkdir(path, 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating runbox cgroup limit for %d: %s\n", child_pid, strerror(errno));
            return 1;
        }
    }

    if (limits->cpu_enabled) {
        char cpu_max[64];

        if (limits->cpus == 0) {
            // Unlimited CPU
            snprintf(cpu_max, sizeof(cpu_max), "max");
        } else {
            int period = 100000;
            int quota = (int)(limits->cpus * period);

            if (quota <= 0)
                quota = 1;  // safety

            snprintf(cpu_max, sizeof(cpu_max), "%d %d", quota, period);
        }

        snprintf(path, sizeof(path),
                "/sys/fs/cgroup/runbox/%d/cpu.max", child_pid);

        if (write_file(path, cpu_max) != 0) {
            printf("Failed to write cpu.max\n");
            return 1;
        }
    }

    if (limits->memory_enabled) {
        snprintf(path, sizeof(path),
                "/sys/fs/cgroup/runbox/%d/memory.max", child_pid);

        if (write_file(path, limits->memory_max) != 0) {
            printf("Failed to write memory.max\n");
            return 1;
        }
    }

    if (limits->pids_enabled) {
        char pids_val[32];

        if (limits->pids_max == -2) {
            snprintf(pids_val, sizeof(pids_val), "max");
        } else {
            snprintf(pids_val, sizeof(pids_val), "%d", limits->pids_max);
        }

        snprintf(path, sizeof(path),
                "/sys/fs/cgroup/runbox/%d/pids.max", child_pid);

        if (write_file(path, pids_val) != 0) {
            printf("Failed to write pids.max\n");
            return 1;
        }
    }

    char pidbuf[32];
    snprintf(path, sizeof(path), "/sys/fs/cgroup/runbox/%d/cgroup.procs", (int)child_pid);
    snprintf(pidbuf, sizeof(pidbuf), "%d", (int)child_pid);


    if (write_file(path, pidbuf) != 0) {
        printf("Failed to write cgroup.procs\n");
        return 1;
    }

    return 0;
}

int validate_and_enable_host_controllers(struct CgroupLimits *limits) {
    char buffer[1024];

    // Read the list of controllers supported by the host
    if (read_file("/sys/fs/cgroup/cgroup.controllers", buffer, sizeof(buffer)) != 0) {
        printf("Failed to read cgroup.controllers\n");
        return -1;
    }

    char enable_buf[256];
    enable_buf[0] = '\0';

    // CPU controller
    if (limits->cpu_enabled) {
        if (!contains_controller(buffer, "cpu")) {
            printf("CPU controller not supported on this system\n");
            return -1;
        }

        strcat(enable_buf, "+cpu ");
    }

    // Memory controller
    if (limits->memory_enabled) {
        if (!contains_controller(buffer, "memory")) {
            printf("Memory controller not supported on this system\n");
            return -1;
        }

        strcat(enable_buf, "+memory ");
    }

    // PIDs controller
    if (limits->pids_enabled) {
        if (!contains_controller(buffer, "pids")) {
            printf("PIDs controller not supported on this system\n");
            return -1;
        }

        strcat(enable_buf, "+pids ");
    }

    // If nothing to enable, all good
    if (enable_buf[0] == '\0')
        return 0;

    // 5. Enable controllers by writing to cgroup.subtree_control
    if (write_file("/sys/fs/cgroup/cgroup.subtree_control", enable_buf) != 0) {
        printf("Failed to enable controllers in host subtree_control\n");
        return -1;
    }

    return 0;
}

int create_sandbox_cgroup_and_enable_controllers(struct CgroupLimits *limits) {
    if (mkdir("/sys/fs/cgroup/runbox", 0755) == -1) {
        if (errno != EEXIST) {
            printf("failed creating runbox cgroup: %s\n", strerror(errno));
            return 1;
        }
    }

    char enable_buf[256];
    enable_buf[0] = '\0';

    if (limits->cpu_enabled) {
        strcat(enable_buf, "+cpu ");
    }

    if (limits->memory_enabled) {
        strcat(enable_buf, "+memory ");
    }

    if (limits->pids_enabled) {
        strcat(enable_buf, "+pids ");
    }

    // If nothing to enable, all good
    if (enable_buf[0] == '\0')
        return 0;

    // 5. Enable controllers by writing to cgroup.subtree_control
    if (write_file("/sys/fs/cgroup/runbox/cgroup.subtree_control", enable_buf) != 0) {
        printf("Failed to enable controllers in runbox cgroup subtree_control\n");
        return -1;
    }

    return 0;
}

int validate_cgroup_limits(struct CgroupLimits *limits) {
    if (limits->cpu_enabled && validate_cpu_max(limits->cpu_max)) {
        printf("Invalid cpu.max\n");
        return 1;
    }

    if (limits->memory_enabled && validate_memory_max(limits->memory_max)) {
        printf("Invalid memory.max\n");
        return 1;
    }

    if (limits->pids_enabled && validate_pids_max(limits->pids_max)) {
        printf("Invalid pids.max\n");
        return 1;
    }

    return 0;
}

int validate_cpu_max(int cpu) {
    // if cpu == 0, it refers to "max"
    if (cpu < 0) return 1;

    if (cpu > 1024) return 1;
    
    return 0;
}

int validate_memory_max(const char *mem) {
    if (!mem) return 0;

    if (strcmp(mem, "max") == 0)
        return 0;

    // Example: `mem` = 256M, `end` = M, `val` = 256
    char *end;
    long val = strtol(mem, &end, 10);

    if (val <= 0) return 1;

    if (*end == 0 || strcmp(end, "K")==0 || strcmp(end, "M")==0 || strcmp(end, "G")==0)
        return 0;

    return 1;
}

int validate_pids_max(int pids) {
    if (pids == -2) // for max
        return 0;

    if (pids == 0)
        return 1; // 0 means "no process allowed" → wrong

    if (pids > 0)
        return 0;

    return 1;
}

int write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Error opening %s: %s\n", path, strerror(errno));
        return 1;
    }

    if (fprintf(f, "%s", text) < 0) {
        printf("Error writing to %s: %s\n", path, strerror(errno));
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}

int read_file(const char *path, char *buffer, size_t size) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Error reading %s: %s\n", path, strerror(errno));
        return 1;
    }

    size_t n = fread(buffer, 1, size - 1, f);
    buffer[n] = '\0';

    fclose(f);
    return 0;
}

int contains_controller(const char *enabled_controllers, const char *controller) {
    const char *p = enabled_controllers;

    size_t wlen = strlen(controller);

    while ((p = strstr(p, controller)) != NULL) {
        // ensure it's a full controller boundary (not inside another token)
        if ((p == enabled_controllers || p[-1] == ' ') &&
            (p[wlen] == '\0' || p[wlen] == ' ' || p[wlen] == '\n')) {
            return 1;
        }
        p += wlen;
    }

    return 0;
}
