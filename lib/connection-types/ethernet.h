/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __ETHERNET_H__
#define __ETHERNET_H__

#include <glib.h>
#include <NetworkManager.h>

G_BEGIN_DECLS

typedef struct _EthernetConnection EthernetConnection;

void initialize_ethernet_connection(NMDevice *device);
GVariant *ethernet_create_connection_gvariant(const gchar *id);

G_END_DECLS

#endif /* __ETHERNET_H__ */
