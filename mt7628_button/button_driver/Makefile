include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=$(shell pwd | sed 's/\/.*.\///g')
PKG_CONFIG:=$(shell echo $(PKG_NAME) | tr '[a-z]' '[A-Z]')
PKG_RELEASE:=0.1

#99为最后装载
define KernelPackage/$(PKG_NAME)
	CATEGORY:=Topband
	TITLE:=$(PKG_NAME)
	AUTOLOAD:=$(call AutoLoad,99,$(PKG_NAME))
	FILES:=$(PKG_BUILD_DIR)/$(PKG_NAME).$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/$(PKG_NAME)/description
	A Simple Kernel Module of Hello
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -R ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	modules
endef

define KernelPackage/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc
	echo $(PKG_RELEASE) > $(1)/etc/$(PKG_NAME).ver
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
