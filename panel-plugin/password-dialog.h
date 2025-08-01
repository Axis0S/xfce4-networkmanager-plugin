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

/* Simple password dialog */
gchar *password_dialog_show(GtkWindow *parent, const gchar *network_name);

/* Enterprise authentication info structure */
typedef struct {
    gchar *eap_method;
    gchar *identity;
    gchar *password;
    gchar *anonymous_identity;
    gchar *ca_cert;
    gchar *client_cert;
    gchar *private_key;
    gchar *private_key_password;
    gchar *phase2_auth;
} EnterpriseAuthInfo;

/* Function to show enterprise authentication dialog */
EnterpriseAuthInfo *password_dialog_show_enterprise(GtkWindow *parent, const gchar *network_name);

/* Function to free enterprise auth info */
void enterprise_auth_info_free(EnterpriseAuthInfo *info);

G_END_DECLS

#endif /* __PASSWORD_DIALOG_H__ */
