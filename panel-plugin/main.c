/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
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

#include "plugin.h"
#include "nm-interface.h"

static void
networkmanager_plugin_construct(XfcePanelPlugin *plugin)
{
    NetworkManagerPlugin *nm_plugin;
    
    /* Setup translation domain */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    
    /* Create the plugin structure */
    nm_plugin = networkmanager_plugin_new(plugin);
    
    /* Show the configure menu item */
    xfce_panel_plugin_menu_show_configure(plugin);
    
    /* Show the about menu item */
    xfce_panel_plugin_menu_show_about(plugin);
    
    /* Connect plugin signals */
    g_signal_connect(G_OBJECT(plugin), "free-data",
                     G_CALLBACK(networkmanager_plugin_free), nm_plugin);
    
    g_signal_connect(G_OBJECT(plugin), "save",
                     G_CALLBACK(networkmanager_plugin_save), nm_plugin);
    
    g_signal_connect(G_OBJECT(plugin), "size-changed",
                     G_CALLBACK(networkmanager_plugin_size_changed), nm_plugin);
    
    g_signal_connect(G_OBJECT(plugin), "configure-plugin",
                     G_CALLBACK(networkmanager_plugin_configure), nm_plugin);
    
    g_signal_connect(G_OBJECT(plugin), "about",
                     G_CALLBACK(networkmanager_plugin_about), NULL);
}

XFCE_PANEL_PLUGIN_REGISTER(networkmanager_plugin_construct);
