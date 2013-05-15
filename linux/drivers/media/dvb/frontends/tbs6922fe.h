/*
    TurboSight TBS 6922 DVBS/S2 Satellite Demodulator/Tuner driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS6922FE_H
#define TBS6922FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6922fe_config {
	u8 tbs6922fe_address;
	
	int (*tbs6922_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6922_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6922FE) || \
	(defined(CONFIG_DVB_TBS6922FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6922fe_attach(
	const struct tbs6922fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs6922fe_attach(
	const struct tbs6922fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6922FE_H */
