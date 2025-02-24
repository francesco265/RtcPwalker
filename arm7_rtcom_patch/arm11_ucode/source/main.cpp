#include "a11ucode.h"
#include "main_stuff.h"

extern "C" int handleCommand1(u8 param, u32 stage) {
	static u8 opcode = 0;		// 1 = send, 2 = recv, 3 = beginComm, 4 = endComm
	static u8 data_size = 0;
	static u8 i = 0, null_count = 0, enable_check = 1;

	if (stage == 0) {
		switch (param) {
			case 1:
			case 2:
				opcode = param;
				i = 0;
				if (opcode == 2) {
					data_size = ir_recv();
					if (data_size == 0)
						opcode = 0;
					return data_size;
				}
				break;
			case 3:
				ir_beginComm();
				break;
			case 4:
				//ir_endComm(); TODO rimettere a posto qua
				opcode = 4;
				i = 0;
				break;
		}
	} else if (stage == 1 && opcode == 1) {
		data_size = param;
		null_count = 0;
		enable_check = (data_size > 16);
	} else if (opcode == 1) {
		if (enable_check && param == 0 && null_count++ < 5)
			return 1;
		ir_buffer[i++] = param;
		null_count = 0;

		// Transfer to buffer finished
		if (i == data_size)
			ir_send(data_size);
	} else if (opcode == 2) {
		u8 data = ir_buffer[i++] ^ 0xAA;
		*(vu8 *)(RTC_REG_TIMER2 + 2) = ir_buffer[i++] ^ 0xAA;
		*(vu8 *)(RTC_REG_TIMER2 + 1) = ir_buffer[i++] ^ 0xAA;
		*(vu8 *)(RTC_REG_TIMER2) = ir_buffer[i++] ^ 0xAA;
		*(vu8 *)(RTC_CNT) |= 0x40;

		return data;
	} else if (opcode == 4) {
		return ir_buffer[i++];
	}
    return 0;
}
