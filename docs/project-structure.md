# Project Structure

```
my-own-os/
├── arch/
│   └── x86_64/
│       └── boot/
│           └── start.asm
├── build/                 # Build outputs (generated)
├── docs/
│   └── project-structure.md
├── include/
│   ├── arch/
│   │   └── x86_64/
│   │       └── io.h
│   └── kernel/
│       ├── panic.h
│       └── serial.h
├── iso/                   # ISO staging (generated)
├── kernel/
│   └── src/
│       ├── kernel.c
│       ├── panic.c
│       └── serial.c
├── limine.cfg
├── linker.ld
└── Makefile
```

## Directory Overview

- `arch/x86_64/`: Architecture-specific assembly and low-level boot artifacts.
- `include/`: Public kernel headers, split between architecture-specific and generic.
- `kernel/src/`: C sources for core kernel subsystems.
- `docs/`: Design notes and project documentation.
- `build/` and `iso/`: Generated build artifacts and ISO staging area.
