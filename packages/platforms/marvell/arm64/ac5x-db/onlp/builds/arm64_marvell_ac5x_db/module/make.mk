############################################################
#
#
#
############################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
arm64_marvell_ac5x_db_INCLUDES := -I $(THIS_DIR)inc
arm64_marvell_ac5x_db_INTERNAL_INCLUDES := -I $(THIS_DIR)src
arm64_marvell_ac5x_db_DEPENDMODULE_ENTRIES := init:arm64_marvell_ac5x_db ucli:arm64_marvell_ac5x_db
