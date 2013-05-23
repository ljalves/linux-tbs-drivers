/*
    Technotrend TT-budget S2-4100 driver
    
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>
*/

#ifndef __SAA716x_TT_DRV_H
#define __SAA716x_TT_DRV_H

extern int tt_drv1(struct saa716x_dev *dev, int tt1);
extern int tt_drv2(struct saa716x_dev *dev, int tt1, int tt2);

#endif
