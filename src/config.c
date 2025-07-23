/*
 * Configuration management for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 */

#include "xfce-launcher.h"

gchar* get_config_file_path(void) {
    return g_build_filename(g_get_user_config_dir(), "xfce4", "launcher", "config.xml", NULL);
}

void save_configuration(LauncherPlugin *launcher) {
    gchar *config_path = get_config_file_path();
    gchar *config_dir = g_path_get_dirname(config_path);
    GString *xml;
    GList *iter;
    
    /* Create config directory if it doesn't exist */
    g_mkdir_with_parents(config_dir, 0700);
    
    /* Build XML content */
    xml = g_string_new("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    g_string_append(xml, "<launcher-config>\n");
    
    /* Save folders */
    g_string_append(xml, "  <folders>\n");
    for (iter = launcher->folder_list; iter != NULL; iter = g_list_next(iter)) {
        FolderInfo *folder = (FolderInfo *)iter->data;
        g_string_append_printf(xml, "    <folder id=\"%s\" name=\"%s\" icon=\"%s\"/>\n",
                              folder->id, folder->name, folder->icon);
    }
    g_string_append(xml, "  </folders>\n");
    
    /* Save app states */
    g_string_append(xml, "  <apps>\n");
    for (iter = launcher->app_list; iter != NULL; iter = g_list_next(iter)) {
        AppInfo *app = (AppInfo *)iter->data;
        if (app->is_hidden || app->folder_id) {
            g_string_append_printf(xml, "    <app name=\"%s\" hidden=\"%s\"",
                                  app->name, app->is_hidden ? "true" : "false");
            if (app->folder_id) {
                g_string_append_printf(xml, " folder=\"%s\"", app->folder_id);
            }
            g_string_append(xml, "/>\n");
        }
    }
    g_string_append(xml, "  </apps>\n");
    g_string_append(xml, "</launcher-config>\n");
    
    /* Write to file */
    g_file_set_contents(config_path, xml->str, xml->len, NULL);
    
    g_string_free(xml, TRUE);
    g_free(config_dir);
    g_free(config_path);
}

void load_configuration(LauncherPlugin *launcher) {
    gchar *config_path = get_config_file_path();
    gchar *contents = NULL;
    gsize length;
    
    if (g_file_test(config_path, G_FILE_TEST_EXISTS) &&
        g_file_get_contents(config_path, &contents, &length, NULL)) {
        
        /* Simple XML parsing (for now, we'll implement a basic parser) */
        /* TODO: Implement proper XML parsing using GMarkup or similar */
        
        g_free(contents);
    }
    
    g_free(config_path);
}
