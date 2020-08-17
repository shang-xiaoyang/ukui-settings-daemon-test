/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef __USD_A11Y_SETTINGS_PLUGIN_H__
#define __USD_A11Y_SETTINGS_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include "ukui-settings-plugin.h"

G_BEGIN_DECLS

#define USD_TYPE_A11Y_SETTINGS_PLUGIN                (usd_a11y_settings_plugin_get_type ())
#define USD_A11Y_SETTINGS_PLUGIN(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_A11Y_SETTINGS_PLUGIN, UsdA11ySettingsPlugin))
#define USD_A11Y_SETTINGS_PLUGIN_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_A11Y_SETTINGS_PLUGIN, UsdA11ySettingsPluginClass))
#define USD_IS_A11Y_SETTINGS_PLUGIN(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_A11Y_SETTINGS_PLUGIN))
#define USD_IS_A11Y_SETTINGS_PLUGIN_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_A11Y_SETTINGS_PLUGIN))
#define USD_A11Y_SETTINGS_PLUGIN_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_A11Y_SETTINGS_PLUGIN, UsdA11ySettingsPluginClass))

typedef struct UsdA11ySettingsPluginPrivate UsdA11ySettingsPluginPrivate;

typedef struct
{
        UkuiSettingsPlugin           parent;
        UsdA11ySettingsPluginPrivate *priv;
} UsdA11ySettingsPlugin;

typedef struct
{
        UkuiSettingsPluginClass parent_class;
} UsdA11ySettingsPluginClass;

GType   usd_a11y_settings_plugin_get_type            (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_ukui_settings_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __USD_A11Y_SETTINGS_PLUGIN_H__ */
