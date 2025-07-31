/*
 * Copyright (C) 2023 XFCE4 NetworkManager Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __POPUP_WINDOW_H__
#define __POPUP_WINDOW_H__

#include <gtk/gtk.h>
#include "plugin.h"
#include "nm-interface.h"
#include "nm-interface.h"

G_BEGIN_DECLS

typedef struct _PopupWindow PopupWindow;

struct _PopupWindow {
    GtkWidget            *window;
    GtkWidget            *main_box;
    GtkWidget            *header_box;
    GtkWidget            *content_box;
    GtkWidget            *scrolled_window;
    GtkWidget            *network_list;
    GtkWidget            *search_entry;
    
    NetworkManagerPlugin *plugin;
    NMInterface          *nm_interface;
    
    /* Current filter */
    gchar                *filter_text;
    
    /* Update timer */
    guint                 update_timer;
};

/* Function prototypes */
PopupWindow    *popup_window_new               (NetworkManagerPlugin *plugin);
void            popup_window_free              (PopupWindow *popup);
void            popup_window_show              (PopupWindow *popup,
                                               GdkRectangle *button_rect);
void            popup_window_hide              (PopupWindow *popup);
void            popup_window_toggle            (PopupWindow *popup,
                                               GdkRectangle *button_rect);
void            popup_window_update_networks   (PopupWindow *popup);
void            popup_window_set_transparency  (PopupWindow *popup,
                                               gint transparency);

G_END_DECLS

#endif /* __POPUP_WINDOW_H__ */
