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

#endif /* __MT76x35_H */
