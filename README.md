# Runbox

## Overview

Runbox is a lightweight Linux sandboxing system built from scratch in C. Its main purpose is educational: to help understand and implement standard sandboxing practices, process isolation, and resource management using Linux namespaces and related features.

## Table of Contents
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Supported Flags](#supported-flags)
- [Cgroups](#cgroups)
- [TODO](#todo)
- [License](#license)
- [Contributing](#contributing)


## Features

- **User Namespace:** Isolates user and group IDs for contained processes.
- **PID Namespace:** Provides a separate isolated process tree.
- **Mount Namespace:** Creates an isolated filesystem using tmpfs and mounts essential directories as read-only.
- **IPC Namespace:** Provides isolation for System V IPC objects (message queues, semaphores, shared memory) and POSIX message queues, giving the namespace its own independent set of IPC resources.
- **UTS Namespace:** Provides isolation of system identifiers like hostname and NIS domain name, giving each namespace its own values.
- **Network Namespace:** Runbox currently supports full network isolation (default) or no isolation (`--enable-network`). More advanced setups like veth pairs, custom interfaces, or controlled connectivity are planned.
- **Pivot Root:** Replaces the process’s root filesystem with an isolated one using pivot_root.
- **Minimal Shell Environment:** Launches an interactive shell inside the sandbox.
- **Limited Capabilities:** Drops powerful privileges (like `CAP_SYS_ADMIN`, `CAP_NET_ADMIN`) and keeps only safe defaults for basic operations.
- **Seccomp:** Implements a syscall allowlist filter using BPF to restrict the sandbox to essential syscalls required by the shell and filesystem operations (currently architecture-specific to aarch64).
- **Cgroups v2:** Uses cgroups v2 for limiting resource usage by the sandbox. Currently supports `cpu`, `memory` & `pids` resource limitation

## Prerequisites

- Linux (tested on Ubuntu-24.04 arm64 LTS)
- GCC (or compatible C compiler)
- `make`

## Build Instructions

```sh
make
```

## Usage

Runbox is intended to be run from the command line. After building, you can start a sandboxed shell:

```sh
./build/runbox --enable-network --disable-cgroups
# You should see a shell prompt like: runbox@root:/#
# Try running commands like 'ls', 'ps', 'ipcs' etc. inside the sandbox.
```

This will launch a shell inside an isolated environment. The root filesystem is set to `/tmp/runbox` and essential binaries are bind-mounted read-only.

## Supported Flags
Runbox supports several command-line flags for configuring the sandbox:

- `--cpu=<value>`        Limit CPU quota using cgroups v2 (use 0 for "max")
- `--memory=<value>`     Limit memory usage (supports values like 256M, 1G, or "max")
- `--pids=<value>`       Limit maximum number of processes (use "max" for no limit)
- `--enable-network`     Allow the sandbox to keep network access (disabled by default)
- `--disable-cgroups`    Disables cgroup limitations.

You can combine multiple flags:

```sh
./build/runbox --cpu=2 --memory=512M --pids=10 --enable-network
```

## Cgroups
Runbox uses a dedicated delegated cgroup subtree under `/sys/fs/cgroup/runbox/`.
Each sandbox instance creates a child cgroup for the process running as PID 1 inside the PID namespace.

Internally Runbox:
- Validates controller availability on the host
- Enables `cpu`, `memory`, and `pids` in `cgroup.subtree_control`
- Creates a per-sandbox cgroup directory
- Writes the sandbox PID to `cgroup.procs`
- Applies limits using `cpu.max`, `memory.max`, and `pids.max`

## TODO

- [ ] Full network namespace support (veth pairs, virtual interfaces, controlled connectivity)
- [x] Add support for cgroups for resource management.
- [x] Integrate seccomp or syscall filtering.

## License

MIT License. See [LICENSE](./LICENSE) for details.

## Contributing

Contributions are welcome! Please open issues or pull requests for suggestions, bug reports, or improvements.

---