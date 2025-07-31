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

#include "nm-interface.h"
#include "utils.h"

/* D-Bus paths and interfaces */
#define NM_DBUS_SERVICE                   "org.freedesktop.NetworkManager"
#define NM_DBUS_PATH                      "/org/freedesktop/NetworkManager"
#define NM_DBUS_INTERFACE                 "org.freedesktop.NetworkManager"
#define NM_DBUS_INTERFACE_DEVICE          "org.freedesktop.NetworkManager.Device"
#define NM_DBUS_INTERFACE_DEVICE_WIRELESS "org.freedesktop.NetworkManager.Device.Wireless"
#define NM_DBUS_INTERFACE_ACCESS_POINT    "org.freedesktop.NetworkManager.AccessPoint"
#define NM_DBUS_INTERFACE_SETTINGS        "org.freedesktop.NetworkManager.Settings"
#define NM_DBUS_INTERFACE_CONNECTION      "org.freedesktop.NetworkManager.Settings.Connection"

/* NMInterface structure */
struct _NMInterface {
    GDBusConnection         *connection;
    GDBusProxy              *nm_proxy;
    GDBusProxy              *settings_proxy;
    GHashTable              *devices;
    GHashTable              *connections;
    
    /* Current state */
    NMState                  nm_state;
    gboolean                 wireless_enabled;
    gboolean                 networking_enabled;

    /* Signal handlers */
    NMStateChangedCallback   state_changed_cb;
    NMDeviceCallback         device_added_cb;
    NMDeviceCallback         device_removed_cb;
    gpointer                 user_data;
    
    /* Signal subscription IDs */
    guint                    state_changed_id;
    guint                    device_added_id;
    guint                    device_removed_id;
};


NMInterface *
nm_interface_new(void)
{
    NMInterface *nm_interface;

    nm_interface = g_new0(NMInterface, 1);
    nm_interface->devices = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    nm_interface->connections = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    return nm_interface;
}

void
nm_interface_free(NMInterface *nm_interface)
{
    g_clear_object(&nm_interface->connection);
    g_hash_table_destroy(nm_interface->devices);
    g_hash_table_destroy(nm_interface->connections);
    g_free(nm_interface);
}

/* Initialize NMInterface */
gboolean
nm_interface_init(NMInterface *nm_interface, GError **error)
{
    nm_interface->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, error);
    if (!nm_interface->connection) {
        return FALSE;
    }
    
    /* Create NetworkManager proxy */
    nm_interface->nm_proxy = g_dbus_proxy_new_sync(
        nm_interface->connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        NM_DBUS_PATH,
        NM_DBUS_INTERFACE,
        NULL,
        error);
    
    if (!nm_interface->nm_proxy) {
        return FALSE;
    }
    
    /* Create Settings proxy */
    nm_interface->settings_proxy = g_dbus_proxy_new_sync(
        nm_interface->connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        "/org/freedesktop/NetworkManager/Settings",
        NM_DBUS_INTERFACE_SETTINGS,
        NULL,
        error);
    
    if (!nm_interface->settings_proxy) {
        g_clear_object(&nm_interface->nm_proxy);
        return FALSE;
    }
    
    /* Get initial state */
    nm_interface_update_state(nm_interface);
    
    /* Load devices */
    nm_interface_load_devices(nm_interface);
    
    /* Setup signal handlers */
    nm_interface_setup_signals(nm_interface);

    return TRUE;
}

void
nm_interface_shutdown(NMInterface *nm_interface)
{
    /* Disconnect signal handlers */
    if (nm_interface->state_changed_id > 0) {
        g_dbus_connection_signal_unsubscribe(nm_interface->connection, nm_interface->state_changed_id);
    }
    if (nm_interface->device_added_id > 0) {
        g_dbus_connection_signal_unsubscribe(nm_interface->connection, nm_interface->device_added_id);
    }
    if (nm_interface->device_removed_id > 0) {
        g_dbus_connection_signal_unsubscribe(nm_interface->connection, nm_interface->device_removed_id);
    }
    
    /* Clear proxies */
    g_clear_object(&nm_interface->nm_proxy);
    g_clear_object(&nm_interface->settings_proxy);
    g_clear_object(&nm_interface->connection);
}

/* Retrieve devices */
GList *
nm_interface_get_devices(NMInterface *nm_interface)
{
    return g_hash_table_get_values(nm_interface->devices);
}

/* Utility functions */
const gchar *
nm_interface_device_type_to_string(NMDeviceType type)
{
    switch (type) {
        case NM_DEVICE_TYPE_ETHERNET:
            return "Ethernet";
        case NM_DEVICE_TYPE_WIFI:
            return "Wi-Fi";
        case NM_DEVICE_TYPE_MOBILE:
            return "Mobile";
        case NM_DEVICE_TYPE_VPN:
            return "VPN";
        case NM_DEVICE_TYPE_BLUETOOTH:
            return "Bluetooth";
        default:
            return "Unknown";
    }
}

const gchar *
nm_interface_state_to_string(NMConnectionState state)
{
    switch (state) {
        case NM_CONNECTION_STATE_DISCONNECTED:
            return "Disconnected";
        case NM_CONNECTION_STATE_CONNECTING:
            return "Connecting";
        case NM_CONNECTION_STATE_CONNECTED:
            return "Connected";
        case NM_CONNECTION_STATE_FAILED:
            return "Failed";
        default:
            return "Unknown";
    }
}

/* Update NetworkManager state */
static void
nm_interface_update_state(NMInterface *nm_interface)
{
    GVariant *state_variant;
    GVariant *wireless_enabled_variant;
    GVariant *networking_enabled_variant;
    
    if (!nm_interface->nm_proxy)
        return;
    
    /* Get State property */
    state_variant = g_dbus_proxy_get_cached_property(nm_interface->nm_proxy, "State");
    if (state_variant) {
        nm_interface->nm_state = g_variant_get_uint32(state_variant);
        g_variant_unref(state_variant);
    }
    
    /* Get WirelessEnabled property */
    wireless_enabled_variant = g_dbus_proxy_get_cached_property(nm_interface->nm_proxy, "WirelessEnabled");
    if (wireless_enabled_variant) {
        nm_interface->wireless_enabled = g_variant_get_boolean(wireless_enabled_variant);
        g_variant_unref(wireless_enabled_variant);
    }
    
    /* Get NetworkingEnabled property */
    networking_enabled_variant = g_dbus_proxy_get_cached_property(nm_interface->nm_proxy, "NetworkingEnabled");
    if (networking_enabled_variant) {
        nm_interface->networking_enabled = g_variant_get_boolean(networking_enabled_variant);
        g_variant_unref(networking_enabled_variant);
    }
}

/* Load devices from NetworkManager */
static void
nm_interface_load_devices(NMInterface *nm_interface)
{
    GVariant *result;
    GVariantIter *iter;
    const gchar *device_path;
    GError *error = NULL;
    
    if (!nm_interface->nm_proxy)
        return;
    
    /* Call GetDevices method */
    result = g_dbus_proxy_call_sync(
        nm_interface->nm_proxy,
        "GetDevices",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    
    if (error) {
        g_warning("Failed to get devices: %s", error->message);
        g_error_free(error);
        return;
    }
    
    if (result) {
        g_variant_get(result, "(ao)", &iter);
        
        while (g_variant_iter_next(iter, "o", &device_path)) {
            NMDeviceInfo *device_info = nm_interface_create_device_info(nm_interface, device_path);
            if (device_info) {
                g_hash_table_insert(nm_interface->devices, g_strdup(device_path), device_info);
            }
        }
        
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
}

/* Signal handler for StateChanged signal */
static void
on_state_changed(GDBusProxy *proxy,
                 GVariant   *changed_properties,
                 GStrv       invalidated_properties,
                 gpointer    user_data)
{
    NMInterface *nm_interface = (NMInterface *)user_data;
    GVariant *state_variant;
    
    state_variant = g_variant_lookup_value(changed_properties, "State", G_VARIANT_TYPE_UINT32);
    if (state_variant) {
        nm_interface->nm_state = g_variant_get_uint32(state_variant);
        g_variant_unref(state_variant);
        
        if (nm_interface->state_changed_cb) {
            nm_interface->state_changed_cb(nm_interface, nm_interface->nm_state, nm_interface->user_data);
        }
    }
}

/* Create device info from D-Bus path */
static NMDeviceInfo *
nm_interface_create_device_info(NMInterface *nm_interface, const gchar *device_path)
{
    NMDeviceInfo *device_info;
    GDBusProxy *device_proxy;
    GVariant *variant;
    GError *error = NULL;
    guint device_type;
    
    /* Create device proxy */
    device_proxy = g_dbus_proxy_new_sync(
        nm_interface->connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        device_path,
        NM_DBUS_INTERFACE_DEVICE,
        NULL,
        &error);
    
    if (error) {
        g_warning("Failed to create device proxy for %s: %s", device_path, error->message);
        g_error_free(error);
        return NULL;
    }
    
    device_info = g_new0(NMDeviceInfo, 1);
    
    /* Get device type */
    variant = g_dbus_proxy_get_cached_property(device_proxy, "DeviceType");
    if (variant) {
        device_type = g_variant_get_uint32(variant);
        g_variant_unref(variant);
        
        /* Map NetworkManager device types to our enum */
        switch (device_type) {
            case 1: /* NM_DEVICE_TYPE_ETHERNET */
                device_info->type = NM_DEVICE_TYPE_ETHERNET;
                break;
            case 2: /* NM_DEVICE_TYPE_WIFI */
                device_info->type = NM_DEVICE_TYPE_WIFI;
                break;
            case 8: /* NM_DEVICE_TYPE_MODEM */
                device_info->type = NM_DEVICE_TYPE_MOBILE;
                break;
            case 5: /* NM_DEVICE_TYPE_BT */
                device_info->type = NM_DEVICE_TYPE_BLUETOOTH;
                break;
            default:
                device_info->type = NM_DEVICE_TYPE_UNKNOWN;
                break;
        }
    }
    
    /* Get interface name */
    variant = g_dbus_proxy_get_cached_property(device_proxy, "Interface");
    if (variant) {
        device_info->interface = g_variant_dup_string(variant, NULL);
        g_variant_unref(variant);
    }
    
    /* Get state */
    variant = g_dbus_proxy_get_cached_property(device_proxy, "State");
    if (variant) {
        device_info->state = g_variant_get_uint32(variant);
        g_variant_unref(variant);
    }
    
    /* Get managed status */
    variant = g_dbus_proxy_get_cached_property(device_proxy, "Managed");
    if (variant) {
        device_info->managed = g_variant_get_boolean(variant);
        g_variant_unref(variant);
    }
    
    g_object_unref(device_proxy);
    
    return device_info;
}

/* Get device info */
NMDeviceInfo *
nm_interface_get_device_info(NMInterface *nm_interface, const gchar *device_path)
{
    return g_hash_table_lookup(nm_interface->devices, device_path);
}

/* Free device info */
void
nm_interface_free_device_info(NMDeviceInfo *info)
{
    if (!info)
        return;
    
    g_free(info->name);
    g_free(info->interface);
    
    /* Free type-specific data */
    switch (info->type) {
        case NM_DEVICE_TYPE_ETHERNET:
            g_free(info->specific.ethernet.carrier);
            break;
        case NM_DEVICE_TYPE_WIFI:
            g_free(info->specific.wifi.active_ap);
            g_list_free(info->specific.wifi.access_points);
            break;
        case NM_DEVICE_TYPE_MOBILE:
            g_free(info->specific.mobile.operator_name);
            break;
        default:
            break;
    }
    
    g_free(info);
}

/* Get connections */
GList *
nm_interface_get_connections(NMInterface *nm_interface)
{
    return g_hash_table_get_values(nm_interface->connections);
}

/* Activate connection */
gboolean
nm_interface_activate_connection(NMInterface *nm_interface,
                                const gchar *connection_uuid,
                                const gchar *device_path,
                                GError **error)
{
    GVariant *result;
    
    result = g_dbus_proxy_call_sync(
        nm_interface->nm_proxy,
        "ActivateConnection",
        g_variant_new("(ooo)", 
                      connection_uuid ? connection_uuid : "/",
                      device_path ? device_path : "/",
                      "/"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);
    
    if (result) {
        g_variant_unref(result);
        return TRUE;
    }
    
    return FALSE;
}

/* Set callback functions */
void
nm_interface_set_state_changed_cb(NMInterface *nm_interface,
                                 NMStateChangedCallback callback,
                                 gpointer user_data)
{
    nm_interface->state_changed_cb = callback;
    nm_interface->user_data = user_data;
}

void
nm_interface_set_device_added_cb(NMInterface *nm_interface,
                                NMDeviceCallback callback,
                                gpointer user_data)
{
    nm_interface->device_added_cb = callback;
    nm_interface->user_data = user_data;
}

void
nm_interface_set_device_removed_cb(NMInterface *nm_interface,
                                  NMDeviceCallback callback,
                                  gpointer user_data)
{
    nm_interface->device_removed_cb = callback;
    nm_interface->user_data = user_data;
}

/* Get access points for a Wi-Fi device */
GList *
nm_interface_get_access_points(NMInterface *nm_interface, const gchar *device_path)
{
    GList *access_points = NULL;
    GDBusProxy *wifi_proxy;
    GVariant *result;
    GVariantIter *iter;
    const gchar *ap_path;
    GError *error = NULL;
    
    /* Create Wi-Fi device proxy */
    wifi_proxy = g_dbus_proxy_new_sync(
        nm_interface->connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        device_path,
        NM_DBUS_INTERFACE_DEVICE_WIRELESS,
        NULL,
        &error);
    
    if (error) {
        g_warning("Failed to create Wi-Fi device proxy: %s", error->message);
        g_error_free(error);
        return NULL;
    }
    
    /* Get all access points */
    result = g_dbus_proxy_call_sync(
        wifi_proxy,
        "GetAllAccessPoints",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    
    if (error) {
        g_warning("Failed to get access points: %s", error->message);
        g_error_free(error);
        g_object_unref(wifi_proxy);
        return NULL;
    }
    
    if (result) {
        g_variant_get(result, "(ao)", &iter);
        
        while (g_variant_iter_next(iter, "o", &ap_path)) {
            /* TODO: Create AccessPointInfo structure and add to list */
            access_points = g_list_append(access_points, g_strdup(ap_path));
        }
        
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    g_object_unref(wifi_proxy);
    
    return access_points;
}

/* Request a Wi-Fi scan */
gboolean
nm_interface_request_scan(NMInterface *nm_interface, const gchar *device_path, GError **error)
{
    GDBusProxy *wifi_proxy;
    GVariant *result;
    gboolean success = FALSE;
    
    /* Create Wi-Fi device proxy */
    wifi_proxy = g_dbus_proxy_new_sync(
        nm_interface->connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        device_path,
        NM_DBUS_INTERFACE_DEVICE_WIRELESS,
        NULL,
        error);
    
    if (!wifi_proxy) {
        return FALSE;
    }
    
    /* Request scan */
    result = g_dbus_proxy_call_sync(
        wifi_proxy,
        "RequestScan",
        g_variant_new("(a{sv})", NULL),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);
    
    if (result) {
        g_variant_unref(result);
        success = TRUE;
    }
    
    g_object_unref(wifi_proxy);
    
    return success;
}

/* Deactivate connection */
gboolean
nm_interface_deactivate_connection(NMInterface *nm_interface,
                                  const gchar *active_path,
                                  GError **error)
{
    GVariant *result;
    
    result = g_dbus_proxy_call_sync(
        nm_interface->nm_proxy,
        "DeactivateConnection",
        g_variant_new("(o)", active_path),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        error);
    
    if (result) {
        g_variant_unref(result);
        return TRUE;
    }
    
    return FALSE;
}

/* Setup D-Bus signal handlers */
static void
nm_interface_setup_signals(NMInterface *nm_interface)
{
    /* Subscribe to StateChanged signal */
    g_signal_connect(nm_interface->nm_proxy, "g-properties-changed",
                     G_CALLBACK(on_state_changed), nm_interface);
}
