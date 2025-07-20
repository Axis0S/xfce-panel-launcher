# Simple Makefile for XFCE Launcher Plugin
# Use this if you don't want to use autotools

CC = gcc
CFLAGS = -Wall -g -fPIC `pkg-config --cflags gtk+-3.0 libxfce4panel-2.0 libxfce4util-1.0 gio-2.0`
LDFLAGS = -shared `pkg-config --libs gtk+-3.0 libxfce4panel-2.0 libxfce4util-1.0 gio-2.0`

# System directories
PLUGIN_DIR = /usr/share/xfce4/panel/plugins
LIB_DIR = /usr/lib/xfce4/panel/plugins

# User directories (for local install)
USER_PLUGIN_DIR = $(HOME)/.local/share/xfce4/panel/plugins
USER_LIB_DIR = $(HOME)/.local/lib/xfce4/panel/plugins

all: libxfcelauncher.so

libxfcelauncher.so: xfce-launcher.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install: libxfcelauncher.so xfce-launcher.desktop
	sudo mkdir -p $(PLUGIN_DIR)
	sudo mkdir -p $(LIB_DIR)
	sudo cp libxfcelauncher.so $(LIB_DIR)/
	sudo cp xfce-launcher.desktop $(PLUGIN_DIR)/

# Local user installation (no sudo required)
install-local: libxfcelauncher.so xfce-launcher.desktop
	mkdir -p $(USER_PLUGIN_DIR)
	mkdir -p $(USER_LIB_DIR)
	cp libxfcelauncher.so $(USER_LIB_DIR)/
	cp xfce-launcher.desktop $(USER_PLUGIN_DIR)/

xfce-launcher.desktop: xfce-launcher.desktop.in
	cp xfce-launcher.desktop.in xfce-launcher.desktop

clean:
	rm -f libxfcelauncher.so xfce-launcher.desktop

uninstall:
	sudo rm -f $(LIB_DIR)/libxfce-launcher.so
	sudo rm -f $(LIB_DIR)/libxfcelauncher.so
	sudo rm -f $(PLUGIN_DIR)/xfce-launcher.desktop

# Uninstall from user directory
uninstall-local:
	rm -f $(USER_LIB_DIR)/libxfce-launcher.so
	rm -f $(USER_LIB_DIR)/libxfcelauncher.so
	rm -f $(USER_PLUGIN_DIR)/xfce-launcher.desktop

.PHONY: all install install-local clean uninstall uninstall-local
