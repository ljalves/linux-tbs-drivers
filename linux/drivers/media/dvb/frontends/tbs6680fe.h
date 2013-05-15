/*
    TurboSight TBS 6680 DVBC frontend driver
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS6680FE_H
#define TBS6680FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6680fe_config {
	u8 tbs6680fe_address;

	int (*tbs6680_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6680_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6680FE) || \
	(defined(CONFIG_DVB_TBS6680FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6680fe_attach(
	const struct tbs6680fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs6680fe_attach(
	const struct tbs6680fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6680FE_H */
