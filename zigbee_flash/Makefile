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
TARGET := zigbee-flash

VERSION_MAJOR := 1
VERSION_MINOR := 0

SRC_DIR := .
SOURCE := $(wildcard $(SRC_DIR)/*.c)

CFLAGS += -I./include/
PROJ_DFLAGS := -D_REENTRANT -DPOSIX -DVERSION_MAJOR="\"0\"" -DVERSION_MINOR="\"11\""
PROJ_LIBS ?= -L./lib
PROJ_LIBS +=

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

clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS) *.d* 
 
install_depends:
	sudo install libsqlite3-dev libjson-c-dev libzeromq-dev libczmq-dev

install:
	install -m 0555 -s -D $(TARGET) /usr/local/bin/

uninstall:
	rm -rf /etc/zigbee_daemon/

