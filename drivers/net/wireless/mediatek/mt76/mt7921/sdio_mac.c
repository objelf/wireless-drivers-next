// SPDX-License-Identifier: ISC
/* Copyright (C) 2021 MediaTek Inc. */

#include "mt7921.h"
#include "mac.h"

int mt7921s_wfsys_reset(struct mt7921_dev *dev)
{
	return 0;
}

int mt7921s_mac_reset(struct mt7921_dev *dev)
{
	return 0;
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
	pad = round_up(skb->len, 4) - skb->len;

	return mt76_skb_adjust_pad(skb, pad);
}

void mt7921s_tx_complete_skb(struct mt76_dev *mdev, struct mt76_queue_entry *e)
{
	unsigned int headroom = MT_SDIO_TXD_SIZE;

	skb_pull(e->skb, headroom);
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
