/*
    TurboSight TBS 6923 DVBS/S2 Satellite Demodulator/Tuner driver
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS6923FE_H
#define TBS6923FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6923fe_config {
	u8 tbs6923fe_address;
	
	int (*tbs6923_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6923_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6923FE) || \
	(defined(CONFIG_DVB_TBS6923FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6923fe_attach(
	const struct tbs6923fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs6923fe_attach(
	const struct tbs6923fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6923FE_H */
