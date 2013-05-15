/*
    TurboSight TBS 6981 Dual DVBS/S2 frontend driver
    Copyright (C) 2009 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2009 TurboSight.com
*/

#ifndef TBS6981FE_H
#define TBS6981FE_H

#include <linux/dvb/frontend.h>
#include "cx23885.h"

struct tbs6981fe_config {
	u8 tbs6981fe_address;

	int (*tbs6981_ctrl1)(struct cx23885_dev *dev, int a);
	int (*tbs6981_ctrl2)(struct cx23885_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6981FE) || \
	(defined(CONFIG_DVB_TBS6981FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6981fe_attach(
	const struct tbs6981fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6981fe_attach(
	const struct tbs6981fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6981FE_H */
