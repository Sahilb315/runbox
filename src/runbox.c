#include "namespaces.h"
#include "seccomp.h"

int setup_sandbox() {
    setup_all_namespaces(1);
    return 0;
}
