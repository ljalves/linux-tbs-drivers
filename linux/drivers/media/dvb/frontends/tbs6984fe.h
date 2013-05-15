/*
    TurboSight TBS 6984 Dual DVBS/S2 frontend driver
    Copyright (C) 2010 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2010 TurboSight.com
*/

#ifndef TBS6984FE_H
#define TBS6984FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6984fe_config {
	u8 tbs6984fe_address;

	int (*tbs6984_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6984_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6984FE) || \
	(defined(CONFIG_DVB_TBS6984FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6984fe_attach(
	const struct tbs6984fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6984fe_attach(
	const struct tbs6984fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6984FE_H */
