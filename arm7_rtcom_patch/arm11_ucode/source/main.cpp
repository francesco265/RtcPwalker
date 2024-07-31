#include "a11ucode.h"
#include "main_stuff.h"

extern "C" int handleCommand1(u8 param, u32 stage) {
	static u8 opcode = 0;		// 1 = send, 2 = recv, 3 = beginComm, 4 = endComm
	static u8 data_size = 0;

	if (stage == 0) {
		switch (param) {
			case 1:
			case 2:
				opcode = param;
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
				opcode = 2;
				data_size = 8;
				break;
		}
	} else if (stage == 1 && opcode == 1) {
		data_size = param;
	} else if (opcode == 1) {
		ir_buffer[stage - 2] = param;

		// Transfer to buffer finished
		if (stage == data_size + 1) {
			opcode = 0;
			ir_send(data_size);
			data_size = 0;
		}
	} else if (opcode == 2 && stage <= data_size) {
		if (stage == data_size) {
			opcode = 0;
			data_size = 0;
		}
		return ir_buffer[stage - 1];
	}
    return 0;
}
