/*
 * XFCE Launcher - Full screen application launcher for XFCE
 * Similar to macOS Launchpad
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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <gio/gdesktopappinfo.h>
#include <string.h>
#include <math.h>

/* Plugin structure */
typedef struct {
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
    GList           *folder_list;     /* List of FolderInfo* */
    gint            current_page;
    gint            total_pages;
    gboolean        drag_mode;        /* Track if in drag mode */
    struct AppInfo  *drag_source;     /* App being dragged */
} LauncherPlugin;

/* Application info structure */
typedef struct {
    gchar *name;
    gchar *exec;
    gchar *icon;
    GDesktopAppInfo *desktop_info;
    gboolean is_hidden;  /* Track if app is hidden */
    gchar *folder_id;    /* ID of folder containing this app, NULL if not in folder */
} AppInfo;

/* Folder structure */
typedef struct {
    gchar *id;           /* Unique folder ID */
    gchar *name;         /* Folder display name */
    gchar *icon;         /* Folder icon name */
    GList *apps;         /* List of AppInfo* in this folder */
    gboolean is_open;    /* Track if folder is open in UI */
} FolderInfo;

/* Function prototypes */
static void launcher_construct(XfcePanelPlugin *plugin);
static void launcher_free(XfcePanelPlugin *plugin, LauncherPlugin *launcher);
static void launcher_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, LauncherPlugin *launcher);
static gboolean launcher_size_changed(XfcePanelPlugin *plugin, guint size, LauncherPlugin *launcher);
static void launcher_button_clicked(GtkWidget *button, LauncherPlugin *launcher);
static void create_overlay_window(LauncherPlugin *launcher);
static void hide_overlay(LauncherPlugin *launcher);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, LauncherPlugin *launcher);
static void on_swipe_gesture(GtkGestureSwipe *gesture, gdouble velocity_x, gdouble velocity_y, LauncherPlugin *launcher);
static void on_search_changed(GtkSearchEntry *entry, LauncherPlugin *launcher);
static void launch_application(GtkWidget *button, AppInfo *app_info);
static void free_app_info(AppInfo *app_info);
static void free_folder_info(FolderInfo *folder_info);
static GList* load_applications(void);
static gint compare_app_names(gconstpointer a, gconstpointer b);
static void update_page_dots(LauncherPlugin *launcher);
static void on_dot_clicked(GtkWidget *dot, gpointer data);
static void populate_current_page(LauncherPlugin *launcher);
static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, LauncherPlugin *launcher);
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                                  GtkSelectionData *data, guint info, guint time, LauncherPlugin *launcher);
static void on_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data,
                             guint info, guint time, AppInfo *app_info);
static gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, AppInfo *app_info);
static void hide_application(AppInfo *app_info, LauncherPlugin *launcher);
static gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, LauncherPlugin *launcher);
static FolderInfo* create_folder(const gchar *name);
static FolderInfo* find_folder_by_id(LauncherPlugin *launcher, const gchar *folder_id);
static void add_app_to_folder(LauncherPlugin *launcher, AppInfo *app, const gchar *folder_id);
static void remove_app_from_folder(LauncherPlugin *launcher, AppInfo *app);
static gchar* get_config_file_path(void);
static void save_configuration(LauncherPlugin *launcher);
static void load_configuration(LauncherPlugin *launcher);

/* Register the plugin */
XFCE_PANEL_PLUGIN_REGISTER(launcher_construct);

/* Plugin construction */
static void launcher_construct(XfcePanelPlugin *plugin)
{
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
launcher->icon = gtk_image_new_from_icon_name("view-app-grid", GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(launcher->button), launcher->icon);
    gtk_widget_show(launcher->icon);
    
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
    
    /* Show the panel button */
    gtk_widget_show(launcher->button);
    
    /* Load applications */
g_list_free_full(launcher->app_list, (GDestroyNotify)free_app_info);
    launcher->app_list = load_applications();
    launcher->filtered_list = g_list_copy(launcher->app_list);
    launcher->current_page = 0;
    
    /* Create overlay window (hidden initially) */
    create_overlay_window(launcher);
}

/* Free plugin resources */
static void launcher_free(XfcePanelPlugin *plugin, LauncherPlugin *launcher)
{
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
    
    /* Free the plugin structure */
    g_slice_free(LauncherPlugin, launcher);
}

/* Handle orientation changes */
static void launcher_orientation_changed(XfcePanelPlugin *plugin,
                                       GtkOrientation orientation,
                                       LauncherPlugin *launcher)
{
    /* Update button orientation if needed */
}

/* Handle size changes */
static gboolean launcher_size_changed(XfcePanelPlugin *plugin,
                                    guint size,
                                    LauncherPlugin *launcher)
{
    /* Update icon size */
    gtk_image_set_pixel_size(GTK_IMAGE(launcher->icon), size - 4);
    return TRUE;
}

/* Handle button click */
static void launcher_button_clicked(GtkWidget *button, LauncherPlugin *launcher)
{
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

/* Create the full-screen overlay window */
static void create_overlay_window(LauncherPlugin *launcher)
{
    GtkWidget *main_box, *search_box, *grid_container, *center_box;
    GdkScreen *screen;
    GdkVisual *visual;
    
    /* Create overlay window */
    launcher->overlay_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(launcher->overlay_window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_decorated(GTK_WINDOW(launcher->overlay_window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(launcher->overlay_window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(launcher->overlay_window), TRUE);
    gtk_window_fullscreen(GTK_WINDOW(launcher->overlay_window));
    
    /* Set window transparency */
    screen = gtk_widget_get_screen(launcher->overlay_window);
    visual = gdk_screen_get_rgba_visual(screen);
    if (visual && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(launcher->overlay_window, visual);
    }
    
    /* Style the window with semi-transparent background */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
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
        "}\n",
        -1,
        NULL);
    
    GtkStyleContext *context = gtk_widget_get_style_context(launcher->overlay_window);
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    /* Create main container */
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(launcher->overlay_window), main_box);
    
    /* Create search container */
    search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(search_box, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(search_box), "search-container");
    gtk_widget_set_size_request(search_box, 450, -1);
    gtk_box_pack_start(GTK_BOX(main_box), search_box, FALSE, FALSE, 0);
    
    /* Create search entry */
    launcher->search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(launcher->search_entry), "Search");
    gtk_widget_set_hexpand(launcher->search_entry, TRUE);
    gtk_box_pack_start(GTK_BOX(search_box), launcher->search_entry, TRUE, TRUE, 0);
    
    /* Connect search entry signals */
    g_signal_connect(launcher->search_entry, "search-changed",
                     G_CALLBACK(on_search_changed), launcher);
    
    /* Create center box for grid */
    center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), center_box, TRUE, TRUE, 0);
    
    /* Create grid container */
    grid_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(center_box), grid_container, FALSE, FALSE, 0);
    
    /* Create app grid */
    launcher->app_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(launcher->app_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(launcher->app_grid), 20);
    gtk_widget_set_halign(launcher->app_grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(launcher->app_grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(grid_container), launcher->app_grid, FALSE, FALSE, 0);
    
    /* Create page dots */
    launcher->page_dots = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(launcher->page_dots, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(launcher->page_dots), "page-dots");
    gtk_box_pack_start(GTK_BOX(main_box), launcher->page_dots, FALSE, FALSE, 0);
    
    /* Populate the grid with applications */
    populate_current_page(launcher);
    update_page_dots(launcher);
    
    /* Connect window key events for arrow navigation */
    g_signal_connect(launcher->overlay_window, "key-press-event",
                     G_CALLBACK(on_key_press), launcher);
    
/* Connect scroll event */
    g_signal_connect(launcher->overlay_window, "scroll-event",
                     G_CALLBACK(on_scroll_event), launcher);

    /* Connect drag and drop signals on grid */
    gtk_drag_dest_set(launcher->app_grid, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_MOVE);
    g_signal_connect(launcher->app_grid, "drag-drop",
                     G_CALLBACK(on_drag_drop), launcher);
    
    /* Add swipe gesture for touchpad */
    GtkGesture *swipe_gesture = gtk_gesture_swipe_new(launcher->overlay_window);
    gtk_gesture_single_set_touch_only(GTK_GESTURE_SINGLE(swipe_gesture), FALSE);
    g_signal_connect(swipe_gesture, "swipe",
                     G_CALLBACK(on_swipe_gesture), launcher);
    
    /* Store launcher reference in window */
    g_object_set_data(G_OBJECT(launcher->overlay_window), "launcher", launcher);
    
    /* Apply style to all app buttons */
    gtk_style_context_add_provider_for_screen(screen,
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(provider);
}

/* Hide overlay window */
static void hide_overlay(LauncherPlugin *launcher)
{
    if (launcher->overlay_window) {
        gtk_widget_hide(launcher->overlay_window);
        gtk_entry_set_text(GTK_ENTRY(launcher->search_entry), "");
        
        /* Reset filtered list and page */
        if (launcher->filtered_list) {
            g_list_free(launcher->filtered_list);
        }
        launcher->filtered_list = g_list_copy(launcher->app_list);
        launcher->current_page = 0;
    }
}

/* Handle key press events */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, LauncherPlugin *launcher)
{
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

/* Handle search text changes */
static void on_search_changed(GtkSearchEntry *entry, LauncherPlugin *launcher)
{
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(entry));
    GList *iter;
    
    /* Free old filtered list */
    if (launcher->filtered_list) {
        g_list_free(launcher->filtered_list);
    }
    
    /* Create new filtered list */
    launcher->filtered_list = NULL;
    
    if (strlen(search_text) == 0) {
        /* No search text, show all apps */
        launcher->filtered_list = g_list_copy(launcher->app_list);
    } else {
        /* Filter apps based on search */
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
    
    /* Reset to first page and update display */
    launcher->current_page = 0;
    populate_current_page(launcher);
    update_page_dots(launcher);
}

/* Launch application */
static void launch_application(GtkWidget *button, AppInfo *app_info)
{
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

/* Free AppInfo structure */
static void free_app_info(AppInfo *app_info)
{
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

/* Free FolderInfo structure */
static void free_folder_info(FolderInfo *folder_info)
{
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

/* Load installed applications */
static GList* load_applications(void)
{
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
    
    /* Sort applications by name */
    app_list = g_list_sort(app_list, compare_app_names);
    
    return app_list;
}

/* Compare function for sorting applications by name */
static gint compare_app_names(gconstpointer a, gconstpointer b)
{
    const AppInfo *app_a = (const AppInfo *)a;
    const AppInfo *app_b = (const AppInfo *)b;
    
    if (!app_a->name) return 1;
    if (!app_b->name) return -1;
    
    return g_utf8_collate(app_a->name, app_b->name);
}

/* Update page dots */
static void update_page_dots(LauncherPlugin *launcher)
{
    GList *children, *iter;
    gint i;
    
    /* Clear existing dots */
    children = gtk_container_get_children(GTK_CONTAINER(launcher->page_dots));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    /* Calculate total pages (6 columns x 5 rows = 30 apps per page) */
    gint apps_per_page = 30;
    gint total_apps = g_list_length(launcher->filtered_list);
    launcher->total_pages = (total_apps + apps_per_page - 1) / apps_per_page;
    
    /* Create dots */
    for (i = 0; i < launcher->total_pages; i++) {
        GtkWidget *dot = gtk_button_new();
        gtk_style_context_add_class(gtk_widget_get_style_context(dot), "page-dot");
        gtk_widget_set_can_focus(dot, FALSE);
        
        if (i == launcher->current_page) {
            gtk_style_context_add_class(gtk_widget_get_style_context(dot), "active");
        } else {
            gtk_style_context_remove_class(gtk_widget_get_style_context(dot), "active");
        }
        
        /* Make dots clickable */
        g_object_set_data(G_OBJECT(dot), "page-index", GINT_TO_POINTER(i));
        g_object_set_data(G_OBJECT(dot), "launcher", launcher);
        g_signal_connect(dot, "clicked", G_CALLBACK(on_dot_clicked), NULL);
        
        gtk_box_pack_start(GTK_BOX(launcher->page_dots), dot, FALSE, FALSE, 0);
        gtk_widget_show(dot);
    }
}

/* Populate current page */
static void populate_current_page(LauncherPlugin *launcher)
{
    GList *iter;
    gint row, col;
    gint apps_per_page = 30;
    gint start_index = launcher->current_page * apps_per_page;
    gint index = 0;

    /* Clear existing children */
    gtk_container_foreach(GTK_CONTAINER(launcher->app_grid),
                         (GtkCallback)gtk_widget_destroy, NULL);
    
    /* Add applications to grid */
    for (iter = launcher->filtered_list; iter != NULL && index < start_index + apps_per_page; iter = g_list_next(iter)) {
        AppInfo *app_info = (AppInfo *)iter->data;
        
        /* Skip hidden apps */
        if (app_info->is_hidden) {
            continue;
        }
        
        if (index >= start_index) {
            GtkWidget *button, *box, *icon, *label;
            
            /* Calculate grid position */
            gint grid_index = index - start_index;
            col = grid_index % 6;
            row = grid_index / 6;
            
            /* Create button */
            button = gtk_button_new();
            gtk_style_context_add_class(gtk_widget_get_style_context(button), "app-button");
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
            gtk_widget_set_size_request(button, 130, 130);
            
            /* Create vertical box */
            box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
            gtk_container_add(GTK_CONTAINER(button), box);
            
            /* Create icon */
            if (app_info->icon) {
icon = gtk_image_new_from_icon_name(app_info->icon, GTK_ICON_SIZE_DIALOG);
                gtk_image_set_pixel_size(GTK_IMAGE(icon), 64);
            } else {
                icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
                gtk_image_set_pixel_size(GTK_IMAGE(icon), 64);
            }
            gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
            
            /* Create label, ensure name is valid */
            if (app_info->name) {
                label = gtk_label_new(app_info->name);
            } else {
                g_warning("App name is NULL, skipping app");
                continue;
            }
            gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_label_set_max_width_chars(GTK_LABEL(label), 15);
            gtk_label_set_lines(GTK_LABEL(label), 2);
            gtk_widget_set_margin_start(label, 5);
            gtk_widget_set_margin_end(label, 5);
            gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
            
            /* Add drag-and-drop capability */
            gtk_drag_source_set(button, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_MOVE);
            gtk_drag_dest_set(button, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_MOVE);
            g_signal_connect(button, "drag-data-received",
                            G_CALLBACK(on_drag_data_received), launcher);
            g_signal_connect(button, "drag-data-get",
                            G_CALLBACK(on_drag_data_get), app_info);

            /* Connect click and right-click context menu signals */
            g_signal_connect(button, "button-press-event",
                            G_CALLBACK(on_button_press_event), app_info);
            g_signal_connect(button, "clicked",
                            G_CALLBACK(launch_application), app_info);
            
            /* Add to grid */
            gtk_grid_attach(GTK_GRID(launcher->app_grid), button, col, row, 1, 1);
            
            /* Store app_info and launcher reference */
            g_object_set_data(G_OBJECT(button), "app-info", app_info);
            g_object_set_data(G_OBJECT(button), "launcher", launcher);
            
            gtk_widget_show_all(button);
        }
        index++;
    }
}

/* Handle scroll events for page navigation */
static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, LauncherPlugin *launcher)
{
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
            /* Handle smooth scrolling (touchpad) */
            if (fabs(event->delta_x) > fabs(event->delta_y)) {
                /* Horizontal scrolling */
                if (event->delta_x < -0.3) {
                    /* Swipe right (go to previous page) */
                    if (launcher->current_page > 0) {
                        launcher->current_page--;
                        changed = TRUE;
                    }
                } else if (event->delta_x > 0.3) {
                    /* Swipe left (go to next page) */
                    if (launcher->current_page < launcher->total_pages - 1) {
                        launcher->current_page++;
                        changed = TRUE;
                    }
                }
            } else {
                /* Vertical scrolling */
                if (event->delta_y < -0.3) {
                    /* Scroll up (go to previous page) */
                    if (launcher->current_page > 0) {
                        launcher->current_page--;
                        changed = TRUE;
                    }
                } else if (event->delta_y > 0.3) {
                    /* Scroll down (go to next page) */
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

/* Handle drag-and-drop data received */
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
                                  GtkSelectionData *data, guint info, guint time, LauncherPlugin *launcher)
{
    AppInfo *app_info = g_object_get_data(G_OBJECT(widget), "app-info");
    FolderInfo *folder_info = find_folder_by_id(launcher, (const gchar *)data);

    if (app_info && folder_info) {
        add_app_to_folder(launcher, app_info, folder_info->id);
        populate_current_page(launcher);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}

/* Handle drag-and-drop data get */
static void on_drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *data,
                             guint info, guint time, AppInfo *app_info)
{
    if (app_info && app_info->folder_id) {
        gtk_selection_data_set_text(data, app_info->folder_id, -1);
    }
}

/* Helper structure to pass both app_info and launcher to hide callback */
typedef struct {
    AppInfo *app_info;
    LauncherPlugin *launcher;
} HideCallbackData;

/* Callback wrapper for hide menu item */
static void on_hide_menu_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    HideCallbackData *data = (HideCallbackData *)user_data;
    if (data && data->app_info && data->launcher) {
        hide_application(data->app_info, data->launcher);
    }
    g_free(data);
}

/* Handle button press for context menu */
static gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, AppInfo *app_info)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) { // Right-click
        GtkWidget *menu = gtk_menu_new();
        GtkWidget *hide_item = gtk_menu_item_new_with_label("Hide");
        LauncherPlugin *launcher = g_object_get_data(G_OBJECT(widget), "launcher");
        
        if (!launcher) {
            g_warning("Launcher reference not found in button data");
            return FALSE;
        }
        
        /* Create callback data with both app_info and launcher */
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

/* Hide application */
static void hide_application(AppInfo *app_info, LauncherPlugin *launcher)
{
    app_info->is_hidden = TRUE;
    /* Update UI immediately */
    populate_current_page(launcher);
    update_page_dots(launcher);
    save_configuration(launcher); // Save the configuration to reflect hidden state
}

/* Handle swipe gestures for page navigation */
static void on_swipe_gesture(GtkGestureSwipe *gesture, gdouble velocity_x, gdouble velocity_y, LauncherPlugin *launcher)
{
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

/* Handle dot click events */
static void on_dot_clicked(GtkWidget *dot, gpointer data)
{
    gint page_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dot), "page-index"));
    LauncherPlugin *launcher = g_object_get_data(G_OBJECT(dot), "launcher");
    
    if (launcher && page_index != launcher->current_page) {
        launcher->current_page = page_index;
        populate_current_page(launcher);
        update_page_dots(launcher);
    }
}

/* Handle drag drop event */
static gboolean on_drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, LauncherPlugin *launcher)
{
    /* This is called when something is dropped on the grid
     * We'll use it to create folders when apps are dropped on each other */

    AppInfo *target_app = NULL;
    GValue value = G_VALUE_INIT;

    /* Get the application at the drop location */
    gtk_container_child_get_property(GTK_CONTAINER(launcher->app_grid),
                                     gtk_grid_get_child_at(GTK_GRID(launcher->app_grid), x / 130, y / 130),
                                     "app-info", &value);

    if (G_VALUE_HOLDS_POINTER(&value)) {
        target_app = g_value_get_pointer(&value);
    }

    if (target_app && launcher->drag_source && launcher->drag_source != target_app) {
        FolderInfo *folder = create_folder("New Folder");
        add_app_to_folder(launcher, (AppInfo *)launcher->drag_source, folder->id);
        add_app_to_folder(launcher, (AppInfo *)target_app, folder->id);

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

/* Create a new folder */
static FolderInfo* create_folder(const gchar *name)
{
    FolderInfo *folder = g_new0(FolderInfo, 1);
    folder->id = g_strdup_printf("folder_%ld", g_get_monotonic_time());
    folder->name = g_strdup(name);
    folder->icon = g_strdup("folder");
    folder->apps = NULL;
    folder->is_open = FALSE;
    return folder;
}

/* Find folder by ID */
static FolderInfo* find_folder_by_id(LauncherPlugin *launcher, const gchar *folder_id)
{
    GList *iter;
    for (iter = launcher->folder_list; iter != NULL; iter = g_list_next(iter)) {
        FolderInfo *folder = (FolderInfo *)iter->data;
        if (g_strcmp0(folder->id, folder_id) == 0) {
            return folder;
        }
    }
    return NULL;
}

/* Add app to folder */
static void add_app_to_folder(LauncherPlugin *launcher, AppInfo *app, const gchar *folder_id)
{
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

/* Remove app from folder */
static void remove_app_from_folder(LauncherPlugin *launcher, AppInfo *app)
{
    if (app && app->folder_id) {
        FolderInfo *folder = find_folder_by_id(launcher, app->folder_id);
        if (folder) {
            folder->apps = g_list_remove(folder->apps, app);
        }
        g_free(app->folder_id);
        app->folder_id = NULL;
    }
}

/* Get config file path */
static gchar* get_config_file_path(void)
{
    return g_build_filename(g_get_user_config_dir(), "xfce4", "launcher", "config.xml", NULL);
}

/* Save configuration */
static void save_configuration(LauncherPlugin *launcher)
{
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

/* Load configuration */
static void load_configuration(LauncherPlugin *launcher)
{
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
