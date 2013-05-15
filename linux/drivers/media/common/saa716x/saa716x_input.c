/*
    Driver for the SAA716x IR receiver
    Copyright (C) 2012 Konstantin Dimitrov <kosio.dimitrov@gmail.com>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/slab.h>
#include <media/rc-core.h>

#include "saa716x_input.h"
#include "saa716x_reg.h"

#define MODULE_NAME "saa716x"

static void saa716x_ir_raw_decode_timer_end(unsigned long data)
{
	struct saa716x_dev *saa716x = (struct saa716x_dev *)data;
	struct saa716x_ir *ir = saa716x->ir;

	ir_raw_event_handle(saa716x->ir->rc);

	ir->active = false;
}

static int saa716x_input_ir_start(void *priv)
{
	struct saa716x_dev *saa716x = priv;
	struct saa716x_ir *ir;

	if (saa716x->ir == NULL)
		return -EINVAL;

	ir = saa716x->ir;

	if (ir->running)
		return 0;

	ir->running = true;
	ir->active = false;

	setup_timer(&ir->timer, saa716x_ir_raw_decode_timer_end,
		(unsigned long)saa716x);

	return 0;
}

static int saa716x_input_ir_open(struct rc_dev *rc)
{
	struct saa716x_dev *saa716x = rc->priv;

	if (saa716x == NULL)
		return -ENODEV;

	return saa716x_input_ir_start(saa716x);
}

static void saa716x_input_ir_stop(void *priv)
{
	struct saa716x_dev *saa716x = priv;
	struct saa716x_ir *ir;

	if (saa716x->ir == NULL)
		return;

	ir = saa716x->ir;

	if (!ir->running)
		return;

	del_timer_sync(&ir->timer);

	ir->active = false;
	ir->running = false;

	return;
}

static void saa716x_input_ir_close(struct rc_dev *rc)
{
	struct saa716x_dev *saa716x = rc->priv;

	if (saa716x != NULL)
		saa716x_input_ir_stop(saa716x);
}

int saa716x_input_init(struct saa716x_dev *saa716x)
{
	struct saa716x_ir *ir;
	struct rc_dev *rc;

	int ret;

	/* saa716x board instance IR state */
	ir = kzalloc(sizeof(struct saa716x_ir), GFP_KERNEL);
	if (ir == NULL)
		return -ENOMEM;

	ir->name = kasprintf(GFP_KERNEL, "saa716x IR (%s)",
		saa716x->config->model_name);
	ir->phys = kasprintf(GFP_KERNEL, "pci-%s/ir0",
		pci_name(saa716x->pdev));

	/* input device */
	rc = rc_allocate_device();
	if (!rc) {
		ret = -ENOMEM;
		goto err_out_free;
	}

	ir->rc = rc;
	rc->input_name = ir->name;
	rc->input_phys = ir->phys;
	rc->input_id.bustype = BUS_PCI;
	rc->input_id.version = 1;
	if (saa716x->pdev->subsystem_vendor) {
		rc->input_id.vendor  = saa716x->pdev->subsystem_vendor;
		rc->input_id.product = saa716x->pdev->subsystem_device;
	} else {
		rc->input_id.vendor  = saa716x->pdev->vendor;
		rc->input_id.product = saa716x->pdev->device;
	}
	rc->dev.parent = &saa716x->pdev->dev;
	rc->driver_type = RC_DRIVER_IR_RAW;
	rc->priv = saa716x;
	rc->open = saa716x_input_ir_open;
	rc->close = saa716x_input_ir_close;
	rc->driver_name = MODULE_NAME;

	/* hardware specific */
	rc->map_name = RC_MAP_TBS_NEC;
	ir->mask_keyevent  = 1 << 4;

	saa716x->ir = ir;
	ret = rc_register_device(rc);
	if (ret)
		goto err_out_stop;

	return 0;

err_out_stop:
	saa716x->ir = NULL;
	rc_free_device(rc);
err_out_free:
	kfree(ir->phys);
	kfree(ir->name);
	kfree(ir);
	return ret;
}

void saa716x_input_fini(struct saa716x_dev *saa716x)
{
	if (saa716x->ir == NULL)
		return;
	saa716x_input_ir_stop(saa716x);
	rc_unregister_device(saa716x->ir->rc);
	kfree(saa716x->ir->phys);
	kfree(saa716x->ir->name);
	kfree(saa716x->ir);
	saa716x->ir = NULL;
}

static int saa716x_ir_raw_decode_irq(struct saa716x_dev *saa716x)
{
	struct saa716x_ir *ir = saa716x->ir;
	unsigned long timeout;
	int space;

	/* key event */
	space = SAA716x_EPRD(GPIO, GPIO_RD) & ir->mask_keyevent;
	ir_raw_event_store_edge(saa716x->ir->rc, space ? IR_SPACE : IR_PULSE);

	/* 15ms wait time before start processing the first event */
	if (!ir->active) {
		timeout = jiffies + msecs_to_jiffies(15);
		mod_timer(&ir->timer, timeout);
		ir->active = true;
	}

	return 1;
}

void saa716x_input_irq_handler(struct saa716x_dev *saa716x)
{
	struct saa716x_ir *ir;

	if (!saa716x || !saa716x->ir)
		return;

	ir = saa716x->ir;
	
	if (!ir->running)
		return;

	saa716x_ir_raw_decode_irq(saa716x);
}
