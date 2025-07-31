/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __NM_INTERFACE_H__
#define __NM_INTERFACE_H__

#include <glib.h>
#include <gio/gio.h>
#include <NetworkManager.h>

G_BEGIN_DECLS

/* Forward declarations */
typedef struct _NMInterface NMInterface;
typedef struct _NMDeviceInfo NMDeviceInfo;
typedef struct _NMConnectionInfo NMConnectionInfo;

/* Device types we support */
typedef enum {
    NM_DEVICE_TYPE_ETHERNET,
    NM_DEVICE_TYPE_WIFI,
    NM_DEVICE_TYPE_MOBILE,
    NM_DEVICE_TYPE_VPN,
    NM_DEVICE_TYPE_BLUETOOTH,
    NM_DEVICE_TYPE_UNKNOWN
} NMDeviceType;

/* Connection states */
typedef enum {
    NM_CONNECTION_STATE_UNKNOWN,
    NM_CONNECTION_STATE_DISCONNECTED,
    NM_CONNECTION_STATE_CONNECTING,
    NM_CONNECTION_STATE_CONNECTED,
    NM_CONNECTION_STATE_FAILED
} NMConnectionState;

/* Device information structure */
struct _NMDeviceInfo {
    gchar            *name;
    gchar            *interface;
    NMDeviceType      type;
    NMDeviceState     state;
    gboolean          managed;
    gboolean          available;
    
    /* Type-specific data */
    union {
        struct {
            gchar    *carrier;
            guint     speed;
        } ethernet;
        
        struct {
            gchar    *active_ap;
            guint8    strength;
            GList    *access_points;  /* List of NMAccessPoint */
        } wifi;
        
        struct {
            gchar    *operator_name;
            guint     signal_quality;
        } mobile;
    } specific;
};

/* Connection information structure */
struct _NMConnectionInfo {
    gchar                *uuid;
    gchar                *id;
    gchar                *type;
    NMConnectionState     state;
    gboolean              autoconnect;
    guint64               timestamp;
    
    /* Connection-specific settings */
    GHashTable           *settings;
};

/* Callback types */
typedef void (*NMInterfaceCallback)        (NMInterface *nm_interface,
                                           gpointer user_data);
typedef void (*NMDeviceCallback)           (NMInterface *nm_interface,
                                           NMDeviceInfo *device,
                                           gpointer user_data);
typedef void (*NMConnectionCallback)       (NMInterface *nm_interface,
                                           NMConnectionInfo *connection,
                                           gpointer user_data);
typedef void (*NMStateChangedCallback)     (NMInterface *nm_interface,
                                           NMState state,
                                           gpointer user_data);

/* NMInterface functions */
NMInterface         *nm_interface_new                    (void);
void                 nm_interface_free                   (NMInterface *nm_interface);

/* Connection management */
gboolean             nm_interface_init                   (NMInterface *nm_interface,
                                                         GError **error);
void                 nm_interface_shutdown               (NMInterface *nm_interface);

/* Device operations */
GList               *nm_interface_get_devices            (NMInterface *nm_interface);
NMDeviceInfo        *nm_interface_get_device_info        (NMInterface *nm_interface,
                                                         const gchar *device_path);
void                 nm_interface_free_device_info       (NMDeviceInfo *info);

/* Connection operations */
GList               *nm_interface_get_connections        (NMInterface *nm_interface);
NMConnectionInfo    *nm_interface_get_connection_info    (NMInterface *nm_interface,
                                                         const gchar *connection_path);
void                 nm_interface_free_connection_info   (NMConnectionInfo *info);

gboolean             nm_interface_activate_connection    (NMInterface *nm_interface,
                                                         const gchar *connection_uuid,
                                                         const gchar *device_path,
                                                         GError **error);
gboolean             nm_interface_deactivate_connection  (NMInterface *nm_interface,
                                                         const gchar *active_path,
                                                         GError **error);

/* Wi-Fi specific operations */
GList               *nm_interface_get_access_points      (NMInterface *nm_interface,
                                                         const gchar *device_path);
gboolean             nm_interface_request_scan          (NMInterface *nm_interface,
                                                         const gchar *device_path,
                                                         GError **error);

/* Signal handlers */
void                 nm_interface_set_state_changed_cb   (NMInterface *nm_interface,
                                                         NMStateChangedCallback callback,
                                                         gpointer user_data);
void                 nm_interface_set_device_added_cb    (NMInterface *nm_interface,
                                                         NMDeviceCallback callback,
                                                         gpointer user_data);
void                 nm_interface_set_device_removed_cb  (NMInterface *nm_interface,
                                                         NMDeviceCallback callback,
                                                         gpointer user_data);

/* Utility functions */
const gchar         *nm_interface_device_type_to_string  (NMDeviceType type);
const gchar         *nm_interface_state_to_string        (NMConnectionState state);

G_END_DECLS

#endif /* __NM_INTERFACE_H__ */
