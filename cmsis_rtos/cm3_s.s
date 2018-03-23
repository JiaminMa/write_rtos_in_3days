
.text
.code 16
.syntax unified
/*Export*/
.global reset_handler
.global _p_stack_top
.global get_psp
.global set_psp
.global get_msp
.global get_control_reg
.global pendsv_handler
.global set_primask
.global get_primask
.global disable_irq
.global enable_irq

/*Import*/
.global g_current_task
.global g_next_task
.global main

reset_handler:

    /*Set the stack as process stack*/
    mov     r0, #33
    mrs     r0, CONTROL
    mov     r1, #2
    orr     r0, r1
    msr     CONTROL, r0

    ldr     r0, =_p_stack_top
    mov     sp, r0

    ldr     r0, =main
    blx     r0
    b .

get_psp:
    mrs     r0, PSP
    blx     lr

set_psp:
    msr     PSP, r0
    blx     lr

get_msp:
    mrs     r0, MSP
    blx     lr

get_control_reg:
    mrs     r0, CONTROL
    blx     lr

pendsv_handler:
    /*CM3 will push the r0-r3, r12, r14, r15, xpsr by hardware*/
    mrs     r0, psp
    cbz     r0, pendsv_handler_nosave

    /* g_current_task->psp-- = r11;
     * ...
     * g_current_task->psp-- = r4;
     * g_current_task->stack = psp;
     */
    stmdb   r0!, {r4-r11}
    ldr     r1, =g_current_task
    ldr     r1, [r1]
    str     r0, [r1]

pendsv_handler_nosave:

    /* *g_current_task = *g_next_task */
    ldr     r0, =g_current_task
    ldr     r1, =g_next_task
    ldr     r2, [r1]
    str     r2, [r0]

    /*r0 = g_current_task->stack*/
    ldr     r0, [r2]
    ldmia   r0!, {r4-r11}

    msr     psp, r0
    orr     lr, lr, #0x04   /*Swtich to PSP*/
    bx      lr

get_primask:
    mrs     r0, PRIMASK
    blx     lr

set_primask:
    msr     PRIMASK, r0
    blx     lr

disable_irq:
    cpsid   i
    blx     lr

enable_irq:
    cpsie   i
    blx     lr
