/* SPDX-License-Identifier: ISC */
/* Copyright (C) 2020-2021 MediaTek Inc.
 *
 * Author: Sean Wang <sean.wang@mediatek.com>
 */

#ifndef __MT76_CONNAC_SDIO_H
#define __MT76_CONNAC_SDIO_H

#define MCR_WCIR			0x0000
#define MCR_WHLPCR			0x0004
#define WHLPCR_FW_OWN_REQ_CLR		BIT(9)
#define WHLPCR_FW_OWN_REQ_SET		BIT(8)
#define WHLPCR_IS_DRIVER_OWN		BIT(8)
#define WHLPCR_INT_EN_CLR		BIT(1)
#define WHLPCR_INT_EN_SET		BIT(0)

#define MCR_WSDIOCSR			0x0008
#define MCR_WHCR			0x000C
#define W_INT_CLR_CTRL			BIT(1)
#define RECV_MAILBOX_RD_CLR_EN		BIT(2)
#define WF_WHOLE_PATH_RSTB		BIT(5) /* supported in V2 */
#define WF_SDIO_WF_PATH_RSTB		BIT(6) /* supported in V2 */
#define MAX_HIF_RX_LEN_NUM		GENMASK(13, 8)
#define MAX_HIF_RX_LEN_NUM_V2		GENMASK(14, 8) /* supported in V2 */
#define WF_RST_DONE			BIT(15) /* supported in V2 */
#define RX_ENHANCE_MODE			BIT(16)

#define MCR_WHISR			0x0010
#define MCR_WHIER			0x0014
#define WHIER_D2H_SW_INT		GENMASK(31, 8)
#define WHIER_FW_OWN_BACK_INT_EN	BIT(7)
#define WHIER_ABNORMAL_INT_EN		BIT(6)
#define WHIER_WDT_INT_EN		BIT(5) /* supported in V2 */
#define WHIER_RX1_DONE_INT_EN		BIT(2)
#define WHIER_RX0_DONE_INT_EN		BIT(1)
#define WHIER_TX_DONE_INT_EN		BIT(0)
#define WHIER_DEFAULT			(WHIER_RX0_DONE_INT_EN	| \
					 WHIER_RX1_DONE_INT_EN	| \
					 WHIER_TX_DONE_INT_EN	| \
					 WHIER_ABNORMAL_INT_EN	| \
					 WHIER_D2H_SW_INT)

#define MCR_WASR			0x0020
#define MCR_WSICR			0x0024
#define WSICR_H2D_SW_INT		GENMASK(31, 16)

#define MCR_WTSR0			0x0028
#define TQ0_CNT				GENMASK(7, 0)
#define TQ1_CNT				GENMASK(15, 8)
#define TQ2_CNT				GENMASK(23, 16)
#define TQ3_CNT				GENMASK(31, 24)

#define MCR_WTSR1			0x002c
#define TQ4_CNT				GENMASK(7, 0)
#define TQ5_CNT				GENMASK(15, 8)
#define TQ6_CNT				GENMASK(23, 16)
#define TQ7_CNT				GENMASK(31, 24)

#define MCR_WTDR1			0x0034
#define MCR_WRDR0			0x0050
#define MCR_WRDR1			0x0054
#define MCR_WRDR(p)			(0x0050 + 4 * (p))
#define MCR_H2DSM0R			0x0070
#define H2D_SW_INT_READ			BIT(16)
#define H2D_SW_INT_WRITE		BIT(17)

#define MCR_H2DSM1R			0x0074
#define MCR_D2HRM0R			0x0078
#define MCR_D2HRM1R			0x007c
#define MCR_D2HRM2R			0x0080
#define MCR_WRPLR			0x0090
#define RX0_PACKET_LENGTH		GENMASK(15, 0)
#define RX1_PACKET_LENGTH		GENMASK(31, 16)

#define MCR_WTMDR			0x00b0
#define MCR_WTMCR			0x00b4
#define MCR_WTMDPCR0			0x00b8
#define MCR_WTMDPCR1			0x00bc
#define MCR_WPLRCR			0x00d4
#define MCR_WSR				0x00D8
#define MCR_CLKIOCR			0x0100
#define MCR_CMDIOCR			0x0104
#define MCR_DAT0IOCR			0x0108
#define MCR_DAT1IOCR			0x010C
#define MCR_DAT2IOCR			0x0110
#define MCR_DAT3IOCR			0x0114
#define MCR_CLKDLYCR			0x0118
#define MCR_CMDDLYCR			0x011C
#define MCR_ODATDLYCR			0x0120
#define MCR_IDATDLYCR1			0x0124
#define MCR_IDATDLYCR2			0x0128
#define MCR_ILCHCR			0x012C
#define MCR_WTQCR0			0x0130
#define MCR_WTQCR1			0x0134
#define MCR_WTQCR2			0x0138
#define MCR_WTQCR3			0x013C
#define MCR_WTQCR4			0x0140
#define MCR_WTQCR5			0x0144
#define MCR_WTQCR6			0x0148
#define MCR_WTQCR7			0x014C
#define MCR_WTQCR(x)                   (0x130 + 4 * (x))
#define TXQ_CNT_L			GENMASK(15, 0)
#define TXQ_CNT_H			GENMASK(31, 16)

#define MCR_SWPCDBGR			0x0154

#define MCR_H2DSM2R			0x0160 /* supported in V2 */
#define MCR_H2DSM3R			0x0164 /* supported in V2 */
#define MCR_D2HRM3R			0x0174 /* supported in V2 */
#define MCR_WTQCR8			0x0190 /* supported in V2 */
#define MCR_WTQCR9			0x0194 /* supported in V2 */
#define MCR_WTQCR10			0x0198 /* supported in V2 */
#define MCR_WTQCR11			0x019C /* supported in V2 */
#define MCR_WTQCR12			0x01A0 /* supported in V2 */
#define MCR_WTQCR13			0x01A4 /* supported in V2 */
#define MCR_WTQCR14			0x01A8 /* supported in V2 */
#define MCR_WTQCR15			0x01AC /* supported in V2 */

enum mt76_connac_sdio_ver {
	MT76_CONNAC_SDIO_VER1,
	MT76_CONNAC_SDIO_VER2,
};

struct mt76_connac_sdio_intr_v2 {
	u32 isr;
	struct {
		u32 wtqcr[16];
	} tx;
	struct {
		u16 num[2];
		u16 len0[16];
		u16 len1[128];
	} rx;
	u32 rec_mb[2];
} __packed;


struct mt76_connac_sdio_intr_v1 {
	u32 isr;
	struct {
		u32 wtqcr[8];
	} tx;
	struct {
		u16 num[2];
		u16 len[2][16];
	} rx;
	u32 rec_mb[2];
} __packed;

struct mt76s_intr {
	u32 isr;
	struct {
		u32 *wtqcr;
	} tx;
	struct {
		u16 num[2];
		u16 *len[2];
	} rx;
	u32 rec_mb[2];
};

u32 mt76_connac_sdio_read_pcr(struct mt76_dev *dev);
u32 mt76_connac_sdio_read_mailbox(struct mt76_dev *dev, u32 offset);
void mt76_connac_sdio_write_mailbox(struct mt76_dev *dev, u32 offset, u32 val);
u32 mt76_connac_sdio_rr(struct mt76_dev *dev, u32 offset);
void mt76_connac_sdio_wr(struct mt76_dev *dev, u32 offset, u32 val);
u32 mt76_connac_sdio_rmw(struct mt76_dev *dev, u32 offset, u32 mask, u32 val);
void mt76_connac_sdio_write_copy(struct mt76_dev *dev, u32 offset,
				 const void *data, int len);
void mt76_connac_sdio_read_copy(struct mt76_dev *dev, u32 offset,
				void *data, int len);
int mt76_connac_sdio_wr_rp(struct mt76_dev *dev, u32 base,
			   const struct mt76_reg_pair *data,
			   int len);
int mt76_connac_sdio_rd_rp(struct mt76_dev *dev, u32 base,
			   struct mt76_reg_pair *data,
			   int len);

void mt76_connac_sdio_txrx(struct mt76_dev *dev);
int mt76_connac_sdio_hw_init(struct mt76_dev *dev, struct sdio_func *func,
			     int hw_ver, sdio_irq_handler_t *irq_handler);
int mt76_connac_sdio_init(struct mt76_dev *dev,
			  void (*txrx_worker)(struct mt76_worker *));
void mt76_connac_sdio_enable_irq(struct mt76_dev *dev);
void mt76_connac_sdio_disable_irq(struct mt76_dev *dev);
#endif
