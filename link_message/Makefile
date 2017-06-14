###########################################################################
#
# MODULE:             IOTC - Makefile
#
# REVISION:           $Revision: 1.0 $
#
# DATED:              $Date: 2015-10-28 11:16:28 +0000 $
#
# AUTHOR:             PCT
#
###########################################################################
#
# Copyright panchangtao@gmail.com B.V. 2015. All rights reserved
#
###########################################################################
TARGET := $(shell ls *.c | sed 's/.c//g')

SRC_NAME := .
SRC_DIR := $(SRC_NAME)
INC_DIR := $(SRC_NAME)


SOURCE := $(wildcard $(SRC_DIR)/*.c)
INCLUD := $(wildcard $(INC_DIR)/*.h)

CFLAGS := -I./include/
CFLAGS += -I$(SRC_NAME)/include/
PROJ_DFLAGS := -D_REENTRANT
PROJ_LIBS := -L./lib
PROJ_LIBS += 

CC := gcc
RM := -rm
CFLAGS += -g -Wno-unused-but-set-variable -Wall
#CFLAGS += -O2 -g -Wcomment -Wformat -Wimplicit -Wreturn-type



OBJS := $(patsubst %.c,%.o,$(SOURCE))
INCLUDE := $(patsubst %.c,%.h,$(INCLUD))

vpath %.c $(SRC_DIR)

.PHONY: all clean distclean
all: $(TARGET)
	
include $($(SOURCE):.c=.d)
	
$(TARGET):$(OBJS)
	@$(CC) $(PROJ_DFLAGS) $^ $(PROJ_CFLAGS) $(CFLAGS) $(PROJ_LIBS) -o $@ 
	
%.o:%.c $(INCLUDE) 
	$(CC)  -I. $(CFLAGS) $(PROJ_CFLAGS) -c $< -o $@

	
test:
	@echo $(SOURCE)
	@echo $(CC)
	@echo $(OBJS)
	
clean:
	$(RM) $(TARGET) $(OBJS)

distclean:
	$(RM) $(TARGET) $(OBJS)
 


