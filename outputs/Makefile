TARGET := lrmame2003osvr

# Toolchain
CC := g++
ARM := 1
LD = ${CC}

# Build options
DEBUG         ?= 0
CPPFLAGS := -Wall -Wextra -std=c++11
# Disable optimization when debugging #####################
ifeq ($(DEBUG), 1)
	CPPFLAGS += -O0 -g3
else
	CPPFLAGS += -O3 -DNDEBUG
endif

LDFLAGS ?=
LIBS ?= -ldl

# Paths
INC_DIR		:= inc
CORE_DIR    := src
BUILD_DIR	:= build
CONFIG_DIR	:= config
RESOURCE_DIR_IMAGES  := res/images
SYSTEMD_CONFIG_DIR := $(HOME)/.config/systemd/user
SYSTEMD_CONFIG_FILE = $(TARGET).service
INSTALL_DIR_BIN := $(HOME)/bin
INSTALL_DIR_RES := $(HOME)/.local/$(TARGET)

CPPFLAGS += -I$(INC_DIR)

# Source files
SOURCES_C := \
	$(CORE_DIR)/main.cpp \
	$(CORE_DIR)/utils.cpp \

# Target Feature support
USE_WIRINGPI ?= 0
USE_UDDRPI  ?= 0
USE_GPIOD ?= 0
USE_TM1637PI ?= 0

# Target ROM mapping to required features
ROMS ?= none
# All supported ROMs
ifneq (,$(findstring all,$(ROMS)))
	override ROMS := aburner2 turbo monacogp chasehq
endif

# After Burner 2
ifneq (,$(findstring aburner2,$(ROMS)))
	SOURCES_C += $(CORE_DIR)/aburner.cpp
	CPPFLAGS += -DROM_ABURNER2
	USE_WIRINGPI := 1
	USE_UDDRPI  := 1
endif
# Turbo
ifneq (,$(findstring turbo,$(ROMS)))
	SOURCES_C += $(CORE_DIR)/turbo.cpp
	CPPFLAGS += -DROM_TURBO
	USE_WIRINGPI := 1
	USE_UDDRPI := 1
	USE_TM1637PI := 1
endif
# Monaco GP - not really a MAME ROM but included for convenience
ifneq (,$(findstring monacogp,$(ROMS)))
	SOURCES_C += $(CORE_DIR)/monacogp.cpp
	CPPFLAGS += -DROM_MONACOGP
	USE_WIRINGPI := 1
	USE_UDDRPI := 1
	USE_TM1637PI := 1
endif
# Chase H.Q.
ifneq (,$(findstring chasehq,$(ROMS)))
	SOURCES_C += $(CORE_DIR)/chasehq.cpp
	CPPFLAGS += -DROM_CHASEHQ
	USE_WIRINGPI := 1
	USE_UDDRPI := 1
	USE_TM1637PI := 1
endif

ifeq ($(USE_GPIOD), 1)
	LIBS += -lgpiodcxx
endif
ifeq ($(USE_WIRINGPI), 1)
	LIBS += -lwiringPi
endif
ifeq ($(USE_UDDRPI), 1)
	LIBS += -lwiringPiUDDrpi
endif
ifeq ($(USE_TM1637PI), 1)
	LIBS += -lTM1637Pi
endif

OBJECTS := $(SOURCES_C:.cpp=.o)
DEPS := $(SOURCES_C:.cpp=.d)

%.o: %.cpp
	@echo Compiling $< ...
	$(CC) -MMD -MP -c $(CPPFLAGS) $< -o $@  

$(TARGET): summary $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(OBJECTS) $(LIBS)

all: build

summary:
	@echo Target ROMs: $(ROMS)
ifeq ($(USE_WIRINGPI), 1)
	@echo Using WiringPi
endif
ifeq ($(USE_UDDRPI), 1)
	@echo Using UDD Rpi
endif
ifeq ($(USE_TM1637PI), 1)
	@echo Using TM1637Pi
endif
ifeq ($(USE_GPIOD), 1)
	@echo Using Gpiod
endif

build: $(TARGET)

clean:
	rm -rf ${BUILD_DIR}
	rm -rf $(CORE_DIR)/*.d
	rm -rf $(CORE_DIR)/*.o

install: build
	mkdir -p $(SYSTEMD_CONFIG_DIR)
	mkdir -p $(INSTALL_DIR_BIN)
	mkdir -p $(INSTALL_DIR_RES)
	cp $(BUILD_DIR)/$(TARGET) $(INSTALL_DIR_BIN)/
	cp $(RESOURCE_DIR_IMAGES)/*.bmp $(INSTALL_DIR_RES)/
	cp $(CONFIG_DIR)/$(SYSTEMD_CONFIG_FILE) $(SYSTEMD_CONFIG_DIR)/
	sed -i 's|%TARGET%|$(INSTALL_DIR_BIN)/$(TARGET)|g' $(SYSTEMD_CONFIG_DIR)/$(SYSTEMD_CONFIG_FILE)
	systemctl --user enable $(TARGET)

uninstall:
	systemctl --user stop $(TARGET) 2> /dev/null || true
	killall $(TARGET) 2> /dev/null || true
	systemctl --user disable $(TARGET) 2> /dev/null || true
	rm $(SYSTEMD_CONFIG_DIR)/$(SYSTEMD_CONFIG_FILE) 2> /dev/null || true
	rm -rf $(INSTALL_DIR_RES)
	rm $(INSTALL_DIR_BIN)/$(TARGET) 2> /dev/null || true


#Non-File Targets
.PHONY: all build summary install uninstall clean

-include $(DEPS)