/*
    TurboSight TBS FE module
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBSFE_H
#define TBSFE_H

#include <linux/dvb/frontend.h>

#if defined(CONFIG_DVB_TBSFE) || \
	(defined(CONFIG_DVB_TBSFE_MODULE) && defined(MODULE))
extern struct dvb_frontend *tbsfe_attach(
	struct dvb_frontend *fe);
#else
static inline struct dvb_frontend *tbsfe_attach(
	struct dvb_frontend *fe)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}

#endif /* CONFIG_DVB_TBSFE */

#endif /* TBSFE_H */
