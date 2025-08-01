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

#include "mobile.h"
#include <libnm/NetworkManager.h>

struct MobileConnection {
    NMDevice *device;
    NMConnection *connection;
    gchar *apn;
    gchar *username;
    gchar *password;
};

static MobileConnection *
create_mobile_connection(NMDevice *device) {
    MobileConnection *mobile_conn = g_new0(MobileConnection, 1);
    mobile_conn->device = device;
    mobile_conn->connection = nm_simple_connection_new();

    /* Here we can set up the default connection settings for Mobile */
    /* For now it's a stub */

    return mobile_conn;
}

void
initialize_mobile_connection(NMDevice *device, const gchar *apn) {
    MobileConnection *mobile_conn = create_mobile_connection(device);
    mobile_conn->apn = g_strdup(apn);
    
    /* Initialize or register connection with NetworkManager */
    
    /* Free resources */
    g_object_unref(mobile_conn->connection);
    g_free(mobile_conn->apn);
    g_free(mobile_conn->username);
    g_free(mobile_conn->password);
    g_free(mobile_conn);
}
