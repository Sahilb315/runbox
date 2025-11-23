// runbox.h

#ifndef RUNBOX_H
#define RUNBOX_H

#include "cgroup.h"

int setup_sandbox(int enable_network, struct CgroupLimits *limits);

#endif
