/*
    Technotrend TT-budget S2-4100 DVBS/S2 Satellite Demodulator/Tuner driver
    
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>
*/

#ifndef __TT_S2_4100_H
#define __TT_S2_4100_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tt_s2_4100_config {
	u8 tt_s2_4100_addr;
	
	int (*tt_s2_4100_drv1)(struct saa716x_dev *dev, int tt1);
	int (*tt_s2_4100_drv2)(struct saa716x_dev *dev, int tt1, int tt2);
};

#if defined(CONFIG_DVB_TT_S2_4100) || \
	(defined(CONFIG_DVB_TT_S2_4100_MODULE) && defined(MODULE))
extern struct dvb_frontend *tt_s2_4100_attach(
	const struct tt_s2_4100_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tt_s2_4100_attach(
	const struct tt_s2_4100_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* __TT_S2_4100_H */
