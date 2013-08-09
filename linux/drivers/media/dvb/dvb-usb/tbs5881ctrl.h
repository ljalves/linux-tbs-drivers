/*
    TurboSight TBS 5881 DVB-C CI controls
    Copyright (C) 2013 Konstantin Dimitrov <kosio.dimitrov@gmail.com>

    Copyright (C) 2013 TurboSight.com
*/

#ifndef TBS5881CTRL_H
#define TBS5881CTRL_H

extern int tbs5881ctrl1(struct usb_device *dev, u8 *a);
extern int tbs5881ctrl2(struct usb_device *dev, u8 *a);

#endif
