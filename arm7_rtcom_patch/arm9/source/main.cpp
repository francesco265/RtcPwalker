#include <stdio.h>

#include "sys/_stdint.h"
#include <nds.h>

#define RTCOM_DATA_OUTPUT 0x0C7FF400

typedef struct {
	vu8 opcode;
	vu8 flags;
	vu16 size;
	vu8 *data;
} Ipc_proto;

u16 compute_checksum(const void *pkt, u16 size)
{
	const u8 *data = (u8 *) pkt;
    u16 checksum = 0x0002;

    for (size_t i = 1; i < size; i += 2)
        checksum += data[i];

    for (size_t i = 0; i < size; i += 2)
    {
        if ((data[i] << 8) > UINT16_MAX - checksum)
            ++checksum;

        checksum += data[i] << 8;
    }

    // Swap the bytes
    checksum = ((checksum << 8) & 0xFF00) | ((checksum >> 8) & 0xFF);

    return checksum;
}

int main() {
    consoleDemoInit(); // setup the sub screen for printing

    // set mode 0, enable BG0 and set it to 3D
    videoSetMode(MODE_0_3D);

    // initialize gl
    glInit();

    // setup the rear plane
    glClearColor(0, 0, 0, 31); // BG must be opaque for AA to work
    glClearPolyID(63);         // BG must have a unique polygon ID for AA to work
    glClearDepth(0x7FFF);


	u8 *buffer = (u8 *)0x0C7FFF00;
	u8 *buffer_arm7 = (u8 *)0x027FFF00;
	Ipc_proto *ipc_proto = (Ipc_proto *)RTCOM_DATA_OUTPUT;
	u16 checksum;

    while (true) {
        glViewport(0, 0, 255, 191);

        scanKeys();
        if (keysHeld() & KEY_B) {
            printf("\x1b[13;0HButton B is down    \n");
			buffer[0] = 0xFA;
			buffer[1] = 0x01;
			buffer[2] = 0;
			buffer[3] = 0;
			buffer[4] = 0;
			buffer[5] = 0;
			buffer[6] = 0;
			buffer[7] = 0;
			checksum = compute_checksum(buffer, 8);
			buffer[2] = checksum >> 8;
			buffer[3] = checksum & 0xFF;
			for (u8 i = 0; i < 8; i++)
				buffer[i] ^= 0xAA;
			ipc_proto->opcode = 1;
			ipc_proto->size = 8;
			ipc_proto->data = buffer_arm7;
			ipc_proto->flags = 0xF;
			while (ipc_proto->flags != 0xF0);
			ipc_proto->opcode = 2;
			ipc_proto->data = buffer_arm7;
			ipc_proto->flags = 0xF;
			while (ipc_proto->flags != 0xF0);
			// send 0x20
			buffer[0] = 0x20;
			buffer[1] = 0x01;
			buffer[2] = 0;
			buffer[3] = 0;
			checksum = compute_checksum(buffer, 8);
			buffer[2] = checksum >> 8;
			buffer[3] = checksum & 0xFF;
			for (u8 i = 0; i < 8; i++)
				buffer[i] ^= 0xAA;
			ipc_proto->opcode = 1;
			ipc_proto->size = 8;
			ipc_proto->flags = 0xF;
			while (ipc_proto->flags != 0xF0);
			ipc_proto->opcode = 2;
			ipc_proto->flags = 0xF;
			while (ipc_proto->flags != 0xF0);
        } else {
            printf("\x1b[13;0HButton B is NOT down\n");
        }

        iprintf("\x1b[2;0H\t opcode: %02X", ipc_proto->opcode);
        iprintf("\x1b[3;0H\t flags: %02X", ipc_proto->flags);
        iprintf("\x1b[4;0H\t size: %04X", ipc_proto->size);
        iprintf("\x1b[5;0H\t data ptr: %08X\n", (u32)ipc_proto->data);
        iprintf("\x1b[6;0H\t buffer ptr: %08X\n", (u32)buffer);
        iprintf("\x1b[7;0H\t buffer value 1: %08X\n", *(u32 *)buffer);
        iprintf("\x1b[8;0H\t buffer value 2: %08X\n", *(u32 *)(buffer + 4));

        swiWaitForVBlank();
    }

    return 0;
}
