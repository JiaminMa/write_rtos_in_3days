
.text
.code 16
.global main
.global reset_handler
.global _p_stack_top
.global get_psp
.global get_msp
.global get_control_reg

reset_handler:

    /*Set the stack as process stack*/
    mov r0, #33
    mrs r0, CONTROL
    mov r1, #2
    orr r0, r1
    msr CONTROL, r0

    ldr r0, =_p_stack_top
    mov sp, r0

    ldr r0, =main
    blx r0
    b .

get_psp:
    mrs r0, PSP
    blx lr

get_msp:
    mrs r0, MSP
    blx lr

get_control_reg:
    mrs r0, CONTROL
    blx lr
