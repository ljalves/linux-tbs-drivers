/*
    TurboSight TBS 6982 DVBS/S2 Satellite Demodulator/Tuner driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS6982FE_H
#define TBS6982FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6982fe_config {
	u8 tbs6982fe_address;
	
	int (*tbs6982_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6982_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6982FE) || \
	(defined(CONFIG_DVB_TBS6982FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6982fe_attach(
	const struct tbs6982fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6982fe_attach(
	const struct tbs6982fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6982FE_H */
