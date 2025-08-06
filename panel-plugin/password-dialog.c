/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "password-dialog.h"
#include <gtk/gtk.h>
#include <libintl.h>

#define _(String) gettext(String)

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
    gtk_label_set_markup(GTK_LABEL(label), g_strdup_printf("Enter password for <b>%s</b>:", network_name));
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

/* Function to show enterprise authentication dialog */
EnterpriseAuthInfo *
password_dialog_show_enterprise(GtkWindow *parent, const gchar *network_name)
{
    GtkWidget *dialog, *content_area, *grid;
    GtkWidget *eap_label, *eap_combo;
    GtkWidget *identity_label, *identity_entry;
    GtkWidget *password_label, *password_entry;
    GtkWidget *anon_identity_label, *anon_identity_entry;
    GtkWidget *ca_cert_label, *ca_cert_chooser;
    GtkWidget *client_cert_label, *client_cert_chooser;
    GtkWidget *private_key_label, *private_key_chooser;
    GtkWidget *private_key_password_label, *private_key_password_entry;
    GtkWidget *phase2_label, *phase2_combo;
    GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
    EnterpriseAuthInfo *auth_info = NULL;
    gint row = 0;

    /* Create the dialog window */
    dialog = gtk_dialog_new_with_buttons(_("Enterprise Authentication"),
                                         parent,
                                         flags,
                                         _("Connect"),
                                         GTK_RESPONSE_ACCEPT,
                                         _("Cancel"),
                                         GTK_RESPONSE_REJECT,
                                         NULL);

    /* Content area of the dialog */
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);

    /* Create grid for form layout */
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    /* Network name label */
    GtkWidget *network_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(network_label), 
                         g_strdup_printf(_("Connect to <b>%s</b>"), network_name));
    gtk_grid_attach(GTK_GRID(grid), network_label, 0, row++, 2, 1);

    /* EAP Method */
    eap_label = gtk_label_new(_("EAP method:"));
    gtk_widget_set_halign(eap_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), eap_label, 0, row, 1, 1);
    
    eap_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eap_combo), "peap", "PEAP");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eap_combo), "ttls", "TTLS");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eap_combo), "tls", "TLS");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eap_combo), "leap", "LEAP");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eap_combo), "pwd", "PWD");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(eap_combo), "fast", "FAST");
    gtk_combo_box_set_active(GTK_COMBO_BOX(eap_combo), 0); /* Default to PEAP */
    gtk_widget_set_hexpand(eap_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), eap_combo, 1, row++, 1, 1);

    /* Phase 2 authentication */
    phase2_label = gtk_label_new(_("Phase 2 authentication:"));
    gtk_widget_set_halign(phase2_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), phase2_label, 0, row, 1, 1);
    
    phase2_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(phase2_combo), "mschapv2", "MSCHAPv2");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(phase2_combo), "mschap", "MSCHAP");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(phase2_combo), "pap", "PAP");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(phase2_combo), "chap", "CHAP");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(phase2_combo), "gtc", "GTC");
    gtk_combo_box_set_active(GTK_COMBO_BOX(phase2_combo), 0); /* Default to MSCHAPv2 */
    gtk_widget_set_hexpand(phase2_combo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), phase2_combo, 1, row++, 1, 1);

    /* Identity */
    identity_label = gtk_label_new(_("Identity:"));
    gtk_widget_set_halign(identity_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), identity_label, 0, row, 1, 1);
    
    identity_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(identity_entry), _("username@example.com"));
    gtk_widget_set_hexpand(identity_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), identity_entry, 1, row++, 1, 1);

    /* Anonymous identity */
    anon_identity_label = gtk_label_new(_("Anonymous identity:"));
    gtk_widget_set_halign(anon_identity_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), anon_identity_label, 0, row, 1, 1);
    
    anon_identity_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(anon_identity_entry), _("anonymous@example.com (optional)"));
    gtk_widget_set_hexpand(anon_identity_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), anon_identity_entry, 1, row++, 1, 1);

    /* Password */
    password_label = gtk_label_new(_("Password:"));
    gtk_widget_set_halign(password_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, row, 1, 1);
    
    password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(password_entry), GTK_INPUT_PURPOSE_PASSWORD);
    gtk_widget_set_hexpand(password_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, row++, 1, 1);

    /* CA certificate */
    ca_cert_label = gtk_label_new(_("CA certificate:"));
    gtk_widget_set_halign(ca_cert_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), ca_cert_label, 0, row, 1, 1);
    
    ca_cert_chooser = gtk_file_chooser_button_new(_("Select CA Certificate"), 
                                                  GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(ca_cert_chooser), 
                               gtk_file_filter_new());
    GtkFileFilter *cert_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(cert_filter, _("Certificate files"));
    gtk_file_filter_add_pattern(cert_filter, "*.pem");
    gtk_file_filter_add_pattern(cert_filter, "*.crt");
    gtk_file_filter_add_pattern(cert_filter, "*.cer");
    gtk_file_filter_add_pattern(cert_filter, "*.p12");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(ca_cert_chooser), cert_filter);
    gtk_widget_set_hexpand(ca_cert_chooser, TRUE);
    gtk_grid_attach(GTK_GRID(grid), ca_cert_chooser, 1, row++, 1, 1);

    /* Show all widgets in the dialog */
    gtk_widget_show_all(dialog);

    /* Run dialog and capture response */
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar *identity = gtk_entry_get_text(GTK_ENTRY(identity_entry));
        const gchar *password = gtk_entry_get_text(GTK_ENTRY(password_entry));
        
        if (g_strcmp0(identity, "") == 0 || g_strcmp0(password, "") == 0) {
            GtkWidget *error_dialog = gtk_message_dialog_new(GTK_WINDOW(dialog),
                                                           GTK_DIALOG_MODAL |
                                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_CLOSE,
                                                           _("Identity and password are required!"));
            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
        } else {
            auth_info = g_new0(EnterpriseAuthInfo, 1);
            
            /* Get EAP method */
            auth_info->eap_method = g_strdup(gtk_combo_box_get_active_id(GTK_COMBO_BOX(eap_combo)));
            
            /* Get Phase 2 auth */
            auth_info->phase2_auth = g_strdup(gtk_combo_box_get_active_id(GTK_COMBO_BOX(phase2_combo)));
            
            /* Get identity and password */
            auth_info->identity = g_strdup(identity);
            auth_info->password = g_strdup(password);
            
            /* Get anonymous identity if provided */
            const gchar *anon_identity = gtk_entry_get_text(GTK_ENTRY(anon_identity_entry));
            if (anon_identity && *anon_identity) {
                auth_info->anonymous_identity = g_strdup(anon_identity);
            }
            
            /* Get CA certificate if selected */
            gchar *ca_cert_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ca_cert_chooser));
            if (ca_cert_path) {
                auth_info->ca_cert = ca_cert_path;
            }
        }
    }

    /* Destroy dialog */
    gtk_widget_destroy(dialog);

    return auth_info;
}

/* Function to free enterprise auth info */
void
enterprise_auth_info_free(EnterpriseAuthInfo *info)
{
    if (!info)
        return;
        
    g_free(info->eap_method);
    g_free(info->identity);
    g_free(info->password);
    g_free(info->anonymous_identity);
    g_free(info->ca_cert);
    g_free(info->client_cert);
    g_free(info->private_key);
    g_free(info->private_key_password);
    g_free(info->phase2_auth);
    g_free(info);
}
