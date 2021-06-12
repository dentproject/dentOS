############################################################
#
#
#
############################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
arm64_accton_as4564_26p_INCLUDES := -I $(THIS_DIR)inc
arm64_accton_as4564_26p_INTERNAL_INCLUDES := -I $(THIS_DIR)src
arm64_accton_as4564_26p_DEPENDMODULE_ENTRIES := init:arm64_accton_as4564_26p ucli:arm64_accton_as4564_26p