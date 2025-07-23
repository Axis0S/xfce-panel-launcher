/*
 * Event handlers for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki <novik@axisos.org>
 */

#include "xfce-launcher.h"
#include <string.h>
#include <math.h>

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, LauncherPlugin *launcher) {
    switch (event->keyval) {
        case GDK_KEY_Escape:
            hide_overlay(launcher);
            return TRUE;
        case GDK_KEY_Right:
            if (launcher->current_page < launcher->total_pages - 1) {
                launcher->current_page++;
                populate_current_page(launcher);
                update_page_dots(launcher);
                return TRUE;
            }
            break;
        case GDK_KEY_Left:
            if (launcher->current_page > 0) {
                launcher->current_page--;
                populate_current_page(launcher);
                update_page_dots(launcher);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

void on_search_changed(GtkSearchEntry *entry, LauncherPlugin *launcher) {
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(entry));
    GList *iter;
    
    if (launcher->filtered_list) {
        g_list_free(launcher->filtered_list);
    }
    
    launcher->filtered_list = NULL;
    
    if (strlen(search_text) == 0) {
        launcher->filtered_list = g_list_copy(launcher->app_list);
    } else {
        gchar *search_lower = g_utf8_strdown(search_text, -1);
        
        for (iter = launcher->app_list; iter != NULL; iter = g_list_next(iter)) {
            AppInfo *app_info = (AppInfo *)iter->data;
            if (app_info && app_info->name && !app_info->is_hidden) {
                gchar *name_lower = g_utf8_strdown(app_info->name, -1);
                if (strstr(name_lower, search_lower) != NULL) {
                    launcher->filtered_list = g_list_append(launcher->filtered_list, app_info);
                }
                g_free(name_lower);
            }
        }
        
        g_free(search_lower);
    }
    
    launcher->current_page = 0;
    populate_current_page(launcher);
    update_page_dots(launcher);
}

void on_dot_clicked(GtkWidget *dot, gpointer data) {
    gint page_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dot), "page-index"));
    LauncherPlugin *launcher = g_object_get_data(G_OBJECT(dot), "launcher");
    
    if (launcher && page_index != launcher->current_page) {
        launcher->current_page = page_index;
        populate_current_page(launcher);
        update_page_dots(launcher);
    }
}

gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, LauncherPlugin *launcher) {
    gboolean changed = FALSE;
    
    switch (event->direction) {
        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_UP:
            if (launcher->current_page > 0) {
                launcher->current_page--;
                changed = TRUE;
            }
            break;
            
        case GDK_SCROLL_RIGHT:
        case GDK_SCROLL_DOWN:
            if (launcher->current_page < launcher->total_pages - 1) {
                launcher->current_page++;
                changed = TRUE;
            }
            break;
            
        case GDK_SCROLL_SMOOTH:
            if (fabs(event->delta_x) > fabs(event->delta_y)) {
                if (event->delta_x < -0.3) {
                    if (launcher->current_page > 0) {
                        launcher->current_page--;
                        changed = TRUE;
                    }
                } else if (event->delta_x > 0.3) {
                    if (launcher->current_page < launcher->total_pages - 1) {
                        launcher->current_page++;
                        changed = TRUE;
                    }
                }
            } else {
                if (event->delta_y < -0.3) {
                    if (launcher->current_page > 0) {
                        launcher->current_page--;
                        changed = TRUE;
                    }
                } else if (event->delta_y > 0.3) {
                    if (launcher->current_page < launcher->total_pages - 1) {
                        launcher->current_page++;
                        changed = TRUE;
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    if (changed) {
        populate_current_page(launcher);
        update_page_dots(launcher);
    }
    
    return TRUE;
}

void on_swipe_gesture(GtkGestureSwipe *gesture, gdouble velocity_x, gdouble velocity_y, LauncherPlugin *launcher) {
    gboolean changed = FALSE;
    
    if (velocity_x > 0 && launcher->current_page > 0) {
        launcher->current_page--;
        changed = TRUE;
    } else if (velocity_x < 0 && launcher->current_page < launcher->total_pages - 1) {
        launcher->current_page++;
        changed = TRUE;
    }
    
    if (changed) {
        populate_current_page(launcher);
        update_page_dots(launcher);
    }
}

static void on_hide_menu_activate(GtkMenuItem *menuitem, gpointer user_data) {
    HideCallbackData *data = (HideCallbackData *)user_data;
    if (data && data->app_info && data->launcher) {
        hide_application(data->app_info, data->launcher);
    }
    g_free(data);
}

gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, AppInfo *app_info) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkWidget *menu = gtk_menu_new();
        GtkWidget *hide_item = gtk_menu_item_new_with_label("Hide");
        LauncherPlugin *launcher = g_object_get_data(G_OBJECT(widget), "launcher");
        
        if (!launcher) {
            g_warning("Launcher reference not found in button data");
            return FALSE;
        }
        
        HideCallbackData *callback_data = g_new(HideCallbackData, 1);
        callback_data->app_info = app_info;
        callback_data->launcher = launcher;
        
        g_signal_connect(hide_item, "activate",
                         G_CALLBACK(on_hide_menu_activate), callback_data);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), hide_item);
        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);

        return TRUE;
    }
    return FALSE;
}

/* Drag and drop handlers */
void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                          GtkSelectionData *data, guint info, guint time, LauncherPlugin *launcher) {
    AppInfo *app_info = g_object_get_data(G_OBJECT(widget), "app-info");
    FolderInfo *folder_info = find_folder_by_id(launcher, (const gchar *)gtk_selection_data_get_data(data));

    if (app_info && folder_info) {
        add_app_to_folder(launcher, app_info, folder_info->id);
        populate_current_page(launcher);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

void on_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data,
                     guint info, guint time, AppInfo *app_info) {
    if (app_info && app_info->folder_id) {
        gtk_selection_data_set_text(data, app_info->folder_id, -1);
    }
}

gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, LauncherPlugin *launcher) {
    AppInfo *target_app = NULL;
    GValue value = G_VALUE_INIT;

    gtk_container_child_get_property(GTK_CONTAINER(launcher->app_grid),
                                     gtk_grid_get_child_at(GTK_GRID(launcher->app_grid), x / BUTTON_SIZE, y / BUTTON_SIZE),
                                     "app-info", &value);

    if (G_VALUE_HOLDS_POINTER(&value)) {
        target_app = g_value_get_pointer(&value);
    }

    if (target_app && launcher->drag_source && launcher->drag_source != target_app) {
        FolderInfo *folder = create_folder("New Folder");
        add_app_to_folder(launcher, launcher->drag_source, folder->id);
        add_app_to_folder(launcher, target_app, folder->id);

        launcher->folder_list = g_list_append(launcher->folder_list, folder);

        populate_current_page(launcher);
        update_page_dots(launcher);
        save_configuration(launcher);
        gtk_drag_finish(context, TRUE, FALSE, time);
        return TRUE;
    }

    gtk_drag_finish(context, FALSE, FALSE, time);
    return FALSE;
}
