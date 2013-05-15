/*
    TurboSight TBS 8922 DVBS/S2 Satellite Demodulator/Tuner driver
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS8922FE_H
#define TBS8922FE_H

#include <linux/dvb/frontend.h>
#include "cx88.h"

struct tbs8922fe_config {
	u8 tbs8922fe_address;

	int (*tbs8922_ctrl1)(struct cx88_core *core, int a);
	int (*tbs8922_ctrl2)(struct cx88_core *core, int a, int b);
};

#if defined(CONFIG_DVB_TBS8922FE) || \
	(defined(CONFIG_DVB_TBS8922FE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbs8922fe_attach(
	const struct tbs8922fe_config *config,
	struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *tbs8922fe_attach(
	const struct tbs8922fe_config *config,
	struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* TBS8922FE_H */
