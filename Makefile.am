SUBDIRS = \
	po

plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libxfce-launcher.la

libxfce_launcher_la_SOURCES = \
	xfce-launcher.c

libxfce_launcher_la_CFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-launcher-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS) \
	$(PLATFORM_CFLAGS) \
	$(XFCE4PANEL_CFLAGS) \
	$(XFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS)

libxfce_launcher_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libxfce_launcher_la_LIBADD = \
	$(XFCE4PANEL_LIBS) \
	$(XFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(GIO_LIBS)

#
# Desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = xfce-launcher.desktop

%.desktop: %.desktop.in
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	xfce-launcher.desktop.in

DISTCLEANFILES = \
	$(desktop_DATA)
