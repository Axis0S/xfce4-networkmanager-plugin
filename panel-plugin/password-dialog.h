/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PASSWORD_DIALOG_H__
#define __PASSWORD_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Function to show password dialog and get password */
gchar *password_dialog_show(GtkWindow *parent, const gchar *network_name);

G_END_DECLS

#endif /* __PASSWORD_DIALOG_H__ */
