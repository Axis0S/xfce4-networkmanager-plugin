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

#include "vpn.h"
#include <libnm/NetworkManager.h>

struct VpnConnection {
    NMDevice *device;
    NMConnection *connection;
    gchar *vpn_type;
    gchar *username;
    gchar *password;
};

static VpnConnection *
create_vpn_connection(NMDevice *device) {
    VpnConnection *vpn_conn = g_new0(VpnConnection, 1);
    vpn_conn->device = device;
    vpn_conn->connection = nm_simple_connection_new();

    /* Here we can set up the default connection settings for VPN */
    /* For now it's a stub */

    return vpn_conn;
}

void
initialize_vpn_connection(NMDevice *device, const gchar *vpn_type) {
    VpnConnection *vpn_conn = create_vpn_connection(device);
    vpn_conn->vpn_type = g_strdup(vpn_type);
    
    /* Initialize or register connection with NetworkManager */
    
    /* Free resources */
    g_object_unref(vpn_conn->connection);
    g_free(vpn_conn->vpn_type);
    g_free(vpn_conn->username);
    g_free(vpn_conn->password);
    g_free(vpn_conn);
}
