// SPDX-License-Identifier: ISC
/* Copyright (C) 2021 MediaTek Inc. */

#include "mt7921.h"

void mt7921_unregister_device(struct mt7921_dev *dev)
{
	int i;
	struct mt76_connac_pm *pm = &dev->pm;

	mt76_unregister_device(&dev->mt76);
	mt76_for_each_q_rx(&dev->mt76, i)
		napi_disable(&dev->mt76.napi[i]);
	cancel_delayed_work_sync(&pm->ps_work);
	cancel_work_sync(&pm->wake_work);

	mt7921_tx_token_put(dev);
	mt7921_mcu_drv_pmctrl(dev);
	mt7921_dma_cleanup(dev);
	mt7921_wfsys_reset(dev);
	mt7921_mcu_exit(dev);
	mt7921_mcu_fw_pmctrl(dev);

	tasklet_disable(&dev->irq_tasklet);
	mt76_free_device(&dev->mt76);
}
