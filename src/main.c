#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

int setup_sandbox(int enable_network);

int main(int argc, char **argv) {
    int enable_network = 0;

    static struct option long_opts[] = {
        {"enable-network", no_argument, 0, 1},  // return val = 1
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "", long_opts, &long_index)) != -1) {
        switch (opt) {
            case 1:  // --enable-network
                enable_network = 1;
                break;

            case '?':   // unknown long option
            default:
                fprintf(stderr, "Unknown option. Allowed: --enable-network\n");
                return 1;
        }
    }

    return setup_sandbox(enable_network);
}
