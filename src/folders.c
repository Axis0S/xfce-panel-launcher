/*
 * Folder management functions for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 */

#include "xfce-launcher.h"
#include <string.h>

FolderInfo* create_folder(const gchar *name) {
    FolderInfo *folder = g_new0(FolderInfo, 1);
    folder->id = g_strdup_printf("folder_%ld", g_get_monotonic_time());
    folder->name = g_strdup(name);
    folder->icon = g_strdup("folder");
    folder->apps = NULL;
    folder->is_open = FALSE;
    return folder;
}

void free_folder_info(FolderInfo *folder_info) {
    if (folder_info) {
        g_free(folder_info->id);
        g_free(folder_info->name);
        g_free(folder_info->icon);
        /* Note: The apps list contains pointers to AppInfo structs
         * that are owned by the main app_list, so we don't free them here */
        if (folder_info->apps)
            g_list_free(folder_info->apps);
        g_free(folder_info);
    }
}

FolderInfo* find_folder_by_id(LauncherPlugin *launcher, const gchar *folder_id) {
    GList *iter;
    for (iter = launcher->folder_list; iter != NULL; iter = g_list_next(iter)) {
        FolderInfo *folder = (FolderInfo *)iter->data;
        if (g_strcmp0(folder->id, folder_id) == 0) {
            return folder;
        }
    }
    return NULL;
}

void add_app_to_folder(LauncherPlugin *launcher, AppInfo *app, const gchar *folder_id) {
    FolderInfo *folder = find_folder_by_id(launcher, folder_id);
    if (folder && app) {
        /* Remove from any existing folder */
        if (app->folder_id) {
            FolderInfo *old_folder = find_folder_by_id(launcher, app->folder_id);
            if (old_folder) {
                old_folder->apps = g_list_remove(old_folder->apps, app);
            }
            g_free(app->folder_id);
        }
        
        /* Add to new folder */
        app->folder_id = g_strdup(folder_id);
        folder->apps = g_list_append(folder->apps, app);
    }
}

void remove_app_from_folder(LauncherPlugin *launcher, AppInfo *app) {
    if (app && app->folder_id) {
        FolderInfo *folder = find_folder_by_id(launcher, app->folder_id);
        if (folder) {
            folder->apps = g_list_remove(folder->apps, app);
        }
        g_free(app->folder_id);
        app->folder_id = NULL;
    }
}
