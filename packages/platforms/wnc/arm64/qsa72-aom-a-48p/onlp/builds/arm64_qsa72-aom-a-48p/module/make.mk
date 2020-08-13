###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
arm64_wnc_qsa72_aom_a_48p_INCLUDES := -I $(THIS_DIR)inc
arm64_wnc_qsa72_aom_a_48p_INTERNAL_INCLUDES := -I $(THIS_DIR)src
arm64_wnc_qsa72_aom_a_48p_DEPENDMODULE_ENTRIES := init:arm64_wnc_qsa72_aom_a_48p ucli:arm64_wnc_qsa72_aom_a_48p

