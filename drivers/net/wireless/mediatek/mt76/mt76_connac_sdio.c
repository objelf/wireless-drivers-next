// SPDX-License-Identifier: ISC
/* Copyright (C) 2020-2021 MediaTek Inc.
 *
 * Author: Felix Fietkau <nbd@nbd.name>
 *	   Lorenzo Bianconi <lorenzo@kernel.org>
 *	   Sean Wang <sean.wang@mediatek.com>
 */

#include <linux/iopoll.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>

#include "mt76.h"
#include "mt76_connac_mcu.h"
#include "mt76_connac_sdio.h"

static u32 mt76_connac_sdio_read_whisr(struct mt76_dev *dev)
{
	return sdio_readl(dev->sdio.func, MCR_WHISR, NULL);
}

u32 mt76_connac_sdio_read_pcr(struct mt76_dev *dev)
{
	struct mt76_sdio *sdio = &dev->sdio;

	return sdio_readl(sdio->func, MCR_WHLPCR, NULL);
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_read_pcr);

u32 mt76_connac_sdio_read_mailbox(struct mt76_dev *dev, u32 offset)
{
	struct sdio_func *func = dev->sdio.func;
	u32 val = ~0, status;
	int err;

	sdio_claim_host(func);

	sdio_writel(func, offset, MCR_H2DSM0R, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed setting address [err=%d]\n", err);
		goto out;
	}

	sdio_writel(func, H2D_SW_INT_READ, MCR_WSICR, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed setting read mode [err=%d]\n", err);
		goto out;
	}

	err = readx_poll_timeout(mt76_connac_sdio_read_whisr, dev, status,
				 status & H2D_SW_INT_READ, 0, 1000000);
	if (err < 0) {
		dev_err(dev->dev, "query whisr timeout\n");
		goto out;
	}

	sdio_writel(func, H2D_SW_INT_READ, MCR_WHISR, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed setting read mode [err=%d]\n", err);
		goto out;
	}

	val = sdio_readl(func, MCR_H2DSM0R, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed reading h2dsm0r [err=%d]\n", err);
		goto out;
	}

	if (val != offset) {
		dev_err(dev->dev, "register mismatch\n");
		val = ~0;
		goto out;
	}

	val = sdio_readl(func, MCR_D2HRM1R, &err);
	if (err < 0)
		dev_err(dev->dev, "failed reading d2hrm1r [err=%d]\n", err);

out:
	sdio_release_host(func);

	return val;
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_read_mailbox);

void mt76_connac_sdio_write_mailbox(struct mt76_dev *dev, u32 offset, u32 val)
{
	struct sdio_func *func = dev->sdio.func;
	u32 status;
	int err;

	sdio_claim_host(func);

	sdio_writel(func, offset, MCR_H2DSM0R, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed setting address [err=%d]\n", err);
		goto out;
	}

	sdio_writel(func, val, MCR_H2DSM1R, &err);
	if (err < 0) {
		dev_err(dev->dev,
			"failed setting write value [err=%d]\n", err);
		goto out;
	}

	sdio_writel(func, H2D_SW_INT_WRITE, MCR_WSICR, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed setting write mode [err=%d]\n", err);
		goto out;
	}

	err = readx_poll_timeout(mt76_connac_sdio_read_whisr, dev, status,
				 status & H2D_SW_INT_WRITE, 0, 1000000);
	if (err < 0) {
		dev_err(dev->dev, "query whisr timeout\n");
		goto out;
	}

	sdio_writel(func, H2D_SW_INT_WRITE, MCR_WHISR, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed setting write mode [err=%d]\n", err);
		goto out;
	}

	val = sdio_readl(func, MCR_H2DSM0R, &err);
	if (err < 0) {
		dev_err(dev->dev, "failed reading h2dsm0r [err=%d]\n", err);
		goto out;
	}

	if (val != offset)
		dev_err(dev->dev, "register mismatch\n");

out:
	sdio_release_host(func);
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_write_mailbox);

u32 mt76_connac_sdio_rr(struct mt76_dev *dev, u32 offset)
{
	if (test_bit(MT76_STATE_MCU_RUNNING, &dev->phy.state))
		return mt76_connac_mcu_reg_rr(dev, offset);
	else
		return mt76_connac_sdio_read_mailbox(dev, offset);
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_rr);

void mt76_connac_sdio_wr(struct mt76_dev *dev, u32 offset, u32 val)
{
	if (test_bit(MT76_STATE_MCU_RUNNING, &dev->phy.state))
		mt76_connac_mcu_reg_wr(dev, offset, val);
	else
		mt76_connac_sdio_write_mailbox(dev, offset, val);
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_wr);

u32 mt76_connac_sdio_rmw(struct mt76_dev *dev, u32 offset, u32 mask, u32 val)
{
	val |= mt76_connac_sdio_rr(dev, offset) & ~mask;
	mt76_connac_sdio_wr(dev, offset, val);

	return val;
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_rmw);

void mt76_connac_sdio_write_copy(struct mt76_dev *dev, u32 offset,
				 const void *data, int len)
{
	const u32 *val = data;
	int i;

	for (i = 0; i < len / sizeof(u32); i++) {
		mt76_connac_sdio_wr(dev, offset, val[i]);
		offset += sizeof(u32);
	}
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_write_copy);

void mt76_connac_sdio_read_copy(struct mt76_dev *dev, u32 offset,
				void *data, int len)
{
	u32 *val = data;
	int i;

	for (i = 0; i < len / sizeof(u32); i++) {
		val[i] = mt76_connac_sdio_rr(dev, offset);
		offset += sizeof(u32);
	}
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_read_copy);

int mt76_connac_sdio_wr_rp(struct mt76_dev *dev, u32 base,
			   const struct mt76_reg_pair *data,
			   int len)
{
	int i;

	for (i = 0; i < len; i++) {
		mt76_connac_sdio_wr(dev, data->reg, data->value);
		data++;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_wr_rp);

int mt76_connac_sdio_rd_rp(struct mt76_dev *dev, u32 base,
			   struct mt76_reg_pair *data,
			   int len)
{
	int i;

	for (i = 0; i < len; i++) {
		data->value = mt76_connac_sdio_rr(dev, data->reg);
		data++;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_rd_rp);

int mt76_connac_sdio_hw_init(struct mt76_dev *dev, struct sdio_func *func,
			     int hw_ver, sdio_irq_handler_t *irq_handler)
{
	u32 status, ctrl;
	int ret;

	dev->sdio.hw_ver = hw_ver;

	sdio_claim_host(func);

	ret = sdio_enable_func(func);
	if (ret < 0)
		goto release;

	/* Get ownership from the device */
	sdio_writel(func, WHLPCR_INT_EN_CLR | WHLPCR_FW_OWN_REQ_CLR,
		    MCR_WHLPCR, &ret);
	if (ret < 0)
		goto disable_func;

	ret = readx_poll_timeout(mt76_connac_sdio_read_pcr, dev, status,
				 status & WHLPCR_IS_DRIVER_OWN, 2000, 1000000);
	if (ret < 0) {
		dev_err(dev->dev, "Cannot get ownership from device");
		goto disable_func;
	}

	ret = sdio_set_block_size(func, 512);
	if (ret < 0)
		goto disable_func;

	/* Enable interrupt */
	sdio_writel(func, WHLPCR_INT_EN_SET, MCR_WHLPCR, &ret);
	if (ret < 0)
		goto disable_func;

	ctrl = WHIER_RX0_DONE_INT_EN | WHIER_TX_DONE_INT_EN;
	sdio_writel(func, ctrl, MCR_WHIER, &ret);
	if (ret < 0)
		goto disable_func;

	switch (hw_ver) {
	case MT76_CONNAC_SDIO_VER1:
		/* set WHISR as read clear and Rx aggregation number as 16 */
		ctrl = FIELD_PREP(MAX_HIF_RX_LEN_NUM, 16);
		break;
	default:
		ctrl = FIELD_PREP(MAX_HIF_RX_LEN_NUM_V2, 32);
		break;
	}

	sdio_writel(func, ctrl, MCR_WHCR, &ret);
	if (ret < 0)
		goto disable_func;

	ret = sdio_claim_irq(func, irq_handler);
	if (ret < 0)
		goto disable_func;

	sdio_release_host(func);

	return 0;

disable_func:
	sdio_disable_func(func);
release:
	sdio_release_host(func);

	return ret;
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_hw_init);

int mt76_connac_sdio_init(struct mt76_dev *dev,
			  void (*txrx_worker)(struct mt76_worker *))
{
	int i, ret;

	switch (dev->sdio.hw_ver) {
	case MT76_CONNAC_SDIO_VER1:
		dev->sdio.intr_size = sizeof(struct mt76_connac_sdio_intr_v1);
		break;
	default:
		dev->sdio.intr_size = sizeof(struct mt76_connac_sdio_intr_v2);
		break;
	}

	dev->sdio.intr_data = devm_kmalloc(dev->dev,
					   dev->sdio.intr_size,
					   GFP_KERNEL);
	if (!dev->sdio.intr_data)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(dev->sdio.xmit_buf); i++) {
		dev->sdio.xmit_buf[i] = devm_kmalloc(dev->dev,
						     MT76S_XMIT_BUF_SZ,
						     GFP_KERNEL);
		if (!dev->sdio.xmit_buf[i])
			return -ENOMEM;
	}

	ret = mt76_worker_setup(dev->hw, &dev->sdio.txrx_worker, txrx_worker,
				"sdio-txrx");
	if (ret)
		return ret;

	sched_set_fifo_low(dev->sdio.txrx_worker.task);

	return 0;
}
EXPORT_SYMBOL_GPL(mt76_connac_sdio_init);

MODULE_AUTHOR("Sean Wang <sean.wang@mediatek.com>");
MODULE_AUTHOR("Lorenzo Bianconi <lorenzo@kernel.org>");
MODULE_LICENSE("Dual BSD/GPL");
