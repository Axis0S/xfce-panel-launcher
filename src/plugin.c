/*
 * XFCE Launcher Plugin - Main plugin file
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xfce-launcher.h"
#include "settings.h"

/* Forward declarations for internal functions */
static void launcher_construct(XfcePanelPlugin *plugin);
static void launcher_free(XfcePanelPlugin *plugin, LauncherPlugin *launcher);
static void launcher_configure_plugin(XfcePanelPlugin *plugin, LauncherPlugin *launcher);

/* Register the plugin */
XFCE_PANEL_PLUGIN_REGISTER(launcher_construct);

/* Plugin construction */
static void launcher_construct(XfcePanelPlugin *plugin) {
    LauncherPlugin *launcher;
    
    /* Allocate memory for the plugin structure */
    launcher = g_slice_new0(LauncherPlugin);
    launcher->plugin = plugin;
    
    /* Create the panel button */
    launcher->button = xfce_panel_create_button();
    gtk_button_set_relief(GTK_BUTTON(launcher->button), GTK_RELIEF_NONE);
    gtk_widget_set_name(launcher->button, "xfce-launcher-button");
    gtk_widget_show(launcher->button);
    
    /* Make panel button transparent */
    GtkCssProvider *button_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(button_provider,
        "#xfce-launcher-button {\n"
        "  background: transparent;\n"
        "  background-color: transparent;\n"
        "  background-image: none;\n"
        "  border: none;\n"
        "  outline: none;\n"
        "  padding: 0px;\n"
        "  margin: 0px;\n"
        "  min-width: 16px;\n"
        "  min-height: 16px;\n"
        "}\n"
        "#xfce-launcher-button:hover {\n"
        "  background-color: rgba(255, 255, 255, 0.1);\n"
        "  background-image: none;\n"
        "}\n"
        "#xfce-launcher-button:active {\n"
        "  background-color: rgba(255, 255, 255, 0.2);\n"
        "  background-image: none;\n"
        "}\n"
        ".xfce4-panel #xfce-launcher-button {\n"
        "  background: transparent;\n"
        "  background-color: transparent;\n"
        "}\n",
        -1,
        NULL);
    
    GdkScreen *screen = gtk_widget_get_screen(launcher->button);
    gtk_style_context_add_provider_for_screen(screen,
                                             GTK_STYLE_PROVIDER(button_provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    
    GtkStyleContext *button_context = gtk_widget_get_style_context(launcher->button);
    gtk_style_context_add_provider(button_context,
                                   GTK_STYLE_PROVIDER(button_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    g_object_unref(button_provider);
    
    /* Create icon */
    launcher->icon = gtk_image_new_from_icon_name("xfce-launcher", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(launcher->button), launcher->icon);
    gtk_widget_show(launcher->icon);
    
    /* Initialize settings */
    launcher_settings_init(launcher);
    
    /* Connect button click signal */
    g_signal_connect(G_OBJECT(launcher->button), "clicked",
                     G_CALLBACK(launcher_button_clicked), launcher);
    
    /* Add button to panel */
    gtk_container_add(GTK_CONTAINER(plugin), launcher->button);
    
    /* Connect plugin signals */
    g_signal_connect(G_OBJECT(plugin), "free-data",
                     G_CALLBACK(launcher_free), launcher);
    g_signal_connect(G_OBJECT(plugin), "size-changed",
                     G_CALLBACK(launcher_size_changed), launcher);
    g_signal_connect(G_OBJECT(plugin), "orientation-changed",
                     G_CALLBACK(launcher_orientation_changed), launcher);
    g_signal_connect(G_OBJECT(plugin), "configure-plugin",
                     G_CALLBACK(launcher_configure_plugin), launcher);
    
    /* Show the panel button */
    gtk_widget_show(launcher->button);
    
    /* Enable context menu for properties */
    xfce_panel_plugin_menu_show_configure(plugin);
    
    /* Load applications */
    g_list_free_full(launcher->app_list, (GDestroyNotify)free_app_info);
    launcher->app_list = load_applications_enhanced();
    launcher->filtered_list = g_list_copy(launcher->app_list);
    launcher->current_page = 0;
    
    /* Setup application monitoring for automatic refresh */
    setup_application_monitoring(launcher);
    
    /* Create overlay window (hidden initially) */
    create_overlay_window(launcher);
    
    /* Store launcher reference in overlay window */
    g_object_set_data(G_OBJECT(launcher->overlay_window), "launcher", launcher);
    
    /* Add swipe gesture for touchpad */
    GtkGesture *swipe_gesture = gtk_gesture_swipe_new(launcher->overlay_window);
    gtk_gesture_single_set_touch_only(GTK_GESTURE_SINGLE(swipe_gesture), FALSE);
    g_signal_connect(swipe_gesture, "swipe",
                     G_CALLBACK(on_swipe_gesture), launcher);
    
    /* Connect drag and drop signals on grid */
    gtk_drag_dest_set(launcher->app_grid, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_MOVE);
    g_signal_connect(launcher->app_grid, "drag-drop",
                     G_CALLBACK(on_drag_drop), launcher);
}

/* Free plugin resources */
static void launcher_free(XfcePanelPlugin *plugin, LauncherPlugin *launcher) {
    /* Destroy overlay window */
    if (launcher->overlay_window)
        gtk_widget_destroy(launcher->overlay_window);
    
    /* Free application list */
    if (launcher->app_list) {
        g_list_free_full(launcher->app_list, (GDestroyNotify)free_app_info);
    }
    if (launcher->filtered_list) {
        g_list_free(launcher->filtered_list);
    }
    
    /* Free folder list */
    if (launcher->folder_list) {
        g_list_free_full(launcher->folder_list, (GDestroyNotify)free_folder_info);
    }
    
    /* Free settings resources */
    launcher_settings_free(launcher);
    
    /* Free the plugin structure */
    g_slice_free(LauncherPlugin, launcher);
}

/* Handle orientation changes */
void launcher_orientation_changed(XfcePanelPlugin *plugin,
                                 GtkOrientation orientation,
                                 LauncherPlugin *launcher) {
    /* Update button orientation if needed */
}

/* Handle size changes */
gboolean launcher_size_changed(XfcePanelPlugin *plugin,
                              guint size,
                              LauncherPlugin *launcher) {
    /* Update icon size */
    gtk_image_set_pixel_size(GTK_IMAGE(launcher->icon), size - 4);
    return TRUE;
}

/* Handle configure request */
static void launcher_configure_plugin(XfcePanelPlugin *plugin, LauncherPlugin *launcher) {
    launcher_show_settings_dialog(launcher);
}

/* Handle button click */
void launcher_button_clicked(GtkWidget *button, LauncherPlugin *launcher) {
    if (launcher->overlay_window) {
        /* Reset to first page when opening */
        launcher->current_page = 0;
        populate_current_page(launcher);
        update_page_dots(launcher);
        
        gtk_widget_show_all(launcher->overlay_window);
        gtk_window_present(GTK_WINDOW(launcher->overlay_window));
        gtk_widget_grab_focus(launcher->search_entry);
    }
}
