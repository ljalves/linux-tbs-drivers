/*
    TurboSight TBS 62x0 DVBT/T2/C frontend driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS62X0FE_H
#define TBS62X0FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs62x0fe_config {
	u8 tbs62x0fe_address;

	int (*tbs62x0_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs62x0_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS62X0FE) || \
	(defined(CONFIG_DVB_TBS62X0FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs62x0fe_attach(
	const struct tbs62x0fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs62x0fe_attach(
	const struct tbs62x0fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS62X0FE_H */
