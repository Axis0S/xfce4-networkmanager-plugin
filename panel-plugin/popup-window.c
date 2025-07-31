/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include config.h
#endif

#include "popup-window.h"
#include "nm-interface.h"
#include <string.h>

#define POPUP_UPDATE_INTERVAL 5000  /* Update every 5 seconds */
#define POPUP_WIDTH 320
#define POPUP_HEIGHT 400

/* Forward declarations */
static void on_search_changed(GtkSearchEntry *entry, PopupWindow *popup);
static GtkWidget *create_network_list_item(const gchar *ssid, gint strength, gboolean is_secure, gboolean is_connected);
static void on_network_item_clicked(GtkListBoxRow *row, gpointer user_data);

PopupWindow *
popup_window_new(NetworkManagerPlugin *plugin)
{
    PopupWindow *popup;

    popup = g_new0(PopupWindow, 1);
    popup->plugin = plugin;
    popup->nm_interface = plugin->nm_interface;

    popup->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(popup->window),
                             GDK_WINDOW_TYPE_HINT_POPUP_MENU);

    popup->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(popup->window), popup->main_box);

    popup->header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(popup->main_box), popup->header_box, FALSE, FALSE, 0);

    popup->search_entry = gtk_search_entry_new();
    gtk_box_pack_end(GTK_BOX(popup->header_box), popup->search_entry, FALSE, FALSE, 0);

    popup->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(popup->main_box), popup->scrolled_window, TRUE, TRUE, 0);

    popup->network_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(popup->network_list), GTK_SELECTION_SINGLE);
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(popup->network_list), TRUE);
    gtk_container_add(GTK_CONTAINER(popup->scrolled_window), popup->network_list);

    g_signal_connect(popup->search_entry, "search-changed",
                     G_CALLBACK(on_search_changed), popup);
    g_signal_connect(popup->network_list, "row-activated",
                     G_CALLBACK(on_network_item_clicked), popup);

    return popup;
}

void
popup_window_free(PopupWindow *popup)
{
    gtk_widget_destroy(popup->window);
    g_free(popup->filter_text);
    g_free(popup);
}

void
popup_window_show(PopupWindow *popup, GdkRectangle *button_rect)
{
    /* Reposition the popup window based on the button position */
    gtk_window_move(GTK_WINDOW(popup->window),
                    button_rect->x, button_rect->y + button_rect->height);
    gtk_widget_show_all(popup->window);

    popup_window_update_networks(popup);

    /* Schedule regular updates of the network list */
    popup->update_timer = g_timeout_add(POPUP_UPDATE_INTERVAL,
                                        (GSourceFunc)popup_window_update_networks,
                                        popup);
}

void
popup_window_hide(PopupWindow *popup)
{
    gtk_widget_hide(popup->window);

    if (popup->update_timer)
    {
        g_source_remove(popup->update_timer);
        popup->update_timer = 0;
    }
}

void
popup_window_toggle(PopupWindow *popup, GdkRectangle *button_rect)
{
    if (gtk_widget_get_visible(popup->window))
    {
        popup_window_hide(popup);
    }
    else
    {
        popup_window_show(popup, button_rect);
    }
}

void
popup_window_update_networks(PopupWindow *popup)
{
    GList *devices, *device;
    GList *access_points, *ap;
    GtkWidget *list_item;
    NMDeviceInfo *device_info;
    gint count = 0;
    
    /* Clear existing items */
    gtk_container_foreach(GTK_CONTAINER(popup->network_list), (GtkCallback)gtk_widget_destroy, NULL);
    
    /* Get all devices */
    devices = nm_interface_get_devices(popup->nm_interface);
    
    /* Add header for Wi-Fi networks */
    if (devices) {
        GtkWidget *header = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(header), "<b>Wi-Fi Networks</b>");
        gtk_widget_set_halign(header, GTK_ALIGN_START);
        gtk_widget_set_margin_start(header, 10);
        gtk_widget_set_margin_top(header, 5);
        gtk_widget_set_margin_bottom(header, 5);
        gtk_container_add(GTK_CONTAINER(popup->network_list), header);
    }
    
    for (device = devices; device != NULL; device = device->next) {
        device_info = (NMDeviceInfo *)device->data;
        
        if (device_info->type == NM_DEVICE_TYPE_WIFI) {
            /* Get all access points for Wi-Fi device */
            access_points = nm_interface_get_access_points(popup->nm_interface, device_info->interface);
            
            for (ap = access_points; ap != NULL; ap = ap->next) {
                /* Create a list item for each access point */
                /* TODO: Get actual SSID, strength, and security info from access point */
                gchar *ap_path = (gchar *)ap->data;
                gchar *ssid = g_strdup_printf("Network %d", ++count);
                list_item = create_network_list_item(ssid, 75, TRUE, FALSE);
                g_free(ssid);
                
                g_object_set_data_full(G_OBJECT(list_item), "ap-path", g_strdup(ap_path), g_free);
                gtk_container_add(GTK_CONTAINER(popup->network_list), list_item);
            }
            
            g_list_free_full(access_points, g_free);
        }
        
        /* Also show Ethernet connections */
        if (device_info->type == NM_DEVICE_TYPE_ETHERNET && device_info->state == 100) {
            list_item = create_network_list_item("Wired Connection", 100, FALSE, TRUE);
            gtk_container_add(GTK_CONTAINER(popup->network_list), list_item);
        }
    }
    
    g_list_free(devices);
    
    /* Show all items */
    gtk_widget_show_all(popup->network_list);
}

void
popup_window_set_transparency(PopupWindow *popup, gint transparency)
{
    /* Set transparency level */
    GdkScreen *screen = gtk_widget_get_screen(popup->window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);

    if (visual != NULL && gtk_widget_is_composited(popup->window))
    {
        gtk_widget_set_visual(popup->window, visual);
    }
    
    cairo_region_t *region = cairo_region_create();
    gdk_window_input_shape_combine_region(gtk_widget_get_window(popup->window),
                                          region, 0, 0);
    cairo_region_destroy(region);

    gtk_widget_set_opacity(popup->window, (100.0 - transparency) / 100.0);
}

/* Create a network list item widget */
static GtkWidget *
create_network_list_item(const gchar *ssid, gint strength, gboolean is_secure, gboolean is_connected)
{
    GtkWidget *box;
    GtkWidget *name_label;
    GtkWidget *icon;
    GtkWidget *signal_icon;
    gchar *icon_name;
    
    /* Create horizontal box for the item */
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 5);
    gtk_widget_set_margin_bottom(box, 5);
    
    /* Connection status icon */
    if (is_connected) {
        icon = gtk_image_new_from_icon_name("network-wireless-connected-symbolic", GTK_ICON_SIZE_MENU);
    } else if (is_secure) {
        icon = gtk_image_new_from_icon_name("network-wireless-encrypted-symbolic", GTK_ICON_SIZE_MENU);
    } else {
        icon = gtk_image_new_from_icon_name("network-wireless-symbolic", GTK_ICON_SIZE_MENU);
    }
    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    
    /* Network name */
    name_label = gtk_label_new(ssid);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(name_label), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(box), name_label, TRUE, TRUE, 0);
    
    /* Signal strength icon */
    if (strength >= 80) {
        icon_name = "network-wireless-signal-excellent-symbolic";
    } else if (strength >= 60) {
        icon_name = "network-wireless-signal-good-symbolic";
    } else if (strength >= 40) {
        icon_name = "network-wireless-signal-ok-symbolic";
    } else if (strength >= 20) {
        icon_name = "network-wireless-signal-weak-symbolic";
    } else {
        icon_name = "network-wireless-signal-none-symbolic";
    }
    signal_icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
    gtk_box_pack_end(GTK_BOX(box), signal_icon, FALSE, FALSE, 0);
    
    return box;
}

/* Network item click handler */
static void
on_network_item_clicked(GtkListBoxRow *row, gpointer user_data)
{
    PopupWindow *popup = (PopupWindow *)user_data;
    GtkWidget *child;
    gchar *ap_path;
    
    child = gtk_bin_get_child(GTK_BIN(row));
    ap_path = g_object_get_data(G_OBJECT(child), "ap-path");
    
    if (ap_path) {
        /* TODO: Connect to the selected network */
        g_debug("Network selected: %s", ap_path);
    }
}

/* Search entry callback */
static void
on_search_changed(GtkSearchEntry *entry, PopupWindow *popup)
{
    const gchar *text;
    
    g_free(popup->filter_text);
    text = gtk_entry_get_text(GTK_ENTRY(entry));
    popup->filter_text = g_strdup(text);
    
    /* Update the network list with the new filter */
    popup_window_update_networks(popup);
}
