TARGETS = vicam/firmware.fw dabusb/firmware.fw dabusb/bitstream.bin ttusb-budget/dspbootcode.bin cpia2/stv0672_vp4.bin av7110/bootcode.bin \
	v4l-cx23885-avcore-01.fw dvb-fe-cx24116.fw dvb-fe-ds3000.fw dvb-fe-ds300x.fw dvb-fe-ds3103.fw \
	dvb-usb-dw2101.fw dvb-usb-dw2102.fw dvb-usb-dw2104.fw dvb-usb-dw3101.fw \
	dvb-usb-tbsqbox-id2601.fw dvb-usb-tbsqbox-id5910.fw dvb-usb-tbsqbox-id5920.fw dvb-usb-tbsqbox-id5925.fw dvb-usb-tbsqbox-id5980.fw \
	dvb-usb-tbsqbox-id5921.fw dvb-usb-tbsqbox-id5922.fw dvb-usb-tbsqbox-id5928.fw \
	dvb-usb-tbsqbox-dvbc.fw dvb-usb-tbsqbox-id5680.fw dvb-usb-tbsqbox-id5880.fw \
	dvb-usb-p1100.fw dvb-usb-p1100-old.fw dvb-usb-p7500.fw dvb-usb-s620.fw dvb-usb-s630.fw dvb-usb-s660.fw dvb-usb-sb340.fw \
	dvb-demod-drxk-pctv.fw dvb-fe-tda10045.fw dvb-fe-tda10046.fw dvb-fe-tda10048-1.0.fw dvb-fe-tda10071.fw \
	dvb-usb-tt-s2400-01.fw dvb-usb-pctv-400e-01.fw dvb-usb-pctv-450e-01.fw dvb-usb-pctv-452e-01.fw \
	dvb-usb-SkyStar_USB_HD_FW_v17_63.HEX.fw

FW_DIR  = $(DESTDIR)/lib/firmware

####

DIRS = $(sort $(dir $(TARGETS)))

.PHONY = default clean distclean install rminstall prep

default: $(TARGETS)

$(TARGETS):: ihex2fw

clean:
	-rm -f ihex2fw
	-rm -f $(TARGETS)

distclean: clean
	-for i in $(DIRS); do if [ -d $$i ]; then rm -rf $$i; fi; done


install: default
	@echo -n "Installing firmwares at $(FW_DIR): "
	-@for i in $(DIRS); do if [ ! -d $(FW_DIR)/$$i ]; then mkdir -p $(FW_DIR)/$$i; fi; done
	-@for i in $(TARGETS); do echo -n "$$i "; cp $$i $(FW_DIR)/$$i; done
	@echo

rminstall:
	@echo "Removing firmwares at $(FW_DIR) "
	-for i in $(DIRS); do if [ -d $(FW_DIR)/$$i ]; then rm -rf $(FW_DIR)/$$i; fi; done

######

ihex2fw: ../../linux/firmware/ihex2fw.c
	@echo "  CC  $@"
	@gcc -Wall -o $@ $<

prep:
	@for i in $(DIRS); do						\
	if [ ! -d $$i ]; then mkdir -p $$i; fi;				\
	for j in ../../linux/firmware/$$i/*; do				\
		n=`echo $$j | sed s,../../linux/firmware/,,`;		\
		if [ ! -e $$n ]; then					\
			ln -f $$j $$n;					\
		fi;							\
	done; done

%.fw: %.H16
	@echo Generating $@
	@./ihex2fw -w $< $@

%.fw: %.HEX
	@echo Generating $@
	@./ihex2fw $< $@

%.bin: %.bin.ihex
	@echo Generating $@
	@objcopy -Iihex -Obinary $< $@

# TODO: Use the Firmware Kconfig dependencies
#fw-$(CONFIG_USB_VICAM) += vicam/firmware.fw
#fw-$(CONFIG_USB_DABUSB) += dabusb/firmware.fw dabusb/bitstream.bin
#fw-$(CONFIG_DVB_TTUSB_BUDGET) += ttusb-budget/dspbootcode.bin
#fw-$(CONFIG_VIDEO_CPIA2) += cpia2/stv0672_vp4.bin
#fw-$(CONFIG_DVB_AV7110) += av7110/bootcode.bin

