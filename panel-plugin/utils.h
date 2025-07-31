/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <glib.h>

G_BEGIN_DECLS

/* Utility function prototypes */
gchar    *utils_format_bandwidth     (guint64 bytes);
gchar    *utils_format_time_span     (guint64 seconds);
gboolean  utils_program_exists       (const gchar *program);

G_END_DECLS

#endif /* __UTILS_H__ */
