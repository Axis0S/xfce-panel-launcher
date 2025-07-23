/*
 * UI components for XFCE Launcher
 * 
 * Copyright (C) 2025 Kamil 'Novik' Nowicki
 * 
 * Author: Kamil 'Novik' Nowicki novik@axisos.org
 */

#include "xfce-launcher.h"

void create_overlay_window(LauncherPlugin *launcher) {
    GtkWidget *main_box, *search_box, *grid_container, *center_box;
    GdkScreen *screen;
    GdkVisual *visual;
    
    launcher->overlay_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(launcher->overlay_window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_decorated(GTK_WINDOW(launcher->overlay_window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(launcher->overlay_window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(launcher->overlay_window), TRUE);
    gtk_window_fullscreen(GTK_WINDOW(launcher->overlay_window));
    
    screen = gtk_widget_get_screen(launcher->overlay_window);
    visual = gdk_screen_get_rgba_visual(screen);
    if (visual && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(launcher->overlay_window, visual);
    }
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, get_css_style(), -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(launcher->overlay_window);
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(launcher->overlay_window), main_box);

    search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(search_box, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(search_box), "search-container");
    gtk_widget_set_size_request(search_box, 450, -1);
    gtk_box_pack_start(GTK_BOX(main_box), search_box, FALSE, FALSE, 0);

    launcher->search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(launcher->search_entry), "Search");
    gtk_widget_set_hexpand(launcher->search_entry, TRUE);
    gtk_box_pack_start(GTK_BOX(search_box), launcher->search_entry, TRUE, TRUE, 0);

    g_signal_connect(launcher->search_entry, "search-changed",
                     G_CALLBACK(on_search_changed), launcher);

    center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), center_box, TRUE, TRUE, 0);

    grid_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(center_box), grid_container, FALSE, FALSE, 0);

    launcher->app_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(launcher->app_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(launcher->app_grid), 20);
    gtk_widget_set_halign(launcher->app_grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(launcher->app_grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(grid_container), launcher->app_grid, FALSE, FALSE, 0);

    launcher->page_dots = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(launcher->page_dots, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(launcher->page_dots), "page-dots");
    gtk_box_pack_start(GTK_BOX(main_box), launcher->page_dots, FALSE, FALSE, 0);

    populate_current_page(launcher);
    update_page_dots(launcher);

    g_signal_connect(launcher->overlay_window, "key-press-event",
                     G_CALLBACK(on_key_press), launcher);

    g_signal_connect(launcher->overlay_window, "scroll-event",
                     G_CALLBACK(on_scroll_event), launcher);

    gtk_style_context_add_provider_for_screen(screen,
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

void hide_overlay(LauncherPlugin *launcher) {
    if (launcher->overlay_window) {
        gtk_widget_hide(launcher->overlay_window);
        gtk_entry_set_text(GTK_ENTRY(launcher->search_entry), "");

        if (launcher->filtered_list) {
            g_list_free(launcher->filtered_list);
        }
        launcher->filtered_list = g_list_copy(launcher->app_list);
        launcher->current_page = 0;
    }
}

void populate_current_page(LauncherPlugin *launcher) {
    GList *iter;
    gint row, col;
    gint start_index = launcher->current_page * APPS_PER_PAGE;
    gint index = 0;

    gtk_container_foreach(GTK_CONTAINER(launcher->app_grid),
                         (GtkCallback)gtk_widget_destroy, NULL);

    for (iter = launcher->filtered_list; iter != NULL && index < start_index + APPS_PER_PAGE; iter = g_list_next(iter)) {
        AppInfo *app_info = (AppInfo *)iter->data;

        if (app_info->is_hidden) {
            continue;
        }

        if (index >= start_index) {
            GtkWidget *button, *box, *icon, *label;

            gint grid_index = index - start_index;
            col = grid_index % GRID_COLUMNS;
            row = grid_index / GRID_COLUMNS;

            button = gtk_button_new();
            gtk_style_context_add_class(gtk_widget_get_style_context(button), "app-button");
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
            gtk_widget_set_size_request(button, BUTTON_SIZE, BUTTON_SIZE);

            box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
            gtk_container_add(GTK_CONTAINER(button), box);

            if (app_info->icon) {
                icon = gtk_image_new_from_icon_name(app_info->icon, GTK_ICON_SIZE_DIALOG);
                gtk_image_set_pixel_size(GTK_IMAGE(icon), ICON_SIZE);
            } else {
                icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
                gtk_image_set_pixel_size(GTK_IMAGE(icon), ICON_SIZE);
            }
            gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);

            if (app_info->name) {
                label = gtk_label_new(app_info->name);
            } else {
                continue;
            }
            gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_label_set_max_width_chars(GTK_LABEL(label), 15);
            gtk_label_set_lines(GTK_LABEL(label), 2);
            gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

            gtk_drag_source_set(button, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_MOVE);
            gtk_drag_dest_set(button, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_MOVE);
            g_signal_connect(button, "drag-data-received",
                            G_CALLBACK(on_drag_data_received), launcher);
            g_signal_connect(button, "drag-data-get",
                            G_CALLBACK(on_drag_data_get), app_info);

            g_signal_connect(button, "button-press-event",
                            G_CALLBACK(on_button_press_event), app_info);

            g_signal_connect(button, "clicked",
                            G_CALLBACK(launch_application), app_info);

            gtk_grid_attach(GTK_GRID(launcher->app_grid), button, col, row, 1, 1);

            g_object_set_data(G_OBJECT(button), "app-info", app_info);
            g_object_set_data(G_OBJECT(button), "launcher", launcher);

            gtk_widget_show_all(button);
        }
        index++;
    }
}

void update_page_dots(LauncherPlugin *launcher) {
    GList *children, *iter;
    gint i;

    children = gtk_container_get_children(GTK_CONTAINER(launcher->page_dots));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    launcher->total_pages = (g_list_length(launcher->filtered_list) + APPS_PER_PAGE - 1) / APPS_PER_PAGE;

    for (i = 0; i < launcher->total_pages; i++) {
        GtkWidget *dot = gtk_button_new();
        gtk_style_context_add_class(gtk_widget_get_style_context(dot), "page-dot");

        if (i == launcher->current_page) {
            gtk_style_context_add_class(gtk_widget_get_style_context(dot), "active");
        }

        g_object_set_data(G_OBJECT(dot), "page-index", GINT_TO_POINTER(i));
        g_object_set_data(G_OBJECT(dot), "launcher", launcher);
        g_signal_connect(dot, "clicked", G_CALLBACK(on_dot_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(launcher->page_dots), dot, FALSE, FALSE, 0);
        gtk_widget_show(dot);
    }
}

const gchar* get_css_style(void) {
    return
    "window {\n"
    "  background-color: rgba(40, 40, 40, 0.85);\n"
    "}\n"
    "button.app-button {\n"
    "  background-color: transparent;\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  padding: 15px;\n"
    "  margin: 10px;\n"
    "  border-radius: 16px;\n"
    "}\n"
    "button.app-button:hover {\n"
    "  background-color: rgba(255, 255, 255, 0.1);\n"
    "}\n"
    "button.app-button:active {\n"
    "  background-color: rgba(255, 255, 255, 0.15);\n"
    "}\n"
    "button.app-button:focus {\n"
    "  outline: none;\n"
    "}\n"
    "button.app-button label {\n"
    "  color: rgba(255, 255, 255, 0.9);\n"
    "  font-size: 12px;\n"
    "  font-weight: 400;\n"
    "}\n"
    ".search-container {\n"
    "  background-color: rgba(255, 255, 255, 0.15);\n"
    "  border-radius: 12px;\n"
    "  border: 1px solid rgba(255, 255, 255, 0.2);\n"
    "  margin: 40px;\n"
    "}\n"
    "entry {\n"
    "  background-color: transparent;\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  font-size: 18px;\n"
    "  padding: 16px 20px;\n"
    "  color: white;\n"
    "  caret-color: white;\n"
    "  font-weight: 300;\n"
    "}\n"
    "entry:focus {\n"
    "  outline: none;\n"
    "}\n"
    "entry text {\n"
    "  color: white;\n"
    "}\n"
    "entry text selection {\n"
    "  background-color: rgba(255, 255, 255, 0.3);\n"
    "  color: white;\n"
    "}\n"
    "box.page-dots {\n"
    "  padding: 30px;\n"
    "}\n"
    "button.page-dot {\n"
    "  background-color: rgba(255, 255, 255, 0.3);\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  border-radius: 50%;\n"
    "  min-width: 8px;\n"
    "  min-height: 8px;\n"
    "  margin: 0px 5px;\n"
    "  padding: 4px;\n"
    "}\n"
    "button.page-dot.active {\n"
    "  background-color: rgba(255, 255, 255, 0.9);\n"
    "}\n"
    "button.page-dot:hover {\n"
    "  background-color: rgba(255, 255, 255, 0.5);\n"
    "}\n"
    "button.folder {\n"
    "  background-color: rgba(255, 255, 255, 0.08);\n"
    "  background-image: none;\n"
    "  border: none;\n"
    "  padding: 15px;\n"
    "  margin: 10px;\n"
    "  border-radius: 16px;\n"
    "}\n"
    "button.folder:hover {\n"
    "  background-color: rgba(255, 255, 255, 0.15);\n"
    "}\n"
    "button.folder label {\n"
    "  color: rgba(255, 255, 255, 0.9);\n"
    "  font-size: 12px;\n"
    "  font-weight: 400;\n"
    "}\n";
}






































