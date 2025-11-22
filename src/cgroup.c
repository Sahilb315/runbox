#include "cgroup.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

// Steps to implement cgroup: 
// Validate the limits 
// Check if the controllers are allowed/available in cgroup.controllers
// Enable the available controllers in cgroup.subtree_control
// Create group for the sandbox
// Apply the limits for the specified controllers
// Make the sandbox's process to use the group we created

int setup_cgroup(struct CgroupLimits *limits, pid_t child_pid) {
    // Enable controllers 
    write_file("/sys/fs/cgroup/cgroup.subtree_control", "+cpu +memory +pids");

    // Create group for this sandbox
    char path[256];
    snprintf(path, sizeof(path), "/sys/fs/cgroup/runbox/%d", child_pid);
    mkdir(path, 0755);
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

int validate_cpu_max(const char *cpu) {
    if (cpu == NULL) return 0;  // not set, valid

    if (strcmp(cpu, "max") == 0)
        return 0;

    long quota, period;
    if (sscanf(cpu, "%ld %ld", &quota, &period) != 2)
        return 1; // invalid format

    if (quota < 0 || period <= 0)
        return 1;

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
    if (pids == -1)
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
