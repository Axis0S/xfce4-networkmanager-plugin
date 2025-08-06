/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plugin.h"
#include "nm-interface.h"
#include "popup-window.h"

static void
on_button_clicked(GtkButton *button, NetworkManagerPlugin *nm_plugin)
{
    GdkRectangle rect;
    GtkAllocation allocation;
    
    /* Get button position for popup placement */
    gtk_widget_get_allocation(GTK_WIDGET(button), &allocation);
    gdk_window_get_origin(gtk_widget_get_window(GTK_WIDGET(button)), &rect.x, &rect.y);
    rect.x += allocation.x;
    rect.y += allocation.y;
    rect.width = allocation.width;
    rect.height = allocation.height;
    
    /* Toggle popup window */
    popup_window_toggle(nm_plugin->popup_window, &rect);
}

NetworkManagerPlugin *
networkmanager_plugin_new(XfcePanelPlugin *plugin)
{
    NetworkManagerPlugin *nm_plugin;
    GError *error = NULL;

    /* Allocate memory for the plugin structure */
    nm_plugin = g_new0(NetworkManagerPlugin, 1);
    nm_plugin->plugin = plugin;

    /* Initialize NetworkManager interface */
    nm_plugin->nm_interface = nm_interface_new();
    
    error = NULL;
    if (!nm_interface_init(nm_plugin->nm_interface, &error)) {
        GtkWidget *error_dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Failed to initialize NetworkManager interface.\n\n%s",
            error ? error->message : "Unknown error");
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_destroy(error_dialog);
        
        g_warning("Failed to initialize NetworkManager interface: %s", 
                  error ? error->message : "Unknown error");
        if (error)
            g_error_free(error);
        
        /* Continue with limited functionality */
        nm_plugin->nm_interface = NULL;
    }
    /* Create the panel button */
    nm_plugin->button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(nm_plugin->button), GTK_RELIEF_NONE);
    nm_plugin->icon = gtk_image_new_from_icon_name("network-wired", GTK_ICON_SIZE_MENU);
    gtk_container_add(GTK_CONTAINER(nm_plugin->button), nm_plugin->icon);
    gtk_widget_show(nm_plugin->icon);
    
    /* Connect button click signal */
    g_signal_connect(nm_plugin->button, "clicked",
                     G_CALLBACK(on_button_clicked), nm_plugin);
    
    /* Make button receive events */
    xfce_panel_plugin_add_action_widget(plugin, nm_plugin->button);

    /* Add the button to the plugin */
    gtk_container_add(GTK_CONTAINER(plugin), nm_plugin->button);
    gtk_widget_show(nm_plugin->button);
    
    /* Create popup window */
    nm_plugin->popup_window = (GtkWidget*)popup_window_new(nm_plugin);

    return nm_plugin;
}

void
networkmanager_plugin_free(XfcePanelPlugin *plugin, NetworkManagerPlugin *nm_plugin)
{
    g_clear_object(&nm_plugin->nm_interface);
    g_free(nm_plugin->current_connection);
    g_free(nm_plugin->current_ssid);

    gtk_widget_destroy(nm_plugin->button);
    g_free(nm_plugin);
}

void
networkmanager_plugin_save(XfcePanelPlugin *plugin, NetworkManagerPlugin *nm_plugin)
{
    // Save configuration settings
}

gboolean
networkmanager_plugin_size_changed(XfcePanelPlugin *plugin, gint size, NetworkManagerPlugin *nm_plugin)
{
    // Handle size change
    return TRUE;
}

void
networkmanager_plugin_configure(XfcePanelPlugin *plugin, NetworkManagerPlugin *nm_plugin)
{
    // Configure plugin settings
}

void
networkmanager_plugin_about(XfcePanelPlugin *plugin)
{
    const gchar *authors[] = { "XFCE4 NetworkManager Plugin Team", NULL };
    gtk_show_about_dialog(NULL,
                          "program-name", "XFCE4 Network Manager Plugin",
                          "version", "0.1.0",
                          "comments", "A plugin to manage network connections in XFCE4.",
                          "copyright", "Copyright © 2023",
                          "license-type", GTK_LICENSE_GPL_3_0,
                          "website", "https://github.com/yourusername/xfce4-networkmanager-plugin",
                          "authors", authors,
                          NULL);
}
