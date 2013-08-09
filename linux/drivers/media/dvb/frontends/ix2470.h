/*
	IX2470 Silicon tuner driver

	Copyright (C) Manu Abraham <abraham.manu@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __IX2470_H
#define __IX2470_H


enum ix2470_step {
	IX2470_STEP_1000 = 0,	/* 1000 kHz */
	IX2470_STEP_500		/*  500 kHz */
};

enum ix2470_bbgain {
	IX2470_GAIN_0dB = 1,	/*  0dB Att */
	IX2470_GAIN_2dB,	/* -2dB Att */
	IX2470_GAIN_4dB		/* -4dB Att */
};

#if 0
enum ix2470_cpump {
	IX2470_CP_120uA,	/*  120uA */
	IX2470_CP_260uA,	/*  260uA */
	IX2470_CP_555uA,	/*  555uA */
	IX2470_CP_1200uA,	/* 1200uA */
};
#endif

struct ix2470_cfg {
	u8 			name[32];
	u8 			addr;
	enum ix2470_step 	step_size;
	enum ix2470_bbgain	bb_gain;
	u8			t_lock;
};

struct ix2470_devctl {
	int (*tuner_init) (struct dvb_frontend *fe);
	int (*tuner_sleep) (struct dvb_frontend *fe);
	int (*tuner_set_frequency) (struct dvb_frontend *fe, u32 frequency);
	int (*tuner_get_frequency) (struct dvb_frontend *fe, u32 *frequency);
	int (*tuner_set_bandwidth) (struct dvb_frontend *fe, u32 bandwidth);
	int (*tuner_get_bandwidth) (struct dvb_frontend *fe, u32 *bandwidth);
	int (*tuner_set_bbgain) (struct dvb_frontend *fe, u32 gain);
	int (*tuner_get_bbgain) (struct dvb_frontend *fe, u32 *gain);
	int (*tuner_set_refclk)  (struct dvb_frontend *fe, u32 refclk);
	int (*tuner_get_status) (struct dvb_frontend *fe, u32 *status);
};


#if defined(CONFIG_DVB_IX2470) || (defined(CONFIG_DVB_IX2470_MODULE) && defined(MODULE))

extern struct ix2470_devctl *ix2470_attach(struct dvb_frontend *fe,
					   const struct ix2470_cfg *cfg,
					   struct i2c_adapter *i2c);
#else

static inline struct ix2470_devctl *ix2470_attach(struct dvb_frontend *fe,
						  const struct ix2470_cfg *cfg,
						  struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_IX2470 */

#endif /* __IX2470_H */
