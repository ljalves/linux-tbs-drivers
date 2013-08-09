/*
    TurboSight TBS 5220 DVB-C CI controls
    Copyright (C) 2013 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2013 TurboSight.com
*/

#ifndef TBS5220CTRL_H
#define TBS5220CTRL_H

extern int tbs5220ctrl1(struct usb_device *dev, u8 *a);
extern int tbs5220ctrl2(struct usb_device *dev, u8 *a);

#endif
