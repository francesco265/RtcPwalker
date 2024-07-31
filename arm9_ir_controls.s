
@ r0 = buffer address; returns size of the received data
IrRecvData:
	push {r1, r9}
	
	ldr r9, RTCom_Output
	@ Store opcode 2
	mov r1, #2
	strb r1, [r9]
	@ Store address
	mov r1, #0xA
	add r0, r0, r1, lsl #24
	str r0, [r9, #4]
	@ Start request
	mov r1, #0xf
	strb r1, [r9, #1]
	@ Wait for arm7 ACK
ACKWait:
	ldrb r1, [r9, #1]
	cmp r1, #0xf0
	bne ACKWait

	ldrh r0, [r9, #2]

	pop {r1, r9}
	bx lr

@ r0 = buffer address, r1 = buffer size
IrSendData:
	push {r4-r6, r9}

	ldr r9, RTCom_Output
	@ Store opcode 1
	mov r4, #1
	strb r4, [r9]
	@ Store address
	mov r5, r0
	mov r4, #0xA
	add r0, r0, r4, lsl #24
	str r0, [r9, #4]
	@ Store size
	strh r1, [r9, #2]
	
	@ Flush cache to memory (r5 = address, r6 = size)
	mov r6, #200
	add r6, r6, r5
	bic r5, r5, #31
Flush:
	mcr p15, 0, r5, c7, c14, 1
	add r5, r5, #32
	cmp r5, r6
	blt Flush
DrainWriteBuffer:
	mov r5, #0
	mcr p15, 0, r5, c7, c10, 4
	
	@ Start request
	mov r4, #0xf
	strb r4, [r9, #1]
	@ Wait for arm7 ACK
ACKWait2:
	ldrb r4, [r9, #1]
	cmp r4, #0xf0
	bne ACKWait2

	@@ Prove
@	mov r4, #4
@	strb r4, [r9]
@	mov r4, #0xf
@	strb r4, [r9, #1]
@ACKWait3:
@	ldrb r4, [r9, #1]
@	cmp r4, #0xf0
@	bne ACKWait3

	@ CRASH
	@ cmp r1, #0
	@ bhi Crash

	pop {r4-r6, r9}
	bx lr
@Crash:
@	ldr r8, Invalid_Addr
@	ldr sp, IrBuffer_Addr
@	str r1, [r8]
@	bx lr

RTCom_Output: .long 0x027FF400
IrBuffer_Addr: .long 0x021FF990
Invalid_Addr: .long 0xFFFFFFFF
Delay: .long 0x14744
