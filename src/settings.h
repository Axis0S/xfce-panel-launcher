/*
 * Settings management for XFCE Launcher
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

#ifndef XFCE_LAUNCHER_SETTINGS_H
#define XFCE_LAUNCHER_SETTINGS_H

#include <xfconf/xfconf.h>
#include "xfce-launcher.h"

/* Settings property names */
#define XFCE_LAUNCHER_CHANNEL_NAME "xfce4-panel-launcher"
#define SETTING_ICON_NAME "/icon-name"

/* Default values */
#define DEFAULT_ICON_NAME "xfce-launcher"

/* Settings functions */
void launcher_settings_init(LauncherPlugin *launcher);
void launcher_settings_free(LauncherPlugin *launcher);
gchar* launcher_settings_get_icon_name(LauncherPlugin *launcher);
void launcher_settings_set_icon_name(LauncherPlugin *launcher, const gchar *icon_name);
void launcher_show_settings_dialog(LauncherPlugin *launcher);

#endif /* XFCE_LAUNCHER_SETTINGS_H */
