@ r0 = buffer address; returns size of the received data
IrRecvData:
	push {r1, r4, r5, r9}
	
	ldr r9, RTCom_Output

	@ Store opcode 2
	mov r1, #2
	strb r1, [r9]
	@ Store address
	str r0, [r9, #4]
	@ Start request
	mov r1, #0xf
	strb r1, [r9, #1]

	# IPCSYNC
	ldr r4, Reg_IPCSYNC
	mov r1, #1
	mov r1, r1, lsl #13
	strh r1, [r4]

	@ Wait for arm7 ACK
RecvACKWait:
	ldrb r1, [r9, #1]
	cmp r1, #0xf0
	bne RecvACKWait

	ldrh r0, [r9, #2]
	
	pop {r1, r4, r5, r9}
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
	str r0, [r9, #4]
	@ Store size
	strh r1, [r9, #2]
	
	@ Flush cache to memory (r5 = address, r6 = size)
	mov r6, #136
	add r6, r6, r5
	bic r5, r5, #31
Flush:
	mcr p15, 0, r5, c7, c14, 1
	add r5, r5, #32
	cmp r5, r6
	blt Flush
	mov r5, #0
	mcr p15, 0, r5, c7, c10, 4

	@ Start request
	mov r4, #0xf
	strb r4, [r9, #1]

	# IPCSYNC
	ldr r4, Reg_IPCSYNC
	mov r1, #1
	mov r1, r1, lsl #13
	strh r1, [r4]
	
	@ Wait for arm7 ACK
SendACKWait:
	ldrb r4, [r9, #1]
	cmp r4, #0xf0
	bne SendACKWait

	pop {r4-r6, r9}
	bx lr

Reg_IPCSYNC: .long 0x04000180
RTCom_Output: .long 0x027FF400
IrBuffer_Addr: .long 0x021FF990
Invalid_Addr: .long 0xFFFFFFFF
Timer1_CR: .long 0x04000106
Timer2_CR: .long 0x0400010A
Timer1_Data: .long 0x04000104
Timer2_Data: .long 0x04000108
Offset: .long 0x4
Offset_Addr: .long (0x023C0040 + Offset)
