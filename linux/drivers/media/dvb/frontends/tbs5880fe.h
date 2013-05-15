/*
    TurboSight TBS 5880 DVBT/T2/C frontend driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS5880FE_H
#define TBS5880FE_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbs5880fe_config {
	u8 tbs5880fe_address;

	int (*tbs5880_ctrl)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBS5880FE) || \
	(defined(CONFIG_DVB_TBS5880FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs5880fe_attach(
	const struct tbs5880fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs5880fe_attach(
	const struct tbs5880fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS5880FE_H */
