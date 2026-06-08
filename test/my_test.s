    .text
    .globl _start
_start:
    lui     a0, 256
    auipc   a1, 0
    jal     ra, func
    beq     a0, a1, blahblahblah
    bne     a2, a3, 1f
    blt     a4, a5, blahblahblah
    bge     s0, s1, blahblahblah
    bltu    t1, t2, blahblahblah
    bgeu    s2, s3, blahblahblah
    lb      a0, 0(sp)
    lh      a1, 2(sp)
    lw      a2, 4(sp)
    lbu     a3, 1(sp)
    lhu     a4, 2(sp)
    sb      a0, 0(sp)
    sh      a1, 2(sp)
    sw      a2, 4(sp)
    addi    a0, a1, -5
    slti    a0, a1, 7
    sltiu   a0, a1, 8
    xori    a0, a1, 255
    ori     a0, a1, 16
    andi    a0, a1, 1
    slli    a0, a1, 3
    srli    a0, a1, 4
    srai    a0, a1, 2
    add     a0, a1, a2
    sub     a0, a1, a2
    sll     a0, a1, a2
    slt     a0, a1, a2
    sltu    a0, a1, a2
    xor     a0, a1, a2
    srl     a0, a1, a2
    sra     a0, a1, a2
    or      a0, a1, a2
    and     a0, a1, a2
    mul     a0, a1, a2
    mulh    a0, a1, a2
    mulhsu  a0, a1, a2
    mulhu   a0, a1, a2
    div     a0, a1, a2
    divu    a0, a1, a2
    rem     a0, a1, a2
    remu    a0, a1, a2
    jalr    zero, 0(ra)
    ecall
    ebreak
1:
    ret
blahblahblah:
    ret
func:
    .word   0x00000000
