/*
 * password.c
 *
 * Copyright (C) 2010 Arx Cruz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 121 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors: Arx Cruz <arxcruz@gmail.com>
 */

#include "config.h"
#include <string.h>
#include "matedialog.h"
#include "util.h"

static MateDialogData *zen_data;

static void matedialog_password_dialog_response (GtkWidget *widget, int response, gpointer data);

void matedialog_password_dialog (MateDialogData *data, MateDialogPasswordData *password_data)
{
  GtkWidget *dialog;
  GtkWidget *image;
  GtkWidget *hbox;
  GtkWidget *vbox_labels;
  GtkWidget *vbox_entries;
  GtkWidget *label;
  GtkWidget *align;

  zen_data = data;

  dialog = gtk_dialog_new ();

  gtk_dialog_add_button(GTK_DIALOG(dialog), 
                        GTK_STOCK_CANCEL, 
                        GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button(GTK_DIALOG(dialog), 
                        GTK_STOCK_OK, 
                        GTK_RESPONSE_OK);
  
  image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION,
                                   GTK_ICON_SIZE_DIALOG); 
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), 
                                  GTK_RESPONSE_OK);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox),
                     image,
                     FALSE,
                     FALSE,
                     12);
  label = gtk_label_new(_("Type your password"));
  gtk_box_pack_start(GTK_BOX(hbox),
                     label,
                     FALSE,
                     FALSE,
                     12);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     hbox,
                     FALSE,
                     TRUE,
                     5);
                     
  vbox_labels = gtk_vbox_new(FALSE, 5);
  vbox_entries = gtk_vbox_new(FALSE, 5);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                     hbox,
                     FALSE,
                     TRUE,
                     5);

  gtk_box_pack_start(GTK_BOX(hbox),
                     vbox_labels,
                     FALSE,
                     TRUE,
                     12);
  gtk_box_pack_start(GTK_BOX(hbox),
                     vbox_entries,
                     TRUE,
                     TRUE,
                     12);

  if(password_data->username) {
    align = gtk_alignment_new(0.0, 0.12, 0.0, 0.0);
    label = gtk_label_new(_("Username:"));
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start(GTK_BOX(vbox_labels),
                       align,
                       TRUE,
                       FALSE,
                       12);
    password_data->entry_username = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox_entries),
                       password_data->entry_username,
                       TRUE,
                       TRUE,
                       12);
  }

  align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
  label = gtk_label_new(_("Password:"));
  gtk_container_add(GTK_CONTAINER(align), label);

  gtk_box_pack_start(GTK_BOX(vbox_labels),
                     align,
                     TRUE,
                     FALSE,
                     12);
  password_data->entry_password = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(password_data->entry_password), 
                           FALSE);
  gtk_box_pack_start(GTK_BOX(vbox_entries),
                     password_data->entry_password,
                     TRUE,
                     TRUE,
                     12);

  if (data->dialog_title)
    gtk_window_set_title (GTK_WINDOW (dialog), data->dialog_title);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (matedialog_password_dialog_response),
                    password_data);
  gtk_widget_show_all(GTK_WIDGET(gtk_dialog_get_content_area(GTK_DIALOG(dialog))));
  matedialog_util_show_dialog (dialog);

  if (data->timeout_delay > 0) {
    g_timeout_add (data->timeout_delay * 1000,
                   (GSourceFunc) matedialog_util_timeout_handle,
                   NULL);
  }
  gtk_main();
}

static void
matedialog_password_dialog_response (GtkWidget *widget, int response, gpointer data)
{
  MateDialogPasswordData *password_data = (MateDialogPasswordData *) data;
  switch (response) {
    case GTK_RESPONSE_OK:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_OK);
      g_print ("%s\n", gtk_entry_get_text (GTK_ENTRY(password_data->entry_password)));
      break;

    case GTK_RESPONSE_CANCEL:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_CANCEL);
      break;

    default:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ESC);
      break;
  }

  gtk_main_quit ();
}
