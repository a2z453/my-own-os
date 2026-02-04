## Minimal x86_64 “Hello World” OS (GRUB + Multiboot2)

This repo builds a tiny 64-bit kernel that prints **"Hello, OS World!"** to the VGA text buffer (white on black) and boots via **GRUB (Multiboot2)**.

### Folder structure

```
my-hobby-os/
  kernel/
    kernel.c
    entry.asm
    linker.ld
  iso_root/
    boot/
      grub/
        grub.cfg
  Makefile
```

### Build + run

```bash
make
make run
```

