TARGET := lrmame2003osvr

CPPFLAGS := -O3 -Wall -Wextra -std=c++11
LDFLAGS ?=
LIBS ?= -lwiringPi -lwiringPiUDDrpi -lgpiodcxx -lTM1637Pi -ldl
CC := g++

ARM := 1
LD = ${CC}

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

SOURCES_C := \
	$(CORE_DIR)/main.cpp \
	$(CORE_DIR)/utils.cpp \
	$(CORE_DIR)/chasehq.cpp \
	$(CORE_DIR)/turbo.cpp \
	$(CORE_DIR)/monacogp.cpp \
	$(CORE_DIR)/aburner.cpp

OBJECTS := $(SOURCES_C:.cpp=.o)
DEPS := $(SOURCES_C:.cpp=.d)



%.o: %.cpp
	@echo Compiling $< ...
	$(CC) -MMD -MP -c $(CPPFLAGS) $< -o $@  

$(TARGET): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$@ $(OBJECTS) $(LIBS)

all: build

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

-include $(DEPS)