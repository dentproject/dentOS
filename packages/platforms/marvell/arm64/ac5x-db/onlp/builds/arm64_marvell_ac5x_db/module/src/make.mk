############################################################
#
#
#
############################################################

LIBRARY := arm64_marvell_ac5x_db
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
#$(LIBRARY)_LAST := 1
include $(BUILDER)/lib.mk
