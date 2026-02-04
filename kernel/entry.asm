; entry.asm - Multiboot2 + GRUB entry that transitions to long mode.
;
; GRUB loads us in 32-bit protected mode (Multiboot2), *not* in long mode.
; We provide:
; - A Multiboot2 header (so GRUB recognizes the kernel)
; - A 32-bit entry that sets up paging + long mode
; - A far jump into 64-bit code, sets up a stack, then calls kmain()

; -------------------------
; Multiboot2 header
; -------------------------
BITS 32

section .multiboot2
align 8
multiboot2_header:
    dd 0xE85250D6              ; magic
    dd 0                       ; architecture (0 = i386)
    dd multiboot2_header_end - multiboot2_header ; header length
    dd -(0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header)) ; checksum

    ; End tag
    dw 0
    dw 0
    dd 8
multiboot2_header_end:

; -------------------------
; 32-bit entry point
; -------------------------
section .text
global _start
extern kmain

_start:
    cli

    ; Set up a temporary stack in 32-bit mode.
    mov esp, stack32_top

    ; Build and load minimal 64-bit GDT.
    lgdt [gdt64_ptr]

    ; Enable PAE (CR4.PAE = 1).
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Load PML4 physical address into CR3.
    mov eax, pml4
    mov cr3, eax

    ; Enable long mode (EFER.LME = 1).
    mov ecx, 0xC0000080         ; IA32_EFER
    rdmsr
    or eax, 1 << 8              ; LME
    wrmsr

    ; Enable paging (CR0.PG = 1) and protection already enabled by GRUB.
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Far jump to 64-bit code segment to enter long mode.
    jmp 0x08:long_mode_start

; -------------------------
; 64-bit entry
; -------------------------
BITS 64
long_mode_start:
    ; Load data segments (mostly ignored in long mode, but set them sane).
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    cld                    ; Ensure direction flag is clear for C code

    ; Set up 64-bit stack (16-byte aligned).
    lea rsp, [rel stack64_top]
    and rsp, -16

    ; Pass Multiboot2 info to kmain:
    ;   RDI = multiboot2 info pointer (64-bit)
    ;   ESI = multiboot2 magic (32-bit, zero-extended)
    mov rdi, [rel mb_info_ptr]
    mov esi, [rel mb_magic]
    xor edx, edx           ; Clear upper 32 bits of RDX

    call kmain

.hang:
    hlt
    jmp .hang

; -------------------------
; GDT (64-bit)
; -------------------------
BITS 32
align 8
gdt64:
    dq 0x0000000000000000       ; null
    dq 0x00AF9A000000FFFF       ; 0x08: 64-bit code segment
    dq 0x00CF92000000FFFF       ; 0x10: data segment (L=0)
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dd gdt64

; -------------------------
; Page tables (identity map first 2 MiB using a 2 MiB page)
; -------------------------
align 4096
pml4:
    dq pdpt + 0x003             ; present + writable
    times 511 dq 0

align 4096
pdpt:
    dq pd + 0x003               ; present + writable
    times 511 dq 0

align 4096
pd:
    dq 0x00000000 + 0x083       ; 2 MiB page: present+writable+PS
    times 511 dq 0

; -------------------------
; Stacks
; -------------------------
section .bss
align 8
mb_info_ptr:
    resq 1
mb_magic:
    resd 1

align 16
stack32_bottom:
    resb 4096
stack32_top:

align 16
stack64_bottom:
    resb 16384                  ; 16 KiB
stack64_top:
