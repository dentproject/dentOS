############################################################
#
#
#
############################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
arm64_accton_as45xx_52p_INCLUDES := -I $(THIS_DIR)inc
arm64_accton_as45xx_52p_INTERNAL_INCLUDES := -I $(THIS_DIR)src
arm64_accton_as45xx_52p_DEPENDMODULE_ENTRIES := init:arm64_accton_as45xx_52p ucli:arm64_accton_as45xx_52p
