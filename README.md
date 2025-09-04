# Runbox

## Overview

Runbox is a lightweight Linux sandboxing system built from scratch in C. Its main purpose is educational: to help understand and implement standard sandboxing practices, process isolation, and resource management using Linux namespaces and related features.

## Features

- **User Namespace:** Isolates user and group IDs for contained processes.
- **PID Namespace:** Provides a separate process tree, making the sandboxed process PID 1.
- **Mount Namespace:** Creates an isolated filesystem using tmpfs and bind mounts for essential directories.
- **Network Namespace:** (In progress) Will provide network isolation for sandboxed processes.
- **Pivot Root:** Uses `pivot_root` to fully isolate the filesystem from the host.
- **Minimal Shell Environment:** Launches a shell (`/bin/bash` or `/bin/sh`) inside the sandbox.

## Prerequisites

- Linux (tested on Ubuntu 22.04.5 LTS)
- GCC (or compatible C compiler)
- `make`

## Build Instructions

```sh
make
```

## Usage

Runbox is intended to be run from the command line. After building, you can start a sandboxed shell:

```sh
./runbox
```

This will launch a shell inside an isolated environment. The root filesystem is set to `/tmp/runbox` and essential binaries are bind-mounted read-only.

## Example

```sh
./runbox
# You should see a shell prompt like: runbox@root:/#
# Try running commands like 'ls', 'ps', etc. inside the sandbox.
```

## Limitations & TODO

- Network namespace is not fully implemented.
- No resource limits (CPU, memory, etc.) yet.
- No seccomp or syscall filtering.
- No support for custom command execution (currently launches a shell).
- No persistent storage; everything is ephemeral in `/tmp/runbox`.

## License

MIT License. See [LICENSE](./LICENSE) for details.

## Contributing

Contributions are welcome! Please open issues or pull requests for suggestions, bug reports, or improvements.

---

**Note:** This project is for educational purposes. 
