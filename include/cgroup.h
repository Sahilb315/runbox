// cgroup.h

#ifndef CGROUP_H
#define CGROUP_H

#include <unistd.h>
#define MAX_CPU_LIMIT 1024
#define PIDS_MAX_ALIAS -2

/**
 * CgroupLimits - Structure to specify resource limits for a cgroup.
 *
 * Fields:
 *   cpu_enabled    - Whether CPU controller is enabled (1 = enabled, 0 = disabled).
 *   cpus           - Number of CPUs allowed (floating-point value, e.g., 1.5 for 1.5 CPUs).
 *
 *   memory_max     - Maximum memory limit (string, e.g., "256M", "1G", or "max" for unlimited).
 *   memory_enabled - Whether memory controller is enabled (1 = enabled, 0 = disabled).
 *
 *   pids_max       - Maximum number of processes (integer).
 *                   Special value: -2 means "max" (unlimited).
 *   pids_enabled   - Whether pids controller is enabled (1 = enabled, 0 = disabled).
 */
struct CgroupLimits {
    int  cpu_enabled;     // 1 if CPU controller is enabled, 0 otherwise
    double cpus;          // Number of CPUs allowed (floating-point value recommended, e.g., 1.5)

    char *memory_max;     // Maximum memory limit (e.g., "256M", "1G", or "max" for unlimited)
    int  memory_enabled;  // 1 if memory controller is enabled, 0 otherwise

    int pids_max;         // Maximum number of processes; -2 means "max" (unlimited)
    int pids_enabled;     // 1 if pids controller is enabled, 0 otherwise
};

int setup_cgroup(struct CgroupLimits *limits, pid_t child_pid);

#endif
