/*
    TurboSight TBS 5921 DVBS/S2 frontend driver
    Copyright (C) 2010 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2010 TurboSight.com
*/

#ifndef TBS5921FE_H
#define TBS5921FE_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbs5921fe_config {
	u8 tbs5921fe_address;

	int (*tbs5921_ctrl)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBS5921FE) || \
	(defined(CONFIG_DVB_TBS5921FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs5921fe_attach(
	const struct tbs5921fe_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs5921fe_attach(
	const struct tbs5921fe_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS5921FE_H */
