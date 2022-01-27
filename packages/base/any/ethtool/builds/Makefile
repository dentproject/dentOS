include $(ONL)/make/config.mk
include $(BUILDER)/builder.mk
include $(TOOLCHAIN_DIR)/obj.mk

#
# These are the common configure options:
#
# Cross compile options
#
ifndef TOOLCHAIN
$(error $$TOOLCHAIN not set)
endif

CONFIGURE_TOOLCHAIN_OPTIONS := --host $(TOOLCHAIN)
PKG_CONFIG_DIRS := /usr/aarch64-linux-gnu/lib/pkgconfig:/usr/lib/aarch64-linux-gnu/pkgconfig
#
# All configure options
#
CONFIGURE_OPTIONS := $(CONFIGURE_TOOLCHAIN_OPTIONS) $(CONFIGURE_COMMON_OPTIONS) $(CONFIGURE_$(ARCH)_OPTIONS) $(CONFIGURE_EXTRA_OPTIONS)

build: deps
	rm -rf ethtool
	mkdir ethtool
	( cd $(ONL)/sm/ethtool \
	  && find . ! -name '.git*' -print \
	     | cpio -o ) \
	| ( cd ethtool && cpio -imd )
	$(MAKE) patch
	echo "configure options: $(CONFIGURE_OPTIONS)"
	echo "toolchain dir: $(TOOLCHAIN_DIR)"
	echo "GCC: $(GCC)"
	cd ethtool && ./autogen.sh && export PKG_CONFIG_PATH=$(PKG_CONFIG_DIRS) && ./configure --host $(TOOLCHAIN)
	$(MAKE) V=1 PREFIX=/usr/local CC=$(GCC) -C ethtool

SERIES		:= \
  # THIS LINE INTENTIONALLY LEFT BLANK

patch:
	cd ethtool ;\
	for patch in $(SERIES); do \
	  echo "Applying patch $$patch" ;\
	  patch -p1 -i $(ONL)/packages/base/any/ethtool/builds/$$patch ;\
	done


deps-arm64:

deps-amd64:

deps: deps-$(ARCH)
