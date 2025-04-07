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
	push {lr}

	mov r1, #0
	mov r2, #2
	bl MakeRequest

	pop {lr}
	bx lr

@ r0 = buffer address, r1 = buffer size
IrSendData:
	push {lr}

	@ Make request to arm7
	mov r2, #1
	bl MakeRequest
	
	pop {lr}
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
