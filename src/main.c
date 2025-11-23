#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

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
                limits.memory_max = strdup(optarg);
                break;

            case 3:
                limits.cpu_enabled = 1;
                limits.cpus = atof(optarg);
                break;

            case 4:
                limits.pids_enabled = 1;

                if (strcmp(optarg, "max") == 0) {
                    limits.pids_max = -2;
                } else {
                    limits.pids_max = atoi(optarg);
                }
                break;

            case '?':
            default:
                fprintf(stderr, "Unknown option.\n");
                return 1;
        }
    }

    return setup_sandbox(enable_network, &limits);
}

