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
#include "password-dialog.h"
#include "notification.h"
#include <string.h>

/* CSS Provider */
static GtkCssProvider *css_provider = NULL;

#define POPUP_UPDATE_INTERVAL 5000  /* Update every 5 seconds */
#define POPUP_WIDTH 320
#define POPUP_HEIGHT 400

/* Forward declarations */
static void on_search_changed(GtkSearchEntry *entry, PopupWindow *popup);
static GtkWidget *create_network_list_item(const gchar *ssid, gint strength, gboolean is_secure, gboolean is_connected);
static void on_network_item_clicked(GtkListBoxRow *row, gpointer user_data);
static void start_loading_spinner(PopupWindow *popup);
static void stop_loading_spinner(PopupWindow *popup);
static void show_connection_error(PopupWindow *popup, const gchar *ssid, const gchar *error_message);

PopupWindow *
popup_window_new(NetworkManagerPlugin *plugin)
{
    PopupWindow *popup;

    popup = g_new0(PopupWindow, 1);
    popup->plugin = plugin;
    popup->nm_interface = plugin->nm_interface;
    popup->notification_manager = notification_manager_new();

    popup->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(popup->window),
                             GDK_WINDOW_TYPE_HINT_POPUP_MENU);

    popup->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(popup->window), popup->main_box);

    popup->header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(popup->main_box), popup->header_box, FALSE, FALSE, 0);

    /* Add spinner for loading indication */
    popup->spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(popup->header_box), popup->spinner, FALSE, FALSE, 5);

    popup->search_entry = gtk_search_entry_new();
    gtk_box_pack_end(GTK_BOX(popup->header_box), popup->search_entry, FALSE, FALSE, 0);

    /* Add status bar for inline notifications */
    popup->status_bar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(popup->main_box), popup->status_bar, FALSE, FALSE, 0);

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

    /* Load CSS styling */
    if (!css_provider) {
        css_provider = gtk_css_provider_new();
        gchar *css_path = g_build_filename(PACKAGE_DATA_DIR, "panel-plugin", "style.css", NULL);
        GError *css_error = NULL;
        if (!gtk_css_provider_load_from_path(css_provider, css_path, &css_error)) {
            g_warning("Failed to load CSS: %s", css_error ? css_error->message : "Unknown error");
            g_clear_error(&css_error);
        }
        g_free(css_path);
        
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

    /* Add CSS classes to widgets */
    GtkStyleContext *context = gtk_widget_get_style_context(popup->window);
    gtk_style_context_add_class(context, "nm-plugin-popup");

    context = gtk_widget_get_style_context(popup->main_box);
    gtk_style_context_add_class(context, "nm-plugin-popup");

    context = gtk_widget_get_style_context(popup->network_list);
    gtk_style_context_add_class(context, "nm-network-list");

    context = gtk_widget_get_style_context(popup->status_bar);
    gtk_style_context_add_class(context, "nm-status-bar");

    context = gtk_widget_get_style_context(popup->header_box);
    gtk_style_context_add_class(context, "nm-header-box");

    context = gtk_widget_get_style_context(popup->search_entry);
    gtk_style_context_add_class(context, "nm-search-entry");

    context = gtk_widget_get_style_context(popup->spinner);
    gtk_style_context_add_class(context, "nm-spinner");

    /* Initially hide the spinner */
    gtk_widget_hide(popup->spinner);

    return popup;
}

void
popup_window_free(PopupWindow *popup)
{
    if (popup->connecting_to_ssid)
        g_free(popup->connecting_to_ssid);
    
    if (popup->notification_manager)
        notification_manager_free(popup->notification_manager);
    
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
                NMAccessPointInfo *ap_info = (NMAccessPointInfo *)ap->data;
                
                if (ap_info && ap_info->ssid) {
                    /* Check if this network should be shown based on filter */
                    if (popup->filter_text && *popup->filter_text &&
                        !g_strstr_len(ap_info->ssid, -1, popup->filter_text)) {
                        continue;
                    }
                    
                    gboolean is_secure = (g_strcmp0(ap_info->security, "None") != 0);
                    list_item = create_network_list_item(ap_info->ssid, 
                                                       ap_info->strength,
                                                       is_secure, 
                                                       FALSE);
                    
                    /* Store AP info for connection */
                    g_object_set_data_full(G_OBJECT(list_item), "ap-info", 
                                         ap_info, (GDestroyNotify)nm_interface_free_ap_info);
                    g_object_set_data_full(G_OBJECT(list_item), "device-path", 
                                         g_strdup(device_info->interface), g_free);
                    gtk_container_add(GTK_CONTAINER(popup->network_list), list_item);
                }
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
    NMAccessPointInfo *ap_info;
    const gchar *device_path;
    GError *error = NULL;
    
    child = gtk_bin_get_child(GTK_BIN(row));
    ap_info = g_object_get_data(G_OBJECT(child), "ap-info");
    device_path = g_object_get_data(G_OBJECT(child), "device-path");
    
    if (ap_info && ap_info->ssid) {
        /* Check if we have an existing connection for this SSID */
        NMConnectionInfo *existing_conn = nm_interface_find_connection_by_ssid(popup->nm_interface, ap_info->ssid);
        
        if (popup->connecting) {
            g_message("Connection attempt already in progress. Please wait...");
            return;
        }
        
        start_loading_spinner(popup);
        popup->connecting_to_ssid = g_strdup(ap_info->ssid);
        
        if (existing_conn) {
            /* Use existing connection */
            g_message("Using existing connection for network: %s", ap_info->ssid);
            if (!nm_interface_activate_connection(popup->nm_interface, existing_conn->path, device_path, &error)) {
                g_warning("Failed to activate existing connection: %s", error->message);
                show_connection_error(popup, ap_info->ssid, error->message);
                g_clear_error(&error);
            } else {
                notification_show_connection_status(popup->notification_manager, ap_info->ssid, TRUE, NULL);
            }
        } else if (g_strcmp0(ap_info->security, "802.1X") == 0) {
            /* Show enterprise authentication dialog for 802.1X networks */
            EnterpriseAuthInfo *auth_info = password_dialog_show_enterprise(GTK_WINDOW(popup->window), ap_info->ssid);
            if (auth_info) {
                g_message("Creating new connection for enterprise network: %s", ap_info->ssid);
                if (!nm_interface_add_and_activate_enterprise_connection(popup->nm_interface, device_path, ap_info->path, ap_info->ssid, auth_info, &error)) {
                    g_warning("Failed to create and activate enterprise connection: %s", error->message);
                    show_connection_error(popup, ap_info->ssid, error->message);
                    g_clear_error(&error);
                } else {
                    notification_show_connection_status(popup->notification_manager, ap_info->ssid, TRUE, NULL);
                }
                enterprise_auth_info_free(auth_info);
            }
        } else if (g_strcmp0(ap_info->security, "None") != 0) {
            /* Show password dialog for secured networks */
            gchar *password = password_dialog_show(GTK_WINDOW(popup->window), ap_info->ssid);
            if (password) {
                g_message("Creating new connection for secured network: %s", ap_info->ssid);
                if (!nm_interface_add_and_activate_connection(popup->nm_interface, device_path, ap_info->path, ap_info->ssid, password, ap_info->security, &error)) {
                    g_warning("Failed to create and activate connection: %s", error->message);
                    show_connection_error(popup, ap_info->ssid, error->message);
                    g_clear_error(&error);
                } else {
                    notification_show_connection_status(popup->notification_manager, ap_info->ssid, TRUE, NULL);
                }
                g_free(password);
            }
        } else {
            /* Create new connection for unsecured network */
            g_message("Creating new connection for unsecured network: %s", ap_info->ssid);
            if (!nm_interface_add_and_activate_connection(popup->nm_interface, device_path, ap_info->path, ap_info->ssid, NULL, ap_info->security, &error)) {
                g_warning("Failed to create and activate connection: %s", error->message);
                show_connection_error(popup, ap_info->ssid, error->message);
                g_clear_error(&error);
            } else {
                notification_show_connection_status(popup->notification_manager, ap_info->ssid, TRUE, NULL);
            }
        }
        stop_loading_spinner(popup);
        g_free(popup->connecting_to_ssid);
        popup->connecting_to_ssid = NULL;
    }
}

static void
start_loading_spinner(PopupWindow *popup)
{
    gtk_spinner_start(GTK_SPINNER(popup-spinner));
    gtk_widget_show(popup-spinner);
    gtk_widget_set_sensitive(GTK_WIDGET(popup-header_box), FALSE);
    popup-connecting = TRUE;
}

static void
stop_loading_spinner(PopupWindow *popup)
{
    gtk_spinner_stop(GTK_SPINNER(popup-spinner));
    gtk_widget_hide(popup-spinner);
    gtk_widget_set_sensitive(GTK_WIDGET(popup-header_box), TRUE);
    popup-connecting = FALSE;
}

static void
show_connection_error(PopupWindow *popup,
                      const gchar *ssid,
                      const gchar *error_message)
{
    GtkWidget *inline_message;
    /* Clear existing messages */
    GList *children, *iter;
    children = gtk_container_get_children(GTK_CONTAINER(popup-status_bar));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter-data));
    }
    g_list_free(children);

    /* Create and display an inline error message */
    inline_message = notification_create_inline_message(error_message,
                                                        NOTIFICATION_TYPE_ERROR);
    gtk_container_add(GTK_CONTAINER(popup-status_bar), inline_message);
    gtk_widget_show_all(popup-status_bar);

    /* Show popup notification */
    notification_show_network_error(popup-notification_manager, "Connection Attempt",
                                   error_message);
}

/* Search entry callback */
static void
on_search_changed(GtkSearchEntry *entry, PopupWindow *popup)
{
    const gchar *text;
    
    g_free(popup-filter_text);
    text = gtk_entry_get_text(GTK_ENTRY(entry));
    popup-filter_text = g_strdup(text);
    
    /* Update the network list with the new filter */
    popup_window_update_networks(popup);
}
