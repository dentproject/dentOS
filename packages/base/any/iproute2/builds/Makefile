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
PKG_CONFIG_DIRS := /usr/$(TOOLCHAIN)/lib/pkgconfig:/usr/lib/$(TOOLCHAIN)/pkgconfig
#
# All configure options
#
CONFIGURE_OPTIONS := $(CONFIGURE_TOOLCHAIN_OPTIONS) $(CONFIGURE_COMMON_OPTIONS) $(CONFIGURE_$(ARCH)_OPTIONS) $(CONFIGURE_EXTRA_OPTIONS)

build: deps
	rm -rf iproute2
	mkdir iproute2
	( cd $(ONL)/sm/iproute2 \
	  && find . ! -name '.git*' -print \
	     | cpio -o ) \
	| ( cd iproute2 && cpio -imd )
	$(MAKE) patch
	echo "configure options: $(CONFIGURE_OPTIONS)"
	echo "toolchain dir: $(TOOLCHAIN_DIR)"
	echo "GCC: $(GCC)"
	cd iproute2 && CC=$(GCC) LD=$(TOOLCHAIN)-ld PKG_CONFIG_PATH=$(PKG_CONFIG_DIRS) ./configure --prefix=/usr/local
	$(MAKE) V=1 HOSTCC=gcc -C iproute2

SERIES		:= \
  0100-tc-flower-icmp-vlan.patch \
  # THIS LINE INTENTIONALLY LEFT BLANK

patch:
	cd iproute2 ;\
	for patch in $(SERIES); do \
	  echo "Applying patch $$patch" ;\
	  patch -p1 -i $(ONL)/packages/base/any/iproute2/builds/$$patch ;\
	done

BUILD_DEPS	:= \
  # THIS LINE INTENTIONALLY LEFT BLANK

deps-arm64:

deps-amd64:

deps: deps-$(ARCH)
