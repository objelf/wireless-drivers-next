/* SPDX-License-Identifier: ISC */
/*
 * Copyright (C) 2016 Felix Fietkau <nbd@nbd.name>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __MT7603_H
#define __MT7603_H

#include <linux/interrupt.h>
#include <linux/ktime.h>
#include "../mt76x35.h"
#include "regs.h"

#define MT7603_MAX_INTERFACES	4
#define MT7603_WTBL_SIZE	128
#define MT7603_WTBL_RESERVED	(MT7603_WTBL_SIZE - 1)
#define MT7603_WTBL_STA		(MT7603_WTBL_RESERVED - MT7603_MAX_INTERFACES)

#define MT7603_RATE_RETRY	2

#define MT7603_RX_RING_SIZE     128

#define MT7603_FIRMWARE_E1	"mt7603_e1.bin"
#define MT7603_FIRMWARE_E2	"mt7603_e2.bin"
#define MT7628_FIRMWARE_E1	"mt7628_e1.bin"
#define MT7628_FIRMWARE_E2	"mt7628_e2.bin"

#define MT7603_EEPROM_SIZE	1024

#define MT_AGG_SIZE_LIMIT(_n)	(((_n) + 1) * 4)

#define MT7603_PRE_TBTT_TIME	5000 /* ms */

#define MT7603_WATCHDOG_TIME	100 /* ms */
#define MT7603_WATCHDOG_TIMEOUT	10 /* number of checks */

#define MT7603_EDCCA_BLOCK_TH	10

#define MT7603_CFEND_RATE_DEFAULT	0x69 /* chip default (24M) */
#define MT7603_CFEND_RATE_11B		0x03 /* 11B LP, 11M */

enum {
	MT7603_REV_E1 = 0x00,
	MT7603_REV_E2 = 0x10,
	MT7628_REV_E1 = 0x8a00,
};

enum mt7603_bw {
	MT_BW_20,
	MT_BW_40,
	MT_BW_80,
};

extern const struct mt76_driver_ops mt7603_drv_ops;
extern const struct ieee80211_ops mt7603_ops;
extern struct pci_driver mt7603_pci_driver;
extern struct platform_driver mt76_wmac_driver;

static inline bool is_mt7603(struct mt76x35_dev *dev)
{
	return mt76xx_chip(dev) == 0x7603;
}

static inline bool is_mt7628(struct mt76x35_dev *dev)
{
	return mt76xx_chip(dev) == 0x7628;
}

/* need offset to prevent conflict with ampdu_ack_len */
#define MT_RATE_DRIVER_DATA_OFFSET	4

u32 mt7603_reg_map(struct mt76x35_dev *dev, u32 addr);

irqreturn_t mt7603_irq_handler(int irq, void *dev_instance);

int mt7603_register_device(struct mt76x35_dev *dev);
void mt7603_unregister_device(struct mt76x35_dev *dev);
int mt7603_eeprom_init(struct mt76x35_dev *dev);
int mt7603_dma_init(struct mt76x35_dev *dev);
void mt7603_dma_cleanup(struct mt76x35_dev *dev);
int mt7603_mcu_init(struct mt76x35_dev *dev);
void mt7603_init_debugfs(struct mt76x35_dev *dev);

void mt7603_set_irq_mask(struct mt76x35_dev *dev, u32 clear, u32 set);

static inline void mt7603_irq_enable(struct mt76x35_dev *dev, u32 mask)
{
	mt7603_set_irq_mask(dev, 0, mask);
}

static inline void mt7603_irq_disable(struct mt76x35_dev *dev, u32 mask)
{
	mt7603_set_irq_mask(dev, mask, 0);
}

void mt7603_mac_dma_start(struct mt76x35_dev *dev);
void mt7603_mac_start(struct mt76x35_dev *dev);
void mt7603_mac_stop(struct mt76x35_dev *dev);
void mt7603_mac_work(struct work_struct *work);
void mt7603_mac_set_timing(struct mt76x35_dev *dev);
void mt7603_beacon_set_timer(struct mt76x35_dev *dev, int idx, int intval);
int mt7603_mac_fill_rx(struct mt76x35_dev *dev, struct sk_buff *skb);
void mt7603_mac_add_txs(struct mt76x35_dev *dev, void *data);
void mt7603_mac_rx_ba_reset(struct mt76x35_dev *dev, void *addr, u8 tid);
void mt7603_mac_tx_ba_reset(struct mt76x35_dev *dev, int wcid, int tid, int ssn,
			    int ba_size);

void mt7603_pse_client_reset(struct mt76x35_dev *dev);

int mt7603_mcu_set_channel(struct mt76x35_dev *dev);
int mt7603_mcu_set_eeprom(struct mt76x35_dev *dev);
void mt7603_mcu_exit(struct mt76x35_dev *dev);

void mt7603_wtbl_init(struct mt76x35_dev *dev, int idx, int vif,
		      const u8 *mac_addr);
void mt7603_wtbl_clear(struct mt76x35_dev *dev, int idx);
void mt7603_wtbl_update_cap(struct mt76x35_dev *dev, struct ieee80211_sta *sta);
void mt7603_wtbl_set_rates(struct mt76x35_dev *dev, struct mt76x35_sta *sta,
			   struct ieee80211_tx_rate *probe_rate,
			   struct ieee80211_tx_rate *rates);
int mt7603_wtbl_set_key(struct mt76x35_dev *dev, int wcid,
			struct ieee80211_key_conf *key);
void mt7603_wtbl_set_ps(struct mt76x35_dev *dev, struct mt76x35_sta *sta,
			bool enabled);
void mt7603_wtbl_set_smps(struct mt76x35_dev *dev, struct mt76x35_sta *sta,
			  bool enabled);
void mt7603_filter_tx(struct mt76x35_dev *dev, int idx, bool abort);

int mt7603_tx_prepare_skb(struct mt76_dev *mdev, void *txwi_ptr,
			  struct sk_buff *skb, struct mt76_queue *q,
			  struct mt76_wcid *wcid, struct ieee80211_sta *sta,
			  u32 *tx_info);

void mt7603_tx_complete_skb(struct mt76_dev *mdev, struct mt76_queue *q,
			    struct mt76_queue_entry *e, bool flush);

void mt7603_queue_rx_skb(struct mt76_dev *mdev, enum mt76_rxq_id q,
			 struct sk_buff *skb);
void mt7603_rx_poll_complete(struct mt76_dev *mdev, enum mt76_rxq_id q);
void mt7603_sta_ps(struct mt76_dev *mdev, struct ieee80211_sta *sta, bool ps);
int mt7603_sta_add(struct mt76_dev *mdev, struct ieee80211_vif *vif,
		   struct ieee80211_sta *sta);
void mt7603_sta_assoc(struct mt76_dev *mdev, struct ieee80211_vif *vif,
		      struct ieee80211_sta *sta);
void mt7603_sta_remove(struct mt76_dev *mdev, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta);

void mt7603_pre_tbtt_tasklet(unsigned long arg);

void mt7603_update_channel(struct mt76_dev *mdev);

void mt7603_edcca_set_strict(struct mt76x35_dev *dev, bool val);
void mt7603_cca_stats_reset(struct mt76x35_dev *dev);

#endif
