/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "password-dialog.h"

/* Function to show password dialog and get password */
gchar *
password_dialog_show(GtkWindow *parent, const gchar *network_name)
{
    GtkWidget *dialog, *content_area, *label, *entry;
    GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
    gchar *password = NULL;

    /* Create the dialog window */
    dialog = gtk_dialog_new_with_buttons("Network Password",
                                         parent,
                                         flags,
                                         _("Connect"),
                                         GTK_RESPONSE_ACCEPT,
                                         _("Cancel"),
                                         GTK_RESPONSE_REJECT,
                                         NULL);

    /* Content area of the dialog */
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    /* Label */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), g_strdup_printf("Enter password for \u003cb\u003e%s\u003c/b\u003e:", network_name));
    gtk_container_add(GTK_CONTAINER(content_area), label);

    /* Password entry */
    entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(entry), GTK_INPUT_PURPOSE_PASSWORD);
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    
    /* Show all widgets in the dialog */
    gtk_widget_show_all(dialog);

    /* Run dialog and capture response */
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar *entered_password = gtk_entry_get_text(GTK_ENTRY(entry));
        if (g_strcmp0(entered_password, "") == 0) {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                                           GTK_DIALOG_MODAL |
                                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_CLOSE,
                                                           "Password cannot be empty!");
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        } else {
            password = g_strdup(entered_password);
        }
    }

    /* Destroy dialog */
    gtk_widget_destroy(dialog);

    return password;
}

