############################################################
#
# Default 5.10.4 configuration for arm64 platforms.
#
############################################################
THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(ONL)/make/config.mk

ifndef K_TARGET_DIR
K_TARGET_DIR := $(THIS_DIR)
endif

MODSYNCLIST_EXTRA = arch/arm64/include arch/arm64/Makefile arch/arm64/lib
K_PATCH_SERIES=series.arm64

include ../../kconfig.mk
K_CONFIG := arm64-all.config
K_BUILD_TARGET := Image Image.gz freescale/fsl-ls1043a-rdb.dtb  freescale/fsl-ls1046a-rdb-sdk.dtb freescale/fsl-ls2080a-rdb.dtb freescale/fsl-ls2088a-rdb.dtb freescale/fsl-ls1088a-rdb.dtb marvell/delta-tx4810.dtb marvell/delta-tn48m.dtb marvell/delta-tn4810m.dtb marvell/delta-tn48m-dn.dtb marvell/delta-tn4810m-dn.dtb marvell/wnc-qsd61-aom-a-48.dtb marvell/wnc-qsa72-aom-a-48p.dtb marvell/accton-as4224.dtb marvell/accton-as5114.dtb marvell/accton-as4564-26p.dtb marvell/marvell-ac5x-db-slim.dtb
K_COPY_SRC := arch/arm64/boot/Image
K_COPY_GZIP := 1
ifndef K_COPY_DST
K_COPY_DST := kernel-5.10-lts-arm64-all.bin.gz
endif

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
DTS_LIST := fsl

include $(ONL)/make/kbuild.mk
