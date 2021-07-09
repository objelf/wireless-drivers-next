// SPDX-License-Identifier: ISC
/* Copyright (C) 2021 MediaTek Inc. */

#include <linux/etherdevice.h>
#include "mt7921.h"
#include "mac.h"
#include "mcu.h"
#include "eeprom.h"

void mt7921s_unregister_device(struct mt7921_dev *dev)
{
	mt76_unregister_device(&dev->mt76);
	mt76s_deinit(&dev->mt76);
	mt7921s_wfsys_reset(dev);
	mt7921_mcu_exit(dev);

	mt76_free_device(&dev->mt76);
}
