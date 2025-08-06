/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MOBILE_H__
#define __MOBILE_H__

#include <glib.h>
#include <NetworkManager.h>

G_BEGIN_DECLS

typedef struct _MobileConnection MobileConnection;

void initialize_mobile_connection(NMDevice *device, const gchar *apn);
GVariant *mobile_create_connection_gvariant(const gchar *id, const gchar *apn);

G_END_DECLS

#endif /* __MOBILE_H__ */
