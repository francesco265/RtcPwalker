#include "i2c.h"

static I2cBus *const g_i2cBus = (I2cBus*)I2C3_REGS_BASE;

static bool checkAck(I2cBus *const i2cBus)
{
	// If we received a NACK stop the transfer.
	if((i2cBus->cnt & I2C_ACK) == 0)
	{
		i2cBus->cnt = I2C_EN | I2C_IRQ_EN | I2C_ERROR | I2C_STOP;
		return false;
	}

	return true;
}

static void sendByte(I2cBus *const i2cBus, const u8 data, const u8 params)
{
	i2cBus->data = data;
	i2cBus->cnt = I2C_EN | I2C_IRQ_EN | I2C_DIR_S | params;
	while (i2cBus->cnt & I2C_EN);
}

static u8 recvByte(I2cBus *const i2cBus, u8 params)
{
	i2cBus->cnt = I2C_EN | I2C_IRQ_EN | I2C_DIR_R | params;
	while (i2cBus->cnt & I2C_EN);
	return i2cBus->data;
}

void I2C_init(void)
{
	static bool inited = false;
	if(inited) return;
	inited = true;

	while(g_i2cBus->cnt & I2C_EN);
	g_i2cBus->cntex = I2C_CLK_STRETCH_EN;
	g_i2cBus->scl = I2C_DELAYS(3u, 0u);
}

static bool startTransfer(const u32 devAddr, const u32 regAddr, const bool read)
{
	u32 tries = 8;
	do
	{
		// Edge case on previous transfer error (NACK).
		// This is a special case where we can't predict when or if
		// the IRQ has fired. If it fires after checking but
		// before a wfi this would hang.
		while(g_i2cBus->cnt & I2C_EN);

		// Select device for write and start.
		sendByte(g_i2cBus, devAddr, I2C_START);
		if(!checkAck(g_i2cBus)) continue;

		// Select register.
		sendByte(g_i2cBus, regAddr, 0);
		if(!checkAck(g_i2cBus)) continue;

		// Select device in read mode for read transfer.
		if(read) {
			sendByte(g_i2cBus, devAddr | BIT(0), I2C_START);
			if(!checkAck(g_i2cBus)) continue;
		}

		break;
	} while(--tries > 0);

	return tries > 0;
}

bool I2C_readArray(const u32 regAddr, void *out, u32 size)
{
	u8 *ptr8 = (u8 *) out;


	bool res = true;
	if(startTransfer(I2C_IR_ADDR, regAddr, true))
	{
		while(--size) *ptr8++ = recvByte(g_i2cBus, I2C_ACK);

		// Last byte transfer.
		*ptr8 = recvByte(g_i2cBus, I2C_STOP);
	}
	else res = false;

	return res;
}

bool I2C_writeArray(const u32 regAddr, const void *in, u32 size)
{
	const u8 devAddr = I2C_IR_ADDR;
	const u8 *ptr8 = (u8 *) in;


	if(!startTransfer(devAddr, regAddr, false)) return false;

	while(--size) {
		sendByte(g_i2cBus, *ptr8++, 0);
		if(!checkAck(g_i2cBus)) return false;
	}

	// Last byte transfer.
	sendByte(g_i2cBus, *ptr8, I2C_STOP);
	if(!checkAck(g_i2cBus)) return false;

	return true;
}

u8 I2C_read(const u32 regAddr)
{
	u8 data;
	if(!I2C_readArray(regAddr, &data, 1)) return 0xFF;
	return data;
}

bool I2C_write(const u32 regAddr, const u8 data)
{
	return I2C_writeArray(regAddr, &data, 1);
}
