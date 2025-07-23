/*
 * XFCE Launcher - Full screen application launcher for XFCE
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 *
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 * Website: www.axisos.org
 * Repository: https://github.com/Axis0S/xfce-panel-launcher
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef XFCE_LAUNCHER_H
#define XFCE_LAUNCHER_H

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <gio/gdesktopappinfo.h>

/* Forward declarations */
typedef struct _LauncherPlugin LauncherPlugin;
typedef struct _AppInfo AppInfo;
typedef struct _FolderInfo FolderInfo;

/* Application info structure */
struct _AppInfo {
    gchar *name;
    gchar *exec;
    gchar *icon;
    GDesktopAppInfo *desktop_info;
    gboolean is_hidden;
    gchar *folder_id;
};

/* Folder structure */
struct _FolderInfo {
    gchar *id;
    gchar *name;
    gchar *icon;
    GList *apps;
    gboolean is_open;
};

/* Plugin structure */
struct _LauncherPlugin {
    XfcePanelPlugin *plugin;
    GtkWidget       *button;
    GtkWidget       *icon;
    GtkWidget       *overlay_window;
    GtkWidget       *search_entry;
    GtkWidget       *app_grid;
    GtkWidget       *page_dots;
    GtkWidget       *scrolled_window;
    GList           *app_list;
    GList           *filtered_list;
    GList           *folder_list;
    gint            current_page;
    gint            total_pages;
    gboolean        drag_mode;
    AppInfo         *drag_source;
};

/* Helper structure for callbacks */
typedef struct {
    AppInfo *app_info;
    LauncherPlugin *launcher;
} HideCallbackData;

/* Constants */
#define APPS_PER_PAGE 30
#define GRID_COLUMNS 6
#define GRID_ROWS 5
#define ICON_SIZE 64
#define BUTTON_SIZE 130

/* Application management functions */
GList* load_applications(void);
void free_app_info(AppInfo *app_info);
gint compare_app_names(gconstpointer a, gconstpointer b);
void launch_application(GtkWidget *button, AppInfo *app_info);
void hide_application(AppInfo *app_info, LauncherPlugin *launcher);

/* UI functions */
void create_overlay_window(LauncherPlugin *launcher);
void hide_overlay(LauncherPlugin *launcher);
void populate_current_page(LauncherPlugin *launcher);
void update_page_dots(LauncherPlugin *launcher);
const gchar* get_css_style(void);

/* Event handlers */
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, LauncherPlugin *launcher);
void on_search_changed(GtkSearchEntry *entry, LauncherPlugin *launcher);
void on_dot_clicked(GtkWidget *dot, gpointer data);
gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, LauncherPlugin *launcher);
void on_swipe_gesture(GtkGestureSwipe *gesture, gdouble velocity_x, gdouble velocity_y, LauncherPlugin *launcher);
gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, AppInfo *app_info);

/* Drag and drop handlers */
void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                          GtkSelectionData *data, guint info, guint time, LauncherPlugin *launcher);
void on_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data,
                     guint info, guint time, AppInfo *app_info);
gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, LauncherPlugin *launcher);

/* Folder management */
FolderInfo* create_folder(const gchar *name);
void free_folder_info(FolderInfo *folder_info);
FolderInfo* find_folder_by_id(LauncherPlugin *launcher, const gchar *folder_id);
void add_app_to_folder(LauncherPlugin *launcher, AppInfo *app, const gchar *folder_id);
void remove_app_from_folder(LauncherPlugin *launcher, AppInfo *app);

/* Configuration */
gchar* get_config_file_path(void);
void save_configuration(LauncherPlugin *launcher);
void load_configuration(LauncherPlugin *launcher);

/* Plugin lifecycle callbacks */
void launcher_button_clicked(GtkWidget *button, LauncherPlugin *launcher);
void launcher_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, LauncherPlugin *launcher);
gboolean launcher_size_changed(XfcePanelPlugin *plugin, guint size, LauncherPlugin *launcher);

#endif /* XFCE_LAUNCHER_H */
