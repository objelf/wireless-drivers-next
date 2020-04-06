// SPDX-License-Identifier: ISC

#include "mt7615.h"
#include "mcu.h"

static int
mt7615_radar_pattern_set(void *data, u64 val)
{
	struct mt7615_dev *dev = data;

	if (!mt7615_wait_for_mcu_init(dev))
		return 0;

	return mt7615_mcu_rdd_send_pattern(dev);
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_radar_pattern, NULL,
			 mt7615_radar_pattern_set, "%lld\n");

static int
mt7615_scs_set(void *data, u64 val)
{
	struct mt7615_dev *dev = data;
	struct mt7615_phy *ext_phy;

	if (!mt7615_wait_for_mcu_init(dev))
		return 0;

	mt7615_mac_set_scs(&dev->phy, val);
	ext_phy = mt7615_ext_phy(dev);
	if (ext_phy)
		mt7615_mac_set_scs(ext_phy, val);

	return 0;
}

static int
mt7615_scs_get(void *data, u64 *val)
{
	struct mt7615_dev *dev = data;

	*val = dev->phy.scs_en;

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_scs, mt7615_scs_get,
			 mt7615_scs_set, "%lld\n");

static int
mt7615_dbdc_set(void *data, u64 val)
{
	struct mt7615_dev *dev = data;

	if (!mt7615_wait_for_mcu_init(dev))
		return 0;

	if (val)
		mt7615_register_ext_phy(dev);
	else
		mt7615_unregister_ext_phy(dev);

	return 0;
}

static int
mt7615_dbdc_get(void *data, u64 *val)
{
	struct mt7615_dev *dev = data;

	*val = !!mt7615_ext_phy(dev);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_dbdc, mt7615_dbdc_get,
			 mt7615_dbdc_set, "%lld\n");

static int
mt7615_fw_debug_set(void *data, u64 val)
{
	struct mt7615_dev *dev = data;

	if (!mt7615_wait_for_mcu_init(dev))
		return 0;

	dev->fw_debug = val;
	mt7615_mcu_fw_log_2_host(dev, dev->fw_debug ? 2 : 0);

	return 0;
}

static int
mt7615_fw_debug_get(void *data, u64 *val)
{
	struct mt7615_dev *dev = data;

	*val = dev->fw_debug;

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_fw_debug, mt7615_fw_debug_get,
			 mt7615_fw_debug_set, "%lld\n");

static int
mt7615_reset_test_set(void *data, u64 val)
{
	struct mt7615_dev *dev = data;
	struct sk_buff *skb;

	if (!mt7615_wait_for_mcu_init(dev))
		return 0;

	skb = alloc_skb(1, GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

	skb_put(skb, 1);
	mt76_tx_queue_skb_raw(dev, 0, skb, 0);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_reset_test, NULL,
			 mt7615_reset_test_set, "%lld\n");

static int
mt7615_ampdu_stat_read(struct seq_file *file, void *data)
{
	struct mt7615_dev *dev = file->private;
	int bound[7], i, range;

	range = mt76_rr(dev, MT_AGG_ASRCR0);
	for (i = 0; i < 4; i++)
		bound[i] = MT_AGG_ASRCR_RANGE(range, i) + 1;
	range = mt76_rr(dev, MT_AGG_ASRCR1);
	for (i = 0; i < 3; i++)
		bound[i + 4] = MT_AGG_ASRCR_RANGE(range, i) + 1;

	seq_printf(file, "Length: %8d | ", bound[0]);
	for (i = 0; i < ARRAY_SIZE(bound) - 1; i++)
		seq_printf(file, "%3d -%3d | ",
			   bound[i], bound[i + 1]);
	seq_puts(file, "\nCount:  ");
	for (i = 0; i < ARRAY_SIZE(bound); i++)
		seq_printf(file, "%8d | ", dev->mt76.aggr_stats[i]);
	seq_puts(file, "\n");

	return 0;
}

static int
mt7615_ampdu_stat_open(struct inode *inode, struct file *f)
{
	return single_open(f, mt7615_ampdu_stat_read, inode->i_private);
}

static const struct file_operations fops_ampdu_stat = {
	.open = mt7615_ampdu_stat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void
mt7615_radio_read_phy(struct mt7615_phy *phy, struct seq_file *s)
{
	struct mt7615_dev *dev = dev_get_drvdata(s->private);
	bool ext_phy = phy != &dev->phy;

	if (!phy)
		return;

	seq_printf(s, "Radio %d sensitivity: ofdm=%d cck=%d\n", ext_phy,
		   phy->ofdm_sensitivity, phy->cck_sensitivity);
	seq_printf(s, "Radio %d false CCA: ofdm=%d cck=%d\n", ext_phy,
		   phy->false_cca_ofdm, phy->false_cca_cck);
}

static int
mt7615_radio_read(struct seq_file *s, void *data)
{
	struct mt7615_dev *dev = dev_get_drvdata(s->private);

	mt7615_radio_read_phy(&dev->phy, s);
	mt7615_radio_read_phy(mt7615_ext_phy(dev), s);

	return 0;
}

static int mt7615_read_temperature(struct seq_file *s, void *data)
{
	struct mt7615_dev *dev = dev_get_drvdata(s->private);
	int temp;

	if (!mt7615_wait_for_mcu_init(dev))
		return 0;

	/* cpu */
	temp = mt7615_mcu_get_temperature(dev, 0);
	seq_printf(s, "Temperature: %d\n", temp);

	return 0;
}

static int
mt7615_queues_acq(struct seq_file *s, void *data)
{
	struct mt7615_dev *dev = dev_get_drvdata(s->private);
	int i;

	for (i = 0; i < 16; i++) {
		int j, acs = i / 4, index = i % 4;
		u32 ctrl, val, qlen = 0;

		val = mt76_rr(dev, MT_PLE_AC_QEMPTY(acs, index));
		ctrl = BIT(31) | BIT(15) | (acs << 8);

		for (j = 0; j < 32; j++) {
			if (val & BIT(j))
				continue;

			mt76_wr(dev, MT_PLE_FL_Q0_CTRL,
				ctrl | (j + (index << 5)));
			qlen += mt76_get_field(dev, MT_PLE_FL_Q3_CTRL,
					       GENMASK(11, 0));
		}
		seq_printf(s, "AC%d%d: queued=%d\n", acs, index, qlen);
	}

	return 0;
}

static int
mt7615_queues_read(struct seq_file *s, void *data)
{
	struct mt7615_dev *dev = dev_get_drvdata(s->private);
	static const struct {
		char *queue;
		int id;
	} queue_map[] = {
		{ "PDMA0", MT_TXQ_BE },
		{ "MCUQ", MT_TXQ_MCU },
		{ "MCUFWQ", MT_TXQ_FWDL },
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(queue_map); i++) {
		struct mt76_sw_queue *q = &dev->mt76.q_tx[queue_map[i].id];

		if (!q->q)
			continue;

		seq_printf(s,
			   "%s:	queued=%d head=%d tail=%d\n",
			   queue_map[i].queue, q->q->queued, q->q->head,
			   q->q->tail);
	}

	return 0;
}

#define FIELD1(_name) {	\
		.name = #_name,	\
		.len = 1,	\
		.rsv = false,	\
		}

#define FIELD2(_name) {	\
		.name = #_name,	\
		.len = 2,	\
		.rsv = false	\
		}

#define FIELD4(_name) {	\
		.name = #_name,	\
		.len = 4,	\
		.rsv = false	\
		}

#define FIELDX(_name, x, reserved) { \
		.name = #_name,	\
		.len = x,	\
		.rsv = reserved,	\
		}

static int
mt7615_mib_read(struct seq_file *s, void *data)
{
	struct mt7615_dev *dev = dev_get_drvdata(s->private);
	struct sk_buff *skb;
	int ret, i;

	static const struct {
		const char *name;
		size_t len;
		bool rsv;
	} mib[] = {
		FIELD4(rx_fcs_err_cnt),
		FIELD4(rx_fifo_full_cnt),
		FIELD4(rx_mpdu_cnt),
		FIELD4(rx_ampducnt),
		FIELD4(rx_total_byte),
		FIELD4(rx_valid_ampdusf),
		FIELD4(rx_valid_byte),
		FIELD4(channel_idle_cnt),
		FIELD4(rx_vector_drop_cnt),
		FIELD4(delimiter_failed_cnt),
		FIELD4(rx_vector_mismatch_cnt),
		FIELD4(mdrdy_cnt),
		FIELD4(cckmdrdy_cnt),
		FIELD4(ofdmlgmix_mdrdy),
		FIELD4(ofdmgreen_mdrdy),
		FIELD4(pfdrop_cnt),
		FIELD4(rx_len_mismatch_cnt),
		FIELD4(pcca_time),
		FIELD4(scca_time),
		FIELD4(cca_nav_tx),
		FIELD4(pedtime),
		FIELD4(beacon_tx_cnt),
		FIELD4(ba_missed_cnt_0),
		FIELD4(ba_missed_cnt_1),
		FIELD4(ba_missed_cnt_2),
		FIELD4(ba_missed_cnt_3),
		FIELD4(rts_tx_cnt_0),
		FIELD4(rts_tx_cnt_1),
		FIELD4(rts_tx_cnt_2),
		FIELD4(rts_tx_cnt_3),
		FIELD4(frame_retry_cnt_0),
		FIELD4(frame_retry_cnt_1),
		FIELD4(frame_retry_cnt_2),
		FIELD4(frame_retry_cnt_3),
		FIELD4(frame_retry2cnt_0),
		FIELD4(frame_retry2cnt_1),
		FIELD4(frame_retry2cnt_2),
		FIELD4(frame_retry2cnt_3),
		FIELD4(rts_retry_cnt_0),
		FIELD4(rts_retry_cnt_1),
		FIELD4(rts_retry_cnt_2),
		FIELD4(rts_retry_cnt_3),
		FIELD4(ack_failed_cnt_0),
		FIELD4(ack_failed_cnt_1),
		FIELD4(ack_failed_cnt_2),
		FIELD4(ack_failed_cnt_3),
		FIELD4(tx40mhz_cnt),
		FIELD4(tx80mhz_cnt),
		FIELD4(tx160mhz_cnt),
		FIELD4(tx_sf_cnt),
		FIELD4(tx_ack_sf_cnt),
		FIELD4(tx_ampdu_cnt),
		FIELD4(tx_rsp_ba_cnt),
		FIELD2(tx_early_stop_cnt),
		FIELD2(tx_range1ampdu_cnt),
		FIELD2(tx_range2ampdu_cnt),
		FIELD2(tx_range3ampdu_cnt),
		FIELD2(tx_range4ampdu_cnt),
		FIELD2(tx_range5ampdu_cnt),
		FIELD2(tx_range6ampdu_cnt),
		FIELD2(tx_range7ampdu_cnt),
		FIELD2(tx_range8ampdu_cnt),
		FIELD2(tx_range9ampdu_cnt)
	};

	ret = mt7615_mcu_get_mib_info(&dev->phy, &skb);
	if (ret)
		goto out;

	for (i = 0; i < ARRAY_SIZE(mib); i++) {
		const char *name = mib[i].name;
		size_t len = mib[i].len;

		if (skb->len < len) {
			ret = -EINVAL;
			goto out;
		}

		if (len == 4) {
			__le32 *tmp = (__le32 *)skb->data;

			seq_printf(s, "%s: %u\n", name, le32_to_cpu(*tmp));
		} else if (len == 2) {
			__le16 *tmp = (__le16 *)skb->data;

			seq_printf(s, "%s: %u\n", name, le16_to_cpu(*tmp));
		}

		skb_pull(skb, len);
	}

	if (skb->len)
		dev_err(dev->mt76.dev, "MIB event length mismatched\n");

	dev_kfree_skb(skb);

out:
	return ret;
}

int mt7615_init_debugfs(struct mt7615_dev *dev)
{
	struct dentry *dir;

	dir = mt76_register_debugfs(&dev->mt76);
	if (!dir)
		return -ENOMEM;

	if (is_mt7615(&dev->mt76))
		debugfs_create_devm_seqfile(dev->mt76.dev, "queues", dir,
					    mt7615_queues_read);
	else
		debugfs_create_devm_seqfile(dev->mt76.dev, "queues", dir,
					    mt76_queues_read);
	debugfs_create_devm_seqfile(dev->mt76.dev, "acq", dir,
				    mt7615_queues_acq);
	debugfs_create_devm_seqfile(dev->mt76.dev, "mib_info", dir,
				    mt7615_mib_read);
	debugfs_create_file("ampdu_stat", 0400, dir, dev, &fops_ampdu_stat);
	debugfs_create_file("scs", 0600, dir, dev, &fops_scs);
	debugfs_create_file("dbdc", 0600, dir, dev, &fops_dbdc);
	debugfs_create_file("fw_debug", 0600, dir, dev, &fops_fw_debug);
	debugfs_create_devm_seqfile(dev->mt76.dev, "radio", dir,
				    mt7615_radio_read);
	debugfs_create_u32("dfs_hw_pattern", 0400, dir, &dev->hw_pattern);
	/* test pattern knobs */
	debugfs_create_u8("pattern_len", 0600, dir,
			  &dev->radar_pattern.n_pulses);
	debugfs_create_u32("pulse_period", 0600, dir,
			   &dev->radar_pattern.period);
	debugfs_create_u16("pulse_width", 0600, dir,
			   &dev->radar_pattern.width);
	debugfs_create_u16("pulse_power", 0600, dir,
			   &dev->radar_pattern.power);
	debugfs_create_file("radar_trigger", 0200, dir, dev,
			    &fops_radar_pattern);
	debugfs_create_file("reset_test", 0200, dir, dev,
			    &fops_reset_test);
	debugfs_create_devm_seqfile(dev->mt76.dev, "temperature", dir,
				    mt7615_read_temperature);

	return 0;
}
EXPORT_SYMBOL_GPL(mt7615_init_debugfs);
