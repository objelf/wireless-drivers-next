// SPDX-License-Identifier: ISC
/* Copyright (C) 2021 MediaTek Inc.
 *
 */

#include <linux/kernel.h>
#include <linux/iopoll.h>
#include <linux/module.h>

#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>

#include "mt7921.h"
#include "../mt76_connac_sdio.h"
#include "mac.h"
#include "mcu.h"

static const struct sdio_device_id mt7921s_table[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x7901) },
	{ }	/* Terminating entry */
};

static void mt7921s_txrx_worker(struct mt76_worker *w)
{
	struct mt76_sdio *sdio = container_of(w, struct mt76_sdio,
					      txrx_worker);
	struct mt76_dev *mdev = container_of(sdio, struct mt76_dev, sdio);
	struct mt7921_dev *dev = container_of(mdev, struct mt7921_dev, mt76);

	if (!mt76_connac_pm_ref(&dev->mphy, &dev->pm)) {
		queue_work(mdev->wq, &dev->pm.wake_work);
		return;
	}

	mt76_connac_sdio_txrx(mdev);

	mt76_connac_pm_unref(&dev->mphy, &dev->pm);
}

static void mt7921s_irq(struct sdio_func *func)
{
	struct mt7921_dev *dev = sdio_get_drvdata(func);
	struct mt76_sdio *sdio = &dev->mt76.sdio;

	if (!test_bit(MT76_STATE_INITIALIZED, &dev->mt76.phy.state) ||
	     test_bit(MT76_MCU_RESET, &dev->mt76.phy.state))
		return;

	mt76_worker_schedule(&sdio->txrx_worker);
}

static const struct mt7921_hif_ops mt7921_sdio_ops = {
	.reset = mt7921s_mac_reset,
	.mcu_init = mt7921s_mcu_init,
	.drv_own = mt7921s_mcu_drv_pmctrl,
	.fw_own = mt7921s_mcu_fw_pmctrl,
};

static int mt7921s_probe(struct sdio_func *func,
			 const struct sdio_device_id *id)
{
	static const struct mt76_driver_ops drv_ops = {
		.txwi_size = MT_SDIO_TXD_SIZE,
		.drv_flags = MT_DRV_TXWI_NO_FREE |
			     MT_DRV_AMSDU_OFFLOAD,
		.tx_prepare_skb = mt7921s_tx_prepare_skb,
		.tx_complete_skb = mt7921s_tx_complete_skb,
		.tx_status_data = mt7921s_tx_status_data,
		.rx_skb = mt7921_queue_rx_skb,
		.sta_ps = mt7921_sta_ps,
		.sta_add = mt7921_mac_sta_add,
		.sta_assoc = mt7921_mac_sta_assoc,
		.sta_remove = mt7921_mac_sta_remove,
		.update_survey = mt7921_update_channel,
	};
	static const struct mt76_bus_ops mt7921s_ops = {
		.rr = mt76_connac_sdio_rr,
		.rmw = mt76_connac_sdio_rmw,
		.wr = mt76_connac_sdio_wr,
		.write_copy = mt76_connac_sdio_write_copy,
		.read_copy = mt76_connac_sdio_read_copy,
		.wr_rp = mt76_connac_sdio_wr_rp,
		.rd_rp = mt76_connac_sdio_rd_rp,
		.type = MT76_BUS_SDIO,
	};
	struct mt7921_dev *dev;
	struct mt76_dev *mdev;
	int ret;

	mdev = mt76_alloc_device(&func->dev, sizeof(*dev), &mt7921_ops,
				 &drv_ops);
	if (!mdev)
		return -ENOMEM;

	dev = container_of(mdev, struct mt7921_dev, mt76);
	dev->hif_ops = &mt7921_sdio_ops;

	sdio_set_drvdata(func, dev);

	ret = mt76s_init(mdev, func, &mt7921s_ops);
	if (ret < 0)
		goto error;

	ret = mt76_connac_sdio_hw_init(mdev, func, MT76_CONNAC_SDIO_VER2,
				       mt7921s_irq);
	if (ret)
		goto error;

	mdev->rev = (mt76_rr(dev, MT_HW_CHIPID) << 16) |
		    (mt76_rr(dev, MT_HW_REV) & 0xff);
	dev_dbg(mdev->dev, "ASIC revision: %04x\n", mdev->rev);

	ret = mt76_connac_sdio_init(mdev, mt7921s_txrx_worker);
	if (ret)
		goto error;

	ret = mt76s_alloc_queues(&dev->mt76);
	if (ret)
		goto error;

	ret = mt7921_register_device(dev);
	if (ret)
		goto error;

	return 0;

error:
	mt76s_deinit(&dev->mt76);
	mt76_free_device(&dev->mt76);

	return ret;
}

static void mt7921s_remove(struct sdio_func *func)
{
	struct mt7921_dev *dev = sdio_get_drvdata(func);

	mt7921s_unregister_device(dev);
}

#ifdef CONFIG_PM
static int mt7921s_suspend(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	struct mt7921_dev *mdev = sdio_get_drvdata(func);
	int err;

	if (!test_bit(MT76_STATE_SUSPEND, &mdev->mphy.state)) {
		int err;

		err = mt76_connac_mcu_set_hif_suspend(&mdev->mt76, true);
		if (err < 0)
			return err;
	}

	sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);

	err = mdev->hif_ops->fw_own(mdev);
	if (err)
		return err;

	mt76_worker_disable(&mdev->mt76.sdio.txrx_worker);
	mt76_worker_disable(&mdev->mt76.sdio.status_worker);
	mt76_worker_disable(&mdev->mt76.sdio.net_worker);

	cancel_work_sync(&mdev->mt76.sdio.stat_work);
	clear_bit(MT76_READING_STATS, &mdev->mphy.state);

	mt76_tx_status_check(&mdev->mt76, NULL, true);

	return 0;
}

static int mt7921s_resume(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	struct mt7921_dev *mdev = sdio_get_drvdata(func);
	int err;

	mt76_worker_enable(&mdev->mt76.sdio.txrx_worker);
	mt76_worker_enable(&mdev->mt76.sdio.status_worker);
	mt76_worker_enable(&mdev->mt76.sdio.net_worker);

	err = mdev->hif_ops->drv_own(mdev);
	if (err)
		return err;

	if (!test_bit(MT76_STATE_SUSPEND, &mdev->mphy.state))
		err = mt76_connac_mcu_set_hif_suspend(&mdev->mt76, false);

	return err;
}

static const struct dev_pm_ops mt7921s_pm_ops = {
	.suspend = mt7921s_suspend,
	.resume = mt7921s_resume,
};
#endif

MODULE_DEVICE_TABLE(sdio, mt7921s_table);
MODULE_FIRMWARE(MT7921_FIRMWARE_WM);
MODULE_FIRMWARE(MT7921_ROM_PATCH);

static struct sdio_driver mt7921s_driver = {
	.name		= KBUILD_MODNAME,
	.probe		= mt7921s_probe,
	.remove		= mt7921s_remove,
	.id_table	= mt7921s_table,
#ifdef CONFIG_PM
	.drv = {
		.pm = &mt7921s_pm_ops,
	}
#endif
};
module_sdio_driver(mt7921s_driver);
MODULE_AUTHOR("Sean Wang <sean.wang@mediatek.com>");
MODULE_LICENSE("Dual BSD/GPL");
