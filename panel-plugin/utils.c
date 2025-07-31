/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include config.h
#endif

#include "utils.h"

gchar *
utils_format_bandwidth(guint64 bytes)
{
    if (bytes < 1024)
        return g_strdup_printf("%lu B", bytes);
    else if (bytes < 1024 * 1024)
        return g_strdup_printf("%.1f KB", bytes / 1024.0);
    else if (bytes < 1024 * 1024 * 1024)
        return g_strdup_printf("%.1f MB", bytes / (1024.0 * 1024.0));
    else
        return g_strdup_printf("%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
}

gchar *
utils_format_time_span(guint64 seconds)
{
    if (seconds < 60)
        return g_strdup_printf("%lu sec", seconds);
    else if (seconds < 3600)
        return g_strdup_printf("%lu min", seconds / 60);
    else if (seconds < 86400)
        return g_strdup_printf("%lu hours", seconds / 3600);
    else
        return g_strdup_printf("%lu days", seconds / 86400);
}

gboolean
utils_program_exists(const gchar *program)
{
    gchar *path;
    gboolean exists;

    path = g_find_program_in_path(program);
    exists = (path != NULL);
    g_free(path);

    return exists;
}
