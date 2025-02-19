#include <stddef.h>

#include "a11ucode.h"
#include "main_stuff.h"
#include "i2c.h"

#define RELOC_ADDR ((void *)0x100000)
#define RELOC_SIZE (0x28000)

// IR I2C registers
#define REG_FIFO	0x00	// Receive / Transmit Holding Register
#define REG_DLL		0x00	// Baudrate Divisor Latch Register Low
#define REG_IER		0x08	// Interrupt Enable Register
#define REG_DLH		0x08	// Baudrate Divisor Latch Register High
#define REG_FCR		0x10	// FIFO Control Register
#define REG_EFR		0x10	// Enhanced Feature Register
#define REG_LCR		0x18	// Line Control Register
#define REG_MCR		0x20	// Modem Control Register
#define REG_LSR		0x28	// Line Status Register
#define REG_TXLVL	0x40	// Transmitter FIFO Level Register
#define REG_RXLVL	0x48	// Receiver FIFO Level Register
#define REG_IOSTATE	0x58	// IOState Register
#define	REG_EFCR	0x78	// Extra Features Control Register

#define	RX_MAX_WAIT	40

u8 ir_buffer[136];
u8 ir_buffer_size = 0;

void ir_init() {
	I2C_init();

	// Set baud rate
	u8 lcr = I2C_read(REG_LCR);

	// Enable access to DLL and DLH
	I2C_write(REG_LCR, lcr | BIT(7));
	// Disable sleep mode
	I2C_write(REG_IER, 0);

	I2C_write(REG_DLL, 10);
	I2C_write(REG_DLH, 0);

	I2C_write(REG_LCR, lcr);
	I2C_write(REG_IER, BIT(4));
}

void ir_beginComm() {
	static u8 inited = 0;
	if (!inited)
		ir_init();
	inited = 1;

	// Disable sleep mode
	I2C_write(REG_IER, 0);
	// IOState must be 0
	I2C_write(REG_IOSTATE, 0);

	ir_buffer_size = 0;
}

void ir_endComm() {
	// Reset and disable FIFO
	I2C_write(REG_FCR, 0x06);
	// Enable sleep mode
	I2C_write(REG_IER, BIT(4));
	I2C_write(REG_IOSTATE, BIT(0));
}

// Send data using IR and start listening for incoming data
void ir_send(u8 size) {
	u8 *ptr = ir_buffer;

	// I2C_write(REG_FCR, 0x07); // TODO rimuovere
	// Enable transmitter / Disable receiver
	I2C_write(REG_EFCR, 0x02);

	if (size <= 64) {
		I2C_writeArray(REG_FIFO, ptr, size);
	} else {
		u8 txlvl, to_send;
		do {
			txlvl = I2C_read(REG_TXLVL);
			if (txlvl) {
				to_send = size > txlvl ? txlvl : size;
				I2C_writeArray(REG_FIFO, ptr, to_send);
				ptr += to_send;
				size -= to_send;
			}
		} while (size);
	}

	// Wait until THR and TSR are empty
	while (!(I2C_read(REG_LSR) & BIT(6)));

	// Enable receiver / Disable transmitter
	I2C_write(REG_EFCR, 0x04);

	ptr = ir_buffer;
	u8 rxlvl, tc = 0;

	u16 i;
	u16 timeout = 1000;
	do {
		i = 0;
		/* while (!(ir_reg_read(REG_LSR) & BIT(0)) && i < 80) */
		/* 	i++; */
		rxlvl = I2C_read(REG_RXLVL);
		while (!rxlvl && i < timeout) {
			i++;
			rxlvl = I2C_read(REG_RXLVL);
		}
		if (i == timeout)
			break;
		timeout = RX_MAX_WAIT;

		//i2c_read(0, &rxlvl, 0xE, REG_RXLVL, 1);
		I2C_readArray(REG_FIFO, ptr, rxlvl);
		ptr += rxlvl;
		tc += rxlvl;
	} while (tc < 136);

	// Disable transmitter and receiver
	I2C_write(REG_EFCR, 0x06);

	ir_buffer_size = tc;
}

u8 ir_recv() {
	if (ir_buffer_size) {
		u8 tc = ir_buffer_size;
		ir_buffer_size = 0;
		return tc;
	}

	u8 *ptr = ir_buffer;
	u8 i, rxlvl;

	// Reset and enable FIFO
	I2C_write(REG_FCR, 0x07);
	// Enable receiver
	I2C_write(REG_EFCR, 0x04);

	do {
		i = 0;
		// while (!(ir_reg_read(REG_LSR) & BIT(0)) && i < RX_MAX_WAIT)
		rxlvl = I2C_read(REG_RXLVL);
		while (!rxlvl && i < RX_MAX_WAIT) {
			i++;
			rxlvl = I2C_read(REG_RXLVL);
		}
		if (i == RX_MAX_WAIT)
			break;

		//i2c_read(0, &rxlvl, 0xE, REG_RXLVL, 1);
		I2C_readArray(REG_FIFO, ptr, rxlvl);
		ptr += rxlvl;
	} while (1);

	// Disable transmitter and receiver
	// I2C_write(REG_EFCR, 0x06);

	//return tc;
	return ptr - ir_buffer;
}
