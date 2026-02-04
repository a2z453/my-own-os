[BITS 64]
[SECTION .text]

global _start
extern kmain

_start:
    cli
    call kmain
.hang:
    hlt
    jmp .hang
