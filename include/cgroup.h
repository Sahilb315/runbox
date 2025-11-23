// cgroup.h

#ifndef CGROUP_H
#define CGROUP_H

#include <unistd.h>
#define MAX_CPU_LIMIT 1024
#define PIDS_MAX_ALIAS -2

struct CgroupLimits {
    int  cpu_enabled;
    double cpus;

    char *memory_max;
    int  memory_enabled;

    int pids_max;
    int pids_enabled;
};

int setup_cgroup(struct CgroupLimits *limits, pid_t child_pid);

#endif
