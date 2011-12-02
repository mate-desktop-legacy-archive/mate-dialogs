/*
 * calendar.c
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

#include <time.h>
#include "matedialog.h"
#include "util.h"


static GtkWidget *calendar;
static MateDialogCalendarData *zen_cal_data;

static void matedialog_calendar_dialog_response (GtkWidget *widget, int response, gpointer data);
static void matedialog_calendar_double_click (GtkCalendar *calendar, gpointer data);

void 
matedialog_calendar (MateDialogData *data, MateDialogCalendarData *cal_data)
{
  GtkBuilder *builder;
  GtkWidget *dialog;
  GObject *text;

  zen_cal_data = cal_data;

  builder = matedialog_util_load_ui_file ("matedialog_calendar_dialog", NULL);

  if (builder == NULL) {
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }
	
  gtk_builder_connect_signals (builder, NULL);

  dialog = GTK_WIDGET (gtk_builder_get_object (builder,
  					       "matedialog_calendar_dialog"));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (matedialog_calendar_dialog_response), data);

  if (data->dialog_title)	
    gtk_window_set_title (GTK_WINDOW (dialog), data->dialog_title);

  matedialog_util_set_window_icon (dialog, data->window_icon, MATEDIALOG_IMAGE_FULLPATH ("matedialog-calendar.png"));

  if (data->width > -1 || data->height > -1)
    gtk_window_set_default_size (GTK_WINDOW (dialog), data->width, data->height);

  text = gtk_builder_get_object (builder, "matedialog_calendar_text");

  if (cal_data->dialog_text)
    gtk_label_set_markup (GTK_LABEL (text), g_strcompress (cal_data->dialog_text));

  calendar = GTK_WIDGET (gtk_builder_get_object (builder, "matedialog_calendar"));
	
  if (cal_data->month > 0 || cal_data->year > 0)
    gtk_calendar_select_month (GTK_CALENDAR (calendar), cal_data->month - 1, cal_data->year);
  if (cal_data->day > 0)
    gtk_calendar_select_day (GTK_CALENDAR (calendar), cal_data->day);

  g_signal_connect (calendar, "day-selected-double-click",
		    G_CALLBACK (matedialog_calendar_double_click), data);

  gtk_label_set_mnemonic_widget (GTK_LABEL (text), calendar);
  matedialog_util_show_dialog (dialog);

  if(data->timeout_delay > 0) {
    g_timeout_add_seconds (data->timeout_delay, (GSourceFunc) matedialog_util_timeout_handle, NULL);
  }

  g_object_unref (builder);

  gtk_main ();
}

static void
matedialog_calendar_dialog_response (GtkWidget *widget, int response, gpointer data)
{
  MateDialogData *zen_data;
  guint day, month, year;
  gchar time_string[128];
  GDate *date = NULL;

  zen_data = data;

  switch (response) {
    case GTK_RESPONSE_OK:
      gtk_calendar_get_date (GTK_CALENDAR (calendar), &day, &month, &year);
      date = g_date_new_dmy (year, month + 1, day);
      g_date_strftime (time_string, 127, zen_cal_data->date_format, date);
      g_print ("%s\n", time_string);
    
      if (date != NULL)
        g_date_free (date);
    
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
matedialog_calendar_double_click (GtkCalendar *cal, gpointer data)
{
  matedialog_calendar_dialog_response (NULL, GTK_RESPONSE_OK, data);
}
