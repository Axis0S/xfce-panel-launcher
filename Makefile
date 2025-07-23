# Simple Makefile for XFCE Launcher Plugin
# Use this if you don't want to use autotools

CC = gcc
CFLAGS = -Wall -g -fPIC `pkg-config --cflags gtk+-3.0 libxfce4panel-2.0 libxfce4util-1.0 gio-2.0 libxfconf-0`
LDFLAGS = -shared `pkg-config --libs gtk+-3.0 libxfce4panel-2.0 libxfce4util-1.0 gio-2.0 libxfconf-0`

# Allow PREFIX override for packaging
PREFIX ?= /usr
DESTDIR ?=

# Detect multiarch if available
DEB_HOST_MULTIARCH ?= $(shell dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null)

# System directories
PLUGIN_DIR = $(DESTDIR)$(PREFIX)/share/xfce4/panel/plugins
ICON_DIR = $(DESTDIR)$(PREFIX)/share/icons/hicolor
ifdef DEB_HOST_MULTIARCH
LIB_DIR = $(DESTDIR)$(PREFIX)/lib/$(DEB_HOST_MULTIARCH)/xfce4/panel/plugins
else
LIB_DIR = $(DESTDIR)$(PREFIX)/lib/xfce4/panel/plugins
endif

# User directories (for local install)
USER_PLUGIN_DIR = $(HOME)/.local/share/xfce4/panel/plugins
USER_LIB_DIR = $(HOME)/.local/lib/xfce4/panel/plugins
USER_ICON_DIR = $(HOME)/.local/share/icons/hicolor

# Source files
SOURCES = src/plugin.c src/application.c src/application-loader.c src/ui.c src/events.c src/folders.c src/config.c src/settings.c
OBJECTS = $(SOURCES:.c=.o)

all: libxfce-launcher.so xfce-launcher.desktop

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libxfce-launcher.so: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

install: libxfce-launcher.so xfce-launcher.desktop
	mkdir -p $(PLUGIN_DIR)
	mkdir -p $(LIB_DIR)
	mkdir -p $(ICON_DIR)/16x16/apps
	mkdir -p $(ICON_DIR)/22x22/apps
	mkdir -p $(ICON_DIR)/24x24/apps
	cp libxfce-launcher.so $(LIB_DIR)/
	cp xfce-launcher.desktop $(PLUGIN_DIR)/
	cp data/icons/16x16/xfce-launcher.svg $(ICON_DIR)/16x16/apps/
	cp data/icons/22x22/xfce-launcher.svg $(ICON_DIR)/22x22/apps/
	cp data/icons/24x24/xfce-launcher.svg $(ICON_DIR)/24x24/apps/

# Local user installation (no sudo required)
install-local: libxfce-launcher.so xfce-launcher.desktop
	mkdir -p $(USER_PLUGIN_DIR)
	mkdir -p $(USER_LIB_DIR)
	mkdir -p $(USER_ICON_DIR)/16x16/apps
	mkdir -p $(USER_ICON_DIR)/22x22/apps
	mkdir -p $(USER_ICON_DIR)/24x24/apps
	cp libxfce-launcher.so $(USER_LIB_DIR)/
	cp xfce-launcher.desktop $(USER_PLUGIN_DIR)/
	cp data/icons/16x16/xfce-launcher.svg $(USER_ICON_DIR)/16x16/apps/
	cp data/icons/22x22/xfce-launcher.svg $(USER_ICON_DIR)/22x22/apps/
	cp data/icons/24x24/xfce-launcher.svg $(USER_ICON_DIR)/24x24/apps/

xfce-launcher.desktop: data/xfce-launcher.desktop.in
	cp data/xfce-launcher.desktop.in xfce-launcher.desktop

clean:
	rm -f libxfce-launcher.so xfce-launcher.desktop $(OBJECTS)

uninstall:
	sudo rm -f $(LIB_DIR)/libxfce-launcher.so
	sudo rm -f $(LIB_DIR)/libxfcelauncher.so
	sudo rm -f $(PLUGIN_DIR)/xfce-launcher.desktop
	sudo rm -f $(ICON_DIR)/16x16/apps/xfce-launcher.svg
	sudo rm -f $(ICON_DIR)/22x22/apps/xfce-launcher.svg
	sudo rm -f $(ICON_DIR)/24x24/apps/xfce-launcher.svg

# Uninstall from user directory
uninstall-local:
	rm -f $(USER_LIB_DIR)/libxfce-launcher.so
	rm -f $(USER_LIB_DIR)/libxfcelauncher.so
	rm -f $(USER_PLUGIN_DIR)/xfce-launcher.desktop
	rm -f $(USER_ICON_DIR)/16x16/apps/xfce-launcher.svg
	rm -f $(USER_ICON_DIR)/22x22/apps/xfce-launcher.svg
	rm -f $(USER_ICON_DIR)/24x24/apps/xfce-launcher.svg

.PHONY: all install install-local clean uninstall uninstall-local
