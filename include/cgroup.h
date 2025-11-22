// cgroup.h

#ifndef CGROUP_H
#define CGROUP_H

struct CgroupLimits {
    char *cpu_max;      // e.g. "50000 100000" or "max"
    int  cpu_enabled;     // 1 = user wants cpu controller

    char *memory_max;
    int  memory_enabled;

    int pids_max;
    int pids_enabled;
};

int setup_cgroup(struct CgroupLimits *limits, pid_t child_pid);

#endif
