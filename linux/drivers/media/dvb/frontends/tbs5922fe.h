/*
    TurboSight TBS 5922 DVBS/S2 frontend driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS5922FE_H
#define TBS5922FE_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbs5922fe_config {
	u8 tbs5922fe_address;

	int (*tbs5922_ctrl)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBS5922FE) || \
	(defined(CONFIG_DVB_TBS5922FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs5922fe_attach(
	const struct tbs5922fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs5922fe_attach(
	const struct tbs5922fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS5922FE_H */
