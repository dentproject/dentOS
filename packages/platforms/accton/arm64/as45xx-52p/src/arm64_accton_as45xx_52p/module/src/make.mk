############################################################
#
#
#
############################################################

LIBRARY := arm64_accton_as45xx_52p
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
#$(LIBRARY)_LAST := 1
include $(BUILDER)/lib.mk
