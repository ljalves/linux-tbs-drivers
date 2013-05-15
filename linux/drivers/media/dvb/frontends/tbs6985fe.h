/*
    TurboSight TBS 6985 DVBS/S2 frontend driver
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS6985FE_H
#define TBS6985FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6985fe_config {
	u8 tbs6985fe_address;

	int (*tbs6985_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6985_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6985FE) || \
	(defined(CONFIG_DVB_TBS6985FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6985fe_attach(
	const struct tbs6985fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6985fe_attach(
	const struct tbs6985fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6985FE_H */
