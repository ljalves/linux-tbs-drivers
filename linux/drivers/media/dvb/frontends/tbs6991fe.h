/*
    TurboSight TBS 6991 DVBS/S2 Satellite Demodulator/Tuner driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS6991FE_H
#define TBS6991FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6991fe_config {
	u8 tbs6991fe_address;
	
	int (*tbs6991_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6991_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6991FE) || \
	(defined(CONFIG_DVB_TBS6991FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6991fe_attach(
	const struct tbs6991fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6991fe_attach(
	const struct tbs6991fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6991FE_H */
