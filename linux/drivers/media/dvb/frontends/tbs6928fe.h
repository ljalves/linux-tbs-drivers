/*
    TurboSight TBS 6928 DVBS/S2 Satellite Demodulator/Tuner driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS6928FE_H
#define TBS6928FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6928fe_config {
	u8 tbs6928fe_address;
	
	int (*tbs6928_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6928_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6928FE) || \
	(defined(CONFIG_DVB_TBS6928FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6928fe_attach(
	const struct tbs6928fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs6928fe_attach(
	const struct tbs6928fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6928FE_H */
