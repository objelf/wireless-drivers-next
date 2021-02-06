// SPDX-License-Identifier: ISC

#include "mt7921.h"
#include "mcu.h"

enum {
	TM_SWITCH_MODE,
	TM_SET_AT_CMD,
	TM_QUERY_AT_CMD,
};

enum {
	MT7921_NORMAL,
	MT7921_TESTMODE,
};

enum mt7921_testmode_attr {
	MT7921_TM_ATTR_UNSPEC,
	MT7921_TM_ATTR_SET,
	MT7921_TM_ATTR_QUERY,
	MT7921_TM_ATTR_RSP,
	MT7921_TM_ATTR_HQA_OPEN,
	MT7921_TM_ATTR_HQA_CLOSE,

	/* keep last */
	NUM_MT7921_TM_ATTRS,
	MT7921_TM_ATTR_MAX = NUM_MT7921_TM_ATTRS - 1,
};

static const struct nla_policy mt7921_tm_policy[NUM_MT7921_TM_ATTRS] = {
	[MT7921_TM_ATTR_SET] = NLA_POLICY_EXACT_LEN(sizeof(struct mt7921_tm_cmd)),
	[MT7921_TM_ATTR_QUERY] = NLA_POLICY_EXACT_LEN(sizeof(struct mt7921_tm_cmd)),
};

static int
mt7921_tm_set(struct mt7921_dev *dev, struct mt7921_tm_cmd *req)
{
	struct mt76_connac_pm *pm = &dev->pm;
	struct mt7921_rftest_cmd cmd = {
		.action = req->action,
		.data0 = cpu_to_le32(req->data0),
		.data1 = cpu_to_le32(req->data1),
	};
	int ret;

	mutex_lock(&dev->mt76.mutex);

	if (req->action == TM_SWITCH_MODE &&
	    req->data0 == MT7921_TESTMODE) {
		/* Testmode starts full power mode */
		mt76_connac_pm_wake(&dev->mphy, pm);
		pm->enable = false;
	}

	ret = mt76_mcu_send_msg(&dev->mt76, MCU_CMD_TEST_CTRL, &cmd,
				sizeof(cmd), false);

	mutex_unlock(&dev->mt76.mutex);

	return ret;
}

static int
mt7921_tm_query(struct mt7921_dev *dev, struct mt7921_tm_cmd *req,
		struct mt7921_tm_evt *resp)
{
	struct mt7921_rftest_cmd cmd = {
		.action = req->action,
		.data0 = cpu_to_le32(req->data0),
		.data1 = cpu_to_le32(req->data1),
	};
	struct mt7921_rftest_evt *evt;
	struct sk_buff *skb;
	int ret;

	mutex_lock(&dev->mt76.mutex);

	ret = mt76_mcu_send_and_get_msg(&dev->mt76, MCU_CMD_TEST_CTRL,
					&cmd, sizeof(cmd),
					true, &skb);
	mutex_unlock(&dev->mt76.mutex);

	if (ret)
		goto out;

	evt = (struct mt7921_rftest_evt *)(skb->data);
	resp->data0 = le32_to_cpu(evt->data0);
	resp->data1 = le32_to_cpu(evt->data1);
out:
	dev_kfree_skb(skb);

	return ret;
}


int mt7921_testmode_cmd(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			void *data, int len)
{
	struct mt76_phy *mphy = hw->priv;
	struct mt7921_phy *phy = mphy->priv;
	struct mt7921_dev *dev = phy->dev;
	struct nlattr *tb[NUM_MT7921_TM_ATTRS];
	int err;

	err = nla_parse_deprecated(tb, MT7921_TM_ATTR_MAX, data, len,
				   mt7921_tm_policy, NULL);
	if (err)
		return err;

	if (tb[MT7921_TM_ATTR_SET])
		err = mt7921_tm_set(dev, nla_data(tb[MT7921_TM_ATTR_SET]));

	return err;
}

int mt7921_testmode_dump(struct ieee80211_hw *hw, struct sk_buff *msg,
		       struct netlink_callback *cb, void *data, int len)
{
	struct mt76_phy *mphy = hw->priv;
	struct mt7921_phy *phy = mphy->priv;
	struct mt7921_dev *dev = phy->dev;
	struct nlattr *tb[NUM_MT7921_TM_ATTRS];
	struct mt7921_tm_evt resp;
	int err;

	if (cb->args[2]++ > 0)
		return -ENOENT;

	err = nla_parse_deprecated(tb, MT7921_TM_ATTR_MAX, data, len,
				   mt7921_tm_policy, NULL);
	if (err)
		return err;

	err = -EINVAL;
	if (tb[MT7921_TM_ATTR_QUERY]) {
		err = mt7921_tm_query(dev, nla_data(tb[MT7921_TM_ATTR_QUERY]), &resp);
		if (err)
			goto out;

		err = nla_put(msg, MT7921_TM_ATTR_RSP, sizeof(resp), &resp);
	}
out:
	return err;
}
