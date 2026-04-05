    .section .text._start
    .globl _start
_start:
    la gp, __global_pointer$

    la sp, __stack_top
    addi sp, sp, -16
    andi sp, sp, -16

    la t0, __bss_start
    la t1, __bss_end
1:
    beq t0, t1, 2f
    sw x0, 0(t0)
    addi t0, t0, 4
    j 1b
2:
    la t0, __sbss_start
    la t1, __sbss_end
3:
    beq t0, t1, 4f
    sw x0, 0(t0)
    addi t0, t0, 4
    j 3b
4:
    call main

hang:
    j hang
