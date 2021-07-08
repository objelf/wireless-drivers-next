// SPDX-License-Identifier: ISC
/* Copyright (C) 2020 MediaTek Inc. */
#include "mt7921.h"

void mt7921_unregister_device(struct mt7921_dev *dev)
{
	mt76_unregister_device(&dev->mt76);
	mt7921_tx_token_put(dev);
	mt7921_dma_cleanup(dev);
	mt7921_wfsys_reset(dev);
	mt7921_mcu_exit(dev);

	tasklet_disable(&dev->irq_tasklet);
	mt76_free_device(&dev->mt76);
}
