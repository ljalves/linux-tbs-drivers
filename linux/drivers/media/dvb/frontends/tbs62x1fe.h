/*
    TurboSight TBS 62x1 DVBT/T2/C frontend driver
    Copyright (C) 2013 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2013 TurboSight.com
*/

#ifndef TBS62X1FE_H
#define TBS62X1FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs62x1fe_config {
	u8 tbs62x1fe_address;

	int (*tbs62x1_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs62x1_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS62X1FE) || \
	(defined(CONFIG_DVB_TBS62X1FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs62x1fe_attach(
	const struct tbs62x1fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs62x1fe_attach(
	const struct tbs62x1fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS62X1FE_H */
