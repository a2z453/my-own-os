; interrupts.asm - x86_64 ISR stubs for our IDT
;
; We implement:
; - isr0: Divide-by-zero exception (#DE, vector 0)
; - isr13: General Protection Fault (#GP, vector 13)
; - isr14: Page Fault (#PF, vector 14)
; - isr_common: saves registers, calls C isr_handler(ctx), restores, iretq

BITS 64

global isr0, isr13, isr14, isr33
extern isr_handler
extern keyboard_irq_handler

section .text

isr0:
    ; Exceptions without an error code: push a dummy error code
    push qword 0          ; error
    push qword 0          ; vector
    jmp isr_common

isr13:
    ; General Protection Fault - has error code pushed by CPU
    push qword 13         ; vector
    jmp isr_common_with_error

isr14:
    ; Page Fault - has error code pushed by CPU
    push qword 14         ; vector
    jmp isr_common_with_error

isr33:
    ; IRQ 1 (Keyboard) - no error code
    push qword 0          ; error
    push qword 33         ; vector
    jmp isr_common

; For exceptions that already push an error code
isr_common_with_error:
    ; stack: ... [error][vector]
    jmp isr_common

isr_common:
    ; Save general purpose registers.
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Pass pointer to struct isr_context in RDI (SysV ABI).
    mov rdi, rsp

    ; Call C handler.
    call isr_handler

    ; Restore registers.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Drop vector + error code.
    add rsp, 16

    iretq

