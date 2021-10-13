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
	rm -rf iproute2
	mkdir iproute2
	( cd $(ONL)/sm/iproute2 \
	  && find . ! -name '.git*' -print \
	     | cpio -o ) \
	| ( cd iproute2 && cpio -imd )
	$(MAKE) patch
	echo "pkg-config path: $(PKG_CONFIG_DIRS)"
	echo "configure options: $(CONFIGURE_OPTIONS)"
	echo "toolchain dir: $(TOOLCHAIN_DIR)"
	echo "GCC: $(GCC)"
	cd iproute2 && export PKG_CONFIG_PATH=$(PKG_CONFIG_DIRS) && ./configure --host $(TOOLCHAIN)
	$(MAKE) V=1 PREFIX=/usr/local CC=$(GCC) -C iproute2

SERIES		:= \
  0004-sync-iptables-header.patch \
  0100-tc-flower-icmp-vlan.patch \
  0101-devlink-add.patch \
  # THIS LINE INTENTIONALLY LEFT BLANK

patch:
	cd iproute2 ;\
	for patch in $(SERIES); do \
	  echo "Applying patch $$patch" ;\
	  patch -p1 -i $(ONL)/packages/base/any/iproute2/builds/$$patch ;\
	done

BUILD_DEPS	:= \
  libmnl-dev \
  libelf-dev \
  libcap-dev \
  libxtables-dev \
  libdb-dev \
  # THIS LINE INTENTIONALLY LEFT BLANK

deps-arm64:
	sudo xapt -a arm64 $(BUILD_DEPS)

deps-amd64:
	sudo apt-get -y install $(BUILD_DEPS)

deps: deps-$(ARCH)
