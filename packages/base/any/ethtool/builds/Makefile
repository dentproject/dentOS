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
	cd ethtool && ./autogen.sh && ./configure --host $(TOOLCHAIN)
	$(MAKE) V=1 PREFIX=/usr/local CC=$(GCC) -C ethtool

SERIES		:= \
  # THIS LINE INTENTIONALLY LEFT BLANK

patch:
	cd ethtool ;\
	for patch in $(SERIES); do \
	  echo "Applying patch $$patch" ;\
	  patch -p1 -i $(ONL)/packages/base/any/ethtool/builds/$$patch ;\
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
