/*
    TurboSight TBS FE module
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#include "dvb_frontend.h"
#include "tbsfe.h"

/* phase poise enhancements */
static int pne;
module_param(pne, int, 0644);
MODULE_PARM_DESC(pne, "Phase noise enhancements 0:Off, "\
	"1:On (default:0)");

/* define how SNR measurement is reported */
static int esno;
module_param(esno, int, 0644);
MODULE_PARM_DESC(esno, "SNR is reported in 0:Percentage, "\
	"1:(EsNo dB)*10 (default:0)");

/* define how signal measurement is reported */
static int dbm;
module_param(dbm, int, 0644);
MODULE_PARM_DESC(dbm, "Signal is reported in 0:Percentage, "\
	"1:-1*dBm (default:0)");

/* define how outer code correction is performed */
static int occ = 1;
module_param(occ, int, 0644);
MODULE_PARM_DESC(occ, "0:outer code correction is disabled, "\
	"1: outer code correction is enabled(default:1)");

static int tbsfe_params(struct dvb_frontend *fe, long arg)
{
	return (occ << 3) | (pne << 2) | (esno << 1) | dbm;
}

static void tbsfe_release(struct dvb_frontend *fe)
{
}

struct dvb_frontend *tbsfe_attach(struct dvb_frontend *fe)
{
	fe->ops.release_sec = tbsfe_release;
	fe->ops.enable_high_lnb_voltage = tbsfe_params;

	return fe;
}
EXPORT_SYMBOL(tbsfe_attach);

MODULE_DESCRIPTION("TurboSight TBS FE module");
MODULE_AUTHOR("Konstantin Dimitrov <kosio.dimitrov@gmail.com>");
MODULE_LICENSE("GPL");
