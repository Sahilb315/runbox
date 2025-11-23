#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "cgroup.h"
#include "runbox.h"

int main(int argc, char **argv) {
    int enable_network = 0;

    struct CgroupLimits limits = {0};

    static struct option long_opts[] = {
        {"enable-network", no_argument,       0, 1},
        {"memory",         required_argument, 0, 2},
        {"cpu",            required_argument, 0, 3},
        {"pids",           required_argument, 0, 4},
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "", long_opts, &long_index)) != -1) {
        switch (opt) {
            case 1:
                enable_network = 1;
                break;

            case 2:
                limits.memory_enabled = 1;
                limits.memory_max = optarg;
                break;

            case 3:
                limits.cpu_enabled = 1;
                double val = atof(optarg);
                if (val <= 0) {
                    fprintf(stderr, "Invalid value for --cpu: '%s'. Must be a positive number.\n", optarg);
                    return -1;
                }
                limits.cpus = val;
                break;

            case 4:
                limits.pids_enabled = 1;

                if (strcmp(optarg, "max") == 0) {
                    limits.pids_max = PIDS_MAX_ALIAS;
                } else {
                    int ret = atoi(optarg);
                    if (ret <= 0) {
                        fprintf(stderr, "Invalid value for --pids: '%s'. Must be a positive number.\n", optarg);
                        return -1;
                    }
                    limits.pids_max = ret;
                }
                break;

            case '?':
            default:
                fprintf(stderr, "Unknown option.\n");
                return -1;
        }
    }

    return setup_sandbox(enable_network, &limits);
}

