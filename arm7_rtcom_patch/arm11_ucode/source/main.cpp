#include "a11ucode.h"
#include "main_stuff.h"

extern "C" int handleCommand1(u8 param, u32 stage) {
	static u8 opcode = 0;		// 1 = send, 2 = recv, 3 = beginComm, 4 = endComm
	static u8 data_size = 0;
	static u8 i = 0;

	if (stage == 0) {
		opcode = param;
		i = 0;
		switch (param) {
			case 2:
				data_size = ir_recv();
				if (data_size == 0)
					opcode = 0;
				return data_size;
			case 3:
				ir_beginComm();
				break;
			case 4:
				ir_endComm();
				break;
		}
	} else if (stage == 1 && opcode == 1) {
		data_size = param;
	} else if (opcode == 1) {
		ir_buffer[i++] = param;
		for (s8 j = 2; j >= 0 && i < data_size; j--)
			ir_buffer[i++] = *(vu8 *)(RTC_REG_TIMER2 + j);

		// Transfer to buffer finished
		if (i == data_size)
			ir_send(data_size);
	} else if (opcode == 2) {
		u8 data = ir_buffer[i++] ^ 0xAA;
		for (s8 j = 2; j >= 0; j--)
			*(vu8 *)(RTC_REG_TIMER2 + j) = ir_buffer[i++] ^ 0xAA;
		*(vu8 *)(RTC_CNT) |= 0x40;

		return data;
	}
    return 0;
}
