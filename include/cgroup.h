// cgroup.h

#ifndef CGROUP_H
#define CGROUP_H

#include <unistd.h>

struct CgroupLimits {
    char *cpu_max;
    int  cpu_enabled;
    int  cpus;

    char *memory_max;
    int  memory_enabled;

    int pids_max;
    int pids_enabled;
};

int setup_cgroup(struct CgroupLimits *limits, pid_t child_pid);

int create_and_apply_limits(struct CgroupLimits *limits, pid_t child_pid);
int validate_and_enable_host_controllers(struct CgroupLimits *limits);
int create_sandbox_cgroup_and_enable_controllers(struct CgroupLimits *limits);
int validate_cgroup_limits(struct CgroupLimits *limits);
int validate_cpu_max(int cpu);
int validate_memory_max(const char *mem);
int validate_pids_max(int pids);
int write_file(const char *path, const char *text);
int read_file(const char *path, char *buffer, size_t size);
int contains_controller(const char *enabled_controllers, const char *controller);

#endif
