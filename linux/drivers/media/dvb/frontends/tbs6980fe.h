/*
    TurboSight TBS 6980 Dual DVBS/S2 frontend driver
    Copyright (C) 2009 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2009 TurboSight.com
*/

#ifndef TBS6980FE_H
#define TBS6980FE_H

#include <linux/dvb/frontend.h>
#include "cx23885.h"

struct tbs6980fe_config {
	u8 tbs6980fe_address;

	int (*tbs6980_ctrl1)(struct cx23885_dev *dev, int a);
	int (*tbs6980_ctrl2)(struct cx23885_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6980FE) || \
	(defined(CONFIG_DVB_TBS6980FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6980fe_attach(
	const struct tbs6980fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6980fe_attach(
	const struct tbs6980fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6980FE_H */
