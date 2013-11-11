/*
    TurboSight TBS 6985SE DVBS/S2 Satellite Demodulator/Tuner driver
    Copyright (C) 2013 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2013 TurboSight.com
*/

#ifndef TBS6985SE_H
#define TBS6985SE_H

#include <linux/dvb/frontend.h>
#include "saa716x_priv.h"

struct tbs6985se_config {
	u8 tbs6985se_address;
	
	int (*tbs6985se_ctrl1)(struct saa716x_dev *dev, int a);
	int (*tbs6985se_ctrl2)(struct saa716x_dev *dev, int a, int b);
};

#if defined(CONFIG_DVB_TBS6985SE) || \
	(defined(CONFIG_DVB_TBS6985SE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs6985se_attach(
	const struct tbs6985se_config *config,
	struct i2c_adapter *i2c, int demod);
#else
static inline struct dvb_frontend *tbs6985se_attach(
	const struct tbs6985se_config *config,
	struct i2c_adapter *i2c, int demod)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS6985SE_H */
