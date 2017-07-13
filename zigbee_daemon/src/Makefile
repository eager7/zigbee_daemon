###########################################################################
#
# MODULE:             IOTC-zigbee - Makefile
#
# REVISION:           $Revision: 1.0 $
#
# DATED:              $Date: 2016-12-06 11:16:28 +0000 $
#
# AUTHOR:             PCT
#
###########################################################################
#
# Copyright panchangtao@gmail.com 2016. All rights reserved
#
###########################################################################
TARGET := zigbee_daemon

SRC_ROOT := $(shell pwd)

SOURCE := $(wildcard $(SRC_ROOT)/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/zigbee/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/comm/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/control-bridge/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/utils/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/devices/*.c)

CFLAGS += -I./zigbee
CFLAGS += -I./comm
CFLAGS += -I./control-bridge
CFLAGS += -I./utils
CFLAGS += -I./devices

PROJ_DFLAGS := -D_REENTRANT
PROJ_LIBS ?= -L./lib
PROJ_LIBS += -lpthread -ljson-c -lsqlite3 -lzmq

CC ?= gcc
RM ?= -rm
CFLAGS += -ggdb -Wno-unused-but-set-variable -Wall -O0 

OBJS := $(patsubst %.c,%.o,$(SOURCE))

vpath %.c $(SRC_DIR)

.PHONY: all clean distclean
all: $(TARGET)
	
$(TARGET):$(OBJS)
	@$(CC) $(PROJ_DFLAGS) $^ $(PROJ_CFLAGS) $(CFLAGS) $(PROJ_LIBS) $(LDFLAGS) -o $@ 
	
%.o:%.c
	$(CC) $(PROJ_DFLAGS) -I. $(CFLAGS) -c $< -o $@

%d:%c
	@set -e;$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's/.o:/.o $@:/g' < $@.$$$$ > $@; \
	rm $@.$$$$
-include $(SOURCE:.c=.d)

test:
	@echo $(SRC_ROOT)
	@echo $(SOURCE)
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS) *.d* 
 
install_depends:
	sudo install libsqlite3-dev libjson-c-dev libzeromq-dev libczmq-dev

install:
	install -m 0555 -s -D $(TARGET) /usr/local/bin/
	mkdir -p /etc/zigbee_daemon
	cp zigbee_daemon.sh /etc/init.d/
	chmod +x /etc/init.d/zigbee_daemon.sh
	cd /etc/init.d/;sudo update-rc.d zigbee_daemon.sh defaults 95

uninstall:
	$(RM) /usr/local/bin/$(TARGET)
	cd /etc/init.d/;sudo update-rc.d -f zigbee_daemon remove
	$(RM) /etc/init.d/zigbee_daemon.sh
	rm -rf /etc/zigbee_daemon/

