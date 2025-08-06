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
#include <uuid/uuid.h>

struct _EthernetConnection {
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

GVariant *ethernet_create_connection_gvariant(const gchar *id)
{
    GVariantBuilder connection_builder;
    GVariant *connection_dict;
    gchar *uuid;

    uuid = g_new0(gchar, 37);
    uuid_t uuid_bin;
    uuid_generate(uuid_bin);
    uuid_unparse(uuid_bin, uuid);

    g_variant_builder_init(&connection_builder, G_VARIANT_TYPE("a{sa{sv}}"));

    GVariantBuilder connection_section_builder;
    g_variant_builder_init(&connection_section_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&connection_section_builder, "{sv}", "type", g_variant_new_string("802-3-ethernet"));
    g_variant_builder_add(&connection_section_builder, "{sv}", "id", g_variant_new_string(id));
    g_variant_builder_add(&connection_section_builder, "{sv}", "uuid", g_variant_new_string(uuid));
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "connection", &connection_section_builder);

    GVariantBuilder ethernet_section_builder;
    g_variant_builder_init(&ethernet_section_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&ethernet_section_builder, "{sv}", "auto-negotiate", g_variant_new_boolean(TRUE));
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "802-3-ethernet", &ethernet_section_builder);

    GVariantBuilder ipv4_section_builder;
    g_variant_builder_init(&ipv4_section_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&ipv4_section_builder, "{sv}", "method", g_variant_new_string("auto"));
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "ipv4", &ipv4_section_builder);

    GVariantBuilder ipv6_section_builder;
    g_variant_builder_init(&ipv6_section_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&ipv6_section_builder, "{sv}", "method", g_variant_new_string("auto"));
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "ipv6", &ipv6_section_builder);

    connection_dict = g_variant_builder_end(&connection_builder);

    g_free(uuid);

    return connection_dict;
}
