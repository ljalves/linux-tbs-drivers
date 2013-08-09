/*
    TurboSight TBS 5881 DVBC frontend driver
    Copyright (C) 2013 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2013 TurboSight.com
*/

#ifndef TBS5881FE_H
#define TBS5881FE_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbs5881fe_config {
	u8 tbs5881fe_address;

	int (*tbs5881_ctrl1)(struct usb_device *dev, u8 *a);
	int (*tbs5881_ctrl2)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBS5881FE) || \
	(defined(CONFIG_DVB_TBS5881FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs5881fe_attach(
	const struct tbs5881fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs5881fe_attach(
	const struct tbs5881fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS5881FE_H */
