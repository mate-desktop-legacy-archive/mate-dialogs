/*
 * tree.c
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
 *          Jonathan Blanford <jrb@redhat.com>
 *          Kristian Rietveld <kris@gtk.org>
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include "matedialog.h"
#include "util.h"

#define MAX_ELEMENTS_BEFORE_SCROLLING 5
#define PRINT_HIDE_COLUMN_SEPARATOR ","

static GtkBuilder *builder;
static GSList *selected;
static gchar *separator;
static gboolean print_all_columns = FALSE;
static gint *print_columns = NULL;
static gint *hide_columns = NULL;

static int *matedialog_tree_extract_column_indexes (char *indexes, gint n_columns);
static gboolean matedialog_tree_column_is_hidden (gint column_index);
static void matedialog_tree_dialog_response (GtkWidget *widget, int response, gpointer data);
static void matedialog_tree_row_activated (GtkTreeView *tree_view, GtkTreePath *tree_path, 
                                       GtkTreeViewColumn *tree_col, gpointer data);

static gboolean
matedialog_tree_dialog_untoggle (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  GValue toggle_value = {0, };

  gtk_tree_model_get_value (model, iter, 0, &toggle_value);

  if (g_value_get_boolean (&toggle_value))
    gtk_list_store_set (GTK_LIST_STORE (model), iter, 0, FALSE, -1);
  return FALSE;
}

static void
matedialog_tree_toggled_callback (GtkCellRendererToggle *cell, gchar *path_string, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;
  gboolean value;

  model = GTK_TREE_MODEL (data);

  /* Because this is a radio list, we should untoggle the previous toggle so that 
   * we only have one selection at any given time
   */

  if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model), "radio")) == 1) {
    gtk_tree_model_foreach (model, matedialog_tree_dialog_untoggle, NULL);
  }

  path = gtk_tree_path_new_from_string (path_string);
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, 0, &value, -1);

  value = !value;
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, value, -1);

  gtk_tree_path_free (path);
}

static gboolean
matedialog_tree_handle_stdin (GIOChannel  *channel,
                          GIOCondition condition,
                          gpointer     data)
{
  static GtkTreeView *tree_view;
  GtkTreeModel *model;
  static GtkTreeIter iter;
  static gint column_count = 0;
  static gint row_count = 0;
  static gint n_columns;
  static gboolean editable;
  static gboolean toggles;
  static gboolean first_time = TRUE;

  tree_view = GTK_TREE_VIEW (data);
  n_columns = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree_view), "n_columns"));
  editable = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree_view), "editable"));
  toggles = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree_view), "toggles"));

  model = gtk_tree_view_get_model (tree_view);

  if (first_time) {
    first_time = FALSE;
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  }

  if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP)) {
    GString *string;
    GError *error = NULL;

    string = g_string_new (NULL);

    while (channel->is_readable != TRUE)
      ;
    do {
      gint status;

      do {
        status = g_io_channel_read_line_string (channel, string, NULL, &error);

        while (gtk_events_pending ())
          gtk_main_iteration ();

      } while (status == G_IO_STATUS_AGAIN);

      if (status != G_IO_STATUS_NORMAL) {
        if (error) {
          g_warning ("matedialog_tree_handle_stdin () : %s", error->message);
          g_error_free (error);
          error = NULL;
        }
        continue;
      }
    
      if (column_count == n_columns) {
        /* We're starting a new row */
        column_count = 0;
        row_count++;
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      }

      if (toggles && column_count == 0) {
        if (strcmp (g_ascii_strdown (matedialog_util_strip_newline (string->str), -1), "true") == 0)
          gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, TRUE, -1);
        else
          gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, FALSE, -1);
      }
      else {
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, matedialog_util_strip_newline (string->str), -1);        
      }

      if (editable) {
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, n_columns, TRUE, -1);
      }

      if (row_count == MAX_ELEMENTS_BEFORE_SCROLLING) {
        GtkWidget *scrolled_window;
        GtkRequisition rectangle;

        gtk_widget_size_request (GTK_WIDGET (tree_view), &rectangle);
        scrolled_window = GTK_WIDGET (gtk_builder_get_object (builder,
							 "matedialog_tree_window"));
        gtk_widget_set_size_request (scrolled_window, -1, rectangle.height);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      }

      column_count++;

    } while (g_io_channel_get_buffer_condition (channel) == G_IO_IN); 
    g_string_free (string, TRUE);
  }

  if (condition != G_IO_IN) {
    g_io_channel_shutdown (channel, TRUE, NULL);
    return FALSE;
  }
  return TRUE;
}

static void
matedialog_tree_fill_entries_from_stdin (GtkTreeView  *tree_view,
                                     gint          n_columns,
                                     gboolean      toggles,
                                     gboolean      editable)
{
  GIOChannel *channel;

  g_object_set_data (G_OBJECT (tree_view), "n_columns", GINT_TO_POINTER (n_columns));
  g_object_set_data (G_OBJECT (tree_view), "toggles", GINT_TO_POINTER (toggles));
  g_object_set_data (G_OBJECT (tree_view), "editable", GINT_TO_POINTER (editable)); 

  channel = g_io_channel_unix_new (0);
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
  g_io_add_watch (channel, G_IO_IN | G_IO_HUP, matedialog_tree_handle_stdin, tree_view);
}

static void
matedialog_tree_fill_entries (GtkTreeView  *tree_view, 
                          const gchar **args, 
                          gint          n_columns, 
                          gboolean      toggles, 
                          gboolean      editable)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint i = 0;

  model = gtk_tree_view_get_model (tree_view);

  g_object_set_data (G_OBJECT (tree_view), "n_columns", GINT_TO_POINTER (n_columns));

  while (args[i] != NULL) {
    gint j;

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);

    for (j = 0; j < n_columns; j++) {
        
      if (toggles && j == 0) {
        if (strcmp (g_ascii_strdown ((gchar *) args[i+j], -1), "true") == 0)
          gtk_list_store_set (GTK_LIST_STORE (model), &iter, j, TRUE, -1);
        else 
          gtk_list_store_set (GTK_LIST_STORE (model), &iter, j, FALSE, -1);
      }
      else
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, j, args[i+j], -1);        
    }

    if (editable)
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, n_columns, TRUE, -1);

    if (i == MAX_ELEMENTS_BEFORE_SCROLLING) {
      GtkWidget *scrolled_window;
      GtkRequisition rectangle;

      gtk_widget_size_request (GTK_WIDGET (tree_view), &rectangle);
      scrolled_window = GTK_WIDGET (gtk_builder_get_object (builder,
      							 "matedialog_tree_window"));
      gtk_widget_set_size_request (scrolled_window, -1, rectangle.height);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    }

  i += n_columns;
  }
}

static void
matedialog_cell_edited_callback (GtkCellRendererText *cell, 
                             const gchar         *path_string, 
                             const gchar         *new_text, 
                             gpointer             data) 
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint column;

  model = GTK_TREE_MODEL (data);
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));
  gtk_tree_model_get_iter (model, &iter, path);
        
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);

  gtk_tree_path_free (path);
}

void
matedialog_tree (MateDialogData *data, MateDialogTreeData *tree_data)
{
  GtkWidget *dialog;
  GObject *tree_view;
  GObject *text;
  GtkTreeViewColumn *column;
  GtkListStore *model;
  GType *column_types;
  GSList *tmp;
  gboolean first_column = FALSE;
  gint i, column_index, n_columns;

  builder = matedialog_util_load_ui_file ("matedialog_tree_dialog", NULL);

  if (builder == NULL) {
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }
        
  separator = g_strcompress (tree_data->separator);

  n_columns = g_slist_length (tree_data->columns);

  if (tree_data->print_column) {
    if (strcmp (g_ascii_strdown (tree_data->print_column, -1), "all") == 0)
      print_all_columns = TRUE;
    else 
      print_columns = matedialog_tree_extract_column_indexes (tree_data->print_column, n_columns);
  }
  else { 
    print_columns = g_new (gint, 2);
    print_columns[0] = (tree_data->radiobox || tree_data->checkbox ? 2 : 1);
    print_columns[1] = 0;
  }

  if (tree_data->hide_column) 
    hide_columns = matedialog_tree_extract_column_indexes (tree_data->hide_column, n_columns);

  if (n_columns == 0) {
    g_printerr (_("No column titles specified for List dialog.\n")); 
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }

  if (tree_data->checkbox && tree_data->radiobox) {
    g_printerr (_("You should use only one List dialog type.\n")); 
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }

  gtk_builder_connect_signals (builder, NULL);

  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "matedialog_tree_dialog"));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (matedialog_tree_dialog_response), data);

  if (data->dialog_title)
    gtk_window_set_title (GTK_WINDOW (dialog), data->dialog_title);

  text = gtk_builder_get_object (builder, "matedialog_tree_text");
                                                                                
  if (tree_data->dialog_text)
    gtk_label_set_markup (GTK_LABEL (text), g_strcompress (tree_data->dialog_text));

  matedialog_util_set_window_icon (dialog, data->window_icon, MATEDIALOG_IMAGE_FULLPATH ("matedialog-list.png"));

  if (data->width > -1 || data->height > -1)
    gtk_window_set_default_size (GTK_WINDOW (dialog), data->width, data->height);

  tree_view = gtk_builder_get_object (builder, "matedialog_tree_view");

  if (!(tree_data->radiobox || tree_data->checkbox)) 
    g_signal_connect (tree_view, "row-activated", 
                      G_CALLBACK (matedialog_tree_row_activated), data);
 
  /* Create an empty list store */
  model = g_object_new (GTK_TYPE_LIST_STORE, NULL);

  if (tree_data->editable)
    column_types = g_new (GType, n_columns + 1);
  else
    column_types = g_new (GType, n_columns);

  for (i = 0; i < n_columns; i++) {
    /* Have the limitation that the radioboxes and checkboxes are in the first column */
    if (i == 0 && (tree_data->checkbox || tree_data->radiobox))
      column_types[i] = G_TYPE_BOOLEAN;
    else
      column_types[i] = G_TYPE_STRING;
  }

  if (tree_data->editable)
    column_types[n_columns] = G_TYPE_BOOLEAN;

  if (tree_data->editable)
    gtk_list_store_set_column_types (model, n_columns + 1, column_types);
  else
    gtk_list_store_set_column_types (model, n_columns, column_types);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));

  if (!(tree_data->radiobox || tree_data->checkbox)) {
    if (tree_data->multi)
      gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
                                   GTK_SELECTION_MULTIPLE);
    else
      gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
                                   GTK_SELECTION_SINGLE);
  } 
  else  
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
                                   GTK_SELECTION_NONE);

  column_index = 0;

  for (tmp = tree_data->columns; tmp; tmp = tmp->next) {
    if (!first_column) {
      if (tree_data->checkbox || tree_data->radiobox) {
        GtkCellRenderer *cell_renderer;
    
        cell_renderer = gtk_cell_renderer_toggle_new ();
                                
        if (tree_data->radiobox) {
          g_object_set (G_OBJECT (cell_renderer), "radio", TRUE, NULL);
          g_object_set_data (G_OBJECT (model), "radio", GINT_TO_POINTER (1));
        }

        g_signal_connect (cell_renderer, "toggled",
                          G_CALLBACK (matedialog_tree_toggled_callback), model);

        column = gtk_tree_view_column_new_with_attributes (tmp->data,
                                                           cell_renderer, 
                                                           "active", column_index, NULL);
      }
      else  {
        if (tree_data->editable) {
          GtkCellRenderer *cell_renderer;

          cell_renderer = gtk_cell_renderer_text_new ();
          g_signal_connect (G_OBJECT (cell_renderer), "edited",
                            G_CALLBACK (matedialog_cell_edited_callback),
          gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view)));
          g_object_set_data (G_OBJECT (cell_renderer), "column", GINT_TO_POINTER (column_index));

          column = gtk_tree_view_column_new_with_attributes (tmp->data, 
                                                             cell_renderer, 
                                                             "text", column_index, 
                                                             "editable", n_columns, 
                                                             NULL);
        } 
        else  {
          column = gtk_tree_view_column_new_with_attributes (tmp->data, 
                                                             gtk_cell_renderer_text_new (), 
                                                             "text", column_index, 
                                                             NULL);
        }

        gtk_tree_view_column_set_sort_column_id (column, column_index);
        gtk_tree_view_column_set_resizable (column, TRUE);
      }
      if (matedialog_tree_column_is_hidden (1))
        gtk_tree_view_column_set_visible (column, FALSE);
 
      first_column = TRUE;
    }
    else {
      if (tree_data->editable) {
        GtkCellRenderer *cell_renderer;

        cell_renderer = gtk_cell_renderer_text_new ();
        g_signal_connect (G_OBJECT (cell_renderer), "edited",
                          G_CALLBACK (matedialog_cell_edited_callback),
        gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view)));
        g_object_set_data (G_OBJECT (cell_renderer), "column", GINT_TO_POINTER (column_index));

        column = gtk_tree_view_column_new_with_attributes (tmp->data, 
                                                           cell_renderer, 
                                                           "text", column_index, 
                                                           "editable", n_columns, 
                                                           NULL);
      }
      else {
        column = gtk_tree_view_column_new_with_attributes (tmp->data,
                                                           gtk_cell_renderer_text_new (), 
                                                           "text", column_index, NULL);
      }

      gtk_tree_view_column_set_sort_column_id (column, column_index);
      gtk_tree_view_column_set_resizable (column, TRUE);

      if (matedialog_tree_column_is_hidden (column_index + 1))
        gtk_tree_view_column_set_visible (column, FALSE);
    }
        
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
    column_index++;
  }

  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);

  if (tree_data->hide_header)
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  if (tree_data->radiobox || tree_data->checkbox) {
    if (tree_data->data && *tree_data->data)
      matedialog_tree_fill_entries (GTK_TREE_VIEW (tree_view), tree_data->data, n_columns, TRUE, tree_data->editable);
    else
      matedialog_tree_fill_entries_from_stdin (GTK_TREE_VIEW (tree_view), n_columns, TRUE, tree_data->editable);
  }
  else {
    if (tree_data->data && *tree_data->data)
      matedialog_tree_fill_entries (GTK_TREE_VIEW (tree_view), tree_data->data, n_columns, FALSE, tree_data->editable);
    else
      matedialog_tree_fill_entries_from_stdin (GTK_TREE_VIEW (tree_view), n_columns, FALSE, tree_data->editable);
  }

  matedialog_util_show_dialog (dialog);

  if(data->timeout_delay > 0) {
    g_timeout_add_seconds (data->timeout_delay, (GSourceFunc) matedialog_util_timeout_handle, NULL);
  }

  gtk_main ();

  g_object_unref (builder);
}

static void 
matedialog_tree_dialog_get_selected (GtkTreeModel *model, GtkTreePath *path_buf, GtkTreeIter *iter, GtkTreeView *tree_view)
{
  GValue value = {0, };
  gint n_columns, i;

  n_columns = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree_view), "n_columns"));

  if (print_all_columns) {
    for (i = 0; i < n_columns; i++) {
      gtk_tree_model_get_value (model, iter, i, &value);

      selected = g_slist_append (selected, g_value_dup_string (&value));
      g_value_unset (&value);
    }
    return;
  }

  for (i = 0; print_columns[i] != 0; i++) {
    gtk_tree_model_get_value  (model, iter, print_columns[i] - 1, &value);

    selected = g_slist_append (selected, g_value_dup_string (&value));
    g_value_unset (&value);
  }
}

static gboolean
matedialog_tree_dialog_toggle_get_selected (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GtkTreeView *tree_view)
{
  GValue toggle_value = {0, };
  gint n_columns, i;

  n_columns = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree_view), "n_columns"));

  gtk_tree_model_get_value (model, iter, 0, &toggle_value);

  if (g_value_get_boolean (&toggle_value)) {
    GValue value = {0, };

    if (print_all_columns) {
      for (i = 1; i < n_columns; i++) {
        gtk_tree_model_get_value (model, iter, i, &value);
        
        selected = g_slist_append (selected, g_value_dup_string (&value));
        g_value_unset (&value);
      }
      g_value_unset (&toggle_value);
      return FALSE;
    }

    for (i = 0; print_columns[i] != 0; i++) {
      gtk_tree_model_get_value (model, iter, print_columns[i] - 1, &value);

      selected = g_slist_append (selected, g_value_dup_string (&value));
      g_value_unset (&value);
    }
  }
  
  g_value_unset (&toggle_value);

  return FALSE;
}

static void
matedialog_tree_dialog_output (void)
{
  GSList *tmp;

  for (tmp = selected; tmp; tmp = tmp->next) {
    if (tmp->next != NULL) {
        g_print ("%s%s", (gchar *) tmp->data, separator);
    }
    else
      g_print ("%s\n", (gchar *) tmp->data);
  }

  g_free (print_columns);
  g_free (hide_columns);
  g_free (separator);
  g_slist_foreach (selected, (GFunc) g_free, NULL);
  selected = NULL;
}

static void
matedialog_tree_dialog_response (GtkWidget *widget, int response, gpointer data)
{
  MateDialogData *zen_data = data;
  GObject *tree_view;
  GtkTreeSelection *selection; 
  GtkTreeModel *model;

  switch (response) {
    case GTK_RESPONSE_OK:
      tree_view = gtk_builder_get_object (builder, "matedialog_tree_view");
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

      if (gtk_tree_model_get_column_type (model, 0) == G_TYPE_BOOLEAN)
        gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) matedialog_tree_dialog_toggle_get_selected,
                                GTK_TREE_VIEW (tree_view));
      else {
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
        gtk_tree_selection_selected_foreach (selection, 
                                             (GtkTreeSelectionForeachFunc) matedialog_tree_dialog_get_selected, 
                                             GTK_TREE_VIEW (tree_view));
      }
      matedialog_tree_dialog_output ();
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_OK);
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

static void
matedialog_tree_row_activated (GtkTreeView *tree_view, GtkTreePath *tree_path, 
                           GtkTreeViewColumn *tree_col, gpointer data)
{
  MateDialogData *zen_data = data;
  GtkTreeSelection *selection; 
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  gtk_tree_selection_selected_foreach (selection, 
                                       (GtkTreeSelectionForeachFunc) matedialog_tree_dialog_get_selected, 
                                       GTK_TREE_VIEW (tree_view));
 
  matedialog_tree_dialog_output ();
  zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_OK);
  gtk_main_quit ();
}

static gboolean
matedialog_tree_column_is_hidden (gint column_index)
{
  gint i;

  if (hide_columns != NULL)
    for (i = 0; hide_columns[i] != 0; i++)
      if (hide_columns[i] == column_index)
        return TRUE;

  return FALSE;
}

static gint *
matedialog_tree_extract_column_indexes (char *indexes, int n_columns)
{
  char **tmp;  
  gint *result;
  gint i, j, index;

  tmp = g_strsplit (indexes, 
                    PRINT_HIDE_COLUMN_SEPARATOR, 0);

  result = g_new (gint, 1);

  for (j = i = 0; tmp[i] != NULL; i++) {
    index = atoi (tmp[i]);

    if (index > 0 && index <= n_columns) {
      result[j] = index;
      j++;
      result = g_renew (gint, result, j + 1);
    }
  }
  result[j] = 0;

  g_strfreev (tmp);

  return result;
}
