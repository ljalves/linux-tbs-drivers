/*
    TurboSight TBS 5925 DVB-S/S2 controls
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2012 TurboSight.com
*/

#ifndef TBS5925CTRL_H
#define TBS5925CTRL_H

extern int tbs5925ctrl1(struct usb_device *dev, u8 *a);
extern int tbs5925ctrl2(struct usb_device *dev, u8 *a);

#endif
