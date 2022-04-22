TARGET := lrmame2003osvr

CFLAGS := -O2 -Wall -Wextra
LDFLAGS ?=
LIBS ?= -lwiringPi

ARM := 1
LD = ${CC}

CORE_DIR    := src
BUILD_DIR	:= build
CONFIG_DIR	:= config
SYSTEMD_CONFIG_DIR := $(HOME)/.config/systemd/user
SYSTEMD_CONFIG_FILE = $(CONFIG_DIR)/$(TARGET).service
INSTALL_DIR := $(HOME)/bin

SOURCES_C := \
	$(CORE_DIR)/main.c

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

install:
	mkdir -p $(SYSTEMD_CONFIG_DIR)
	mkdir -p $(INSTALL_DIR)
	cp $(BUILD_DIR)/$(TARGET) $(INSTALL_DIR)/
	cp $(SYSTEMD_CONFIG_FILE) $(SYSTEMD_CONFIG_DIR)
	systemctl --user enable $(TARGET)


