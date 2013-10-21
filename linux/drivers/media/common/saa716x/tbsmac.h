/*
	Read MAC address from TBS board based on SAA716x PCIe bridge
	Copyright (c) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>
	
	Copyright (C) 2011 TurboSight.com
 */

#ifndef TBSMAC_H
#define TBSMAC_H

extern int tbs_read(struct i2c_adapter *i2c_adap, u8 addr);
extern void tbs_read_mac(struct i2c_adapter *i2c_adap, u8 addr, u8 *mac);
extern int saa716x_read(struct i2c_adapter *i2c_adap, u16 addr);
extern void saa716x_read_mac(struct i2c_adapter *i2c_adap, u16 addr, u8 *mac);

#endif
