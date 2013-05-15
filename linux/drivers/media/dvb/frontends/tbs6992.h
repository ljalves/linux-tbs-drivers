/*
	TurboSight TBS 6992 Silicon Tuner driver
    	Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    	Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS6992_H
#define TBS6992_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6992_config {
	u8 tbs6992_address;
	
	int (*tbs6992_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6992_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

struct tbs6992_state {
	struct i2c_adapter *i2c;
	const struct tbs6992_config *config;
	struct dvb_frontend *frontend;
	int status;
};

#if defined(CONFIG_DVB_TBS6992) || \
	(defined(CONFIG_DVB_TBS6992_MODULE) && defined(MODULE))

extern struct dvb_frontend *tbs6992_attach(
	struct dvb_frontend *fe,
	const struct tbs6992_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs6992_attach(
	struct dvb_frontend *fe,
	const struct tbs6992_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6992_H */
