#
# Copyright (C) 2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

TARGET := zigbee_daemon
PKG_NAME:=zigbee_daemon
PKG_RELEASE:=1.0

PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)


define Package/$(PKG_NAME)
  SECTION:=topband
  CATEGORY:=Topband
  TITLE:=zigbee_daemon -- new zigbee daemon for smart door lock
  DEPENDS:=+libjson-c +libpthread +libsqlite3
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(TARGET) $(1)/bin/
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/zigbee_daemon $(1)/etc/init.d
	$(INSTALL_BIN) ./files/button_mknod.sh $(1)/etc/init.d/
endef

define Package/$(PKG_NAME)/postinst
#!/bin/sh
# check if we are on real system
echo "Enabling rc.d symlink for zigbee-daemon"
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/zigbee_daemon enable || true
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
