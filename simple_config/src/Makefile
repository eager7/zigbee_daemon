###########################################################################
#
# MODULE:             IOTC-wifi config - Makefile
#
# REVISION:           $Revision: 1.0 $
#
# DATED:              $Date: 2017-08-24 11:16:28 +0000 $
#
# AUTHOR:             PCT
#
###########################################################################
#
# Copyright panchangtao@gmail.com 2017. All rights reserved
#
###########################################################################
TARGET := simple_config

SRC_ROOT := $(shell pwd)

SOURCE := $(wildcard $(SRC_ROOT)/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/utils/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/wifi/*.c)
SOURCE += $(wildcard $(SRC_ROOT)/comm/*.c)

CFLAGS += -I./utils
CFLAGS += -I./wifi
CFLAGS += -I./comm

PROJ_DFLAGS := -D_REENTRANT
PROJ_LIBS ?= -L./lib
PROJ_LIBS += -lpthread -ljson-c

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

#%d:%c
#	@set -e;$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
#	sed 's/.o:/.o $@:/g' < $@.$$$$ > $@; \
#	rm $@.$$$$
#-include $(SOURCE:.c=.d)

test:
	@echo $(SOURCE)
	
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS) *.d* $(SRC_ROOT)/utils/*.d* $(SRC_ROOT)/wifi/*.d* $(SRC_ROOT)/comm/*.d*
 
