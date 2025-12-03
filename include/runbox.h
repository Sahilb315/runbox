// runbox.h

#ifndef RUNBOX_H
#define RUNBOX_H

#include "cgroup.h"

struct Config {
    int enable_network;
    int disable_cgroups;
};

int setup_sandbox(struct Config *config, struct CgroupLimits *limits);

#endif
