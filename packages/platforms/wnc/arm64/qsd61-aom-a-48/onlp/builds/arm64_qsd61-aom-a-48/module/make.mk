###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
arm64_wnc_qsd61_aom_a_48_INCLUDES := -I $(THIS_DIR)inc
arm64_wnc_qsd61_aom_a_48_INTERNAL_INCLUDES := -I $(THIS_DIR)src
arm64_wnc_qsd61_aom_a_48_DEPENDMODULE_ENTRIES := init:arm64_wnc_qsd61_aom_a_48 ucli:arm64_wnc_qsd61_aom_a_48

