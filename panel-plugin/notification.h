/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __NOTIFICATION_H__
#define __NOTIFICATION_H__

#include <gtk/gtk.h>
#include <libnotify/notify.h>

G_BEGIN_DECLS

typedef enum {
    NOTIFICATION_TYPE_INFO,
    NOTIFICATION_TYPE_SUCCESS,
    NOTIFICATION_TYPE_WARNING,
    NOTIFICATION_TYPE_ERROR
} NotificationType;

typedef struct _NotificationManager NotificationManager;

/* Notification manager functions */
NotificationManager *notification_manager_new              (void);
void                 notification_manager_free             (NotificationManager *manager);

/* Show notifications */
void                 notification_show                     (NotificationManager *manager,
                                                           const gchar *title,
                                                           const gchar *message,
                                                           NotificationType type);

void                 notification_show_connection_status   (NotificationManager *manager,
                                                           const gchar *connection_name,
                                                           gboolean connected,
                                                           const gchar *error_message);

void                 notification_show_network_error       (NotificationManager *manager,
                                                           const gchar *operation,
                                                           const gchar *error_message);

/* Inline status messages (for popup window) */
GtkWidget           *notification_create_inline_message    (const gchar *message,
                                                           NotificationType type);

void                 notification_update_inline_message    (GtkWidget *message_widget,
                                                           const gchar *message,
                                                           NotificationType type);

G_END_DECLS

#endif /* __NOTIFICATION_H__ */
