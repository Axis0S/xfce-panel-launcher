/*
 * Enhanced application loader with Snap and Flatpak support
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

#include "xfce-launcher.h"
#include <gio/gio.h>

/* Directory paths to monitor */
static const gchar *desktop_dirs[] = {
    "/usr/share/applications",
    "/usr/local/share/applications",
    "/var/lib/snapd/desktop/applications",
    "/var/lib/flatpak/exports/share/applications",
    NULL
};

/* Get user-specific directories */
static gchar** get_user_desktop_dirs(void) {
    GPtrArray *dirs = g_ptr_array_new();
    gchar *home = g_strdup(g_get_home_dir());
    
    /* User's local applications */
    g_ptr_array_add(dirs, g_build_filename(home, ".local/share/applications", NULL));
    
    /* Snap user applications */
    g_ptr_array_add(dirs, g_build_filename(home, "snap", NULL));
    
    /* Flatpak user applications */
    g_ptr_array_add(dirs, g_build_filename(home, ".local/share/flatpak/exports/share/applications", NULL));
    
    g_free(home);
    g_ptr_array_add(dirs, NULL);
    
    return (gchar**)g_ptr_array_free(dirs, FALSE);
}

/* Load applications from Snap */
static void load_snap_applications(GList **app_list) {
    const gchar *snap_dir = "/var/lib/snapd/desktop/applications";
    GDir *dir;
    const gchar *filename;
    GError *error = NULL;
    
    if (!g_file_test(snap_dir, G_FILE_TEST_IS_DIR))
        return;
    
    dir = g_dir_open(snap_dir, 0, &error);
    if (!dir) {
        if (error) {
            g_warning("Failed to open snap directory: %s", error->message);
            g_error_free(error);
        }
        return;
    }
    
    while ((filename = g_dir_read_name(dir)) != NULL) {
        if (g_str_has_suffix(filename, ".desktop")) {
            gchar *desktop_path = g_build_filename(snap_dir, filename, NULL);
            GDesktopAppInfo *desktop_info = g_desktop_app_info_new_from_filename(desktop_path);
            
            if (desktop_info && g_app_info_should_show(G_APP_INFO(desktop_info))) {
                AppInfo *app_info = g_new0(AppInfo, 1);
                app_info->name = g_strdup(g_app_info_get_display_name(G_APP_INFO(desktop_info)));
                app_info->exec = g_strdup(g_app_info_get_commandline(G_APP_INFO(desktop_info)));
                
                GIcon *gicon = g_app_info_get_icon(G_APP_INFO(desktop_info));
                if (gicon && G_IS_THEMED_ICON(gicon)) {
                    const gchar * const *icon_names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
                    if (icon_names && icon_names[0])
                        app_info->icon = g_strdup(icon_names[0]);
                }
                
                app_info->desktop_info = desktop_info;
                app_info->position = -1;
                *app_list = g_list_prepend(*app_list, app_info);
            }
            
            g_free(desktop_path);
        }
    }
    
    g_dir_close(dir);
}

/* Load applications from Flatpak */
static void load_flatpak_applications(GList **app_list) {
    const gchar *flatpak_dirs[] = {
        "/var/lib/flatpak/exports/share/applications",
        NULL
    };
    
    /* Also check user flatpak dir */
    gchar *user_flatpak_dir = g_build_filename(g_get_home_dir(), 
                                               ".local/share/flatpak/exports/share/applications", 
                                               NULL);
    
    for (int i = 0; flatpak_dirs[i] != NULL; i++) {
        const gchar *flatpak_dir = flatpak_dirs[i];
        GDir *dir;
        const gchar *filename;
        GError *error = NULL;
        
        if (!g_file_test(flatpak_dir, G_FILE_TEST_IS_DIR))
            continue;
        
        dir = g_dir_open(flatpak_dir, 0, &error);
        if (!dir) {
            if (error) {
                g_warning("Failed to open flatpak directory: %s", error->message);
                g_error_free(error);
            }
            continue;
        }
        
        while ((filename = g_dir_read_name(dir)) != NULL) {
            if (g_str_has_suffix(filename, ".desktop")) {
                gchar *desktop_path = g_build_filename(flatpak_dir, filename, NULL);
                GDesktopAppInfo *desktop_info = g_desktop_app_info_new_from_filename(desktop_path);
                
                if (desktop_info && g_app_info_should_show(G_APP_INFO(desktop_info))) {
                    AppInfo *app_info = g_new0(AppInfo, 1);
                    app_info->name = g_strdup(g_app_info_get_display_name(G_APP_INFO(desktop_info)));
                    app_info->exec = g_strdup(g_app_info_get_commandline(G_APP_INFO(desktop_info)));
                    
                    GIcon *gicon = g_app_info_get_icon(G_APP_INFO(desktop_info));
                    if (gicon && G_IS_THEMED_ICON(gicon)) {
                        const gchar * const *icon_names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
                        if (icon_names && icon_names[0])
                            app_info->icon = g_strdup(icon_names[0]);
                    }
                    
                    app_info->desktop_info = desktop_info;
                    app_info->position = -1;
                    *app_list = g_list_prepend(*app_list, app_info);
                }
                
                g_free(desktop_path);
            }
        }
        
        g_dir_close(dir);
    }
    
    /* Check user flatpak directory */
    if (g_file_test(user_flatpak_dir, G_FILE_TEST_IS_DIR)) {
        GDir *dir;
        const gchar *filename;
        GError *error = NULL;
        
        dir = g_dir_open(user_flatpak_dir, 0, &error);
        if (dir) {
            while ((filename = g_dir_read_name(dir)) != NULL) {
                if (g_str_has_suffix(filename, ".desktop")) {
                    gchar *desktop_path = g_build_filename(user_flatpak_dir, filename, NULL);
                    GDesktopAppInfo *desktop_info = g_desktop_app_info_new_from_filename(desktop_path);
                    
                    if (desktop_info && g_app_info_should_show(G_APP_INFO(desktop_info))) {
                        AppInfo *app_info = g_new0(AppInfo, 1);
                        app_info->name = g_strdup(g_app_info_get_display_name(G_APP_INFO(desktop_info)));
                        app_info->exec = g_strdup(g_app_info_get_commandline(G_APP_INFO(desktop_info)));
                        
                        GIcon *gicon = g_app_info_get_icon(G_APP_INFO(desktop_info));
                        if (gicon && G_IS_THEMED_ICON(gicon)) {
                            const gchar * const *icon_names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
                            if (icon_names && icon_names[0])
                                app_info->icon = g_strdup(icon_names[0]);
                        }
                        
                        app_info->desktop_info = desktop_info;
                        app_info->position = -1;
                        *app_list = g_list_prepend(*app_list, app_info);
                    }
                    
                    g_free(desktop_path);
                }
            }
            g_dir_close(dir);
        }
    }
    
    g_free(user_flatpak_dir);
}

/* Enhanced load_applications function */
GList* load_applications_enhanced(void) {
    GList *app_list = NULL;
    GList *apps = g_app_info_get_all();
    GList *iter;
    
    /* Load standard applications */
    for (iter = apps; iter != NULL; iter = g_list_next(iter)) {
        GAppInfo *gapp_info = G_APP_INFO(iter->data);
        
        if (g_app_info_should_show(gapp_info)) {
            AppInfo *app_info = g_new0(AppInfo, 1);
            app_info->name = g_strdup(g_app_info_get_display_name(gapp_info));
            app_info->exec = g_strdup(g_app_info_get_commandline(gapp_info));
            
            GIcon *gicon = g_app_info_get_icon(gapp_info);
            if (gicon && G_IS_THEMED_ICON(gicon)) {
                const gchar * const *icon_names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
                if (icon_names && icon_names[0])
                    app_info->icon = g_strdup(icon_names[0]);
            }
            
            app_info->desktop_info = G_DESKTOP_APP_INFO(g_object_ref(gapp_info));
            app_info->position = -1;
            
            app_list = g_list_prepend(app_list, app_info);
        }
    }
    
    g_list_free_full(apps, g_object_unref);
    
    /* Load Snap applications */
    load_snap_applications(&app_list);
    
    /* Load Flatpak applications */
    load_flatpak_applications(&app_list);
    
    /* Remove duplicates based on desktop file name or app name */
    GHashTable *seen_apps = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GList *unique_list = NULL;
    
    for (iter = app_list; iter != NULL; iter = g_list_next(iter)) {
        AppInfo *app = (AppInfo *)iter->data;
        gchar *key = g_strdup(app->name);
        
        if (!g_hash_table_contains(seen_apps, key)) {
            g_hash_table_insert(seen_apps, key, app);
            unique_list = g_list_prepend(unique_list, app);
        } else {
            g_free(key);
            free_app_info(app);
        }
    }
    
    g_list_free(app_list);
    g_hash_table_destroy(seen_apps);
    
    /* Sort applications */
    unique_list = g_list_sort(unique_list, compare_app_names);
    
    return unique_list;
}

/* Directory monitor callback */
static void on_directory_changed(GFileMonitor *monitor,
                               GFile *file,
                               GFile *other_file,
                               GFileMonitorEvent event_type,
                               gpointer user_data) {
    LauncherPlugin *launcher = (LauncherPlugin *)user_data;
    
    /* Only refresh on meaningful changes */
    if (event_type == G_FILE_MONITOR_EVENT_CREATED ||
        event_type == G_FILE_MONITOR_EVENT_DELETED ||
        event_type == G_FILE_MONITOR_EVENT_CHANGED) {
        
        gchar *basename = g_file_get_basename(file);
        
        /* Only process .desktop files */
        if (basename && g_str_has_suffix(basename, ".desktop")) {
            g_debug("Application change detected: %s", basename);
            
            /* Refresh applications list */
            if (launcher->app_list) {
                g_list_free_full(launcher->app_list, (GDestroyNotify)free_app_info);
            }
            launcher->app_list = load_applications_enhanced();
            
            /* Update filtered list */
            if (launcher->filtered_list) {
                g_list_free(launcher->filtered_list);
            }
            launcher->filtered_list = g_list_copy(launcher->app_list);
            
            /* Refresh the UI if overlay is visible */
            if (launcher->overlay_window && 
                gtk_widget_get_visible(launcher->overlay_window)) {
                populate_current_page(launcher);
                update_page_dots(launcher);
            }
        }
        
        g_free(basename);
    }
}

/* Setup directory monitoring */
void setup_application_monitoring(LauncherPlugin *launcher) {
    GFile *file;
    GFileMonitor *monitor;
    GError *error = NULL;
    
    /* Monitor system directories */
    for (int i = 0; desktop_dirs[i] != NULL; i++) {
        if (g_file_test(desktop_dirs[i], G_FILE_TEST_IS_DIR)) {
            file = g_file_new_for_path(desktop_dirs[i]);
            monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL, &error);
            
            if (monitor) {
                g_signal_connect(monitor, "changed", 
                               G_CALLBACK(on_directory_changed), launcher);
                /* Keep reference to prevent garbage collection */
                g_object_set_data_full(G_OBJECT(launcher->plugin), 
                                     g_strdup_printf("monitor-%d", i),
                                     monitor, g_object_unref);
            } else if (error) {
                g_warning("Failed to monitor %s: %s", desktop_dirs[i], error->message);
                g_error_free(error);
                error = NULL;
            }
            
            g_object_unref(file);
        }
    }
    
    /* Monitor user directories */
    gchar **user_dirs = get_user_desktop_dirs();
    for (int i = 0; user_dirs[i] != NULL; i++) {
        if (g_file_test(user_dirs[i], G_FILE_TEST_IS_DIR)) {
            file = g_file_new_for_path(user_dirs[i]);
            monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL, &error);
            
            if (monitor) {
                g_signal_connect(monitor, "changed", 
                               G_CALLBACK(on_directory_changed), launcher);
                /* Keep reference to prevent garbage collection */
                g_object_set_data_full(G_OBJECT(launcher->plugin), 
                                     g_strdup_printf("user-monitor-%d", i),
                                     monitor, g_object_unref);
            } else if (error) {
                g_warning("Failed to monitor %s: %s", user_dirs[i], error->message);
                g_error_free(error);
                error = NULL;
            }
            
            g_object_unref(file);
        }
        g_free(user_dirs[i]);
    }
    g_free(user_dirs);
}
