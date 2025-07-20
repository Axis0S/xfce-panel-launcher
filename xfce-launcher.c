/*
 * XFCE Launcher - Full screen application launcher for XFCE
 * Similar to macOS Launchpad
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
    gint            current_page;
    gint            total_pages;
} LauncherPlugin;

/* Application info structure */
typedef struct {
    gchar *name;
    gchar *exec;
    gchar *icon;
    GDesktopAppInfo *app_info;
} AppInfo;

/* Function prototypes */
static void launcher_construct(XfcePanelPlugin *plugin);
static void launcher_free(XfcePanelPlugin *plugin, LauncherPlugin *launcher);
static void launcher_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, LauncherPlugin *launcher);
static gboolean launcher_size_changed(XfcePanelPlugin *plugin, guint size, LauncherPlugin *launcher);
static void launcher_button_clicked(GtkWidget *button, LauncherPlugin *launcher);
static void create_overlay_window(LauncherPlugin *launcher);
static void hide_overlay(LauncherPlugin *launcher);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, LauncherPlugin *launcher);
static gboolean on_right_left_key(GtkWidget *widget, GdkEventKey *event, LauncherPlugin *launcher);
static void on_search_changed(GtkSearchEntry *entry, LauncherPlugin *launcher);
static void launch_application(GtkWidget *button, AppInfo *app_info);
static void free_app_info(AppInfo *app_info);
static GList* load_applications(void);
static gint compare_app_names(gconstpointer a, gconstpointer b);
static void update_page_dots(LauncherPlugin *launcher);
static void on_dot_clicked(GtkWidget *dot, gpointer data);
static void populate_current_page(LauncherPlugin *launcher);
static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, LauncherPlugin *launcher);

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
    gtk_widget_show(launcher->button);
    
    /* Make panel button transparent */
    GtkCssProvider *button_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(button_provider,
        ".xfce4-panel button {\n"
        "  background: transparent;\n"
        "  background-image: none;\n"
        "  border: none;\n"
        "  box-shadow: none;\n"
        "  padding: 2px;\n"
        "}\n"
        ".xfce4-panel button:hover {\n"
        "  background: rgba(255, 255, 255, 0.1);\n"
        "}\n",
        -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(launcher->button),
                                   GTK_STYLE_PROVIDER(button_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(button_provider);
    
    /* Create icon */
launcher-icon = gtk_image_new_from_icon_name("view-app-grid", GTK_ICON_SIZE_BUTTON);
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
        "  background: radial-gradient(ellipse at center, rgba(40, 40, 40, 0.85) 0%, rgba(20, 20, 20, 0.95) 100%);\n"
        "  backdrop-filter: blur(50px);\n"
        "}\n"
        "button.app-button {\n"
        "  background: transparent;\n"
        "  background-image: none;\n"
        "  border: none;\n"
        "  outline: none;\n"
        "  box-shadow: none;\n"
        "  padding: 15px;\n"
        "  margin: 10px;\n"
        "  border-radius: 16px;\n"
        "  transition: all 200ms cubic-bezier(0.25, 0.46, 0.45, 0.94);\n"
        "}\n"
        "button.app-button:hover {\n"
        "  background-color: rgba(255, 255, 255, 0.1);\n"
        "  transform: scale(1.05);\n"
        "}\n"
        "button.app-button:active {\n"
        "  transform: scale(0.98);\n"
        "}\n"
        "button.app-button:focus {\n"
        "  outline: none;\n"
        "  box-shadow: none;\n"
        "}\n"
        "button.app-button label {\n"
        "  color: rgba(255, 255, 255, 0.9);\n"
        "  font-size: 12px;\n"
        "  font-weight: 400;\n"
        "  text-shadow: 0 1px 3px rgba(0, 0, 0, 0.5);\n"
        "}\n"
        ".search-container {\n"
        "  background: rgba(255, 255, 255, 0.15);\n"
        "  border-radius: 12px;\n"
        "  border: 1px solid rgba(255, 255, 255, 0.2);\n"
        "  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);\n"
        "  margin: 40px;\n"
        "  backdrop-filter: blur(20px);\n"
        "}\n"
        "entry {\n"
        "  background: transparent;\n"
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
        "entry::placeholder {\n"
        "  color: rgba(255, 255, 255, 0.5);\n"
        "}\n"
        "box.page-dots {\n"
        "  padding: 30px;\n"
        "}\n"
        "button.page-dot {\n"
        "  background: rgba(255, 255, 255, 0.3);\n"
        "  border: none;\n"
        "  border-radius: 50%;\n"
        "  min-width: 8px;\n"
        "  min-height: 8px;\n"
        "  max-width: 8px;\n"
        "  max-height: 8px;\n"
        "  margin: 0 5px;\n"
        "  padding: 4px;\n"
        "  transition: all 200ms ease;\n"
        "}\n"
        "button.page-dot.active {\n"
        "  background: rgba(255, 255, 255, 0.9);\n"
        "}\n"
        "button.page-dot:hover {\n"
        "  background: rgba(255, 255, 255, 0.5);\n"
        "}\n",
        -1, NULL);
    
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
    g_signal_connect(launcher-overlay_window, "key-press-event",
                     G_CALLBACK(on_key_press), launcher);
    
    /* Connect scroll event */
    g_signal_connect(launcher->overlay_window, "scroll-event",
                     G_CALLBACK(on_scroll_event), launcher);
    
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
    switch (event-keyval) {
        case GDK_KEY_Escape:
            hide_overlay(launcher);
            return TRUE;
        case GDK_KEY_Right:
            if (launcher-current_page < launcher-total_pages - 1) {
                launcher-current_page++;
                populate_current_page(launcher);
                update_page_dots(launcher);
                return TRUE;
            }
            break;
        case GDK_KEY_Left:
            if (launcher-current_page > 0) {
                launcher-current_page--;
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
            if (app_info && app_info->name) {
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
    
    if (app_info && app_info->app_info) {
        g_app_info_launch(G_APP_INFO(app_info->app_info), NULL, NULL, &error);
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
        if (app_info->app_info)
            g_object_unref(app_info->app_info);
        g_free(app_info);
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
            
            app_info->app_info = G_DESKTOP_APP_INFO(g_object_ref(gapp_info));
            
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
        if (index >= start_index) {
            AppInfo *app_info = (AppInfo *)iter->data;
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
icon = gtk_image_new_from_icon_name(launcher-icon, GTK_ICON_SIZE_DIALOG);
                gtk_image_set_pixel_size(GTK_IMAGE(icon), 64);
            } else {
                icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
                gtk_image_set_pixel_size(GTK_IMAGE(icon), 64);
            }
            gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
            
            /* Create label */
            label = gtk_label_new(app_info->name);
            gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_label_set_max_width_chars(GTK_LABEL(label), 15);
            gtk_label_set_lines(GTK_LABEL(label), 2);
            gtk_widget_set_margin_start(label, 5);
            gtk_widget_set_margin_end(label, 5);
            gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
            
            /* Connect click signal */
            g_signal_connect(button, "clicked",
                            G_CALLBACK(launch_application), app_info);
            
            /* Add to grid */
            gtk_grid_attach(GTK_GRID(launcher->app_grid), button, col, row, 1, 1);
            
            /* Store app_info reference for search */
            g_object_set_data(G_OBJECT(button), "app-info", app_info);
            
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
