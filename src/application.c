/*
 * Application management functions
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 */

#include "xfce-launcher.h"

GList* load_applications(void) {
    GList *app_list = NULL;
    GList *apps = g_app_info_get_all();
    GList *iter;
    
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
            
            app_list = g_list_prepend(app_list, app_info);
        }
    }
    
    g_list_free_full(apps, g_object_unref);
    
    app_list = g_list_sort(app_list, compare_app_names);
    
    return app_list;
}

void free_app_info(AppInfo *app_info) {
    if (app_info) {
        g_free(app_info->name);
        g_free(app_info->exec);
        g_free(app_info->icon);
        g_free(app_info->folder_id);
        if (app_info->desktop_info)
            g_object_unref(app_info->desktop_info);
        g_free(app_info);
    }
}

void launch_application(GtkWidget *button, AppInfo *app_info) {
    GError *error = NULL;
    GtkWidget *toplevel = gtk_widget_get_toplevel(button);
    LauncherPlugin *launcher = g_object_get_data(G_OBJECT(toplevel), "launcher");
    
    if (app_info && app_info->desktop_info) {
        g_app_info_launch(G_APP_INFO(app_info->desktop_info), NULL, NULL, &error);
        if (error) {
            g_warning("Failed to launch application: %s", error->message);
            g_error_free(error);
        } else if (launcher) {
            hide_overlay(launcher);
        }
    }
}

void hide_application(AppInfo *app_info, LauncherPlugin *launcher) {
    app_info->is_hidden = TRUE;
    populate_current_page(launcher);
    update_page_dots(launcher);
    save_configuration(launcher);
}

gint compare_app_names(gconstpointer a, gconstpointer b) {
    const AppInfo *app_a = (const AppInfo *)a;
    const AppInfo *app_b = (const AppInfo *)b;
    
    if (!app_a->name) return 1;
    if (!app_b->name) return -1;
    
    return g_utf8_collate(app_a->name, app_b->name);
}

