/*
    TurboSight TBS 5220 DVBC frontend driver
    Copyright (C) 2013 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2013 TurboSight.com
*/

#ifndef TBS5220FE_H
#define TBS5220FE_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbs5220fe_config {
	u8 tbs5220fe_address;

	int (*tbs5220_ctrl1)(struct usb_device *dev, u8 *a);
	int (*tbs5220_ctrl2)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBS5220FE) || \
	(defined(CONFIG_DVB_TBS5220FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs5220fe_attach(
	const struct tbs5220fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs5220fe_attach(
	const struct tbs5220fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS5220FE_H */
