.section ".crt0","ax"
.global _start
.align	4
.arm
push {r1-r12, lr}
bl handleCommand1
pop {r1-r12, pc}

_start:
.align
.pool
.end
