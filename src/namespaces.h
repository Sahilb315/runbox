// namespaces.h

#ifndef NAMESPACES_H
#define NAMESPACES_H

int setup_user_namespace(void);
int setup_mount_namespace(void);
int setup_pid_namespace(void);
int setup_network_namespace(int enable_network);
int setup_ipc_namespace(void);
int setup_uts_namespace(void);
int setup_all_namespaces(int enable_network);

int exec_shell(void);
int setup_pivot_root(void);

#endif // NAMESPACES_H
