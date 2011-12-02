/*
 * entry.c
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors: Glynn Foster <glynn.foster@sun.com>
 */

#include "config.h"

#include "matedialog.h"
#include "util.h"

static void matedialog_entry_dialog_response (GtkWidget *widget, int response, gpointer data);

static GtkWidget *entry;
static gint n_entries = 0;

static void
matedialog_entry_fill_entries (GSList **entries, const gchar **args)
{
  gint i = 0;

  while (args[i] != NULL) {
    *entries = g_slist_append (*entries, (gchar *) args[i]);
    i++;
  }
}

static void
matedialog_entry_combo_activate_default (GtkEntry *entry, gpointer window)
{
  gtk_window_activate_default (GTK_WINDOW (window));
}

void 
matedialog_entry (MateDialogData *data, MateDialogEntryData *entry_data)
{
  GtkBuilder *builder = NULL;
  GtkWidget *dialog;
  GObject *text;
  GSList *entries = NULL; 
  GSList *tmp;
  GObject *vbox;
  
  builder = matedialog_util_load_ui_file ("matedialog_entry_dialog", NULL);

  if (builder == NULL) {
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }
	
  gtk_builder_connect_signals (builder, NULL);
	
  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "matedialog_entry_dialog"));
	
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (matedialog_entry_dialog_response), data);

  if (data->dialog_title)
    gtk_window_set_title (GTK_WINDOW (dialog), data->dialog_title);
	
  matedialog_util_set_window_icon (dialog, data->window_icon, MATEDIALOG_IMAGE_FULLPATH ("matedialog-entry.png"));

  if (data->width > -1 || data->height > -1)
    gtk_window_set_default_size (GTK_WINDOW (dialog), data->width, data->height);

  text = gtk_builder_get_object (builder, "matedialog_entry_text");

  if (entry_data->dialog_text)
    gtk_label_set_text_with_mnemonic (GTK_LABEL (text), g_strcompress (entry_data->dialog_text));
  
  vbox = gtk_builder_get_object (builder, "vbox4");
  
  matedialog_entry_fill_entries(&entries, entry_data->data);
  
  n_entries = g_slist_length (entries);

  if (n_entries > 1) {
    entry = gtk_combo_box_entry_new_text ();

    for (tmp = entries; tmp; tmp = tmp->next) {
      gtk_combo_box_append_text (GTK_COMBO_BOX (entry), tmp->data);
    }

    if (entry_data->entry_text) {
      gtk_combo_box_prepend_text (GTK_COMBO_BOX (entry), entry_data->entry_text);
      gtk_combo_box_set_active (GTK_COMBO_BOX (entry), 0);
    }

    g_signal_connect (gtk_bin_get_child (GTK_BIN (entry)), "activate",
                      G_CALLBACK (matedialog_entry_combo_activate_default),
                      GTK_WINDOW (dialog));
  } else {
    entry = gtk_entry_new();

    gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
    
    if (entry_data->entry_text)
      gtk_entry_set_text (GTK_ENTRY (entry), entry_data->entry_text);

    if (entry_data->hide_text)
      g_object_set (G_OBJECT (entry), "visibility", FALSE, NULL);
  }

  gtk_widget_show (entry);

  gtk_box_pack_end (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  gtk_label_set_mnemonic_widget (GTK_LABEL (text), entry);

  g_object_unref (builder);

  matedialog_util_show_dialog (dialog);

  if(data->timeout_delay > 0) {
    g_timeout_add_seconds (data->timeout_delay, (GSourceFunc) matedialog_util_timeout_handle, NULL);
  }

  gtk_main ();
}

static void
matedialog_entry_dialog_response (GtkWidget *widget, int response, gpointer data)
{
  MateDialogData *zen_data = data;
  const gchar *text;

  switch (response) {
    case GTK_RESPONSE_OK:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_OK);
      if (n_entries > 1) {
        text = gtk_combo_box_get_active_text (GTK_COMBO_BOX (entry));
      }
      else {
        text = gtk_entry_get_text (GTK_ENTRY (entry));      
      }

      if (text != NULL)
        g_print ("%s\n", text);

      break;

    case GTK_RESPONSE_CANCEL:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_CANCEL);
      break;

    default:
      /* Esc dialog */
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ESC);
      break;
  }
  gtk_main_quit ();
}
