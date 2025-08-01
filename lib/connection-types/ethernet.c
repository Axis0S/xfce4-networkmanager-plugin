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

#include "ethernet.h"
#include <libnm/NetworkManager.h>

struct EthernetConnection {
    NMDevice *device;
    NMConnection *connection;
};

static EthernetConnection *
create_ethernet_connection(NMDevice *device) {
    EthernetConnection *ethernet_conn = g_new0(EthernetConnection, 1);
    ethernet_conn->device = device;
    ethernet_conn->connection = nm_simple_connection_new();

    /* Here we can set up the default connection settings for Ethernet */
    /* For now it's a stub */

    return ethernet_conn;
}

void
initialize_ethernet_connection(NMDevice *device) {
    EthernetConnection *ethernet_conn = create_ethernet_connection(device);
    
    /* Initialize or register connection with NetworkManager */
    
    /* Free resources */
    g_object_unref(ethernet_conn->connection);
    g_free(ethernet_conn);
}
