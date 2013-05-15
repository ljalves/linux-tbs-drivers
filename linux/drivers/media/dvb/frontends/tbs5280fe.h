/*
    TurboSight TBS 5880 DVBT/T2/C frontend driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS5280FE_H
#define TBS5280FE_H

#include <linux/dvb/frontend.h>
#include "cx231xx.h"

struct tbs5280fe_config {
	u8 tbs5280fe_address;

	int (*tbs5280_ctrl1)(struct cx231xx *dev, int a);
	u32 (*tbs5280_ctrl2)(struct cx231xx *dev, int a);
	void (*tbs5280_ctrl3)(struct cx231xx *dev, int a, u32 b);
};

#if defined(CONFIG_DVB_TBS5280FE) || \
	(defined(CONFIG_DVB_TBS5280FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs5280fe_attach(
	const struct tbs5280fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs5280fe_attach(
	const struct tbs5280fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS5280FE_H */
