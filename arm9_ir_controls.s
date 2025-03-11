@ Make request to arm7; r0 = address, r1 = size, r2 = opcode; returns data size if opcode is 2
MakeRequest:
	push {r4-r6}
	ldr r5, Reg_IPCSYNC
	ldr r6, RTCom_Output

	str r0, [r6, #4]
	strh r1, [r6, #2]
	strb r2, [r6]

	@ Start request
	mov r4, #0xf
	strb r4, [r6, #1]
	mov r4, #1
	lsl r4, r4, #13
	strh r4, [r5]

WaitACK:
	ldrb r0, [r6, #1]
	cmp r0, #0xf0
	bne WaitACK

	ldrh r0, [r6, #2]
	pop {r4-r6}
	bx lr

@ r0 = buffer address; returns size of the received data
IrRecvData:
	push {r5, r6, lr}

	mov r5, r0
	mov r1, #0
	mov r2, #2
	bl MakeRequest

	cmp r0, #1
	bls Return
	
	mov r6, r0
	@ Invalidate cache (r5 = address, r6 = size)
	add	r6, r6, r5
	tst	r5, #31
	mcrne	p15, 0, r5, c7, c10, 1
	tst	r6, #31
	mcrne	p15, 0, r6, c7, c10, 1
	bic	r5, r5, #31
Invalidate:
	mcr	p15, 0, r5, c7, c6, 1
	add	r5, r5, #31
	cmp	r5, r6
	blt	Invalidate
	
Return:
	pop {r5, r6, lr}
	bx lr

@ r0 = buffer address, r1 = buffer size
IrSendData:
	push {r5, r6, lr}

	mov r5, r0
	mov r6, r1
	@ Flush cache to memory (r5 = address, r6 = size)
	add r6, r6, r5
	bic r5, r5, #31
Flush:
	mcr p15, 0, r5, c7, c14, 1
	add r5, r5, #32
	cmp r5, r6
	blt Flush
	mov r5, #0
	mcr p15, 0, r5, c7, c10, 4

	@ Make request to arm7
	mov r2, #1
	bl MakeRequest
	
	pop {r5, r6, lr}
	bx lr

IrInit:
	push {lr}

	mov r0, #0
	mov r1, #0
	mov r2, #3
	bl MakeRequest

	pop {lr}
	bx lr

.thumb_func
IrEnd:
	push {r0-r2}

	mov r0, #0
	mov r1, #0
	mov r2, #4
	blx MakeRequest

	pop {r0-r2}
	pop {r3-r5, pc}

Reg_IPCSYNC: .long 0x04000180
RTCom_Output: .long 0x027FF400
