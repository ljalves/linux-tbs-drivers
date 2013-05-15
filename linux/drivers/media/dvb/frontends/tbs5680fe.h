/*
    TurboSight TBS 5680 DVBC frontend driver
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS5680FE_H
#define TBS5680FE_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbs5680fe_config {
	u8 tbs5680fe_address;

	int (*tbs5680_ctrl1)(struct usb_device *dev, u8 *a);
	int (*tbs5680_ctrl2)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBS5680FE) || \
	(defined(CONFIG_DVB_TBS5680FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs5680fe_attach(
	const struct tbs5680fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs5680fe_attach(
	const struct tbs5680fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS5680FE_H */
