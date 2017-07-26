#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

TARGET := button_application
PKG_NAME:=button_application
PKG_RELEASE:=1.0

PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)


define Package/$(PKG_NAME)
  SECTION:=topband
  CATEGORY:=topband
  TITLE:=button_application
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(TARGET) $(1)/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
