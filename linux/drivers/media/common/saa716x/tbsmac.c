/*
	Read MAC address from TBS board based on SAA716x PCIe bridge
	Copyright (c) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>
	
	Copyright (C) 2011 TurboSight.com
 */

#include "saa716x_priv.h"
#include "tbsmac.h"

int tbs_read(struct i2c_adapter *i2c_adap, u8 addr)
{
	u8 b[] = { 0x02, addr };
	int ret;

	struct i2c_msg msg[] = {
		{ .addr	= 0x50, .flags = 0,
			.buf = b, .len = 2 }, 
		{ .addr	= 0x50, .flags = I2C_M_RD,
			.buf = b, .len = 1 }
	};

	ret = i2c_transfer(i2c_adap, msg, 2);

	if (ret != 2) {
		printk(KERN_ERR "%s: error=%d\n", __func__, ret);
		return -1;
	}

	return b[0];
};

void tbs_read_mac(struct i2c_adapter *i2c_adap, u8 addr, u8 *mac)
{
	int i, j;

	for (i = addr, j = 0; i < (addr + 6); i++, j++)
		mac[j] = tbs_read(i2c_adap, i);

#if 0
	printk(KERN_INFO "TBS MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif
};

int saa716x_read(struct i2c_adapter *i2c_adap, u16 addr)
{
	u8 b[] = { addr>>8, addr&0xff };
	int ret;

	struct i2c_msg msg[] = {
		{ .addr	= 0x50, .flags = 0,
			.buf = b, .len = 2 }, 
		{ .addr	= 0x50, .flags = I2C_M_RD,
			.buf = b, .len = 1 }
	};

	ret = i2c_transfer(i2c_adap, msg, 2);

	if (ret != 2) {
		printk(KERN_ERR "%s: error=%d\n", __func__, ret);
		return -1;
	}

	return b[0];
};

void saa716x_read_mac(struct i2c_adapter *i2c_adap, u16 addr, u8 *mac)
{
	int i, j;

	for (i = addr, j = 0; i < (addr + 6); i++, j++)
		mac[j] = saa716x_read(i2c_adap, i);
};
