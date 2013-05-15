/*
    TurboSight TBS 5925 FE DVBS/S2 controls
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS5925FECTRL_H
#define TBS5925FECTRL_H

#include <linux/dvb/frontend.h>
#include "dvb-usb.h"

struct tbs5925fe_ctrl_config {
	u8 tbs5925fe_ctrl_address;

	int (*tbs5925fe_ctrl1)(struct usb_device *dev, u8 *a);
	int (*tbs5925fe_ctrl2)(struct usb_device *dev, u8 *a);
};

#if defined(CONFIG_DVB_TBS5925FECTRL) || \
	(defined(CONFIG_DVB_TBS5925FECTRL_MODULE) && defined(MODULE))


extern struct dvb_frontend *tbs5925fe_ctrl_attach(
	struct dvb_frontend *fe,
	struct i2c_adapter *i2c,
	const struct tbs5925fe_ctrl_config *config);

#else
static inline struct dvb_frontend *tbs5925fe_ctrl_attach(
	struct dvb_frontend *fe,
	struct i2c_adapter *i2c,
	const struct tbs5925fe_ctrl_config *config)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}

#endif /* CONFIG_DVB_TBS5925FECTRL */

#endif /* TBS5925FECTRL_H */
