#ifndef _TBSDVBC_H_
#define _TBSDVBC_H_

#define DVB_USB_LOG_PREFIX "tbsdvbc"
#include "dvb-usb.h"

#define deb_xfer(args...) dprintk(dvb_usb_tbsdvbc_debug, 0x02, args)
#endif
