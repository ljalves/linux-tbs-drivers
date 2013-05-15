/*
    TurboSight TBS 6618 DVBC frontend driver
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS6618FE_H
#define TBS6618FE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6618fe_config {
	u8 tbs6618fe_address;

	int (*tbs6618_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6618_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6618FE) || \
	(defined(CONFIG_DVB_TBS6618FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6618fe_attach(
	const struct tbs6618fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs6618fe_attach(
	const struct tbs6618fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6618FE_H */
