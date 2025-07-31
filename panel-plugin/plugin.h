/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __NETWORKMANAGER_PLUGIN_H__
#define __NETWORKMANAGER_PLUGIN_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>

G_BEGIN_DECLS

/* Plugin structure */
typedef struct _NetworkManagerPlugin
{
    XfcePanelPlugin *plugin;
    
    /* Plugin widgets */
    GtkWidget       *button;
    GtkWidget       *icon;
    GtkWidget       *label;
    GtkWidget       *popup_window;
    
    /* NetworkManager interface */
    gpointer         nm_interface;
    
    /* Current connection info */
    gchar           *current_connection;
    gchar           *current_ssid;
    gint             signal_strength;
    
    /* Settings */
    gboolean         show_label;
    gboolean         show_notifications;
    gint             transparency;
    gint             scan_interval;
    
    /* Update timeout */
    guint            update_timer;
    
} NetworkManagerPlugin;

/* Function prototypes */
NetworkManagerPlugin *networkmanager_plugin_new                (XfcePanelPlugin *plugin);
void                  networkmanager_plugin_free               (XfcePanelPlugin *plugin,
                                                               NetworkManagerPlugin *nm_plugin);
void                  networkmanager_plugin_save               (XfcePanelPlugin *plugin,
                                                               NetworkManagerPlugin *nm_plugin);
gboolean              networkmanager_plugin_size_changed       (XfcePanelPlugin *plugin,
                                                               gint size,
                                                               NetworkManagerPlugin *nm_plugin);
void                  networkmanager_plugin_configure          (XfcePanelPlugin *plugin,
                                                               NetworkManagerPlugin *nm_plugin);
void                  networkmanager_plugin_about              (XfcePanelPlugin *plugin);

G_END_DECLS

#endif /* !__NETWORKMANAGER_PLUGIN_H__ */
