############################################################
#
#
#
############################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
arm64_accton_as4224_INCLUDES := -I $(THIS_DIR)inc
arm64_accton_as4224_INTERNAL_INCLUDES := -I $(THIS_DIR)src
arm64_accton_as4224_DEPENDMODULE_ENTRIES := init:arm64_accton_as4224 ucli:arm64_accton_as4224