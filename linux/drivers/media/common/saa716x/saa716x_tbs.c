#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/device.h>

#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/i2c.h>

#include "saa716x_mod.h"

#include "saa716x_msi_reg.h"
#include "saa716x_gpio_reg.h"
#include "saa716x_dma_reg.h"
#include "saa716x_fgpi_reg.h"
#include "saa716x_greg_reg.h"

#include "saa716x_vip.h"
#include "saa716x_aip.h"
#include "saa716x_msi.h"
#include "saa716x_adap.h"
#include "saa716x_gpio.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

#include "saa716x_input.h"
#include <media/rc-core.h>

#include "saa716x_tbs.h"
#include "tbs62x0fe.h"
#include "tbsctrl.h"

#include "tda18212.h"
#include "cxd2820r.h"

#include "stv6110x.h"
#include "stv090x.h"

#include "stb6100.h"
#include "stb6100_cfg.h"

#include "tbs6984fe.h"
#include "isl6423.h"

#include "tbs6992.h"
#include "tbs6992_cfg.h"

#include "tbsci-i2c.h"
#include "tbsci.h"

#include "tbs6922fe.h"
#include "tbs6928fe.h"
#include "tbs6928se.h"
#include "tbs6982fe.h"
#include "tbs6991fe.h"

#include "tbs6618fe.h"
#include "tbs6680fe.h"

#include "tbs6985fe.h"

#include "tbs6926.h"
#include "tbs6926_cfg.h"
#include "tbs6926ctrl.h"

#include "tbs6923fe.h"

#include "tbs6982se.h"

#include "tbs62x1fe.h"

#include "tbsfe.h"

#include "tbsmac.h"

#include "saa716x_tt_drv.h"
#include "tt_s2_4100.h"

unsigned int verbose;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "verbose startup messages, default is 1 (yes)");

unsigned int int_type;
module_param(int_type, int, 0644);
MODULE_PARM_DESC(int_type, "force Interrupt Handler type: 0=INT-A, 1=MSI, 2=MSI-X. default INT-A mode");

unsigned int ci_mode;
module_param(ci_mode, int, 0644);
MODULE_PARM_DESC(ci_mode, "for internal use only: default 0");

unsigned int ci_spd;
module_param(ci_spd, int, 0644);
MODULE_PARM_DESC(ci_spd, "for internal use only: default 0");

static unsigned int enable_ir = 1;
module_param(enable_ir, int, 0644);
MODULE_PARM_DESC(enable_ir, "Enable IR support for TBS cards: default 1");

static unsigned int cxd2820r = 1;
module_param(cxd2820r, int, 0644);
MODULE_PARM_DESC(cxd2820r, "Enable open-source TDA18212/CXD2820r drivers for TBS 62x0, 6284 cards: default 1");

#define DRIVER_NAME "SAA716x TBS"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devinit saa716x_tbs_pci_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
#else
static int saa716x_tbs_pci_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
#endif
{
	struct saa716x_dev *saa716x;
	int err = 0;
	u32 data;

	saa716x = kzalloc(sizeof (struct saa716x_dev), GFP_KERNEL);
	if (saa716x == NULL) {
		printk(KERN_ERR "saa716x_tbs_pci_probe ERROR: out of memory\n");
		err = -ENOMEM;
		goto fail0;
	}

	saa716x->verbose	= verbose;
	saa716x->int_type	= int_type;
	saa716x->pdev		= pdev;
	saa716x->config	= (struct saa716x_config *) pci_id->driver_data;

	err = saa716x_pci_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x PCI Initialization failed");
		goto fail1;
	}

	err = saa716x_cgu_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x CGU Init failed");
		goto fail1;
	}

	err = saa716x_core_boot(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x Core Boot failed");
		goto fail2;
	}
	dprintk(SAA716x_DEBUG, 1, "SAA716x Core Boot Success");

	err = saa716x_msi_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x MSI Init failed");
		goto fail2;
	}

	err = saa716x_jetpack_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x Jetpack core initialization failed");
		goto fail1;
	}

	if (ci_spd) {
		if ((saa716x->config->model_name[17] == 0x39) &&
			(saa716x->config->model_name[18] == 0x31))
		{
				saa716x->config->i2c_rate[0] = SAA716x_I2C_RATE_100;
                                saa716x->config->i2c_rate[1] = SAA716x_I2C_RATE_100;
		}

		if ((saa716x->config->model_name[18] == 0x38) ||
			((saa716x->config->model_name[16] == 0x36) &&
			(saa716x->config->model_name[17] == 0x38)))
			saa716x->config->i2c_rate[1] = SAA716x_I2C_RATE_100;
	}

	err = saa716x_i2c_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x I2C Initialization failed");
		goto fail3;
	}
	saa716x_gpio_init(saa716x);

	if (enable_ir) {
		data = SAA716x_EPRD(MSI, MSI_CONFIG37);
		data &= 0xFCFFFFFF;
		data |= MSI_INT_POL_EDGE_ANY;
		SAA716x_EPWR(MSI, MSI_CONFIG37, data);
		SAA716x_EPWR(MSI, MSI_INT_ENA_SET_H, MSI_INT_EXTINT_4);

		saa716x_gpio_set_input(saa716x, saa716x->config->rc_gpio_in);
		msleep(1);
	
		saa716x_input_init(saa716x,saa716x->config->rc_gpio_in, saa716x->config->rc_map_name);
	}

	/* set default port mapping */
	SAA716x_EPWR(GREG, GREG_VI_CTRL, 0x2C688F0A);
	/* enable FGPI3, FGPI2, FGPI1 and FGPI0 for TS input from Port 2 and 6 */
	SAA716x_EPWR(GREG, GREG_FGPI_CTRL, 0x322);

	err = saa716x_dvb_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x DVB initialization failed");
		goto fail4;
	}

	return 0;

fail4:
	saa716x_dvb_exit(saa716x);
fail3:
	saa716x_i2c_exit(saa716x);
fail2:
	saa716x_pci_exit(saa716x);
fail1:
	kfree(saa716x);
fail0:
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static void __devexit saa716x_tbs_pci_remove(struct pci_dev *pdev)
#else
static void saa716x_tbs_pci_remove(struct pci_dev *pdev)
#endif
{
	struct saa716x_dev *saa716x = pci_get_drvdata(pdev);
	struct saa716x_adapter *saa716x_adap = saa716x->saa716x_adap;
	int i;

	for (i = 0; i < saa716x->config->adapters; i++) {
		if (saa716x_adap->tbsci) {
			tbsci_release(saa716x_adap);
			tbsci_i2c_remove(saa716x_adap);
		}
		saa716x_adap++;
	}
	
	if (enable_ir) {
		SAA716x_EPWR(MSI, MSI_INT_ENA_CLR_H, MSI_INT_EXTINT_4);
		saa716x_input_fini(saa716x);
	}

	saa716x_dvb_exit(saa716x);
	saa716x_i2c_exit(saa716x);
	saa716x_pci_exit(saa716x);
	kfree(saa716x);
}

static irqreturn_t saa716x_tbs6220_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);
	
	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6220(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6280_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6280(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_skystar2_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (stat_h & MSI_INT_EXTINT_4)
		saa716x_input_irq_handler(saa716x);

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_skystar2(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}


static irqreturn_t saa716x_tbs6925_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);
	
	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}
		
	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6925(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6984_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_0) {

			fgpiStatus = SAA716x_EPRD(FGPI0, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI0_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[2].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI0, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[3].demux, data, 348);
			}
                        if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_2) {

			fgpiStatus = SAA716x_EPRD(FGPI2, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI2_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI2, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {
			
			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
				if (activeBuffer > 0)
					activeBuffer -= 1;
					else
						activeBuffer = 7;
				if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
						u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
						dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
							data[0], data[1], data[2], data[3]);
					dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
				}
				if (fgpiStatus) {
					SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
					}
				}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6984(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6992_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6992(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6922_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);
	
	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6922(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}


static irqreturn_t saa716x_tbs6928_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6928(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6928se_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6928se(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6618_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6618(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6284_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_0) {

			fgpiStatus = SAA716x_EPRD(FGPI0, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI0_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[3].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI0, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[2].demux, data, 348);
			}
                        if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_2) {

			fgpiStatus = SAA716x_EPRD(FGPI2, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI2_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI2, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {
			
			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
				if (activeBuffer > 0)
					activeBuffer -= 1;
					else
						activeBuffer = 7;
				if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
						u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
						dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
							data[0], data[1], data[2], data[3]);
					dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
				}
				if (fgpiStatus) {
					SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
					}
				}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6284(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6982_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6982(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6982se_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6982se(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6991_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6991(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6680_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6680(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6985_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_0) {

			fgpiStatus = SAA716x_EPRD(FGPI0, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI0_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[2].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI0, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[3].demux, data, 348);
			}
                        if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_2) {

			fgpiStatus = SAA716x_EPRD(FGPI2, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI2_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI2, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {
			
			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
				if (activeBuffer > 0)
					activeBuffer -= 1;
					else
						activeBuffer = 7;
				if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
						u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
						dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
							data[0], data[1], data[2], data[3]);
					dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
				}
				if (fgpiStatus) {
					SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
					}
				}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6985(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6221_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);
	
	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6221(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6281_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6281(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6926_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}
		
	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6926(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6923_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);
	
	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6923(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6925ve_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}
		
	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6925ve(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tbs6285_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_0) {

			fgpiStatus = SAA716x_EPRD(FGPI0, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI0_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[0].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[3].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI0, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {

			fgpiStatus = SAA716x_EPRD(FGPI1, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI1_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[1].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[2].demux, data, 348);
			}
                        if (fgpiStatus) {
				SAA716x_EPWR(FGPI1, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_2) {

			fgpiStatus = SAA716x_EPRD(FGPI2, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI2_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[2].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[1].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI2, INT_CLR_STATUS, fgpiStatus);
			}
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {
			
			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
				if (activeBuffer > 0)
					activeBuffer -= 1;
					else
						activeBuffer = 7;
				if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
						u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
						dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
							data[0], data[1], data[2], data[3]);
					dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
				}
				if (fgpiStatus) {
					SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
					}
				}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tbs6285(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

static irqreturn_t saa716x_tt_s2_4100_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l;
	u32 fgpiStatus;
	u32 activeBuffer;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	if (enable_ir) {
		if (stat_h & MSI_INT_EXTINT_4)
			saa716x_input_irq_handler(saa716x);
	}

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {

			fgpiStatus = SAA716x_EPRD(FGPI3, INT_STATUS);
			activeBuffer = (SAA716x_EPRD(BAM, BAM_FGPI3_DMA_BUF_MODE) >> 3) & 0x7;
			dprintk(SAA716x_DEBUG, 1, "fgpiStatus = %04X, buffer = %d",
				fgpiStatus, activeBuffer);
			if (activeBuffer > 0)
				activeBuffer -= 1;
			else
				activeBuffer = 7;
			if (saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt) {
				u8 * data = (u8 *)saa716x->fgpi[3].dma_buf[activeBuffer].mem_virt;
				dprintk(SAA716x_DEBUG, 1, "%02X%02X%02X%02X",
					data[0], data[1], data[2], data[3]);
				dvb_dmx_swfilter_packets(&saa716x->saa716x_adap[0].demux, data, 348);
			}
			if (fgpiStatus) {
				SAA716x_EPWR(FGPI3, INT_CLR_STATUS, fgpiStatus);
			}
		}
	}

	saa716x_msi_event(saa716x, stat_l, stat_h);

	return IRQ_HANDLED;
}

static int load_config_tt_s2_4100(struct saa716x_dev *saa716x)
{
	int ret = 0;

	return ret;
}

#define SAA716x_MODEL_TURBOSIGHT_TBS6220 "TurboSight TBS 6220"
#define SAA716x_DEV_TURBOSIGHT_TBS6220   "DVB-T/T2/C"

static struct tbs62x0fe_config tbs6220fe_config = {
	.tbs62x0fe_address = 0x6c,

	.tbs62x0_ctrl1 = tbsctrl1,
	.tbs62x0_ctrl2 = tbsctrl2,
};

static struct cxd2820r_config cxd2820r_config0 = {
	.i2c_address = 0x6c, /* (0xd8 >> 1) */
	.ts_mode = 0x38,
	.if_dvbt_6 = 3550,
	.if_dvbt_7 = 3700,
	.if_dvbt_8 = 4150,
	.if_dvbt2_6 = 3250,
	.if_dvbt2_7 = 4000,
	.if_dvbt2_8 = 4000,
	.if_dvbc = 5000,
};

static struct cxd2820r_config cxd2820r_config1 = {
	.i2c_address = 0x6d, /* (0xda >> 1) */
	.ts_mode = 0x38,
	.if_dvbt_6 = 3550,
	.if_dvbt_7 = 3700,
	.if_dvbt_8 = 4150,
	.if_dvbt2_6 = 3250,
	.if_dvbt2_7 = 4000,
	.if_dvbt2_8 = 4000,
	.if_dvbc = 5000,
};

static struct tda18212_config tda18212_config = {
	.i2c_address = 0x60 /* (0xc0 >> 1) */,
	.if_dvbt_6 = 3550,
	.if_dvbt_7 = 3700,
	.if_dvbt_8 = 4150,
	.if_dvbt2_6 = 3250,
	.if_dvbt2_7 = 4000,
	.if_dvbt2_8 = 4000,
	.if_dvbc = 5000,
	.loop_through = 0,
	.xtout = 0
};

static struct tda18212_config tda18212_config0 = {
	.i2c_address = 0x60 /* (0xc0 >> 1) */,
	.if_dvbt_6 = 3550,
	.if_dvbt_7 = 3700,
	.if_dvbt_8 = 4150,
	.if_dvbt2_6 = 3250,
	.if_dvbt2_7 = 4000,
	.if_dvbt2_8 = 4000,
	.if_dvbc = 5000,
	.loop_through = 1,
	.xtout = 1
};

static struct tda18212_config tda18212_config1 = {
	.i2c_address = 0x63 /* (0xc6 >> 1) */,
	.if_dvbt_6 = 3550,
	.if_dvbt_7 = 3700,
	.if_dvbt_8 = 4150,
	.if_dvbt2_6 = 3250,
	.if_dvbt2_7 = 4000,
	.if_dvbt2_8 = 4000,
	.if_dvbc = 5000,
	.loop_through = 0,
	.xtout = 0
};

static int saa716x_tbs6220_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[0];

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6220FE %d", count);
		if (cxd2820r)
		{
			adapter->fe = cxd2820r_attach(&cxd2820r_config0, &i2c->i2c_adapter,NULL);
			if (!adapter->fe)
				goto exit;

			if (!dvb_attach(tda18212_attach, adapter->fe,
				&i2c->i2c_adapter, &tda18212_config)) {
				dvb_frontend_detach(adapter->fe);
				goto exit;
			}
		}
		else
		{
			adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6220fe_config, &i2c->i2c_adapter);
			if (!adapter->fe)
				goto exit;
		}

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6220_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6220,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6220,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6220,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6220_frontend_attach,
	.irq_handler		= saa716x_tbs6220_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6280 "TurboSight TBS 6280"
#define SAA716x_DEV_TURBOSIGHT_TBS6280   "DVB-T/T2/C"

static struct tbs62x0fe_config tbs6280fe_config0 = {
	.tbs62x0fe_address = 0x6c,

	.tbs62x0_ctrl1 = tbsctrl1,
	.tbs62x0_ctrl2 = tbsctrl2,
};

static struct tbs62x0fe_config tbs6280fe_config1 = {
	.tbs62x0fe_address = 0x6d,

	.tbs62x0_ctrl1 = tbsctrl1,
	.tbs62x0_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6280_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	if (count == 0) {
		saa716x_gpio_set_output(saa716x, 2);
		msleep(1);
		saa716x_gpio_write(saa716x, 2, 0);
		msleep(200);
		saa716x_gpio_write(saa716x, 2, 1);
		msleep(400);
	}

	if (count == 0) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6280FE %d", count);
		if (cxd2820r)
		{
			adapter->fe = cxd2820r_attach(&cxd2820r_config0, &i2c0->i2c_adapter, NULL);
			if (!adapter->fe)
				goto exit;

			if (!dvb_attach(tda18212_attach, adapter->fe,
				&i2c0->i2c_adapter, &tda18212_config0)) {
				dvb_frontend_detach(adapter->fe);
				goto exit;
			}
		}
		else
		{
			adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config0, &i2c0->i2c_adapter);
			if (!adapter->fe)
				goto exit;
		}

		tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6280 DVB-T2 card MAC=%pM\n",
			adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 1) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		if (cxd2820r)
		{
			adapter->fe = cxd2820r_attach(&cxd2820r_config1, &i2c0->i2c_adapter, NULL);
			if (!adapter->fe)
				goto exit;

			if (!dvb_attach(tda18212_attach, adapter->fe,
				&i2c0->i2c_adapter, &tda18212_config1)) {
				dvb_frontend_detach(adapter->fe);
				goto exit;
			}
		}
		else
		{
			adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config1, &i2c0->i2c_adapter);
			if (!adapter->fe)
				goto exit;
		}

		tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6280 DVB-T2 card MAC=%pM\n",
			adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6280_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6280,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6280,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6280,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6280_frontend_attach,
	.irq_handler		= saa716x_tbs6280_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 1
		},
		{
			/* adapter 1 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_SKYSTAR2_EXPRESS_HD	"SkyStar 2 eXpress HD"
#define SAA716x_DEV_SKYSTAR2_EXPRESS_HD		"DVB-S/S2"

static struct stv090x_config skystar2_stv090x_config = {
	.device			= STV0903,
	.demod_mode		= STV090x_SINGLE,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= 8000000,
	.address		= 0x68,

	.ts1_mode		= STV090x_TSMODE_DVBCI,
	.ts2_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,

	.repeater_level		= STV090x_RPTLEVEL_16,

	.adc1_range	= STV090x_ADC_1Vpp,
	.tuner_bbgain   = 6,

	.tuner_init		= NULL,
	.tuner_sleep		= NULL,
	.tuner_set_mode		= NULL,
	.tuner_set_frequency	= NULL,
	.tuner_get_frequency	= NULL,
	.tuner_set_bandwidth	= NULL,
	.tuner_get_bandwidth	= NULL,
	.tuner_set_bbgain	= NULL,
	.tuner_get_bbgain	= NULL,
	.tuner_set_refclk	= NULL,
	.tuner_get_status	= NULL,
};

static int skystar2_set_voltage(struct dvb_frontend *fe,
				enum fe_sec_voltage voltage)
{
	int err;
	u8 en = 0;
	u8 sel = 0;

	switch (voltage) {
	case SEC_VOLTAGE_OFF:
		en = 0;
		break;

	case SEC_VOLTAGE_13:
		en = 1;
		sel = 0;
		break;

	case SEC_VOLTAGE_18:
		en = 1;
		sel = 1;
		break;

	default:
		break;
	}

	err = stv090x_set_gpio(fe, 2, 0, en, 0);
	if (err < 0)
		goto exit;
	err = stv090x_set_gpio(fe, 3, 0, sel, 0);
	if (err < 0)
		goto exit;

	return 0;
exit:
	return err;
}

static int skystar2_voltage_boost(struct dvb_frontend *fe, long arg)
{
	int err;
	u8 value;

	if (arg)
		value = 1;
	else
		value = 0;

	err = stv090x_set_gpio(fe, 4, 0, value, 0);
	if (err < 0)
		goto exit;

	return 0;
exit:
	return err;
}

static struct stv6110x_config skystar2_stv6110x_config = {
	.addr			= 0x60,
	.refclk			= 16000000,
	.clk_div		= 2,
};

static int skystar2_express_hd_frontend_attach(struct saa716x_adapter *adapter,
					       int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[SAA716x_I2C_BUS_A];
	struct stv6110x_devctl *ctl;

	if (count < saa716x->config->adapters) {
		dprintk(SAA716x_DEBUG, 1, "Adapter (%d) SAA716x frontend Init",
			count);
		dprintk(SAA716x_DEBUG, 1, "Adapter (%d) Device ID=%02x", count,
			saa716x->pdev->subsystem_device);

		saa716x_gpio_set_output(saa716x, 26);
		msleep(1);

		/* Reset the demodulator */
		saa716x_gpio_write(saa716x, 26, 1);
		msleep(10);
		saa716x_gpio_write(saa716x, 26, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, 26, 1);
		msleep(100);

		adapter->fe = dvb_attach(stv090x_attach,
					 &skystar2_stv090x_config,
					 &i2c->i2c_adapter,
					 STV090x_DEMODULATOR_0);

		if (adapter->fe) {
			dprintk(SAA716x_NOTICE, 1, "found STV0903 @0x%02x",
				skystar2_stv090x_config.address);
		} else {
			goto exit;
		}

		adapter->fe->ops.set_voltage = skystar2_set_voltage;
		adapter->fe->ops.enable_high_lnb_voltage = skystar2_voltage_boost;

		ctl = dvb_attach(stv6110x_attach,
				 adapter->fe,
				 &skystar2_stv6110x_config,
				 &i2c->i2c_adapter);

		if (ctl) {
			dprintk(SAA716x_NOTICE, 1, "found STV6110(A) @0x%02x",
				skystar2_stv6110x_config.addr);

			skystar2_stv090x_config.tuner_init	    = ctl->tuner_init;
			skystar2_stv090x_config.tuner_sleep	    = ctl->tuner_sleep;
			skystar2_stv090x_config.tuner_set_mode	    = ctl->tuner_set_mode;
			skystar2_stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
			skystar2_stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
			skystar2_stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
			skystar2_stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
			skystar2_stv090x_config.tuner_set_bbgain    = ctl->tuner_set_bbgain;
			skystar2_stv090x_config.tuner_get_bbgain    = ctl->tuner_get_bbgain;
			skystar2_stv090x_config.tuner_set_refclk    = ctl->tuner_set_refclk;
			skystar2_stv090x_config.tuner_get_status    = ctl->tuner_get_status;

			/* call the init function once to initialize
			   tuner's clock output divider and demod's
			   master clock */
			if (adapter->fe->ops.init)
				adapter->fe->ops.init(adapter->fe);
		} else {
			goto exit;
		}

		dprintk(SAA716x_ERROR, 1, "Done!");
		return 0;
	}
exit:
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config skystar2_express_hd_config = {
	.model_name		= SAA716x_MODEL_SKYSTAR2_EXPRESS_HD,
	.dev_type		= SAA716x_DEV_SKYSTAR2_EXPRESS_HD,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_skystar2,
	.adapters		= 1,
	.frontend_attach	= skystar2_express_hd_frontend_attach,
	.irq_handler		= saa716x_skystar2_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* Adapter 0 */
			.ts_port = 1, /* using FGPI 1 */
		}
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TECHNISAT_USB2
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6925 "TurboSight TBS 6925"
#define SAA716x_DEV_TURBOSIGHT_TBS6925   "DVB-S/S2"

static struct stv090x_config stv0900_config = {
	.device		= STV0900,
	.demod_mode	= STV090x_SINGLE,
	.clk_mode	= STV090x_CLK_EXT,

	.xtal		= 27000000,
	.address	= 0x68,

	.ts1_mode	= STV090x_TSMODE_PARALLEL_PUNCTURED,
	.ts2_mode	= STV090x_TSMODE_PARALLEL_PUNCTURED,

	.repeater_level		= STV090x_RPTLEVEL_16,

	.adc1_range	= STV090x_ADC_2Vpp,
	.tuner_bbgain   = 8,

	.tuner_get_frequency	= stb6100_get_frequency,
	.tuner_set_frequency	= stb6100_set_frequency,
	.tuner_set_bandwidth	= stb6100_set_bandwidth,
	.tuner_get_bandwidth	= stb6100_get_bandwidth,
};

static struct stb6100_config stb6100_config = {
	.tuner_address	= 0x60,
	.refclock	= 27000000
};

struct saa716x_dev *saa716x_GPIO;

static int tbs6925_set_voltage(struct dvb_frontend *fe, enum fe_sec_voltage voltage)
{
	struct saa716x_dev *saa716x = fe->dvb->priv;

	switch (voltage) {
	case SEC_VOLTAGE_13:
			dprintk(SAA716x_ERROR, 1, "Polarization=[13V]");
			saa716x_gpio_set_output(saa716x_GPIO, 16);
			msleep(1);
			saa716x_gpio_write(saa716x_GPIO, 16, 0);
			break;
	case SEC_VOLTAGE_18:
			dprintk(SAA716x_ERROR, 1, "Polarization=[18V]");
			saa716x_gpio_set_output(saa716x_GPIO, 16);
			msleep(1);
			saa716x_gpio_write(saa716x_GPIO, 16, 1);
			break;
	case SEC_VOLTAGE_OFF:
			dprintk(SAA716x_ERROR, 1, "Frontend (dummy) POWERDOWN");
			break;
	default:
			dprintk(SAA716x_ERROR, 1, "Invalid = (%d)", (u32 ) voltage);
			return -EINVAL;
	}

	return 0;
}


static int saa716x_tbs6925_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	saa716x_GPIO = saa716x;

	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6925 Frontend %d", count);
		adapter->fe = stv090x_attach (&stv0900_config, &i2c0->i2c_adapter, 
								STV090x_DEMODULATOR_0);
		if (adapter->fe) {
				dprintk(SAA716x_ERROR, 1, "TBS6925 Frontend found @0x%02x",
						stv0900_config.address);
				if (!dvb_attach(stb6100_attach, adapter->fe, &stb6100_config,
							&i2c0->i2c_adapter)) {
					dvb_frontend_detach(adapter->fe);
					goto exit;
				}
				tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
				memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
				printk(KERN_INFO "TurboSight TBS6925 DVB-S2 card MAC=%pM\n",
					adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}

		adapter->fe->ops.set_voltage	= tbs6925_set_voltage;

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6925_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6925,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6925,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6925,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6925_frontend_attach,
	.irq_handler		= saa716x_tbs6925_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6984 "TurboSight TBS 6984"
#define SAA716x_DEV_TURBOSIGHT_TBS6984   "DVB-S2"

static struct tbs6984fe_config tbs6984_fe_config0 = {
	.tbs6984fe_address = 0x05,

	.tbs6984_ctrl1 = tbsctrl1,
	.tbs6984_ctrl2 = tbsctrl2,
};

static struct tbs6984fe_config tbs6984_fe_config1 = {
	.tbs6984fe_address = 0x55,

	.tbs6984_ctrl1 = tbsctrl1,
	.tbs6984_ctrl2 = tbsctrl2,
};

static struct isl6423_config tbs_isl6423_config[1] = {
	{
		.current_max		= SEC_CURRENT_515m,
		.curlim			= SEC_CURRENT_LIM_ON,
		.mod_extern		= 1,
		.addr			= 0x08,
	}
};

static int saa716x_tbs6984_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	if (count == 2 || count == 3) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6984FE %d", count);
		adapter->fe = dvb_attach(tbs6984fe_attach, &tbs6984_fe_config0,
 							&i2c0->i2c_adapter, count);

		if (dvb_attach(isl6423_attach,
				adapter->fe,
				&i2c0->i2c_adapter,
				&tbs_isl6423_config[0], (count-2)?1:0) == NULL)
					dvb_attach(tbsfe_attach, adapter->fe);
		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6984 DVB-S2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 0 || count == 1) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6984FE %d", count);
		adapter->fe = dvb_attach(tbs6984fe_attach, &tbs6984_fe_config1,
							&i2c1->i2c_adapter, count);
		if (dvb_attach(isl6423_attach,
				adapter->fe,
				&i2c1->i2c_adapter,
				&tbs_isl6423_config[0], (count)?1:0) == NULL)
					dvb_attach(tbsfe_attach, adapter->fe);
		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6984 DVB-S2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
        }

	if (!adapter->fe) 
		goto exit;

	return 0;
exit:
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6984_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6984,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6984,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6984,
	.adapters		= 4,
	.frontend_attach	= saa716x_tbs6984_frontend_attach,
	.irq_handler		= saa716x_tbs6984_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 2
		},
		{
			/* adapter 1 */
			.ts_port = 3
		},
		{
			/* adapter 2 */
			.ts_port = 0
		},
		{
			/* adapter 3 */
			.ts_port = 1
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6992 "TurboSight TBS 6992"
#define SAA716x_DEV_TURBOSIGHT_TBS6992   "DVB-S/S2 CI"

static struct stv090x_config tbs6992fe_config = {
	.device		= STV0900,
	.demod_mode	= STV090x_DUAL,
	.clk_mode	= STV090x_CLK_EXT,

	.xtal		= 27000000,
	.address	= 0x68,

	.ts1_mode	= STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts2_mode	= STV090x_TSMODE_SERIAL_CONTINUOUS,

	.repeater_level		= STV090x_RPTLEVEL_16,

	.tuner_set_frequency	= tbs6992_set_frequency,
	
	.agc_rf1	= 0x10,
	.agc_rf2	= 0x12,
};

static struct tbs6992_config tbs6992_config = {
	.tbs6992_address = 0x61,

	.tbs6992_ctrl1 = tbsctrl1,
	.tbs6992_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6992_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[0];
	unsigned int data = 0;
	int ret;

	struct tbs6992_state *ctl;

	if (count == 0 || count == 1) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6992 Frontend %d", count);
		adapter->fe = stv090x_attach (&tbs6992fe_config, &i2c->i2c_adapter, 
					count ? STV090x_DEMODULATOR_1 : STV090x_DEMODULATOR_0);
		if (adapter->fe) {
				dprintk(SAA716x_ERROR, 1, "TBS6992 Frontend found @0x%02x",
						tbs6992fe_config.address);
				ctl = dvb_attach(tbs6992_attach, adapter->fe, &tbs6992_config, 
						&i2c->i2c_adapter);
				if (!ctl)
					goto exit;
				dvb_attach(isl6423_attach,
						adapter->fe,
						&i2c->i2c_adapter,
						&tbs_isl6423_config[0], (count)?0:1);
				saa716x_gpio_set_input(saa716x, count ? 6 : 14);
				msleep(1);
				data = saa716x_gpio_read(saa716x, count ? 6 : 14);
				printk("TBS CI Extention for Adapter %d attached and CAM inserted: %s\n", 
					count, data ? "no" : "yes");
				if (data) {
					saa716x_gpio_set_output(saa716x, count ? 16 : 17);
					msleep(1);
					saa716x_gpio_write(saa716x, count ? 16 : 17, 0);
					msleep(5);
				}
				if (!data) {
					ret = tbsci_i2c_probe(adapter, count);
					if (!ret) 
						tbsci_init(adapter, count, 0);
				}
		} else {
			goto exit;
		}
		
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6992_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6992,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6992,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6992,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6992_frontend_attach,
	.irq_handler		= saa716x_tbs6992_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
		{
			/* adapter 1 */
			.ts_port = 1
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6922 "TurboSight TBS 6922"
#define SAA716x_DEV_TURBOSIGHT_TBS6922   "DVB-S/S2"

static struct tbs6922fe_config tbs6922_fe_config = {
	.tbs6922fe_address = 0x68,
	
	.tbs6922_ctrl1 = tbsctrl1,
	.tbs6922_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6922_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	
	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6922 Frontend %d", count);
		adapter->fe = tbs6922fe_attach (&tbs6922_fe_config, &i2c0->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6922 Frontend found @0x%02x",
					tbs6922_fe_config.tbs6922fe_address);
			dvb_attach(tbsfe_attach, adapter->fe);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6922 DVB-S2 card MAC=%pM\n",
				adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}
		
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6922_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6922,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6922,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6922,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6922_frontend_attach,
	.irq_handler		= saa716x_tbs6922_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6928 "TurboSight TBS 6928"
#define SAA716x_DEV_TURBOSIGHT_TBS6928   "DVB-S/S2 CI"

static struct tbs6928fe_config tbs6928_fe_config = {
	.tbs6928fe_address = 0x68,
	
	.tbs6928_ctrl1 = tbsctrl1,
	.tbs6928_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6928_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	/* unsigned int data = 0; */
	int ret;
	
	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6928 Frontend %d", count);
		adapter->fe = tbs6928fe_attach (&tbs6928_fe_config, &i2c0->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6928 Frontend found @0x%02x",
					tbs6928_fe_config.tbs6928fe_address);
			dvb_attach(tbsfe_attach, adapter->fe);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6928 DVB-S2 card MAC=%pM\n",
				adapter->dvb_adapter.proposed_mac);
			saa716x_gpio_set_input(saa716x, 3);
			msleep(1);
			saa716x_gpio_set_input(saa716x, 5);
			msleep(1);
#if 0
			saa716x_gpio_set_input(saa716x, 6);
			msleep(1);
			data = saa716x_gpio_read(saa716x, 6);
			printk("TBS 6928 CI Extention for Adapter 0 attached and CAM inserted: %s\n", 
					data ? "yes" : "no");
			if (!data) {
				saa716x_gpio_set_output(saa716x, 17);
				msleep(1);
				saa716x_gpio_write(saa716x, 17, 0);
				msleep(5);
			}
			if (data) 
#endif
			{
				ret = tbsci_i2c_probe(adapter, ci_mode ? 2 : 3);
				if (!ret) 
					tbsci_init(adapter, 0, ci_mode ? 0 : 1);
			}
		} else {
			goto exit;
		}
		
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6928_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6928,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6928,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6928,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6928_frontend_attach,
	.irq_handler		= saa716x_tbs6928_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6928SE "TurboSight TBS 6928SE"
#define SAA716x_DEV_TURBOSIGHT_TBS6928SE   "DVB-S/S2 CI"

static struct tbs6928se_config tbs6928se_fe_config = {
	.tbs6928se_address = 0x68,
	
	.tbs6928se_ctrl1 = tbsctrl1,
	.tbs6928se_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6928se_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	/* unsigned int data = 0; */
	int ret;
	
	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6928SE Frontend %d", count);
		adapter->fe = tbs6928se_attach (&tbs6928se_fe_config, &i2c0->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6928SE Frontend found @0x%02x",
					tbs6928se_fe_config.tbs6928se_address);
			dvb_attach(tbsfe_attach, adapter->fe);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6928SE DVB-S2 card MAC=%pM\n",
				adapter->dvb_adapter.proposed_mac);
			saa716x_gpio_set_input(saa716x, 3);
			msleep(1);
			saa716x_gpio_set_input(saa716x, 5);
			msleep(1);
			saa716x_gpio_set_output(saa716x, 6);
			msleep(1);
#if 0
			saa716x_gpio_set_input(saa716x, 6);
			msleep(1);
			data = saa716x_gpio_read(saa716x, 6);
			printk("TBS 6928SE CI Extention for Adapter 0 attached and CAM inserted: %s\n", 
					data ? "yes" : "no");
			if (!data) {
				saa716x_gpio_set_output(saa716x, 17);
				msleep(1);
				saa716x_gpio_write(saa716x, 17, 0);
				msleep(5);
			}
			if (data) 
#endif
			{
				ret = tbsci_i2c_probe(adapter, ci_mode ? 2 : 3);
				if (!ret) 
					tbsci_init(adapter, 0, ci_mode ? 0 : 1);
			}
		} else {
			goto exit;
		}
		
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6928se_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6928SE,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6928SE,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6928se,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6928se_frontend_attach,
	.irq_handler		= saa716x_tbs6928se_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6284 "TurboSight TBS 6284"
#define SAA716x_DEV_TURBOSIGHT_TBS6284   "DVB-T/T2/C"

static int saa716x_tbs6284_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	
	if (count == 0) {
		saa716x_gpio_set_output(saa716x, 22);
		msleep(1);
		saa716x_gpio_write(saa716x, 22, 0);
		msleep(200);
		saa716x_gpio_write(saa716x, 22, 1);
		msleep(400);
	}

	if (count == 0) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		if (cxd2820r)
		{
			adapter->fe = cxd2820r_attach(&cxd2820r_config0, &i2c1->i2c_adapter, NULL);
			if (!adapter->fe)
				goto exit;

			if (!dvb_attach(tda18212_attach, adapter->fe,
				&i2c1->i2c_adapter, &tda18212_config0)) {
				dvb_frontend_detach(adapter->fe);
				goto exit;
			}
		}
		else
		{
			adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config0, &i2c1->i2c_adapter);
			if (!adapter->fe)
				goto exit;
		}

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6284 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 1) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		if (cxd2820r)
		{
			adapter->fe = cxd2820r_attach(&cxd2820r_config1, &i2c1->i2c_adapter, NULL);
			if (!adapter->fe)
				goto exit;

			if (!dvb_attach(tda18212_attach, adapter->fe,
				&i2c1->i2c_adapter, &tda18212_config1)) {
				dvb_frontend_detach(adapter->fe);
				goto exit;
			}
		}
		else
		{
			adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config1, &i2c1->i2c_adapter);
			if (!adapter->fe)
				goto exit;
		}

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6284 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 2) {
		saa716x_gpio_set_output(saa716x, 12);
		msleep(1);
		saa716x_gpio_write(saa716x, 12, 0);
		msleep(200);
		saa716x_gpio_write(saa716x, 12, 1);
		msleep(400);
	}

	if (count == 2) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		if (cxd2820r)
		{
			adapter->fe = cxd2820r_attach(&cxd2820r_config0, &i2c0->i2c_adapter, NULL);
			if (!adapter->fe)
				goto exit;

			if (!dvb_attach(tda18212_attach, adapter->fe,
				&i2c0->i2c_adapter, &tda18212_config0)) {
				dvb_frontend_detach(adapter->fe);
				goto exit;
			}
		}
		else
		{
			adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config0, &i2c0->i2c_adapter);
			if (!adapter->fe)
				goto exit;
		}

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6284 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 3) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		if (cxd2820r)
		{
			adapter->fe = cxd2820r_attach(&cxd2820r_config1, &i2c0->i2c_adapter, NULL);
			if (!adapter->fe)
				goto exit;

			if (!dvb_attach(tda18212_attach, adapter->fe,
				&i2c0->i2c_adapter, &tda18212_config1)) {
				dvb_frontend_detach(adapter->fe);
				goto exit;
			}
		}
		else
		{
			adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config1, &i2c0->i2c_adapter);
			if (!adapter->fe)
				goto exit;
		}

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6284 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6284_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6284,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6284,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6284,
	.adapters		= 4,
	.frontend_attach	= saa716x_tbs6284_frontend_attach,
	.irq_handler		= saa716x_tbs6284_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3,
		},
		{
			/* adapter 1 */
			.ts_port = 2
		},
		{
			/* adapter 2 */
			.ts_port = 1
		},
		{
			/* adapter 3 */
			.ts_port = 0
		}
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6982 "TurboSight TBS 6982"
#define SAA716x_DEV_TURBOSIGHT_TBS6982   "DVB-S2"

static struct tbs6982fe_config tbs6982_fe_config = {
	.tbs6982fe_address = 0x68,
	
	.tbs6982_ctrl1 = tbsctrl1,
	.tbs6982_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6982_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	saa716x_gpio_set_output(saa716x, 16);
	msleep(1);
	saa716x_gpio_write(saa716x, 16, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 16, 1);
	msleep(100);

	if (count == 0 || count == 1) {
		saa716x_gpio_set_output(saa716x, count ? 17 : 2);
		msleep(1);
		saa716x_gpio_write(saa716x, count ? 17 : 2, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, count ? 17 : 2, 1);
		msleep(100);

		dprintk(SAA716x_ERROR, 1, "Probing for TBS6982 Frontend %d", count);
		adapter->fe = tbs6982fe_attach (&tbs6982_fe_config, 
				count ? &i2c0->i2c_adapter : &i2c1->i2c_adapter, count);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6982 Frontend found @0x%02x",
					tbs6982_fe_config.tbs6982fe_address);
			dvb_attach(tbsfe_attach, adapter->fe);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6982 DVB-S2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6982_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6982,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6982,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6982,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6982_frontend_attach,
	.irq_handler		= saa716x_tbs6982_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
		{
			/* adapter 1 */
			.ts_port = 1
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

static struct tbs6982se_config tbs6982se_fe_config0 = {
	.tbs6982se_address = 0x60,
	
	.tbs6982se_ctrl1 = tbsctrl1,
	.tbs6982se_ctrl2 = tbsctrl2,
};

static struct tbs6982se_config tbs6982se_fe_config1 = {
	.tbs6982se_address = 0x68,

	.tbs6982se_ctrl1 = tbsctrl1,
	.tbs6982se_ctrl2 = tbsctrl2,
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6982SE "TurboSight TBS 6982SE"
#define SAA716x_DEV_TURBOSIGHT_TBS6982SE   "DVB-S2"

static int saa716x_tbs6982se_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	if (count == 0 || count == 1) {
		saa716x_gpio_set_output(saa716x, count ? 17 : 2);
		msleep(1);
		saa716x_gpio_write(saa716x, count ? 17 : 2, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, count ? 17 : 2, 1);
		msleep(100);

		dprintk(SAA716x_ERROR, 1, "Probing for TBS6982SE Frontend %d", count);
		adapter->fe = tbs6982se_attach (count ? &tbs6982se_fe_config1: &tbs6982se_fe_config0, 
				count ? &i2c0->i2c_adapter : &i2c1->i2c_adapter, count);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6982SE Frontend found @0x%02x",
				count ? tbs6982se_fe_config1.tbs6982se_address : tbs6982se_fe_config0.tbs6982se_address);
			dvb_attach(tbsfe_attach, adapter->fe);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6982SE DVB-S2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6982se_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6982SE,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6982SE,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6982se,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6982se_frontend_attach,
	.irq_handler		= saa716x_tbs6982se_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
		{
			/* adapter 1 */
			.ts_port = 1
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6991 "TurboSight TBS 6991"
#define SAA716x_DEV_TURBOSIGHT_TBS6991   "DVB-S2"

static struct tbs6991fe_config tbs6991_fe_config = {
	.tbs6991fe_address = 0x68,
	
	.tbs6991_ctrl1 = tbsctrl1,
	.tbs6991_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6991_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	/* unsigned int data = 0; */
	int ret;

	if (count == 0 || count == 1) {
		saa716x_gpio_set_output(saa716x, count ? 17 : 20);
		msleep(1);
		saa716x_gpio_write(saa716x, count ? 17 : 20, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, count ? 17 : 20, 1);
		msleep(100);
		
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6991 Frontend %d", count);
		adapter->fe = tbs6991fe_attach (&tbs6991_fe_config, 
				count ? &i2c1->i2c_adapter : &i2c0->i2c_adapter, count);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6991 Frontend found @0x%02x",
					tbs6991_fe_config.tbs6991fe_address);
			dvb_attach(tbsfe_attach, adapter->fe);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6991 DVB-S2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);
			saa716x_gpio_set_input(saa716x, count ? 3 : 14);
			msleep(1);
			saa716x_gpio_set_input(saa716x, count ? 6 : 2);
			msleep(1);
#if 0
			saa716x_gpio_set_input(saa716x, count ? 3 : 14);
			msleep(1);
			data = saa716x_gpio_read(saa716x, count ? 3 : 14);
			printk("TBS CI Extention for Adapter %d attached and CAM inserted: %s\n", 
				count, data ? "yes" : "no");
			if (!data) {
				saa716x_gpio_set_output(saa716x, count ? 6 : 16);
				msleep(1);
				saa716x_gpio_write(saa716x, count ? 6 : 16, 0);
				msleep(5);
			}
			if (data)
#endif
			{
				ret = tbsci_i2c_probe(adapter, count ? 3 : 4);
				if (!ret) 
					tbsci_init(adapter, count, 2);
			}
		} else {
			goto exit;
		}

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6991_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6991,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6991,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6991,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6991_frontend_attach,
	.irq_handler		= saa716x_tbs6991_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 1
		},
		{
			/* adapter 1 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6618 "TurboSight TBS 6618"
#define SAA716x_DEV_TURBOSIGHT_TBS6618   "DVB-C CI"

static struct tbs6618fe_config tbs6618_fe_config = {
	.tbs6618fe_address = 0x0c,
	
	.tbs6618_ctrl1 = tbsctrl1,
	.tbs6618_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6618_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	/* unsigned int data = 0; */
	int ret;
	
	saa716x_gpio_set_output(saa716x, 16);
	msleep(1);
	saa716x_gpio_write(saa716x, 16, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 16, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6618 Frontend %d", count);
		adapter->fe = tbs6618fe_attach (&tbs6618_fe_config, &i2c0->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6618 Frontend found @0x%02x",
					tbs6618_fe_config.tbs6618fe_address);
			/* dvb_attach(tbsfe_attach, adapter->fe); */
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6618 DVB-C card MAC=%pM\n",
				adapter->dvb_adapter.proposed_mac);
			saa716x_gpio_set_input(saa716x, 14);
			msleep(1);
			saa716x_gpio_set_input(saa716x, 6);
			msleep(1);

			ret = tbsci_i2c_probe(adapter, 3);
			if (!ret) 
				tbsci_init(adapter, 0, 5);

		} else {
			goto exit;
		}
		
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6618_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6618,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6618,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6618,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6618_frontend_attach,
	.irq_handler		= saa716x_tbs6618_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6680 "TurboSight TBS 6680"
#define SAA716x_DEV_TURBOSIGHT_TBS6680   "DVB-C CI"

static struct tbs6680fe_config tbs6680_fe_config = {
	.tbs6680fe_address = 0x0c,
	
	.tbs6680_ctrl1 = tbsctrl1,
	.tbs6680_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6680_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	/* unsigned int data = 0; */
	int ret;

	if (count == 0 || count == 1) {
		saa716x_gpio_set_output(saa716x, count ? 20 : 14);
		msleep(1);
		saa716x_gpio_write(saa716x, count ? 20 : 14, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, count ? 20 : 14, 1);
		msleep(100);
		
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6680 Frontend %d", count);
		adapter->fe = tbs6680fe_attach (&tbs6680_fe_config, 
				count ? &i2c1->i2c_adapter : &i2c0->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6680 Frontend found @0x%02x",
					tbs6680_fe_config.tbs6680fe_address);
			/* dvb_attach(tbsfe_attach, adapter->fe); */
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6680 DVB-C card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);
			saa716x_gpio_set_input(saa716x, count ? 16 : 6);
			msleep(1);
			saa716x_gpio_set_input(saa716x, count ? 17 : 5);
			msleep(1);

			ret = tbsci_i2c_probe(adapter, count ? 3 : 4);
			if (!ret) 
				tbsci_init(adapter, count, 6);
		} else {
			goto exit;
		}

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6680_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6680,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6680,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6680,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6680_frontend_attach,
	.irq_handler		= saa716x_tbs6680_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 1
		},
		{
			/* adapter 1 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6985 "TurboSight TBS 6985"
#define SAA716x_DEV_TURBOSIGHT_TBS6985   "DVB-S2"

static struct tbs6985fe_config tbs6985_fe_config0 = {
	.tbs6985fe_address = 0x60,

	.tbs6985_ctrl1 = tbsctrl1,
	.tbs6985_ctrl2 = tbsctrl2,
};

static struct tbs6985fe_config tbs6985_fe_config1 = {
	.tbs6985fe_address = 0x68,

	.tbs6985_ctrl1 = tbsctrl1,
	.tbs6985_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6985_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	if (count == 2 || count == 3) {
		saa716x_gpio_set_output(saa716x, (count-2) ? 3 : 13);
		msleep(1);
		saa716x_gpio_write(saa716x, (count-2) ? 3 : 13, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, (count-2) ? 3 : 13, 1);
		msleep(100);

		dprintk(SAA716x_ERROR, 1, "Probing for TBS6985FE %d", count);
		adapter->fe = dvb_attach(tbs6985fe_attach,
			(count-2) ? &tbs6985_fe_config1 : &tbs6985_fe_config0, 
 							&i2c0->i2c_adapter, count);
		dvb_attach(tbsfe_attach, adapter->fe);
		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6985 DVB-S2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 0 || count == 1) {
		saa716x_gpio_set_output(saa716x, count ? 2 : 5);
		msleep(1);
		saa716x_gpio_write(saa716x, count ? 2 : 5, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, count ? 2 : 5, 1);
		msleep(100);

		dprintk(SAA716x_ERROR, 1, "Probing for TBS6985FE %d", count);
		adapter->fe = dvb_attach(tbs6985fe_attach,
			count ? &tbs6985_fe_config1 : &tbs6985_fe_config0,
							&i2c1->i2c_adapter, count);
		dvb_attach(tbsfe_attach, adapter->fe);
		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6985 DVB-S2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
        }

	if (!adapter->fe) 
		goto exit;

	return 0;
exit:
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6985_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6985,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6985,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6985,
	.adapters		= 4,
	.frontend_attach	= saa716x_tbs6985_frontend_attach,
	.irq_handler		= saa716x_tbs6985_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 2
		},
		{
			/* adapter 1 */
			.ts_port = 3
		},
		{
			/* adapter 2 */
			.ts_port = 0
		},
		{
			/* adapter 3 */
			.ts_port = 1
		}
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6926 "TurboSight TBS 6926"
#define SAA716x_DEV_TURBOSIGHT_TBS6926   "DVB-S/S2"

static struct stv090x_config tbs6926fe_config = {
	.device	= STV0900,
	.demod_mode	= STV090x_SINGLE,
	.clk_mode	= STV090x_CLK_EXT,

	.xtal		= 27000000,
	.address	= 0x68,

	.ts1_mode	= STV090x_TSMODE_PARALLEL_PUNCTURED,
	.ts2_mode	= STV090x_TSMODE_PARALLEL_PUNCTURED,

	.repeater_level = STV090x_RPTLEVEL_16,

	.tuner_set_frequency  = tbs6926_set_frequency,

	.agc_rf1	= 0x10,
	.agc_rf2	= 0x12,
};

static struct tbs6926_config tbs6926_config = {
	.tbs6926_address = 0x0c,

	.tbs6926_ctrl1 = tbsctrl1,
	.tbs6926_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6926_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	saa716x_GPIO = saa716x;

	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6926 Frontend %d", count);
		adapter->fe = stv090x_attach (&tbs6926fe_config, &i2c0->i2c_adapter, 
								STV090x_DEMODULATOR_0);
		if (adapter->fe) {
				dprintk(SAA716x_ERROR, 1, "TBS6926 Frontend found @0x%02x",
						stv0900_config.address);
				if (!dvb_attach(tbs6926_attach, adapter->fe, &tbs6926_config, 
						&i2c0->i2c_adapter)) {
					dvb_frontend_detach(adapter->fe);
					goto exit;
				}
				tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
				memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
				printk(KERN_INFO "TurboSight TBS6926 DVB-S2 card MAC=%pM\n",
					adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}

		adapter->fe->ops.set_voltage	= tbs6925_set_voltage;

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6926_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6926,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6926,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6926,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6926_frontend_attach,
	.irq_handler		= saa716x_tbs6926_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6923 "TurboSight TBS 6923"
#define SAA716x_DEV_TURBOSIGHT_TBS6923   "mini-PCIe DVB-S/S2"

static struct tbs6923fe_config tbs6923_fe_config = {
	.tbs6923fe_address = 0x68,
	
	.tbs6923_ctrl1 = tbsctrl1,
	.tbs6923_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6923_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	
	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6923 Frontend %d", count);
		adapter->fe = tbs6923fe_attach (&tbs6923_fe_config, &i2c0->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TBS6923 Frontend found @0x%02x",
					tbs6923_fe_config.tbs6923fe_address);
			dvb_attach(tbsfe_attach, adapter->fe);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TurboSight TBS6923 DVB-S2 card MAC=%pM\n",
				adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}
		
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6923_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6923,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6923,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6923,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6923_frontend_attach,
	.irq_handler		= saa716x_tbs6923_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6925VE "TurboSight TBS 6925VE"
#define SAA716x_DEV_TURBOSIGHT_TBS6925VE   "DVB-S/S2"

static struct stv090x_config stv0903_config = {
	.device		= STV0903,
	.demod_mode	= STV090x_SINGLE,
	.clk_mode	= STV090x_CLK_EXT,

	.xtal		= 27000000,
	.address	= 0x6a,

	.ts1_mode	= STV090x_TSMODE_PARALLEL_PUNCTURED,
	.ts2_mode	= STV090x_TSMODE_PARALLEL_PUNCTURED,

	.repeater_level		= STV090x_RPTLEVEL_16,

	.adc1_range	= STV090x_ADC_2Vpp,
	.tuner_bbgain   = 8,

	.tuner_get_frequency	= stb6100_get_frequency,
	.tuner_set_frequency	= stb6100_set_frequency,
	.tuner_set_bandwidth	= stb6100_set_bandwidth,
	.tuner_get_bandwidth	= stb6100_get_bandwidth,
};

static struct stb6100_config stb6100ve_config = {
	.tuner_address	= 0x60,
	.refclock	= 27000000
};

static int saa716x_tbs6925ve_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	saa716x_GPIO = saa716x;

	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6925VE Frontend %d", count);
		adapter->fe = stv090x_attach (&stv0903_config, &i2c0->i2c_adapter, 
								STV090x_DEMODULATOR_0);
		if (adapter->fe) {
				dprintk(SAA716x_ERROR, 1, "TBS6925VE Frontend found @0x%02x",
						stv0900_config.address);
				dvb_attach(stb6100_attach, adapter->fe, &stb6100ve_config, 
						&i2c0->i2c_adapter);
				tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
				memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
				printk(KERN_INFO "TurboSight TBS6925VE DVB-S2 card MAC=%pM\n",
					adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}

		adapter->fe->ops.set_voltage	= tbs6925_set_voltage;

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6925ve_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6925VE,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6925VE,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6925ve,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6925ve_frontend_attach,
	.irq_handler		= saa716x_tbs6925ve_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TT_BUDGET_S2_4100	"Technotrend TT-budget S2-4100"
#define SAA716x_DEV_TT_BUDGET_S2_4100 	"DVB-S/S2"

static struct tt_s2_4100_config tt_s2_4100_drv_config = {
	.tt_s2_4100_addr = 0x68,
	
	.tt_s2_4100_drv1 = tt_drv1,
	.tt_s2_4100_drv2 = tt_drv2,
};

static int saa716x_tt_s2_4100_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];
	
	saa716x_gpio_set_output(saa716x, 2);
	msleep(1);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TT-budget S2-4100");
		adapter->fe = tt_s2_4100_attach (&tt_s2_4100_drv_config, &i2c0->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "TT-budget S2-4100 found @0x%02x",
					tt_s2_4100_drv_config.tt_s2_4100_addr);
			tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
			memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
			printk(KERN_INFO "TT-budget S2-4100 MAC address = %pM\n",
				adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}
		
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "TT-budget S2-4100 attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tt_s2_4100_config = {
	.model_name		= SAA716x_MODEL_TT_BUDGET_S2_4100,
	.dev_type		= SAA716x_DEV_TT_BUDGET_S2_4100,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tt_s2_4100,
	.adapters		= 1,
	.frontend_attach	= saa716x_tt_s2_4100_frontend_attach,
	.irq_handler		= saa716x_tt_s2_4100_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_100,
	.i2c_rate[1]            = SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TT_1500
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6285 "TurboSight TBS 6285"
#define SAA716x_DEV_TURBOSIGHT_TBS6285   "DVB-T/T2/C"

static struct tbs62x1fe_config tbs6285fe_config0 = {
	.tbs62x1fe_address = 0x64,

	.tbs62x1_ctrl1 = tbsctrl1,
	.tbs62x1_ctrl2 = tbsctrl2,
};

static struct tbs62x1fe_config tbs6285fe_config1 = {
	.tbs62x1fe_address = 0x66,

	.tbs62x1_ctrl1 = tbsctrl1,
	.tbs62x1_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6285_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	if (count == 2) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x1FE %d", count);
		adapter->fe = dvb_attach(tbs62x1fe_attach, &tbs6285fe_config0,
                                			&i2c0->i2c_adapter);

		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6285 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 3) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x1FE %d", count);
		adapter->fe = dvb_attach(tbs62x1fe_attach, &tbs6285fe_config1,
                                			&i2c0->i2c_adapter);
		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6285 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 0) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x1FE %d", count);
		adapter->fe = dvb_attach(tbs62x1fe_attach, &tbs6285fe_config0,
                                			&i2c1->i2c_adapter);

		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6285 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 1) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x1FE %d", count);
		adapter->fe = dvb_attach(tbs62x1fe_attach, &tbs6285fe_config1,
                                			&i2c1->i2c_adapter);
		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6285 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6285_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6285,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6285,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6285,
	.adapters		= 4,
	.frontend_attach	= saa716x_tbs6285_frontend_attach,
	.irq_handler		= saa716x_tbs6285_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
		{
			/* adapter 1 */
			.ts_port = 2
		},
		{
			/* adapter 2 */
			.ts_port = 1
		},
		{
			/* adapter 3 */
			.ts_port = 0
		}
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6221 "TurboSight TBS 6221"
#define SAA716x_DEV_TURBOSIGHT_TBS6221   "DVB-T/T2/C"

static struct tbs62x1fe_config tbs6221fe_config = {
	.tbs62x1fe_address = 0x64,

	.tbs62x1_ctrl1 = tbsctrl1,
	.tbs62x1_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6221_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[0];

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6221FE %d", count);
		adapter->fe = dvb_attach(tbs62x1fe_attach, &tbs6221fe_config,
                                			&i2c->i2c_adapter);
		if (!adapter->fe)
			goto exit;

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n", 
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6221_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6221,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6221,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6221,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6221_frontend_attach,
	.irq_handler		= saa716x_tbs6221_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

#define SAA716x_MODEL_TURBOSIGHT_TBS6281 "TurboSight TBS 6281"
#define SAA716x_DEV_TURBOSIGHT_TBS6281   "DVB-T/T2/C"

static struct tbs62x1fe_config tbs6281fe_config = {
	.tbs62x1fe_address = 0x64,

	.tbs62x1_ctrl1 = tbsctrl1,
	.tbs62x1_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6281_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	if (count == 0 || count == 1) {
		saa716x_gpio_set_output(saa716x, count ? 2 : 16);
		msleep(1);
		saa716x_gpio_write(saa716x, count ? 2 : 16, 0);
		msleep(50);
		saa716x_gpio_write(saa716x, count ? 2 : 16, 1);
		msleep(100);

		dprintk(SAA716x_ERROR, 1, "Probing for TBS6281FE %d", count);
		adapter->fe = dvb_attach(tbs62x1fe_attach, &tbs6281fe_config,
                                	count ? &i2c1->i2c_adapter : &i2c0->i2c_adapter);

		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6281 DVB-T2 card MAC=%pM\n",
			adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6281_config = {
	.model_name		= SAA716x_MODEL_TURBOSIGHT_TBS6281,
	.dev_type		= SAA716x_DEV_TURBOSIGHT_TBS6281,
	.boot_mode		= SAA716x_EXT_BOOT,
	.load_config		= &load_config_tbs6281,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6281_frontend_attach,
	.irq_handler		= saa716x_tbs6281_pci_irq,
	.i2c_rate[0]		= SAA716x_I2C_RATE_400,
	.i2c_rate[1]            = SAA716x_I2C_RATE_400,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 1
		},
		{
			/* adapter 1 */
			.ts_port = 3
		},
	},
	.rc_gpio_in = 4,
	.rc_map_name = RC_MAP_TBS_NEC
};

static struct pci_device_id saa716x_tbs_pci_table[] = {

	MAKE_ENTRY(TURBOSIGHT_TBS6220_SUBVENDOR, TURBOSIGHT_TBS6220_SUBDEVICE, SAA7160, &saa716x_tbs6220_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6280_SUBVENDOR, TURBOSIGHT_TBS6280_SUBDEVICE, SAA7160, &saa716x_tbs6280_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6925_SUBVENDOR, TURBOSIGHT_TBS6925_SUBDEVICE, SAA7160, &saa716x_tbs6925_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6984_SUBVENDOR, TURBOSIGHT_TBS6984_SUBDEVICE, SAA7160, &saa716x_tbs6984_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6992_SUBVENDOR, TURBOSIGHT_TBS6992_SUBDEVICE, SAA7160, &saa716x_tbs6992_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6922_SUBVENDOR, TURBOSIGHT_TBS6922_SUBDEVICE, SAA7160, &saa716x_tbs6922_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6928_SUBVENDOR, TURBOSIGHT_TBS6928_SUBDEVICE, SAA7160, &saa716x_tbs6928_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6284_SUBVENDOR, TURBOSIGHT_TBS6284_SUBDEVICE, SAA7160, &saa716x_tbs6284_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6982_SUBVENDOR, TURBOSIGHT_TBS6982_SUBDEVICE, SAA7160, &saa716x_tbs6982_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6991_SUBVENDOR, TURBOSIGHT_TBS6991_SUBDEVICE, SAA7160, &saa716x_tbs6991_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6618_SUBVENDOR, TURBOSIGHT_TBS6618_SUBDEVICE, SAA7160, &saa716x_tbs6618_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6680_SUBVENDOR, TURBOSIGHT_TBS6680_SUBDEVICE, SAA7160, &saa716x_tbs6680_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6985_SUBVENDOR, TURBOSIGHT_TBS6985_SUBDEVICE, SAA7160, &saa716x_tbs6985_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6926_SUBVENDOR, TURBOSIGHT_TBS6926_SUBDEVICE, SAA7160, &saa716x_tbs6926_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6985_SUBVENDOR, TURBOSIGHT_TBS6985_SUBDEVICE+1, SAA7160, &saa716x_tbs6985_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6923_SUBVENDOR, TURBOSIGHT_TBS6923_SUBDEVICE, SAA7160, &saa716x_tbs6923_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6925_SUBVENDOR, TURBOSIGHT_TBS6925_SUBDEVICE+1, SAA7160, &saa716x_tbs6925ve_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6928_SUBVENDOR, TURBOSIGHT_TBS6928_SUBDEVICE+1, SAA7160, &saa716x_tbs6928se_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6928_SUBVENDOR, TURBOSIGHT_TBS6928_SUBDEVICE+2, SAA7160, &saa716x_tbs6928se_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6982_SUBVENDOR, TURBOSIGHT_TBS6982_SUBDEVICE+1, SAA7160, &saa716x_tbs6982se_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6285_SUBVENDOR, TURBOSIGHT_TBS6285_SUBDEVICE, SAA7160, &saa716x_tbs6285_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6221_SUBVENDOR, TURBOSIGHT_TBS6221_SUBDEVICE, SAA7160, &saa716x_tbs6221_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6281_SUBVENDOR, TURBOSIGHT_TBS6281_SUBDEVICE, SAA7160, &saa716x_tbs6281_config),
	MAKE_ENTRY(TECHNISAT, SKYSTAR2_EXPRESS_HD, SAA7160, &skystar2_express_hd_config),
	MAKE_ENTRY(TECHNOTREND, BUDGET_S2_4100, SAA7160, &saa716x_tt_s2_4100_config),
	{ }
};
MODULE_DEVICE_TABLE(pci, saa716x_tbs_pci_table);

static struct pci_driver saa716x_tbs_pci_driver = {
	.name		= DRIVER_NAME,
	.id_table	= saa716x_tbs_pci_table,
	.probe		= saa716x_tbs_pci_probe,
	.remove		= saa716x_tbs_pci_remove,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devinit saa716x_tbs_init(void)
#else
static int saa716x_tbs_init(void)
#endif
{
	return pci_register_driver(&saa716x_tbs_pci_driver);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static void __devexit saa716x_tbs_exit(void)
#else
static void saa716x_tbs_exit(void)
#endif
{
	return pci_unregister_driver(&saa716x_tbs_pci_driver);
}

module_init(saa716x_tbs_init);
module_exit(saa716x_tbs_exit);

MODULE_DESCRIPTION("SAA716x TBS driver");
MODULE_AUTHOR("Konstantin Dimitrov <kosio.dimitrov@gmail.com>");
MODULE_LICENSE("GPL");
