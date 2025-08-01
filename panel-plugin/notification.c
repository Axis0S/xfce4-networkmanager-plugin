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

#include "notification.h"
#include <libxfce4ui/libxfce4ui.h>

struct _NotificationManager {
    gboolean notifications_enabled;
    gboolean initialized;
};

NotificationManager *
notification_manager_new(void)
{
    NotificationManager *manager = g_new0(NotificationManager, 1);
    
    /* Initialize libnotify if not already done */
    if (!notify_is_initted()) {
        manager->initialized = notify_init("xfce4-networkmanager-plugin");
    } else {
        manager->initialized = TRUE;
    }
    
    manager->notifications_enabled = TRUE;  /* TODO: Load from settings */
    
    return manager;
}

void
notification_manager_free(NotificationManager *manager)
{
    if (!manager)
        return;
    
    if (manager->initialized && notify_is_initted()) {
        notify_uninit();
    }
    
    g_free(manager);
}

static const gchar *
notification_type_to_icon(NotificationType type)
{
    switch (type) {
        case NOTIFICATION_TYPE_INFO:
            return "dialog-information";
        case NOTIFICATION_TYPE_SUCCESS:
            return "emblem-ok";
        case NOTIFICATION_TYPE_WARNING:
            return "dialog-warning";
        case NOTIFICATION_TYPE_ERROR:
            return "dialog-error";
        default:
            return "dialog-information";
    }
}

static const gchar *
notification_type_to_urgency(NotificationType type)
{
    switch (type) {
        case NOTIFICATION_TYPE_ERROR:
            return "critical";
        case NOTIFICATION_TYPE_WARNING:
            return "normal";
        case NOTIFICATION_TYPE_SUCCESS:
        case NOTIFICATION_TYPE_INFO:
        default:
            return "low";
    }
}

void
notification_show(NotificationManager *manager,
                  const gchar *title,
                  const gchar *message,
                  NotificationType type)
{
    NotifyNotification *notification;
    const gchar *icon_name;
    GError *error = NULL;
    
    if (!manager || !manager->notifications_enabled || !manager->initialized)
        return;
    
    icon_name = notification_type_to_icon(type);
    
    notification = notify_notification_new(title, message, icon_name);
    
    /* Set urgency */
    if (type == NOTIFICATION_TYPE_ERROR) {
        notify_notification_set_urgency(notification, NOTIFY_URGENCY_CRITICAL);
    } else if (type == NOTIFICATION_TYPE_WARNING) {
        notify_notification_set_urgency(notification, NOTIFY_URGENCY_NORMAL);
    } else {
        notify_notification_set_urgency(notification, NOTIFY_URGENCY_LOW);
    }
    
    /* Set timeout */
    notify_notification_set_timeout(notification, 5000);  /* 5 seconds */
    
    /* Show the notification */
    if (!notify_notification_show(notification, &error)) {
        g_warning("Failed to show notification: %s", error->message);
        g_error_free(error);
    }
    
    g_object_unref(notification);
}

void
notification_show_connection_status(NotificationManager *manager,
                                   const gchar *connection_name,
                                   gboolean connected,
                                   const gchar *error_message)
{
    gchar *title;
    gchar *message;
    NotificationType type;
    
    if (connected) {
        title = g_strdup_printf(_("Connected to %s"), connection_name);
        message = g_strdup(_("Network connection established successfully"));
        type = NOTIFICATION_TYPE_SUCCESS;
    } else if (error_message) {
        title = g_strdup_printf(_("Failed to connect to %s"), connection_name);
        message = g_strdup(error_message);
        type = NOTIFICATION_TYPE_ERROR;
    } else {
        title = g_strdup_printf(_("Disconnected from %s"), connection_name);
        message = g_strdup(_("Network connection terminated"));
        type = NOTIFICATION_TYPE_INFO;
    }
    
    notification_show(manager, title, message, type);
    
    g_free(title);
    g_free(message);
}

void
notification_show_network_error(NotificationManager *manager,
                               const gchar *operation,
                               const gchar *error_message)
{
    gchar *title;
    gchar *message;
    
    title = g_strdup_printf(_("Network Error: %s"), operation);
    
    /* Make error messages more user-friendly */
    if (g_str_has_prefix(error_message, "GDBus.Error:")) {
        /* Extract the actual error after the GDBus prefix */
        const gchar *actual_error = strstr(error_message, ": ");
        if (actual_error && *(actual_error + 2)) {
            message = g_strdup(actual_error + 2);
        } else {
            message = g_strdup(error_message);
        }
    } else if (strstr(error_message, "Secrets were required")) {
        message = g_strdup(_("Invalid password or authentication failed"));
    } else if (strstr(error_message, "No suitable device")) {
        message = g_strdup(_("No suitable network device found"));
    } else if (strstr(error_message, "Connection activation failed")) {
        message = g_strdup(_("Failed to activate connection. Please check your settings."));
    } else {
        message = g_strdup(error_message);
    }
    
    notification_show(manager, title, message, NOTIFICATION_TYPE_ERROR);
    
    g_free(title);
    g_free(message);
}

GtkWidget *
notification_create_inline_message(const gchar *message,
                                  NotificationType type)
{
    GtkWidget *box;
    GtkWidget *icon;
    GtkWidget *label;
    const gchar *icon_name;
    const gchar *css_class;
    
    /* Create horizontal box */
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 5);
    gtk_widget_set_margin_bottom(box, 5);
    
    /* Add appropriate CSS class for styling */
    switch (type) {
        case NOTIFICATION_TYPE_ERROR:
            css_class = "error";
            break;
        case NOTIFICATION_TYPE_WARNING:
            css_class = "warning";
            break;
        case NOTIFICATION_TYPE_SUCCESS:
            css_class = "success";
            break;
        default:
            css_class = "info";
            break;
    }
    
    gtk_style_context_add_class(gtk_widget_get_style_context(box), css_class);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "inline-notification");
    
    /* Add icon */
    icon_name = notification_type_to_icon(type);
    icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(box), icon, FALSE, FALSE, 0);
    
    /* Add message label */
    label = gtk_label_new(message);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    
    /* Store the label widget for later updates */
    g_object_set_data(G_OBJECT(box), "label", label);
    g_object_set_data(G_OBJECT(box), "icon", icon);
    
    gtk_widget_show_all(box);
    
    return box;
}

void
notification_update_inline_message(GtkWidget *message_widget,
                                  const gchar *message,
                                  NotificationType type)
{
    GtkWidget *label;
    GtkWidget *icon;
    const gchar *icon_name;
    const gchar *css_class;
    GtkStyleContext *context;
    
    if (!message_widget)
        return;
    
    /* Get stored widgets */
    label = g_object_get_data(G_OBJECT(message_widget), "label");
    icon = g_object_get_data(G_OBJECT(message_widget), "icon");
    
    if (label) {
        gtk_label_set_text(GTK_LABEL(label), message);
    }
    
    if (icon) {
        icon_name = notification_type_to_icon(type);
        gtk_image_set_from_icon_name(GTK_IMAGE(icon), icon_name, GTK_ICON_SIZE_MENU);
    }
    
    /* Update CSS class */
    context = gtk_widget_get_style_context(message_widget);
    
    /* Remove old classes */
    gtk_style_context_remove_class(context, "error");
    gtk_style_context_remove_class(context, "warning");
    gtk_style_context_remove_class(context, "success");
    gtk_style_context_remove_class(context, "info");
    
    /* Add new class */
    switch (type) {
        case NOTIFICATION_TYPE_ERROR:
            css_class = "error";
            break;
        case NOTIFICATION_TYPE_WARNING:
            css_class = "warning";
            break;
        case NOTIFICATION_TYPE_SUCCESS:
            css_class = "success";
            break;
        default:
            css_class = "info";
            break;
    }
    
    gtk_style_context_add_class(context, css_class);
}
