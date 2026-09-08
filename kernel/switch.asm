[bits 32]
section .text
global switch_context
global timer_isr_stub
extern timer_handler

; =============================================================================
; switch_context(uint32_t *old_esp, uint32_t new_esp)
; =============================================================================
switch_context:
    pushfd
    pushad

    mov eax, [esp + 40]     ; old_esp pointer
    test eax, eax
    jz .skip_save
    mov [eax], esp          ; *old_esp = current ESP

.skip_save:
    mov esp, [esp + 44]     ; ESP = new_esp

    popad
    popfd
    ret

; =============================================================================
; timer_isr_stub - Called every 10ms by hardware Timer IRQ0
; =============================================================================
timer_isr_stub:
    pushad
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10            ; Kernel data segment selector
    mov ds, ax
    mov es, ax

    call timer_handler

    pop gs
    pop fs
    pop es
    pop ds
    popad
    iret                    ; Return from interrupt
