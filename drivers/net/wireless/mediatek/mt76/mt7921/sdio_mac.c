// SPDX-License-Identifier: ISC
/* Copyright (C) 2021 MediaTek Inc. */

#include <linux/iopoll.h>
#include <linux/mmc/sdio_func.h>
#include "mt7921.h"
#include "mac.h"
#include "../mt76_connac_sdio.h"

static u32 mt7921s_sdio_read_whcr(struct mt76_dev *dev)
{
	return sdio_readl(dev->sdio.func, MCR_WHCR, NULL);
}

int mt7921s_wfsys_reset(struct mt7921_dev *dev)
{
	struct mt76_sdio *sdio = &dev->mt76.sdio;
	u32 val, status;

	sdio_claim_host(sdio->func);

	val = sdio_readl(sdio->func, MCR_WHCR, NULL);
	val &= ~WF_WHOLE_PATH_RSTB;
	sdio_writel(sdio->func, val, MCR_WHCR, NULL);

	msleep(50);

	val = sdio_readl(sdio->func, MCR_WHCR, NULL);
	val &= ~WF_SDIO_WF_PATH_RSTB;
	sdio_writel(sdio->func, val, MCR_WHCR, NULL);

	msleep(1);

	val = sdio_readl(sdio->func, MCR_WHCR, NULL);
	val |= WF_WHOLE_PATH_RSTB;
	sdio_writel(sdio->func, val, MCR_WHCR, NULL);

	readx_poll_timeout(mt7921s_sdio_read_whcr, &dev->mt76, status,
			   status & WF_RST_DONE, 50000, 2000000);

	sdio_release_host(sdio->func);

	return 0;
}

int mt7921s_mac_reset(struct mt7921_dev *dev)
{
	int err;
return 0;
	mt76_connac_free_pending_tx_skbs(&dev->pm, NULL);

	set_bit(MT76_RESET, &dev->mphy.state);
	set_bit(MT76_MCU_RESET, &dev->mphy.state);
	wake_up(&dev->mt76.mcu.wait);
	skb_queue_purge(&dev->mt76.mcu.res_q);

	mt76_txq_schedule_all(&dev->mphy);
	mt76_worker_disable(&dev->mt76.tx_worker);
	mt76_worker_disable(&dev->mt76.sdio.txrx_worker);
	mt76_worker_disable(&dev->mt76.sdio.status_worker);
	mt76_worker_disable(&dev->mt76.sdio.net_worker);
	cancel_work_sync(&dev->mt76.sdio.stat_work);

	mt76_connac_sdio_disable_irq(&dev->mt76);
	mt76s_free_all_txqs(&dev->mt76);
	mt7921s_wfsys_reset(dev);

	mt76_worker_enable(&dev->mt76.sdio.txrx_worker);
	mt76_worker_enable(&dev->mt76.sdio.status_worker);
	mt76_worker_enable(&dev->mt76.sdio.net_worker);

	clear_bit(MT76_MCU_RESET, &dev->mphy.state);
	mt76_connac_sdio_enable_irq(&dev->mt76);

	err = mt7921_run_firmware(dev);
	if (err)
		goto out;

	err = mt7921_mcu_set_eeprom(dev);
	if (err)
		goto out;

	err = mt7921_mac_init(dev);
	if (err)
		goto out;

	err = __mt7921_start(&dev->phy);
out:
	clear_bit(MT76_RESET, &dev->mphy.state);

	mt76_worker_enable(&dev->mt76.tx_worker);

	return err;
}

static void
mt7921s_write_txwi(struct mt7921_dev *dev, struct mt76_wcid *wcid,
		   enum mt76_txq_id qid, struct ieee80211_sta *sta,
		   struct sk_buff *skb)
{
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_key_conf *key = info->control.hw_key;
	__le32 *txwi;

	txwi = (__le32 *)(skb->data - MT_SDIO_TXD_SIZE);
	memset(txwi, 0, MT_SDIO_TXD_SIZE);
	mt7921_mac_write_txwi(dev, txwi, skb, wcid, key, false);
	skb_push(skb, MT_SDIO_TXD_SIZE);
}

int mt7921s_tx_prepare_skb(struct mt76_dev *mdev, void *txwi_ptr,
			   enum mt76_txq_id qid, struct mt76_wcid *wcid,
			   struct ieee80211_sta *sta,
			   struct mt76_tx_info *tx_info)
{
	struct mt7921_dev *dev = container_of(mdev, struct mt7921_dev, mt76);
	struct sk_buff *skb = tx_info->skb;
	int pad;

	mt7921s_write_txwi(dev, wcid, qid, sta, skb);

	mt7921_skb_add_sdio_hdr(skb, MT7921_SDIO_DATA);
	pad = round_up(skb->len, 4) - skb->len;

	return mt76_skb_adjust_pad(skb, pad);
}

void mt7921s_tx_complete_skb(struct mt76_dev *mdev, struct mt76_queue_entry *e)
{
	unsigned int headroom = MT_SDIO_TXD_SIZE + MT_SDIO_HDR_SIZE;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(e->skb);

	skb_pull(e->skb, headroom);

	if (info->flags & IEEE80211_TX_CTL_AMPDU)
		info->flags |= IEEE80211_TX_STAT_AMPDU;

	if (!(info->flags & IEEE80211_TX_CTL_NO_ACK))
		info->flags |= IEEE80211_TX_STAT_ACK;

	info->status.tx_time = 0;
	mt76_tx_complete_skb(mdev, e->wcid, e->skb);
}

bool mt7921s_tx_status_data(struct mt76_dev *mdev, u8 *update)
{
	struct mt7921_dev *dev = container_of(mdev, struct mt7921_dev, mt76);

	mt7921_mutex_acquire(dev);
	mt7921_mac_sta_poll(dev);
	mt7921_mutex_release(dev);

	return 0;
}
