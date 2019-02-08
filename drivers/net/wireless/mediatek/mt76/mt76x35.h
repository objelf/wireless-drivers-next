/*
 * Copyright (C) 2019 Lorenzo Bianconi <lorenzo.bianconi83@gmail.com>
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

#ifndef __MT76x35_H
#define __MT76x35_H

#include "mt76.h"

struct mt76x35_sta {
	struct mt76_wcid wcid; /* must be first */

	struct mt76x35_vif *vif;

	struct sk_buff_head psq;

	struct ieee80211_tx_rate rates[8];
	u8 rate_count;
	u8 n_rates;

	u8 rate_probe;
	u8 smps;

	u8 ps;
};

struct mt76x35_vif {
	struct mt76x35_sta sta; /* must be first */

	u8 omac_idx;
	u8 band_idx;
	u8 wmm_idx;
	u8 idx;
};

enum mt76x35_reset_cause {
	RESET_CAUSE_TX_HANG,
	RESET_CAUSE_TX_BUSY,
	RESET_CAUSE_RX_BUSY,
	RESET_CAUSE_BEACON_STUCK,
	RESET_CAUSE_RX_PSE_BUSY,
	RESET_CAUSE_MCU_HANG,
	RESET_CAUSE_RESET_FAILED,
	__RESET_CAUSE_MAX
};

struct mt76x35_dev {
	struct mt76_dev mt76; /* must be first */

	const struct mt76_bus_ops *bus_ops;

	u32 rxfilter;

	u8 vif_mask;

	struct mt76x35_sta global_sta;

	u32 agc0, agc3;
	u32 false_cca_ofdm, false_cca_cck;
	unsigned long last_cca_adj;

	u8 rssi_offset[3];

	u8 slottime;
	s16 coverage_class;

	s8 tx_power_limit;

	ktime_t survey_time;
	ktime_t ed_time;
	int beacon_int;

	struct mt76_queue q_rx;

	spinlock_t ps_lock;

	u8 mac_work_count;

	u8 mcu_running;
	u8 ed_monitor;

	s8 ed_trigger;
	u8 ed_strict_mode;
	u8 ed_strong_signal;

	s8 sensitivity;

	u8 beacon_mask;

	u8 beacon_check;
	u8 tx_hang_check;
	u8 tx_dma_check;
	u8 rx_dma_check;
	u8 rx_pse_check;
	u8 mcu_hang;

	enum mt76x35_reset_cause cur_reset_cause;

	u16 tx_dma_idx[4];
	u16 rx_dma_idx;

	u32 reset_test;

	unsigned int reset_cause[__RESET_CAUSE_MAX];

	struct delayed_work mac_work;
	struct tasklet_struct tx_tasklet;
	struct tasklet_struct pre_tbtt_tasklet;
};

#endif /* __MT76x35_H */
