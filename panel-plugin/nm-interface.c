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
#include <string.h>

/* D-Bus paths and interfaces */
#define NM_DBUS_SERVICE                   "org.freedesktop.NetworkManager"
#define NM_DBUS_PATH                      "/org/freedesktop/NetworkManager"
#define NM_DBUS_INTERFACE                 "org.freedesktop.NetworkManager"
#define NM_DBUS_INTERFACE_DEVICE          "org.freedesktop.NetworkManager.Device"
#define NM_DBUS_INTERFACE_DEVICE_WIRELESS "org.freedesktop.NetworkManager.Device.Wireless"
#define NM_DBUS_INTERFACE_ACCESS_POINT    "org.freedesktop.NetworkManager.AccessPoint"
#define NM_DBUS_INTERFACE_SETTINGS        "org.freedesktop.NetworkManager.Settings"
#define NM_DBUS_INTERFACE_CONNECTION      "org.freedesktop.NetworkManager.Settings.Connection"

/* Forward declarations */
static void nm_interface_update_state(NMInterface *nm_interface);
static void nm_interface_load_devices(NMInterface *nm_interface);
static void nm_interface_load_connections(NMInterface *nm_interface);
static void nm_interface_setup_signals(NMInterface *nm_interface);
static NMDeviceInfo *nm_interface_create_device_info(NMInterface *nm_interface, const gchar *device_path);
static NMConnectionInfo *nm_interface_create_connection_info(NMInterface *nm_interface, const gchar *connection_path);

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
    
    /* Load connections */
    nm_interface_load_connections(nm_interface);
    
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

static void
nm_interface_load_connections(NMInterface *nm_interface)
{
    GVariant *result;
    GVariantIter *iter;
    const gchar *connection_path;
    GError *error = NULL;
    if (!nm_interface->settings_proxy)
        return;
    /* Call ListConnections method */
    result = g_dbus_proxy_call_sync(
        nm_interface->settings_proxy,
        "ListConnections",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    if (error) {
        g_warning("Failed to get connections: %s", error->message);
        g_error_free(error);
        return;
    }
    if (result) {
        g_variant_get(result, "(ao)", &iter);
        while (g_variant_iter_next(iter, "o", &connection_path)) {
            NMConnectionInfo *connection_info = nm_interface_create_connection_info(nm_interface, connection_path);
            if (connection_info) {
                g_hash_table_insert(nm_interface->connections, g_strdup(connection_path), connection_info);
            }
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
}

static NMConnectionInfo *
nm_interface_create_connection_info(NMInterface *nm_interface, const gchar *connection_path)
{
    NMConnectionInfo *connection_info;
    GDBusProxy *connection_proxy;
    GVariant *variant;
    GVariant *settings;
    GError *error = NULL;
    /* Create connection proxy */
    connection_proxy = g_dbus_proxy_new_sync(
        nm_interface->connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        connection_path,
        NM_DBUS_INTERFACE_CONNECTION,
        NULL,
        &error);
        
    if (error) {
        g_warning("Failed to create connection proxy for %s: %s", connection_path, error->message);
        g_error_free(error);
        return NULL;
    }
    connection_info = g_new0(NMConnectionInfo, 1);
    connection_info->path = g_strdup(connection_path);
    
    /* Get connection settings */
    settings = g_dbus_proxy_call_sync(
        connection_proxy,
        "GetSettings",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    
    if (error) {
        g_warning("Failed to get connection settings for %s: %s", connection_path, error->message);
        g_error_free(error);
        g_object_unref(connection_proxy);
        g_free(connection_info);
        return NULL;
    }
    
    if (settings) {
        GVariantIter *iter;
        const gchar *group_name;
        GVariant *group_settings;
        
        g_variant_get(settings, "(a{sa{sv}})", &iter);
        while (g_variant_iter_next(iter, "{&s@a{sv}}", &group_name, &group_settings)) {
            if (g_strcmp0(group_name, "connection") == 0) {
                /* Extract connection properties */
                g_variant_lookup(group_settings, "uuid", "s", &connection_info->uuid);
                g_variant_lookup(group_settings, "id", "s", &connection_info->id);
                g_variant_lookup(group_settings, "type", "s", &connection_info->type);
            }
            g_variant_unref(group_settings);
        }
        g_variant_iter_free(iter);
        g_variant_unref(settings);
    }
    
    /* Duplicate the strings to ensure they're owned by the structure */
    if (connection_info->uuid)
        connection_info->uuid = g_strdup(connection_info->uuid);
    if (connection_info->id)
        connection_info->id = g_strdup(connection_info->id);
    if (connection_info->type)
        connection_info->type = g_strdup(connection_info->type);
    
    g_object_unref(connection_proxy);
    return connection_info;
}

void
nm_interface_free_connection_info(NMConnectionInfo *info)
{
    if (!info)
        return;
    g_free(info->path);
    g_free(info->uuid);
    g_free(info->id);
    g_free(info->type);
    if (info->settings)
        g_hash_table_destroy(info->settings);
    g_free(info);
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

/* Get connection path by UUID */
const gchar *
nm_interface_get_connection_path(NMInterface *nm_interface, const gchar *uuid)
{
    GHashTableIter iter;
    gpointer key, value;
    
    if (!nm_interface || !uuid || !nm_interface->connections)
        return NULL;
    
    g_hash_table_iter_init(&iter, nm_interface->connections);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        NMConnectionInfo *conn_info = (NMConnectionInfo *)value;
        
        if (conn_info && conn_info->uuid && g_strcmp0(conn_info->uuid, uuid) == 0) {
            return conn_info->path;
        }
    }
    
    return NULL;
}

/* Find connection by SSID */
NMConnectionInfo *
nm_interface_find_connection_by_ssid(NMInterface *nm_interface, const gchar *ssid)
{
    GHashTableIter iter;
    gpointer key, value;
    
    if (!nm_interface || !ssid || !nm_interface->connections)
        return NULL;
    
    g_hash_table_iter_init(&iter, nm_interface->connections);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        NMConnectionInfo *conn_info = (NMConnectionInfo *)value;
        
        /* Check if this is a WiFi connection with matching SSID */
        if (conn_info && conn_info->type && 
            g_strcmp0(conn_info->type, "802-11-wireless") == 0 &&
            conn_info->id && g_strcmp0(conn_info->id, ssid) == 0) {
            return conn_info;
        }
    }
    
    return NULL;
}

/* Activate connection */
gboolean
nm_interface_activate_connection(NMInterface *nm_interface,
                                const gchar *connection_uuid,
                                const gchar *device_path,
                                GError **error)
{
    GVariant *result;
    GError *local_error = NULL;
    
    if (!nm_interface || !nm_interface->nm_proxy) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                    "NetworkManager interface not initialized");
        return FALSE;
    }
    
    if (!connection_uuid || !device_path) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                    "Invalid connection or device path");
        return FALSE;
    }
    
    result = g_dbus_proxy_call_sync(
        nm_interface->nm_proxy,
        "ActivateConnection",
        g_variant_new("(ooo)", 
                      connection_uuid,
                      device_path,
                      "/"),
        G_DBUS_CALL_FLAGS_NONE,
        30000,  /* 30 second timeout */
        NULL,
        &local_error);
    
    if (result) {
        g_variant_unref(result);
        return TRUE;
    }
    
    /* Provide more helpful error messages */
    if (local_error) {
        if (g_error_matches(local_error, G_DBUS_ERROR, G_DBUS_ERROR_TIMED_OUT)) {
            g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_TIMED_OUT,
                        "Connection attempt timed out. Please try again.");
        } else if (strstr(local_error->message, "Secrets were required")) {
            g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_AUTH_FAILED,
                        "Authentication failed. Please check your password.");
        } else if (strstr(local_error->message, "No suitable device")) {
            g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                        "No suitable network device found");
        } else {
            g_propagate_error(error, local_error);
            local_error = NULL;
        }
        g_clear_error(&local_error);
    }
    
    return FALSE;
}

/* Add and activate a new connection */
gboolean
nm_interface_add_and_activate_connection(NMInterface *nm_interface,
                                       const gchar *device_path,
                                       const gchar *ap_path,
                                       const gchar *ssid,
                                       const gchar *password,
                                       const gchar *security,
                                       GError **error)
{
    GVariantBuilder connection_builder;
    GVariantBuilder wireless_builder;
    GVariantBuilder wireless_security_builder;
    GVariant *connection_dict;
    GVariant *result;
    GError *local_error = NULL;
    
    if (!nm_interface || !nm_interface->nm_proxy) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                    "NetworkManager interface not initialized");
        return FALSE;
    }
    
    if (!device_path || !ap_path || !ssid) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                    "Invalid device path, access point path, or SSID");
        return FALSE;
    }
    
    /* Build the connection settings */
    g_variant_builder_init(&connection_builder, G_VARIANT_TYPE("a{sa{sv}}"));
    
    /* Connection section */
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "connection",
        g_variant_builder_new(G_VARIANT_TYPE("a{sv}")),
        "type", g_variant_new_string("802-11-wireless"),
        "id", g_variant_new_string(ssid),
        "autoconnect", g_variant_new_boolean(TRUE),
        NULL);
    
    /* 802-11-wireless section */
    g_variant_builder_init(&wireless_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&wireless_builder, "{sv}", "ssid",
                         g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,
                                                  ssid, strlen(ssid), 1));
    g_variant_builder_add(&wireless_builder, "{sv}", "mode", g_variant_new_string("infrastructure"));
    
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "802-11-wireless", &wireless_builder);
    
/* 802-11-wireless-security section if password is provided */
    if (password && *password) {
        g_variant_builder_init(&wireless_security_builder, G_VARIANT_TYPE("a{sv}"));

        if (g_strcmp0(security, "WPA") == 0 || g_strcmp0(security, "WPA2") == 0) {
            g_variant_builder_add(&wireless_security_builder, "{sv}", "key-mgmt", g_variant_new_string("wpa-psk"));
            g_variant_builder_add(&wireless_security_builder, "{sv}", "auth-alg", g_variant_new_string("open"));
            g_variant_builder_add(&wireless_security_builder, "{sv}", "psk", g_variant_new_string(password));

        } else if (g_strcmp0(security, "WPA3") == 0) {
            g_variant_builder_add(&wireless_security_builder, "{sv}", "key-mgmt", g_variant_new_string("sae")); /* WPA3 supports SAE */
            g_variant_builder_add(&wireless_security_builder, "{sv}", "psk", g_variant_new_string(password));

        } else if (g_strcmp0(security, "WEP") == 0) {
            g_variant_builder_add(&wireless_security_builder, "{sv}", "key-mgmt", g_variant_new_string("none"));
            g_variant_builder_add(&wireless_security_builder, "{sv}", "wep-key-type", g_variant_new_uint32(0));
            g_variant_builder_add(&wireless_security_builder, "{sv}", "wep-key0", g_variant_new_string(password));
        }

        g_variant_builder_add(&connection_builder, "{sa{sv}}", "802-11-wireless-security", &wireless_security_builder);
    }
    
    /* IPv4 section - use automatic (DHCP) */
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "ipv4",
        g_variant_builder_new(G_VARIANT_TYPE("a{sv}")),
        "method", g_variant_new_string("auto"),
        NULL);
    
    /* IPv6 section - use automatic */
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "ipv6",
        g_variant_builder_new(G_VARIANT_TYPE("a{sv}")),
        "method", g_variant_new_string("auto"),
        NULL);
    
    connection_dict = g_variant_builder_end(&connection_builder);
    
    /* Call AddAndActivateConnection */
    result = g_dbus_proxy_call_sync(
        nm_interface->nm_proxy,
        "AddAndActivateConnection",
        g_variant_new("(a{sa{sv}}oo)",
                     connection_dict,
                     device_path,
                     ap_path),
        G_DBUS_CALL_FLAGS_NONE,
        30000,  /* 30 second timeout */
        NULL,
        &local_error);
    
    if (result) {
        /* The result contains the path of the new connection and active connection */
        const gchar *connection_path, *active_path;
        g_variant_get(result, "(&o&o)", &connection_path, &active_path);
        g_debug("Created connection: %s, Active: %s", connection_path, active_path);
        g_variant_unref(result);
        return TRUE;
    }
    
    /* Provide more helpful error messages */
    if (local_error) {
        if (g_error_matches(local_error, G_DBUS_ERROR, G_DBUS_ERROR_TIMED_OUT)) {
            g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_TIMED_OUT,
                        "Connection attempt timed out. Please try again.");
        } else if (strstr(local_error->message, "Secrets were required")) {
            g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_AUTH_FAILED,
                        "Authentication failed. Please check your password.");
        } else if (strstr(local_error->message, "No suitable device")) {
            g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                        "No suitable network device found");
        } else if (strstr(local_error->message, "access-denied")) {
            g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED,
                        "Access denied. Please check your network permissions.");
        } else {
            g_propagate_error(error, local_error);
            local_error = NULL;
        }
        g_clear_error(&local_error);
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


/* Function to get access point properties */
static NMAccessPointInfo *
nm_interface_get_ap_info(NMInterface *nm_interface, const gchar *ap_path)
{
    NMAccessPointInfo *ap_info = g_new0(NMAccessPointInfo, 1);
    GDBusProxy *ap_proxy;
    GVariant *variant;
    GError *error = NULL;
    
    /* Store the AP path */
    ap_info->path = g_strdup(ap_path);
    
    /* Create access point proxy */
    ap_proxy = g_dbus_proxy_new_sync(
        nm_interface->connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        ap_path,
        NM_DBUS_INTERFACE_ACCESS_POINT,
        NULL,
        &error);
    
    if (error) {
        g_warning("Failed to create access point proxy for %s: %s", ap_path, error->message);
        g_error_free(error);
        g_free(ap_info);
        return NULL;
    }
    /* Get SSID */
    variant = g_dbus_proxy_get_cached_property(ap_proxy, "Ssid");
    if (variant) {
        gsize length;
        const guchar *ssid_data = g_variant_get_fixed_array(variant, &length, sizeof(guchar));
        if (length > 0) {
            ap_info->ssid = g_strndup((const gchar *)ssid_data, length);
        } else {
            ap_info->ssid = g_strdup("(hidden)");
        }
        g_variant_unref(variant);
    }
    
    /* Get signal strength */
    variant = g_dbus_proxy_get_cached_property(ap_proxy, "Strength");
    if (variant) {
        ap_info->strength = g_variant_get_byte(variant);
        g_variant_unref(variant);
    }
    
    /* Get security type - check Flags, WpaFlags, and RsnFlags */
    guint32 flags = 0;
    guint32 wpa_flags = 0;
    guint32 rsn_flags = 0;
    
    variant = g_dbus_proxy_get_cached_property(ap_proxy, "Flags");
    if (variant) {
        flags = g_variant_get_uint32(variant);
        g_variant_unref(variant);
    }
    
    variant = g_dbus_proxy_get_cached_property(ap_proxy, "WpaFlags");
    if (variant) {
        wpa_flags = g_variant_get_uint32(variant);
        g_variant_unref(variant);
    }
    
    variant = g_dbus_proxy_get_cached_property(ap_proxy, "RsnFlags");
    if (variant) {
        rsn_flags = g_variant_get_uint32(variant);
        g_variant_unref(variant);
    }
    
    /* Determine security type */
    if ((flags == 0x01) && (wpa_flags == 0) && (rsn_flags == 0)) {
        ap_info->security = g_strdup("None");
    } else if (rsn_flags & 0x00000400) { /* SAE (WPA3) */
        ap_info->security = g_strdup("WPA3");
    } else if (rsn_flags & 0x00000100 || wpa_flags & 0x00000100) { /* WPA2 */
        ap_info->security = g_strdup("WPA2");
    } else if (wpa_flags != 0) { /* WPA */
        ap_info->security = g_strdup("WPA");
    } else if (flags & 0x01) { /* WEP */
        ap_info->security = g_strdup("WEP");
    } else {
        ap_info->security = g_strdup("Unknown");
    }
    g_object_unref(ap_proxy);
    return ap_info;
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
        nm_interface-connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        device_path,
        NM_DBUS_INTERFACE_DEVICE_WIRELESS,
        NULL,
        error);

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
        error);

    if (error) {
        g_warning("Failed to get access points: %s", error->message);
        g_error_free(error);
        g_object_unref(wifi_proxy);
        return NULL;
    }

    if (result) {
        g_variant_get(result, "(ao)", &iter);

        while (g_variant_iter_next(iter, "o", &ap_path)) {
            NMAccessPointInfo *ap_info = nm_interface_get_ap_info(nm_interface, ap_path);
            if (ap_info) {
                access_points = g_list_append(access_points, ap_info);
            }
        }

        g_variant_iter_free(iter);
        g_variant_unref(result);
    }

    g_object_unref(wifi_proxy);

    return access_points;
}

/* Free access point info */
void
nm_interface_free_ap_info(NMAccessPointInfo *ap_info)
{
    if (!ap_info)
        return;
    
    g_free(ap_info->path);
    g_free(ap_info->ssid);
    g_free(ap_info->security);
    g_free(ap_info);
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
        &error);
    
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

/* Signal handler for device state changes */
static void
on_device_state_changed(GDBusConnection *connection,
                       const gchar *sender_name,
                       const gchar *object_path,
                       const gchar *interface_name,
                       const gchar *signal_name,
                       GVariant *parameters,
                       gpointer user_data)
{
    NMInterface *nm_interface = (NMInterface *)user_data;
    NMDeviceInfo *device_info;
    guint32 new_state, old_state, reason;
    
    /* Extract parameters */
    g_variant_get(parameters, "(uuu)", &new_state, &old_state, &reason);
    
    /* Update device info */
    device_info = g_hash_table_lookup(nm_interface->devices, object_path);
    if (device_info) {
        device_info->state = new_state;
        
        /* Notify callback if set */
        if (nm_interface->device_added_cb) {
            nm_interface->device_added_cb(nm_interface, device_info, nm_interface->user_data);
        }
    }
    
    g_debug("Device %s state changed from %u to %u (reason: %u)", 
            object_path, old_state, new_state, reason);
}

/* Signal handler for device added */
static void
on_device_added(GDBusConnection *connection,
               const gchar *sender_name,
               const gchar *object_path,
               const gchar *interface_name,
               const gchar *signal_name,
               GVariant *parameters,
               gpointer user_data)
{
    NMInterface *nm_interface = (NMInterface *)user_data;
    const gchar *device_path;
    NMDeviceInfo *device_info;
    
    g_variant_get(parameters, "(&o)", &device_path);
    
    /* Create and add new device */
    device_info = nm_interface_create_device_info(nm_interface, device_path);
    if (device_info) {
        g_hash_table_insert(nm_interface->devices, g_strdup(device_path), device_info);
        
        /* Notify callback if set */
        if (nm_interface->device_added_cb) {
            nm_interface->device_added_cb(nm_interface, device_info, nm_interface->user_data);
        }
    }
}

/* Signal handler for device removed */
static void
on_device_removed(GDBusConnection *connection,
                 const gchar *sender_name,
                 const gchar *object_path,
                 const gchar *interface_name,
                 const gchar *signal_name,
                 GVariant *parameters,
                 gpointer user_data)
{
    NMInterface *nm_interface = (NMInterface *)user_data;
    const gchar *device_path;
    NMDeviceInfo *device_info;
    
    g_variant_get(parameters, "(&o)", &device_path);
    
    /* Get device info before removing */
    device_info = g_hash_table_lookup(nm_interface->devices, device_path);
    if (device_info) {
        /* Notify callback if set */
        if (nm_interface->device_removed_cb) {
            nm_interface->device_removed_cb(nm_interface, device_info, nm_interface->user_data);
        }
        
        /* Remove from hash table */
        g_hash_table_remove(nm_interface->devices, device_path);
        nm_interface_free_device_info(device_info);
    }
}

/* Setup D-Bus signal handlers */
static void
nm_interface_setup_signals(NMInterface *nm_interface)
{
    /* Subscribe to StateChanged signal */
    g_signal_connect(nm_interface->nm_proxy, "g-properties-changed",
                     G_CALLBACK(on_state_changed), nm_interface);
    
    /* Subscribe to DeviceAdded signal */
    nm_interface->device_added_id = g_dbus_connection_signal_subscribe(
        nm_interface->connection,
        NM_DBUS_SERVICE,
        NM_DBUS_INTERFACE,
        "DeviceAdded",
        NM_DBUS_PATH,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_device_added,
        nm_interface,
        NULL);
    
    /* Subscribe to DeviceRemoved signal */
    nm_interface->device_removed_id = g_dbus_connection_signal_subscribe(
        nm_interface->connection,
        NM_DBUS_SERVICE,
        NM_DBUS_INTERFACE,
        "DeviceRemoved",
        NM_DBUS_PATH,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_device_removed,
        nm_interface,
        NULL);
    
    /* Subscribe to device state changes for all devices */
    nm_interface->state_changed_id = g_dbus_connection_signal_subscribe(
        nm_interface->connection,
        NM_DBUS_SERVICE,
        NM_DBUS_INTERFACE_DEVICE,
        "StateChanged",
        NULL,  /* Match all device paths */
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_device_state_changed,
        nm_interface,
        NULL);
}
