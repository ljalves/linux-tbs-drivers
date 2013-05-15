/*
    TurboSight TBS 6926 DVBS/S2 controls
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS6926CTRL_H
#define TBS6926CTRL_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6926ctrl_config {
	u8 tbs6926ctrl_address;

	int (*tbs6926_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6926_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6926CTRL) || \
	(defined(CONFIG_DVB_TBS6926CTRL_MODULE) && defined(MODULE))


extern struct dvb_frontend *tbs6926ctrl_attach(
	struct dvb_frontend *fe,
	struct i2c_adapter *i2c,
	const struct tbs6926ctrl_config *config);

#else
static inline struct dvb_frontend *tbs6926ctrl_attach(
	struct dvb_frontend *fe,
	struct i2c_adapter *i2c,
	const struct tbs6926ctrl_config *config)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}

#endif /* CONFIG_DVB_TBS6926CTRL */

#endif /* TBS6926CTRL_H */
