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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <asm/div64.h>

#include "dvb_frontend.h"
#include "ix2470.h"

#define FE_ERROR				0
#define FE_NOTICE				1
#define FE_INFO					2
#define FE_DEBUG				3
#define FE_DEBUGREG				4

#define dprintk(__y, __z, format, arg...) do {						\
	if (__z) {									\
		if	((verbose > FE_ERROR) && (verbose > __y))			\
			printk(KERN_ERR "%s: " format "\n", __func__ , ##arg);		\
		else if	((verbose > FE_NOTICE) && (verbose > __y))			\
			printk(KERN_NOTICE "%s: " format "\n", __func__ , ##arg);	\
		else if ((verbose > FE_INFO) && (verbose > __y))			\
			printk(KERN_INFO "%s: " format "\n", __func__ , ##arg);		\
		else if ((verbose > FE_DEBUG) && (verbose > __y))			\
			printk(KERN_DEBUG "%s: " format "\n", __func__ , ##arg);	\
	} else {									\
		if (verbose > __y)							\
			printk(format, ##arg);						\
	}										\
} while (0)

static unsigned int verbose;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Set Verbosity level");


struct ix2470_state {
	struct dvb_frontend	*fe;
	struct i2c_adapter	*i2c;
	const struct ix2470_cfg *cfg;
	struct ix2470_devctl	*ctl;
	u8 			reg[4];
};

static int ix2470_write(struct ix2470_state *ix2470, u8 *buf, u8 len)
{
	const struct ix2470_cfg *cfg = ix2470->cfg;
	struct i2c_adapter *i2c = ix2470->i2c;
	int ret = 0;
	struct i2c_msg msg = { .addr = cfg->addr, .flags = 0, .buf = buf, .len = len };

	ret = i2c_transfer(i2c, &msg, 1);
	if (ret != 1) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return 0;
}

static int ix2470_read(struct ix2470_state *ix2470, u8 *buf, u8 len)
{
	const struct ix2470_cfg *cfg = ix2470->cfg;
	struct i2c_adapter *i2c = ix2470->i2c;
	int ret = 0;

	struct i2c_msg msg = { .addr = cfg->addr, .flags = I2C_M_RD, .buf = buf, .len = len };

	ret = i2c_transfer(i2c, &msg, 1);
	if (ret != 1) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return 0;
}

static inline u32 ix2470_do_div(u64 n, u32 d)
{
	do_div(n, d);
	return n;
}

static int ix2470_set_frequency(struct dvb_frontend *fe, u32 Frequency)
{
	struct ix2470_state *ix2470 = fe->tuner_priv;

	u64 freq, fVCO, tmp;

	u32 divider;

	u8 cfg[4];
	u32 N,A,P,R, PSC, DIV, BA, REF, PD0, BG, CP;
	u8 b1, PD23, PD45, LPF;
	int ret = 0;

	#define F_XTAL   4000000        // 4MHz Quarz Oscillator

	dprintk(FE_DEBUG, 1, "Frequency: %d ", Frequency);
	freq = Frequency;

	if ((freq < 950000) || (freq > 2150000)) {
		dprintk(FE_ERROR, 1, "SetFrequency - out of range [%llu]!", freq);
		return -EINVAL;
	}

	freq *= 1000; /* convert to Hz */
	if (freq <= 0)
		return -EINVAL;

	fVCO = freq;

	cfg[0] = 0x40;
	cfg[1] = 0x00;
	cfg[2] = 0xE0;
	cfg[3] = 0x00;

	cfg[0] &= 0x9F;
	cfg[0] |= 0x20;
	cfg[2] &= 0xE3;
	cfg[3] &= 0xF3;

	dprintk(FE_DEBUG, 1, "B0-B4: 0x%02x 0x%02x 0x%02x 0x%02x", cfg[0], cfg[1], cfg[2], cfg[3]);
	ret = ix2470_write(ix2470, cfg, 4); /* Init */
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}

	// select band
	if (fVCO <= 986000000) {
		PSC = 1;
		DIV = 1;
		BA = 5;
	} else if (fVCO <= 1073000000) {
		PSC = 1;
		DIV = 1;
		BA = 6;
	} else if (fVCO <= 1154000000) {
		PSC = 0;
		DIV = 1;
		BA = 7;
	} else if (fVCO <= 1291000000) {
		PSC = 0;
		DIV = 0;
		BA = 1;
	} else if (fVCO <= 1447000000) {
		PSC = 0;
		DIV = 0;
		BA = 2;
	} else if (fVCO <= 1615000000) {
		PSC = 0;
		DIV = 0;
		BA = 3;
	} else if (fVCO <= 1791000000) {
		PSC = 0;
		DIV = 0;
		BA = 4;
	} else if (fVCO <= 1972000000) {
		PSC = 0;
		DIV = 0;
		BA = 5;
	} else {
		PSC = 0;
		DIV = 0;
		BA = 6;
	}

	if (fVCO < 1024000000L)
		REF = 1; // divider = 500 kHz
	else
		REF = 0; // divider = 1000 kHz

	if (fVCO <= 1375000000L)
		PSC = 1;
	else
		PSC = 0;

	// calculate divider
	if (REF)
		R = 8;  // xtal/r =  500 kHz
	else
		R = 4;  // xtal/r = 1000 kHz

	if (PSC)
		P = 16;
	else
		P = 32;

	PD0 = 0; //POWER ON
	BG  = 2; //-4dB
	CP  = 3; //CHARGE PUMP

	// charge pump
	if (freq < 1450000000) {
		// lowband
		if (fVCO < 1300000000)
			CP = 1;         // 500 µA
		else if (fVCO < 1375000000)
			CP = 2;         // 1 mA
		else
			CP = 3;         // 2 mA
	} else {
		// highband
		if (fVCO < 1850000000)
			CP = 1;         // 500 µA
		else if (fVCO < 2045000000)
			CP = 2;         // 1 mA
		else
			CP = 3;         // 2 mA
	}

	cfg[3] |= (u8) (PSC << 4);
	cfg[3] |= (u8) (BA << 5);
	cfg[3] |= (u8) (DIV << 1);

	divider = F_XTAL / R;

	// round-divide
	fVCO += (divider / 2);
	do_div(fVCO, divider); // fVCO /= divider

	tmp = fVCO;
	do_div(tmp, P); // N = tmp / P
	N = tmp;

	tmp = fVCO;
	A = tmp - (P * N);

	cfg[0] |= (u8) ((BG & 0x03) << 5);
	cfg[0] |= (u8) ((N >> 3) & 0x1F);
	cfg[1] |= (u8) ((N & 0x07) << 5);
	cfg[1] |= (u8) ((A & 0x1F) << 0);
	cfg[2] |= (u8) (REF & 0x01);
	cfg[2] |= (u8) ((CP & 0x02) << 5);

	b1 = cfg[0];

	dprintk(FE_DEBUG, 1, "B0-B3: 0x%02x 0x%02x 0x%02x 0x%02x", cfg[0], cfg[1], cfg[2], cfg[3]);
	// vco/lpf adjustment mode clear
	ret = ix2470_write(ix2470, cfg, 4);
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}

	// vco/lpf adjustment mode setting
	cfg[2] |= 0x04; // TM = 1

	dprintk(FE_DEBUG, 1, "B2: 0x%02x", cfg[2]);
	ret = ix2470_write(ix2470, &cfg[2], 1);
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}

	msleep(50);
	LPF  = 34; //34MHz
	PD23 = 0;
	PD45 = 0;

	switch (LPF) {
	case  0:
		PD23 = 0;
		PD45 = 0;
		break;
	case 10:
		PD23 = 0;
		PD45 = 3;
		break;
	case 12:
		PD23 = 2;
		PD45 = 0;
		break;
	case 14:
		PD23 = 2;
		PD45 = 2;
		break;
	case 16:
		PD23 = 2;
		PD45 = 1;
		break;
	case 18:
		PD23 = 2;
		PD45 = 3;
		break;
	case 20:
		PD23 = 1;
		PD45 = 0;
		break;
	case 22:
		PD23 = 1;
		PD45 = 2;
		break;
	case 24:
		PD23 = 1;
		PD45 = 1;
		break;
	case 26:
		PD23 = 1;
		PD45 = 3;
		break;
	case 28:
		PD23 = 3;
		PD45 = 0;
		break;
	case 30:
		PD23 = 3;
		PD45 = 2;
		break;
	case 32:
		PD23 = 3;
		PD45 = 1;
		break;
	case 34:
		PD23 = 3;
		PD45 = 3;
		break;
	}

	cfg[2]|=(u8)(PD45 << 3);
	cfg[3]|=(u8)(PD23 << 2);

	dprintk(FE_DEBUG, 1, "B2-B3: 0x%02x 0x%02x", cfg[2], cfg[3]);
	ret = ix2470_write(ix2470, &cfg[2], 2);
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}

	cfg[0] = b1;
	dprintk(FE_DEBUG, 1, "B0: 0x%02x", cfg[0]);
	ret = ix2470_write(ix2470, cfg, 1);
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}
	msleep(100);
err:
	return ret;
}

static int ix2470_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct ix2470_state *state = fe->tuner_priv;
	u8 result;
	int err = 0;

	*status = 0;

	err = ix2470_read(state, &result, 1);
	if (err < 0) {
		printk("%s: I/O Error\n", __func__);
		return err;
	}

	dprintk(FE_DEBUG, 1, "STATUS: REG:0x%02x", result);
	if ((result >> 6) & 0x01) {
		printk("%s: Tuner Phase Locked\n", __func__);
		*status = 1;
	}

	return err;
}

static const struct ix2470_bw_sel {
	u8 val;
	u32 fc;
} bw_sel[] = {
	{ 0x0c, 10000 },
	{ 0x02, 12000 },
	{ 0x0a, 14000 },
	{ 0x06, 16000 },
	{ 0x0e, 18000 },
	{ 0x01, 20000 },
	{ 0x09, 22000 },
	{ 0x05, 24000 },
	{ 0x0d, 26000 },
	{ 0x03, 28000 },
	{ 0x0b, 30000 },
	{ 0x07, 32000 },
	{ 0x0f, 34000 },
};

static int ix2470_set_bw(struct dvb_frontend *fe, u32 bandwidth)
{
	struct ix2470_state *ix2470	= fe->tuner_priv;
	const struct ix2470_cfg *cfg	= ix2470->cfg;
	u8 *b 				= ix2470->reg;
	int i, ret = 0;
	u32 halfbw;

	halfbw = (bandwidth / 1000) >> 1;
	dprintk(FE_DEBUG, 1, "Bandwidth:%d", bandwidth);

	for (i = 0; i < ARRAY_SIZE(bw_sel); i++) {
		if (halfbw <= bw_sel[i].fc) {
			dprintk(FE_DEBUG, 1, "bw:%d halfbw:%d LPF fc:%d", bandwidth, halfbw, bw_sel[i].fc);
			break;
		}
	}
	if (i >= ARRAY_SIZE(bw_sel)) {
		dprintk(FE_ERROR, 1, "Error: out of array bounds!, i=%d", i);
		goto err;
	}

	b[2] |= ((bw_sel[i].val >> 2) & 0x03) << 3; /* Byte 4 PD:5, PD:4 */
	b[3] |= (bw_sel[i].val & 0x03) << 2; /* Byte 5 PD:3, PD:2 */

	dprintk(FE_DEBUG, 1, "B4,B5: 0x%02x 0x%02x", b[2], b[3]);
	ret = ix2470_write(ix2470, &b[2], 2); /* Byte1, Byte4 - Byte5 */
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}
	if (cfg->t_lock)
		msleep(cfg->t_lock);
	else
		msleep(200);
err:
	return ret;
}

static const struct ix2470_losc_sel {
	u32 f_ll;
	u32 f_ul;

	u8 band;
	u8 psc;
	u8 div;
	u8 ba210;
} losc_sel[] = {
	{  950000,  986000, 0x01, 0x10, 0x01, 0x05 },
	{  986000, 1073000, 0x02, 0x10, 0x01, 0x06 },
	{ 1073000, 1154000, 0x03, 0x20, 0x01, 0x07 },
	{ 1154000, 1291000, 0x04, 0x20, 0x00, 0x01 },
	{ 1291000, 1447000, 0x05, 0x20, 0x00, 0x02 },
	{ 1447000, 1615000, 0x06, 0x20, 0x00, 0x03 },
	{ 1615000, 1791000, 0x07, 0x20, 0x00, 0x04 },
	{ 1791000, 1972000, 0x08, 0x20, 0x00, 0x05 },
	{ 1972000, 2150000, 0x09, 0x20, 0x00, 0x06 },
};

static const int steps[2] = { 1000, 500 };

static int ix2470_set_freq(struct dvb_frontend *fe, u32 frequency)
{
	struct ix2470_state *ix2470	= fe->tuner_priv;
	const struct ix2470_cfg *cfg	= ix2470->cfg;
	u8 *b 				= ix2470->reg;

	int i, ret = 0;
	u8 N, A, bb_gain, pump;
	u32 data;

	if (cfg->bb_gain)
		bb_gain = cfg->bb_gain;
	else
		bb_gain = IX2470_GAIN_2dB; /* 0 */

	dprintk(FE_DEBUG, 1, "Set frequency to:%d", frequency);

	/* get dividers corresponding to frequency */
	for (i = 0; i < ARRAY_SIZE(losc_sel); i++) {
		if (losc_sel[i].f_ll <= frequency && frequency < losc_sel[i].f_ul) {
			dprintk(FE_DEBUG, 1, "Freq:%d UL:%d LL:%d Band:%d PSC:%d Div:%d BA210:%d",
				frequency,
				losc_sel[i].f_ul,
				losc_sel[i].f_ll,
				losc_sel[i].band,
				losc_sel[i].psc,
				losc_sel[i].div,
				losc_sel[i].ba210);
			break;
		}
	}
	if (i >= ARRAY_SIZE(losc_sel)) {
		dprintk(FE_ERROR, 1, "Error: out of array bounds!, i=%d", i);
		goto err;
	}

	/*
	 * fvco = (P*N + A) x fosc/R
	 *		fvco	= tune frequency
	 *		fosc	= 4MHz
	 *		R	= 4
	 *
	 *		A	= swallow division ratio (0 - 31; A < N)
	 *		N	= Programmable division ratio (5 - 255)
	 *		P	= Prescaler division ratio
	 */
	data = ix2470_do_div(frequency, steps[cfg->step_size] + 1);
	N = data / losc_sel[i].psc; /* N = data / P */
	A = data - losc_sel[i].psc * N; /* A = data - P * N */
	dprintk(FE_DEBUG, 1, "BBGAIN=%d N=%d A=%d data=%d", cfg->bb_gain, N, A, data);

	b[0] = ((bb_gain & 0x3) << 5) | (N >> 3);
	b[1] = (N << 5) | (A & 0x1f);
	b[2] = 0x80 | ((pump & 0x3) << 5); /* Byte 4 */
	b[3] = (losc_sel[i].ba210 << 5);

	dprintk(FE_DEBUG, 1, "B2-B5: 0x%02x 0x%02x 0x%02x 0x%02x", b[0], b[1], b[2], b[3]);
	ret = ix2470_write(ix2470, b, 4); /* Byte1 - Byte5 */
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}
	b[2] |= (1 << 2); /* TM = 1, VCO/LPF adjustment mode */

	dprintk(FE_DEBUG, 1, "B4: 0x%02x", b[2]);
	ret = ix2470_write(ix2470, &b[2], 1); /* Byte1 & Byte4 */
	if (ret) {
		dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
		goto err;
	}
	msleep(10);
err:
	return ret;
}


static int ix2470_release(struct dvb_frontend *fe)
{
	struct ix2470_state *ix2470 = fe->tuner_priv;

	fe->tuner_priv = NULL;
	kfree(ix2470);
	return 0;
}

static struct dvb_tuner_ops ix2470_ops = {
	.info = {
		.name			= "IX2470",
		.frequency_min		=  950000,
		.frequency_step		= 2150000,
		.frequency_step		=    1000,
	},

	.release	= ix2470_release,
};

static struct ix2470_devctl ix2470_ctl = {
	.tuner_set_frequency	= ix2470_set_frequency,
//	.tuner_set_bandwidth	= ix2470_set_bw,
	.tuner_get_status	= ix2470_get_status,
};


struct ix2470_devctl *ix2470_attach(struct dvb_frontend *fe,
				    const struct ix2470_cfg *cfg,
				    struct i2c_adapter *i2c)
{
	struct ix2470_state *ix2470;

	ix2470 = kzalloc(sizeof (struct ix2470_state), GFP_KERNEL);
	if (!ix2470)
		goto exit;

	ix2470->i2c		= i2c;
	ix2470->fe		= fe;
	ix2470->cfg		= cfg;
	ix2470->ctl		= &ix2470_ctl;

	fe->tuner_priv		= ix2470;
	fe->ops.tuner_ops	= ix2470_ops;
	dprintk(FE_ERROR, 1, "Attaching %s IX2470 QPSK/8PSK tuner", cfg->name);
	return ix2470->ctl;
exit:
	kfree(ix2470);
	return NULL;
}
EXPORT_SYMBOL(ix2470_attach);

MODULE_AUTHOR("Manu Abraham");
MODULE_DESCRIPTION("IX2470 Silicon tuner");
MODULE_LICENSE("GPL");
