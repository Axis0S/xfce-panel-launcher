/*
 * Settings management implementation for XFCE Launcher
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

#include "settings.h"

/* Initialize settings management */
void launcher_settings_init(LauncherPlugin *launcher) {
    gchar *property_base;
    const gchar *icon_name;
    
    /* Initialize Xfconf if not already done */
    if (!xfconf_init(NULL)) {
        g_warning("Failed to initialize Xfconf");
        return;
    }
    
    /* Get the property base for this plugin instance */
    property_base = g_strdup_printf("/plugins/plugin-%d", 
                                   xfce_panel_plugin_get_unique_id(launcher->plugin));
    
    /* Create channel for this plugin */
    launcher->channel = xfconf_channel_new_with_property_base("xfce4-panel",
                                                              property_base);
    g_free(property_base);
    
    /* Load current icon setting or use default */
    icon_name = xfconf_channel_get_string(launcher->channel, SETTING_ICON_NAME, DEFAULT_ICON_NAME);
    
    /* Update the icon widget - check if it's a file path or icon name */
    if (icon_name && g_file_test(icon_name, G_FILE_TEST_EXISTS)) {
        /* It's a file path */
        gtk_image_set_from_file(GTK_IMAGE(launcher->icon), icon_name);
    } else {
        /* It's an icon name */
        gtk_image_set_from_icon_name(GTK_IMAGE(launcher->icon), icon_name, GTK_ICON_SIZE_BUTTON);
    }
}

/* Free settings resources */
void launcher_settings_free(LauncherPlugin *launcher) {
    if (launcher->channel) {
        g_object_unref(launcher->channel);
        launcher->channel = NULL;
    }
}

/* Get current icon name */
gchar* launcher_settings_get_icon_name(LauncherPlugin *launcher) {
    if (!launcher->channel)
        return g_strdup(DEFAULT_ICON_NAME);
        
    return xfconf_channel_get_string(launcher->channel, SETTING_ICON_NAME, DEFAULT_ICON_NAME);
}

/* Set icon name */
void launcher_settings_set_icon_name(LauncherPlugin *launcher, const gchar *icon_name) {
    if (!launcher->channel)
        return;
        
    /* Save to Xfconf */
    xfconf_channel_set_string(launcher->channel, SETTING_ICON_NAME, 
                             icon_name ? icon_name : DEFAULT_ICON_NAME);
    
    /* Update the icon widget - check if it's a file path or icon name */
    if (icon_name && g_file_test(icon_name, G_FILE_TEST_EXISTS)) {
        /* It's a file path */
        gtk_image_set_from_file(GTK_IMAGE(launcher->icon), icon_name);
    } else {
        /* It's an icon name */
        gtk_image_set_from_icon_name(GTK_IMAGE(launcher->icon), 
                                    icon_name ? icon_name : DEFAULT_ICON_NAME, 
                                    GTK_ICON_SIZE_BUTTON);
    }
    
    /* Update icon size based on panel size */
    gint size = xfce_panel_plugin_get_size(launcher->plugin);
    gtk_image_set_pixel_size(GTK_IMAGE(launcher->icon), size - 4);
}

/* Helper to create icon list store with larger icons */
static GtkListStore* create_icon_store(void) {
    GtkListStore *store;
    GtkTreeIter iter;
    GtkIconTheme *icon_theme;
    const gchar *common_icons[] = {
        "application-x-executable",
        "applications-other",
        "applications-system",
        "applications-utilities",
        "preferences-system",
        "system-run",
        "view-grid-symbolic",
        "view-app-grid-symbolic",
        "application-menu",
        "show-apps",
        "xfce-launcher",
        "xfce4-whiskermenu",
        "start-here",
        "distributor-logo",
        "applications-all",
        "applications-accessories",
        "applications-development",
        "applications-games",
        "applications-graphics",
        "applications-internet",
        "applications-multimedia",
        "applications-office",
        "applications-science",
        "folder-applications",
        NULL
    };
    int i;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, GDK_TYPE_PIXBUF);
    icon_theme = gtk_icon_theme_get_default();
    
    for (i = 0; common_icons[i] != NULL; i++) {
        GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(icon_theme, common_icons[i], 48, 0, NULL);
        if (pixbuf) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                             0, common_icons[i],
                             1, pixbuf,
                             -1);
            g_object_unref(pixbuf);
        }
    }
    
    return store;
}

/* Icon selection changed */
static void on_icon_selection_changed(GtkIconView *icon_view, LauncherPlugin *launcher) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *selected;
    gchar *icon_name;
    
    selected = gtk_icon_view_get_selected_items(icon_view);
    if (selected) {
        model = gtk_icon_view_get_model(icon_view);
        if (gtk_tree_model_get_iter(model, &iter, selected->data)) {
            gtk_tree_model_get(model, &iter, 0, &icon_name, -1);
            launcher_settings_set_icon_name(launcher, icon_name);
            g_free(icon_name);
        }
        g_list_free_full(selected, (GDestroyNotify)gtk_tree_path_free);
    }
}

/* Browse for custom icon from file */
static void on_browse_clicked(GtkWidget *button, gpointer data) {
    GtkWidget *dialog;
    GtkWidget *entry = GTK_WIDGET(data);
    GtkWidget *parent_dialog = gtk_widget_get_toplevel(button);
    gint res;
    
    dialog = gtk_file_chooser_dialog_new("Select Icon File",
                                       GTK_WINDOW(parent_dialog),
                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_Open", GTK_RESPONSE_ACCEPT,
                                       NULL);
    
    /* Add file filters */
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image files");
    gtk_file_filter_add_mime_type(filter, "image/svg+xml");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_pattern(filter, "*.svg");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);
    
    /* Set default folder to common icon locations */
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "/usr/share/icons");
    
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(entry), filename);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

/* Icon button clicked - show icon chooser */
static void on_icon_button_clicked(GtkWidget *button, LauncherPlugin *launcher) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *scrolled;
    GtkWidget *icon_view;
    GtkListStore *store;
    GtkWidget *entry;
    GtkWidget *vbox, *hbox;
    GtkWidget *label;
    GtkWidget *browse_button;
    gchar *current_icon;
    GtkTreeIter iter;
    gboolean valid;
    
    /* Create dialog */
    dialog = gtk_dialog_new_with_buttons("Select Icon",
                                       GTK_WINDOW(gtk_widget_get_toplevel(button)),
                                       GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_OK", GTK_RESPONSE_OK,
                                       NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 550, 450);
    
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);
    
    /* Add label */
    label = gtk_label_new("Select an icon from the list below:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    /* Create scrolled window */
    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 250);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    /* Create icon view with larger spacing */
    store = create_icon_store();
    icon_view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view), 1);
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(icon_view), 0);
    gtk_icon_view_set_item_width(GTK_ICON_VIEW(icon_view), 120);
    gtk_icon_view_set_column_spacing(GTK_ICON_VIEW(icon_view), 10);
    gtk_icon_view_set_row_spacing(GTK_ICON_VIEW(icon_view), 10);
    gtk_icon_view_set_margin(GTK_ICON_VIEW(icon_view), 10);
    gtk_container_add(GTK_CONTAINER(scrolled), icon_view);
    
    /* Select current icon if it's in the list */
    current_icon = launcher_settings_get_icon_name(launcher);
    if (current_icon && !g_file_test(current_icon, G_FILE_TEST_EXISTS)) {
        /* Only try to select if it's an icon name, not a file path */
        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
        while (valid) {
            gchar *icon_name;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &icon_name, -1);
            if (g_strcmp0(icon_name, current_icon) == 0) {
                GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
                gtk_icon_view_select_path(GTK_ICON_VIEW(icon_view), path);
                gtk_icon_view_scroll_to_path(GTK_ICON_VIEW(icon_view), path, TRUE, 0.5, 0.5);
                gtk_tree_path_free(path);
                g_free(icon_name);
                break;
            }
            g_free(icon_name);
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
        }
    }
    
    /* Add separator */
    gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);
    
    /* Add custom icon entry with browse button */
    label = gtk_label_new("Or enter a custom icon name or file path:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), current_icon ? current_icon : "");
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
    
    browse_button = gtk_button_new_with_label("Browse...");
    gtk_box_pack_start(GTK_BOX(hbox), browse_button, FALSE, FALSE, 0);
    g_signal_connect(browse_button, "clicked", G_CALLBACK(on_browse_clicked), entry);
    
    g_free(current_icon);
    
    /* Store references */
    g_object_set_data(G_OBJECT(dialog), "icon-view", icon_view);
    g_object_set_data(G_OBJECT(dialog), "entry", entry);
    g_object_set_data(G_OBJECT(dialog), "launcher", launcher);
    
    /* Connect selection signal */
    g_signal_connect(icon_view, "selection-changed",
                     G_CALLBACK(on_icon_selection_changed), launcher);
    
    /* Show dialog */
    gtk_widget_show_all(dialog);
    
    /* Run dialog */
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        /* Use custom entry if no icon selected */
        GList *selected = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(icon_view));
        if (!selected) {
            const gchar *custom_icon = gtk_entry_get_text(GTK_ENTRY(entry));
            if (custom_icon && *custom_icon) {
                launcher_settings_set_icon_name(launcher, custom_icon);
            }
        } else {
            g_list_free_full(selected, (GDestroyNotify)gtk_tree_path_free);
        }
    }
    
    /* Cleanup */
    g_object_unref(store);
    gtk_widget_destroy(dialog);
}

/* Reset icon to default */
static void on_reset_clicked(GtkWidget *button, gpointer data) {
    LauncherPlugin *launcher = (LauncherPlugin *)data;
    GtkWidget *icon_image = g_object_get_data(G_OBJECT(button), "icon-image");
    
    launcher_settings_set_icon_name(launcher, DEFAULT_ICON_NAME);
    gtk_image_set_from_icon_name(GTK_IMAGE(icon_image), DEFAULT_ICON_NAME, GTK_ICON_SIZE_DIALOG);
    gtk_image_set_pixel_size(GTK_IMAGE(icon_image), 48);
}

/* Show settings dialog */
void launcher_show_settings_dialog(LauncherPlugin *launcher) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *vbox, *hbox;
    GtkWidget *label;
    GtkWidget *icon_button;
    GtkWidget *icon_image;
    GtkWidget *reset_button;
    gchar *current_icon;
    
    /* Create dialog */
    dialog = gtk_dialog_new_with_buttons("Launcher Settings",
                                        GTK_WINDOW(gtk_widget_get_toplevel(launcher->button)),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        "_Close", GTK_RESPONSE_CLOSE,
                                        NULL);
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    
    /* Get content area */
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    /* Create main vbox */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);
    
/* Create icon selection row */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    /* Label */
    label = gtk_label_new("Panel Icon:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    
    /* Icon button with current icon */
    icon_button = gtk_button_new();
    current_icon = launcher_settings_get_icon_name(launcher);
    
    /* Create icon image - check if it's a file or icon name */
    if (current_icon && g_file_test(current_icon, G_FILE_TEST_EXISTS)) {
        icon_image = gtk_image_new_from_file(current_icon);
    } else {
        icon_image = gtk_image_new_from_icon_name(current_icon ? current_icon : DEFAULT_ICON_NAME, 
                                                  GTK_ICON_SIZE_DIALOG);
    }
    gtk_image_set_pixel_size(GTK_IMAGE(icon_image), 48);
    gtk_container_add(GTK_CONTAINER(icon_button), icon_image);
    gtk_widget_set_tooltip_text(icon_button, "Click to choose icon");
    g_free(current_icon);
    
    gtk_box_pack_start(GTK_BOX(hbox), icon_button, FALSE, FALSE, 0);
    
    /* Reset button */
    reset_button = gtk_button_new_with_label("Reset to Default");
    gtk_box_pack_start(GTK_BOX(hbox), reset_button, FALSE, FALSE, 0);
    
    /* Store icon image reference for reset button callback */
    g_object_set_data(G_OBJECT(reset_button), "icon-image", icon_image);
    
    /* Connect signals */
    g_signal_connect(icon_button, "clicked",
                     G_CALLBACK(on_icon_button_clicked), launcher);
    g_signal_connect(reset_button, "clicked",
                     G_CALLBACK(on_reset_clicked), launcher);
    
    /* Show dialog */
    gtk_widget_show_all(dialog);
    
    /* Run dialog */
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    /* Cleanup */
    gtk_widget_destroy(dialog);
}

