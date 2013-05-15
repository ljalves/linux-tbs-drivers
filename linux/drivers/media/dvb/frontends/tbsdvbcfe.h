/*
    TurboSight TBS DVBC frontend driver
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBSDVBCFE_H
#define TBSDVBCFE_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbsdvbcfe_config {
	u8 tbsdvbcfe_address;

	int (*tbsdvbc_ctrl)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBSDVBCFE) || \
	(defined(CONFIG_DVB_TBSDVBCFE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbsdvbcfe_attach(
	const struct tbsdvbcfe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbsdvbcfe_attach(
	const struct tbsdvbcfe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBSDVBCFE_H */
