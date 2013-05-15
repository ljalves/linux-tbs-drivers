/*
    TurboSight TBS 6921 DVBS/S2 frontend driver
    Copyright (C) 2010 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2010 TurboSight.com
*/

#ifndef TBS6921FE_H
#define TBS6921FE_H

#include <linux/dvb/frontend.h>
#include "cx23885.h"

struct tbs6921fe_config {
	u8 tbs6921fe_address;

	int (*tbs6921_ctrl1)(struct cx23885_dev *dev, int a);
	int (*tbs6921_ctrl2)(struct cx23885_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6921FE) || \
	(defined(CONFIG_DVB_TBS6921FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6921fe_attach(
	const struct tbs6921fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6921fe_attach(
	const struct tbs6921fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6921FE_H */
