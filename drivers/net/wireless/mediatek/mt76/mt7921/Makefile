# SPDX-License-Identifier: ISC

obj-$(CONFIG_MT7921_COMMON) += mt7921-common.o
obj-$(CONFIG_MT7921E) += mt7921e.o
obj-$(CONFIG_MT7921S) += mt7921s.o

CFLAGS_trace.o := -I$(src)

mt7921-common-y := main.o mac.o mcu.o eeprom.o init.o debugfs.o trace.o

mt7921e-y := pci.o pci_mac.o pci_mcu.o pci_init.o dma.o
mt7921s-y := sdio.o sdio_mac.o sdio_mcu.o sdio_init.o sdio_mcu.o
