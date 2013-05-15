/*
    TurboSight TBS 6925 DVBS/S2 controls
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS6925CTRL_H
#define TBS6925CTRL_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6925ctrl_config {
	u8 tbs6925ctrl_address;

	int (*tbs6925_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6925_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6925CTRL) || \
	(defined(CONFIG_DVB_TBS6925CTRL_MODULE) && defined(MODULE))


extern struct dvb_frontend *tbs6925ctrl_attach(
	struct dvb_frontend *fe,
	struct i2c_adapter *i2c,
	const struct tbs6925ctrl_config *config);

#else
static inline struct dvb_frontend *tbs6925ctrl_attach(
	struct dvb_frontend *fe,
	struct i2c_adapter *i2c,
	const struct tbs6925ctrl_config *config)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}

#endif /* CONFIG_DVB_TBS6925CTRL */

#endif /* TBS6925CTRL_H */
