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

#include "saa716x_tbs.h"
#include "tbs62x0fe.h"
#include "tbsctrl.h"

#include "tbs6925ctrl.h"

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

#include "tbsfe.h"

#include "tbsmac.h"

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

		saa716x_gpio_set_input(saa716x, 4);
		msleep(1);
	
		saa716x_input_init(saa716x);
	}

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

#define SAA716x_MODEL_TURBOSIGHT_TBS6220 "TurboSight TBS 6220"
#define SAA716x_DEV_TURBOSIGHT_TBS6220   "DVB-T/T2/C"

static struct tbs62x0fe_config tbs6220fe_config = {
	.tbs62x0fe_address = 0x6c,

	.tbs62x0_ctrl1 = tbsctrl1,
	.tbs62x0_ctrl2 = tbsctrl2,
};

static int saa716x_tbs6220_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[0];

	if (count == 0 ) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS6220FE %d", count);
		adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6220fe_config,
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
	}
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
		adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config0,
                                			&i2c0->i2c_adapter);

		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6280 DVB-T2 card MAC=%pM\n",
			adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 1) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config1,
                                			&i2c0->i2c_adapter);
		if (!adapter->fe)
			goto exit;

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
	}
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

	.tuner_get_frequency	= stb6100_get_frequency,
	.tuner_set_frequency	= stb6100_set_frequency,
	.tuner_set_bandwidth	= stb6100_set_bandwidth,
	.tuner_get_bandwidth	= stb6100_get_bandwidth,
};

static struct stb6100_config stb6100_config = {
	.tuner_address	= 0x60,
	.refclock	= 27000000
};

static struct tbs6925ctrl_config tbs6925_config[1] = { 
	{
	.tbs6925ctrl_address = 0x08,

	.tbs6925_ctrl1 = tbsctrl1,
	.tbs6925_ctrl2 = tbsctrl2,
	}
};

static int saa716x_tbs6925_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	struct tbs6925ctrl_dev *ctl;

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
				dvb_attach(stb6100_attach, adapter->fe, &stb6100_config, 
						&i2c0->i2c_adapter);
				tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
				memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
				printk(KERN_INFO "TurboSight TBS6925 DVB-S2 card MAC=%pM\n",
					adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}

		ctl = dvb_attach(tbs6925ctrl_attach, adapter->fe, &i2c0->i2c_adapter, 
						&tbs6925_config[0]);

		if (!ctl) 
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
	}
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
		}
	}
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
	}
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
	}
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
	}
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
	}
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
		adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config0,
                                			&i2c1->i2c_adapter);

		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6284 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 1) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config1,
                                			&i2c1->i2c_adapter);
		if (!adapter->fe)
			goto exit;

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
		adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config0,
                                			&i2c0->i2c_adapter);

		if (!adapter->fe)
			goto exit;

		tbs_read_mac(&i2c0->i2c_adapter, 160 + 16*count, mac);
		memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
		printk(KERN_INFO "TurboSight TBS6284 DVB-T2 card port%d MAC=%pM\n",
			count, adapter->dvb_adapter.proposed_mac);

		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	if (count == 3) {
		dprintk(SAA716x_ERROR, 1, "Probing for TBS62x0FE %d", count);
		adapter->fe = dvb_attach(tbs62x0fe_attach, &tbs6280fe_config1,
                                			&i2c0->i2c_adapter);
		if (!adapter->fe)
			goto exit;

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
	}
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
	}
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
	}
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
	}
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
	}
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
	}
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

static struct tbs6926ctrl_config tbs6926ctl_config[1] = { 
	{
	.tbs6926ctrl_address = 0x08,

	.tbs6926_ctrl1 = tbsctrl1,
	.tbs6926_ctrl2 = tbsctrl2,
	}
};

static int saa716x_tbs6926_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	struct tbs6926_state *ctl1;
	struct tbs6926ctrl_dev *ctl2;

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
				ctl1 = dvb_attach(tbs6926_attach, adapter->fe, &tbs6926_config, 
						&i2c0->i2c_adapter);
				if (!ctl1)
					goto exit;
				tbs_read_mac(&i2c1->i2c_adapter, 160 + 16*count, mac);
				memcpy(adapter->dvb_adapter.proposed_mac, mac, 6);
				printk(KERN_INFO "TurboSight TBS6926 DVB-S2 card MAC=%pM\n",
					adapter->dvb_adapter.proposed_mac);
		} else {
			goto exit;
		}

		ctl2 = dvb_attach(tbs6926ctrl_attach, adapter->fe, &i2c0->i2c_adapter, 
						&tbs6926ctl_config[0]);

		if (!ctl2) 
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
	}
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
	}
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

	.tuner_get_frequency	= stb6100_get_frequency,
	.tuner_set_frequency	= stb6100_set_frequency,
	.tuner_set_bandwidth	= stb6100_set_bandwidth,
	.tuner_get_bandwidth	= stb6100_get_bandwidth,
};

static struct stb6100_config stb6100ve_config = {
	.tuner_address	= 0x60,
	.refclock	= 27000000
};

static struct tbs6925ctrl_config tbs6925ve_config[1] = { 
	{
	.tbs6925ctrl_address = 0x08,

	.tbs6925_ctrl1 = tbsctrl1,
	.tbs6925_ctrl2 = tbsctrl2,
	}
};

static int saa716x_tbs6925ve_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[0];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[1];
	u8 mac[6];

	struct tbs6925ctrl_dev *ctl;

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

		ctl = dvb_attach(tbs6925ctrl_attach, adapter->fe, &i2c0->i2c_adapter, 
						&tbs6925ve_config[0]);

		if (!ctl) 
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
	}
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
