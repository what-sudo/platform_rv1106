CROSS_COMPILE := arm-rockchip830-linux-uclibcgnueabihf-

CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)g++
AR		= $(CROSS_COMPILE)ar
RANLIB	= $(CROSS_COMPILE)ranlib

LD		= $(CROSS_COMPILE)ld
AS		= $(CROSS_COMPILE)as
NM		= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump

DIRS := $(shell find ./ -maxdepth 5 -type d )
DIRS += $(shell find ../common -maxdepth 5 -type d )

INCLUDE_DIRS := $(DIRS)
INCLUDE_DIRS += $(shell find ../rv1106_sdk/include -maxdepth 2 -type d )

INCLUDE	:= $(patsubst %, -I %, $(INCLUDE_DIRS))

SOURCE := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))

OBJS 	:= $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

CFLAGS  := -Wall -Wfatal-errors -Os -Wl,--gc-sections -fno-common $(INCLUDE)
CXXFLAGS:= $(CFLAGS)

LIBS += -L ../rv1106_sdk/lib
LIBS += -lrockit_full -lrockchip_mpp -ldrm -lrga -lrkaiq

# $(info $(DIRS))
# $(info $(SOURCE))
# $(info $(OBJS))

TARGET := $(notdir $(shell pwd))

.PHONY : everything objs clean distclean rebuild

everything : $(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) /tftp

all : $(TARGET)

objs : $(OBJS)

rebuild: distclean everything

clean :
	@rm -rf *.*~
	@rm -rf *~
	@rm -rf *.so
	@rm -rf *.o
	@rm -rf $(OBJS)

distclean : clean
	@rm -rf $(TARGET)
	@rm -rf /tftp/$(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@  $(OBJS) $(LDFLAGS) $(LIBS)
