/*
	TurboSight TBS 6926 Silicon Tuner driver
    	Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    	Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS6926_H
#define TBS6926_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6926_config {
	u8 tbs6926_address;
	
	int (*tbs6926_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6926_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

struct tbs6926_state {
	struct i2c_adapter *i2c;
	const struct tbs6926_config *config;
	struct dvb_frontend *frontend;
	int status;
};

#if defined(CONFIG_DVB_TBS6926) || \
	(defined(CONFIG_DVB_TBS6926_MODULE) && defined(MODULE))

extern struct dvb_frontend *tbs6926_attach(
	struct dvb_frontend *fe,
	const struct tbs6926_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs6926_attach(
	struct dvb_frontend *fe,
	const struct tbs6926_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6926_H */
