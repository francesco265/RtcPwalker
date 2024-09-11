#include <stddef.h>

#include "a11ucode.h"
#include "main_stuff.h"

#define RELOC_ADDR ((void *)0x100000)
#define RELOC_SIZE (0x28000)
#define BIT(x) (1 << (x))

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

typedef int (*I2C_Read_Func)(void *x, u8 *dst, int dev, int src_addr, int count);
typedef int (*I2C_Write_Func)(void *x, int dev, int dst_addr, const u8 *dst, int count);

static I2C_Read_Func i2c_read = nullptr;
static I2C_Write_Func i2c_write = nullptr;

static int is_initialized = 0;

u8 ir_buffer[136];
u8 ir_buffer_size = 0;

u8 ir_reg_read(int reg) {
	u8 dst;
    i2c_read(0, &dst, 0xE, reg, 1);
	return dst;
}

void ir_reg_write(int reg, u8 value) {
	i2c_write(0, 0xE, reg, &value, 1);
}

void ir_init() {
    if (is_initialized) {
        return;
    }

    const static uint8_t i2c_senddata_pattern[] = {0x70, 0xb5, 0x06, 0x46, 0x0b, 0x4c, 0x88,
                                                   0x00, 0x0d, 0x18, 0x30, 0x46, 0x61, 0x5d};
    size_t ptr = (size_t)pat_memesearch(i2c_senddata_pattern, nullptr, RELOC_ADDR, RELOC_SIZE,
                                        sizeof(i2c_senddata_pattern), 2);
    if (!ptr) {
        return;
    }
    vu8 *i2c_device_table = *(vu8 **)(ptr + 0x34);
    i2c_device_table[0x46] = 0x2;
    i2c_device_table[0x47] = 0x9A;

    const static uint8_t i2c_read_pattern[] = {0xff, 0xb5, 0x81, 0xb0, 0x14, 0x46, 0x0d, 0x46,
                                               0x1e, 0x46, 0x0a, 0x9f, 0x01, 0x98, 0x11, 0x46};
    ptr = (size_t)pat_memesearch(i2c_read_pattern, nullptr, RELOC_ADDR, RELOC_SIZE,
                                 sizeof(i2c_read_pattern), 2);
    if (!ptr) {
        return;
    }
    i2c_read = (I2C_Read_Func)(ptr | 1);

    const static uint8_t i2c_write_pattern[] = {0xff, 0xb5, 0x81, 0xb0, 0x14, 0x46, 0x05,
                                                0x46, 0x1e, 0x46, 0x0a, 0x9f, 0x02, 0x99};
    ptr = (size_t)pat_memesearch(i2c_write_pattern, nullptr, RELOC_ADDR, RELOC_SIZE,
                                 sizeof(i2c_write_pattern), 2);
    if (!ptr) {
        return;
    }
    i2c_write = (I2C_Write_Func)(ptr | 1);

	// Set baud rate
	u8 lcr = ir_reg_read(REG_LCR);

	// Enable access to DLL and DLH
	ir_reg_write(REG_LCR, lcr | BIT(7));
	// Disable sleep mode
	ir_reg_write(REG_IER, 0);

	ir_reg_write(REG_DLL, 10);
	ir_reg_write(REG_DLH, 0);

	ir_reg_write(REG_LCR, lcr);
	ir_reg_write(REG_IER, BIT(4));

    is_initialized = 1;
}

void ir_beginComm() {
	if (!is_initialized)
		ir_init();

	// Disable sleep mode
	ir_reg_write(REG_IER, 0);
	// IOState must be 0
	ir_reg_write(REG_IOSTATE, 0);
	// Reset and enable FIFO
	//ir_reg_write(REG_FCR, 0x07); //TODO l'ho disattivato e messo in ir_recv
	// Enable receiver
	//ir_reg_write(REG_EFCR, 0x04); //TODO l'ho disattivato e messo in ir_recv
	ir_buffer_size = 0;
}

void ir_endComm() {
	// Reset and disable FIFO
	ir_reg_write(REG_FCR, 0x06);
	// Enable sleep mode
	ir_reg_write(REG_IER, BIT(4));
	ir_reg_write(REG_IOSTATE, BIT(0));
}

// Send data using IR and start listening for incoming data
void ir_send(u8 size) {
	u8 *ptr = ir_buffer;

	// ir_reg_write(REG_FCR, 0x07); // TODO rimuovere
	// Enable transmitter / Disable receiver
	ir_reg_write(REG_EFCR, 0x02);

	if (size <= 64) {
		i2c_write(0, 0xE, REG_FIFO, ptr, size);
	} else {
		u8 txlvl, to_send;
		do {
			i2c_read(0, &txlvl, 0xE, REG_TXLVL, 1);
			if (txlvl) {
				to_send = size > txlvl ? txlvl : size;
				i2c_write(0, 0xE, REG_FIFO, ptr, to_send);
				ptr += to_send;
				size -= to_send;
			}
		} while (size);
	}

	// Wait until THR and TSR are empty
	while (!(ir_reg_read(REG_LSR) & BIT(6)));

	// Enable receiver / Disable transmitter
	ir_reg_write(REG_EFCR, 0x04);

	ptr = ir_buffer;
	u8 i, rxlvl, tc = 0;

	do {
		i = 0;
		i2c_read(0, &rxlvl, 0xE, REG_RXLVL, 1);
		while (!rxlvl && i < 80) {
			i++;
			i2c_read(0, &rxlvl, 0xE, REG_RXLVL, 1);
		}
		if (i == 80)
			break;

		i2c_read(0, ptr, 0xE, REG_FIFO, rxlvl);
		ptr += rxlvl;
		tc += rxlvl;
		if (tc == 136)
			break;
	} while (1);

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
	ir_reg_write(REG_FCR, 0x07);
	// Enable receiver
	ir_reg_write(REG_EFCR, 0x04);

	do {
		i = 0;
		// while (!(ir_reg_read(REG_LSR) & BIT(0)) && i < RX_MAX_WAIT)
		i2c_read(0, &rxlvl, 0xE, REG_RXLVL, 1);
		while (!rxlvl && i < RX_MAX_WAIT) {
			i++;
			i2c_read(0, &rxlvl, 0xE, REG_RXLVL, 1);
		}
		if (i == RX_MAX_WAIT)
			break;

		//i2c_read(0, &rxlvl, 0xE, REG_RXLVL, 1);
		i2c_read(0, ptr, 0xE, REG_FIFO, rxlvl);
		ptr += rxlvl;
	} while (1);

	// Disable transmitter and receiver
	// ir_reg_write(REG_EFCR, 0x06);

	//return tc;
	return ptr - ir_buffer;
}

__attribute__((optimize("Ofast"))) void *pat_memesearch(const void *patptr, const void *bitptr,
                                                        const void *searchptr, u32 searchlen,
                                                        u32 patsize, u32 alignment) {
    const u8 *pat = (const u8 *)patptr;
    const u8 *bit = (const u8 *)bitptr;
    const u8 *src = (const u8 *)searchptr;

    u32 i = 0;
    u32 j = 0;

    searchlen -= patsize;

    if (bit) {
        do {
            if ((src[i + j] & ~bit[j]) == (pat[j] & ~bit[j])) {
                if (++j != patsize) {
                    continue;
                }
                // check alignment
                if (((u32)src & (alignment - 1)) == 0) {
                    return (void *)(src + i);
                }
            }
            ++i;
            j = 0;
        } while (i != searchlen);
    } else {
        do {
            if (src[i + j] == pat[j]) {
                if (++j != patsize) {
                    continue;
                }
                // check alignment
                if (((u32)src & (alignment - 1)) == 0) {
                    return (void *)(src + i);
                }
            }
            ++i;
            j = 0;
        } while (i != searchlen);
    }

    return 0;
}
