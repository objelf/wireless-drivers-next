// SPDX-License-Identifier: ISC
/* Copyright (C) 2019 MediaTek Inc.
 *
 * Author: Ryder Lee <ryder.lee@mediatek.com>
 *         Felix Fietkau <nbd@nbd.name>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "mt7615.h"

static const struct pci_device_id mt7615_pci_device_table[] = {
	{ PCI_DEVICE(0x14c3, 0x7615) },
	{ PCI_DEVICE(0x14c3, 0x7663) },
	{ },
};

static int mt7615_pci_probe(struct pci_dev *pdev,
			    const struct pci_device_id *id)
{
	const u32 *map;
	int ret;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = pcim_iomap_regions(pdev, BIT(0), pci_name(pdev));
	if (ret)
		return ret;

	pci_set_master(pdev);

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (ret < 0)
		return ret;

	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	map = id->device == 0x7663 ? mt7663e_reg_map : mt7615e_reg_map;
	return mt7615_mmio_probe(&pdev->dev, pcim_iomap_table(pdev)[0],
				 pdev->irq, map);
}

static void mt7615_pci_remove(struct pci_dev *pdev)
{
	struct mt76_dev *mdev = pci_get_drvdata(pdev);
	struct mt7615_dev *dev = container_of(mdev, struct mt7615_dev, mt76);

	mt7615_unregister_device(dev);
	pci_free_irq_vectors(pdev);
}

static int __maybe_unused mt7615_pci_suspend(struct pci_dev *pdev,
					     pm_message_t state)
{
	struct mt76_dev *mdev = pci_get_drvdata(pdev);
	struct mt7615_dev *dev = container_of(mdev, struct mt7615_dev, mt76);
	int i;

	if (!test_bit(MT76_STATE_SUSPEND, &dev->mphy.state) &&
	    mt7615_firmware_offload(dev)) {
		int err;

		err = mt7615_mcu_set_hif_suspend(dev, true);
		if (err < 0)
			return err;
	}

	napi_disable(&mdev->tx_napi);
	for (i = 0; i < ARRAY_SIZE(mdev->q_tx); i++)
		mt76_queue_tx_cleanup(dev, i, true);
	tasklet_kill(&mdev->tx_tasklet);

	for (i = 0; i < ARRAY_SIZE(mdev->q_rx); i++) {
		napi_disable(&mdev->napi[i]);
		mt76_queue_rx_reset(dev, i);
	}
	tasklet_kill(&dev->irq_tasklet);

	return 0;
}

static int __maybe_unused mt7615_pci_resume(struct pci_dev *pdev)
{
	struct mt76_dev *mdev = pci_get_drvdata(pdev);
	struct mt7615_dev *dev = container_of(mdev, struct mt7615_dev, mt76);
	int i, err = 0;

	for (i = 0; i < ARRAY_SIZE(mdev->q_rx); i++)
		napi_enable(&mdev->napi[i]);

	napi_enable(&mdev->tx_napi);

	if (!test_bit(MT76_STATE_SUSPEND, &dev->mphy.state) &&
	    mt7615_firmware_offload(dev))
		err = mt7615_mcu_set_hif_suspend(dev, false);

	return err;
}


struct pci_driver mt7615_pci_driver = {
	.name		= KBUILD_MODNAME,
	.id_table	= mt7615_pci_device_table,
	.probe		= mt7615_pci_probe,
	.remove		= mt7615_pci_remove,
#ifdef CONFIG_PM
	.suspend 	= mt7615_pci_suspend,
	.resume 	= mt7615_pci_resume,
#endif /* CONFIG_PM */
};

MODULE_DEVICE_TABLE(pci, mt7615_pci_device_table);
MODULE_FIRMWARE(MT7615_FIRMWARE_CR4);
MODULE_FIRMWARE(MT7615_FIRMWARE_N9);
MODULE_FIRMWARE(MT7615_ROM_PATCH);
MODULE_FIRMWARE(MT7663_FIRMWARE_N9);
MODULE_FIRMWARE(MT7663_ROM_PATCH);
