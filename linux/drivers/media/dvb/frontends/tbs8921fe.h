/*
    TurboSight TBS 8921 DVBS/S2 frontend driver
    Copyright (C) 2010 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2010 TurboSight.com
*/

#ifndef TBS8921FE_H
#define TBS8921FE_H

#include <linux/dvb/frontend.h>
#include "cx88.h"

struct tbs8921fe_config {
	u8 tbs8921fe_address;

	int (*tbs8921_ctrl1)(struct cx88_core *core, int a);
	int (*tbs8921_ctrl2)(struct cx88_core *core, int a, int b);
};

#if defined(CONFIG_DVB_TBS8921FE) || \
	(defined(CONFIG_DVB_TBS8921FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs8921fe_attach(
	const struct tbs8921fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs8921fe_attach(
	const struct tbs8921fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS8921FE_H */
