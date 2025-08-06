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

struct _VpnConnection {
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

GVariant *vpn_create_connection_gvariant(const gchar *id, const gchar *server)
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
    g_variant_builder_add(&connection_section_builder, "{sv}", "type", g_variant_new_string("vpn"));
    g_variant_builder_add(&connection_section_builder, "{sv}", "id", g_variant_new_string(id));
    g_variant_builder_add(&connection_section_builder, "{sv}", "uuid", g_variant_new_string(uuid));
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "connection", &connection_section_builder);

    GVariantBuilder vpn_section_builder;
    g_variant_builder_init(&vpn_section_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&vpn_section_builder, "{sv}", "service-type", g_variant_new_string("org.freedesktop.NetworkManager.openvpn"));
    g_variant_builder_add(&vpn_section_builder, "{sv}", "remote", g_variant_new_string(server));
    g_variant_builder_add(&connection_builder, "{sa{sv}}", "vpn", &vpn_section_builder);

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
