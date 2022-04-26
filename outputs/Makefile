TARGET := lrmame2003osvr

CFLAGS := -O2 -Wall -Wextra
LDFLAGS ?=
LIBS ?= -lwiringPi -lwiringPiUDDrpi
CC := g++

ARM := 1
LD = ${CC}

CORE_DIR    := src
BUILD_DIR	:= build
CONFIG_DIR	:= config
RESOURCE_DIR_IMAGES  := res/images
SYSTEMD_CONFIG_DIR := $(HOME)/.config/systemd/user
SYSTEMD_CONFIG_FILE = $(TARGET).service
INSTALL_DIR_BIN := $(HOME)/bin
INSTALL_DIR_RES := $(HOME)/.local/$(TARGET)

SOURCES_C := \
	$(CORE_DIR)/main.cpp

OBJECTS := $(SOURCES_C:.c=.o)

%.o: %.c
	@echo Compiling $<...
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(OBJECTS) $(LIBS)

all: build

build: $(TARGET)

clean:
	rm -rf ${BUILD_DIR}
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
	killall $(TARGET) 2> /dev/null || true
	systemctl --user disable $(TARGET) 2> /dev/null || true
	rm $(SYSTEMD_CONFIG_DIR)/$(SYSTEMD_CONFIG_FILE) 2> /dev/null || true
	rm -rf $(INSTALL_DIR_RES)
	rm $(INSTALL_DIR_BIN)/$(TARGET) 2> /dev/null || true

