#pragma once

#include "a11ucode.h"

#define I2C_IR_ADDR		0x4Du<<1	// 0x9A
#define I2C3_REGS_BASE  0x1EC48000
#define BIT(n)          (1u<<(n))

// REG_I2C_CNT
#define I2C_STOP            BIT(0)
#define I2C_START           BIT(1)
#define I2C_ERROR           BIT(2)
#define I2C_ACK             BIT(4)
#define I2C_DIR_S           (0u)    // Direction send.
#define I2C_DIR_R           BIT(5)  // Direction receive.
#define I2C_IRQ_EN          BIT(6)
#define I2C_EN              BIT(7)

// REG_I2C_CNTEX
#define I2C_SCL_STATE_MASK  BIT(0)  // Read-only SCL line state?
#define I2C_CLK_STRETCH_EN  BIT(1)  // Enables clock stretching.
#define I2C_UNK_CNTEX15     BIT(15) // "LGCY" Legacy related?

// REG_I2C_SCL
#define I2C_DELAYS(high, low)  ((high)<<8 | (low))

typedef struct
{
	vu8  data;  // 0x0
	vu8  cnt;   // 0x1
	vu16 cntex; // 0x2
	vu16 scl;   // 0x4
} I2cBus;

void I2C_init(void);
bool I2C_readArray(const u32 regAddr, void *out, u32 size);
bool I2C_writeArray(const u32 regAddr, const void *in, u32 size);
u8 I2C_read(const u32 regAddr);
bool I2C_write(const u32 regAddr, const u8 data);
