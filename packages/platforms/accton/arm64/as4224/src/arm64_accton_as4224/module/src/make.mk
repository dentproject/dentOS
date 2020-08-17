############################################################
#
#
#
############################################################

LIBRARY := arm64_accton_as4224
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
#$(LIBRARY)_LAST := 1
include $(BUILDER)/lib.mk