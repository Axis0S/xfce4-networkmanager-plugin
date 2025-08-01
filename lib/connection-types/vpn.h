/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __VPN_H__
#define __VPN_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _VpnConnection VpnConnection;

void initialize_vpn_connection(NMDevice *device, const gchar *vpn_type);

G_END_DECLS

#endif /* __VPN_H__ */
