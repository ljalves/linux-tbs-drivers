/*
    TurboSight TBS 5680 DVB-C CI controls
    Copyright (C) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2011 TurboSight.com
*/

#ifndef TBS5680CTRL_H
#define TBS5680CTRL_H

extern int tbs5680ctrl1(struct usb_device *dev, u8 *a);
extern int tbs5680ctrl2(struct usb_device *dev, u8 *a);

#endif
