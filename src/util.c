/*
 * util.c
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *           (C) 1999, 2000 Red Hat Inc.
 *           (C) 1998 James Henstridge
 *           (C) 1995-2002 Free Software Foundation
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
 *          Havoc Pennington <hp@redhat.com>
 *          James Henstridge <james@daa.com.au>
 *          Tom Tromey <tromey@redhat.com>
 */

#include "config.h"

#include <stdio.h>
#include <locale.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config.h"
#include "util.h"
#include "matedialog.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#define MATEDIALOG_OK_DEFAULT	0
#define MATEDIALOG_CANCEL_DEFAULT	1
#define MATEDIALOG_ESC_DEFAULT	1
#define MATEDIALOG_ERROR_DEFAULT	-1
#define MATEDIALOG_EXTRA_DEFAULT	127

GtkBuilder*
matedialog_util_load_ui_file (const gchar *root_widget, ...)
{
  va_list args;
  gchar *arg = NULL;
  GPtrArray *ptrarray;
  GtkBuilder *builder = gtk_builder_new ();
  GError *error = NULL;
  gchar  **objects;
  guint result = 0;

  gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

  /* We have at least the root_widget and a NULL */
  ptrarray = g_ptr_array_sized_new (2);

  g_ptr_array_add (ptrarray, g_strdup (root_widget));

  va_start (args, root_widget);

  arg = va_arg (args, gchar*);

  while (arg) {
  	g_ptr_array_add (ptrarray, g_strdup (arg));
	arg = va_arg (args, gchar*);
  }
  va_end (args);

  /* Enforce terminating NULL */
  g_ptr_array_add (ptrarray, NULL);
  objects = (gchar**) g_ptr_array_free (ptrarray, FALSE);

  if (g_file_test (MATEDIALOG_UI_FILE_RELATIVEPATH, G_FILE_TEST_EXISTS)) {
    /* Try current dir, for debugging */
    result = gtk_builder_add_objects_from_file (builder,
    						MATEDIALOG_UI_FILE_RELATIVEPATH,
						objects, NULL);
  }

  if (result == 0)
    result = gtk_builder_add_objects_from_file (builder,
    						MATEDIALOG_UI_FILE_FULLPATH,
						objects, &error);

  g_strfreev (objects);

  if (result == 0) {
    g_warning ("Could not load ui file %s: %s", MATEDIALOG_UI_FILE_FULLPATH,
    						error->message);
    g_error_free (error);
    g_object_unref (builder);
    return NULL;
  }

  return builder;
}
gchar*
matedialog_util_strip_newline (gchar *string)
{
    gsize len;
    
    g_return_val_if_fail (string != NULL, NULL);
                                                                                                                                                                             
    len = strlen (string);
    while (len--) 
    {
      if (string[len] == '\n')
        string[len] = '\0';
      else
        break;
    }
            
    return string;
}

gboolean
matedialog_util_fill_file_buffer (GtkTextBuffer *buffer, const gchar *filename) 
{
  GtkTextIter iter, end;
  FILE *f;
  gchar buf[2048];
  gint remaining = 0;

  if (filename == NULL)
    return FALSE;

  f = fopen (filename, "r");

  if (f == NULL) {
    g_warning ("Cannot open file '%s': %s", filename, g_strerror (errno));
    return FALSE;
  }

  gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  while (!feof (f)) {
    gint count;
    const char *leftover;
    int to_read = 2047  - remaining;

    count = fread (buf + remaining, 1, to_read, f);
    buf[count + remaining] = '\0';

    g_utf8_validate (buf, count + remaining, &leftover);

    g_assert (g_utf8_validate (buf, leftover - buf, NULL));
    gtk_text_buffer_insert (buffer, &iter, buf, leftover - buf);

    remaining = (buf + remaining + count) - leftover;
    g_memmove (buf, leftover, remaining);

    if (remaining > 6 || count < to_read)
      break;
  }

  if (remaining) {
    g_warning ("Invalid UTF-8 data encountered reading file '%s'", filename);
    return FALSE;
  }

  /* We had a newline in the buffer to begin with. (The buffer always contains
   * a newline, so we delete to the end of the buffer to clean up.
   */
  
  gtk_text_buffer_get_end_iter (buffer, &end);
  gtk_text_buffer_delete (buffer, &iter, &end);

  gtk_text_buffer_set_modified (buffer, FALSE);

  return TRUE;
}

const gchar *
matedialog_util_stock_from_filename (const gchar *filename)
{
  if (!filename || !filename[0])
    return GTK_STOCK_DIALOG_WARNING; /* default */

  if (!g_ascii_strcasecmp (filename, "warning"))
    return GTK_STOCK_DIALOG_WARNING;
  if (!g_ascii_strcasecmp (filename, "info"))
    return GTK_STOCK_DIALOG_INFO;
  if (!g_ascii_strcasecmp (filename, "question"))
    return GTK_STOCK_DIALOG_QUESTION;
  if (!g_ascii_strcasecmp (filename, "error"))
    return GTK_STOCK_DIALOG_ERROR;
  return NULL;
}

GdkPixbuf *
matedialog_util_pixbuf_new_from_file (GtkWidget *widget, const gchar *filename)
{
  const gchar *stock;

  stock = matedialog_util_stock_from_filename (filename);
  if (stock)
    return gtk_widget_render_icon (widget, stock, GTK_ICON_SIZE_BUTTON, NULL);

 return gdk_pixbuf_new_from_file (filename, NULL);
}

void
matedialog_util_set_window_icon (GtkWidget *widget, const gchar *filename, const gchar *default_file)
{
  GdkPixbuf *pixbuf;

  if (filename != NULL)
    pixbuf = matedialog_util_pixbuf_new_from_file (widget, (gchar *) filename);
  else
    pixbuf = gdk_pixbuf_new_from_file (default_file, NULL);  

  if (pixbuf != NULL) {
    gtk_window_set_icon (GTK_WINDOW (widget), pixbuf);
    g_object_unref (pixbuf);
  }
}

void 
matedialog_util_set_window_icon_from_stock (GtkWidget *widget, const gchar *filename, const gchar *default_stock_id)
{
  GdkPixbuf *pixbuf;

  if (filename != NULL) {
    pixbuf = matedialog_util_pixbuf_new_from_file (widget, (gchar *) filename);
  }
  else {
    pixbuf = gtk_widget_render_icon (widget, default_stock_id, GTK_ICON_SIZE_BUTTON, NULL);
  }

  if (pixbuf != NULL) {
    gtk_window_set_icon (GTK_WINDOW (widget), pixbuf);
    g_object_unref (pixbuf);
  }
}

void
matedialog_util_show_help (GError **error)
{
  gchar *tmp;
  tmp = g_find_program_in_path ("yelp");

  if (tmp) {
    g_free (tmp);
    g_spawn_command_line_async ("yelp ghelp:matedialog", error);
  }
}

gint 
matedialog_util_return_exit_code ( MateDialogExitCode value ) 
{

  const gchar *env_var = NULL;
  gint retval;

  switch (value) {
  
  case MATEDIALOG_OK:
    env_var = g_getenv("MATEDIALOG_OK");
    if (! env_var) 
          env_var = g_getenv("DIALOG_OK");
    if (! env_var) 
          retval = MATEDIALOG_OK_DEFAULT;
    break;
   
  case MATEDIALOG_CANCEL:
    env_var = g_getenv("MATEDIALOG_CANCEL");
    if (! env_var) 
          env_var = g_getenv("DIALOG_CANCEL");
    if (! env_var) 
          retval = MATEDIALOG_CANCEL_DEFAULT;
    break;
    
  case MATEDIALOG_ESC:
    env_var = g_getenv("MATEDIALOG_ESC");
    if (! env_var) 
          env_var = g_getenv("DIALOG_ESC");
    if (! env_var) 
          retval = MATEDIALOG_ESC_DEFAULT;
    break;
    
  case MATEDIALOG_EXTRA:
    env_var = g_getenv("MATEDIALOG_EXTRA");
    if (! env_var) 
          env_var = g_getenv("DIALOG_EXTRA");
    if (! env_var) 
          retval = MATEDIALOG_EXTRA_DEFAULT;
    break;
    
  case MATEDIALOG_ERROR:
    env_var = g_getenv("MATEDIALOG_ERROR");
    if (! env_var) 
          env_var = g_getenv("DIALOG_ERROR");
    if (! env_var) 
          retval = MATEDIALOG_ERROR_DEFAULT;
    break;
    
  default:
    retval = 1;
  }
  
  if (env_var) 
      retval = atoi (env_var);
  return retval; 
}


#ifdef GDK_WINDOWING_X11

static Window
transient_get_xterm (void)
{
  const char *wid_str = g_getenv ("WINDOWID");
  if (wid_str) {
    char *wid_str_end;
    int ret;
    Window wid = strtoul (wid_str, &wid_str_end, 10);
    if (*wid_str != '\0' && *wid_str_end == '\0' && wid != 0) {
      XWindowAttributes attrs;
      gdk_error_trap_push ();
      ret = XGetWindowAttributes (GDK_DISPLAY(), wid, &attrs);
      gdk_flush();
      if (gdk_error_trap_pop () != 0 || ret == 0) {
        return None;
      }
      return wid;
    }
  }
  return None;
}

static void
transient_x_free (void *ptr)
{
  if (ptr)
    XFree (ptr);
}

static gboolean
transient_is_toplevel (Window wid)
{
  XTextProperty prop;
  Display *dpy = GDK_DISPLAY ();
  if (!XGetWMName (dpy, wid, &prop))
    return FALSE;
  transient_x_free (prop.value);
  return !!prop.value;
}

/*
 * MATE Terminal doesn't give us its toplevel window, but the WM needs a
 * toplevel XID for proper stacking.  Other terminals work fine without this
 * magic.  We can't use GDK here since "xterm" is a foreign window.
 */

static Window
transient_get_xterm_toplevel (void)
{
  Window xterm = transient_get_xterm ();
  Display *dpy = GDK_DISPLAY ();
  while (xterm != None && !transient_is_toplevel (xterm))
  {
    Window root, parent, *children;
    unsigned nchildren;
    XQueryTree (dpy, xterm,
                &root, &parent,
                &children, &nchildren);
    transient_x_free (children);
    if (parent == root)
      xterm = None;
    else
      xterm = parent;
  }
  return xterm;
}

static void
matedialog_util_make_transient (GdkWindow *window)
{
  Window xterm = transient_get_xterm_toplevel ();
  if (xterm != None) {
    GdkWindow *gdkxterm = gdk_window_foreign_new (xterm);
    if (gdkxterm) {
      gdk_window_set_transient_for (window, gdkxterm);
      g_object_unref (G_OBJECT (gdkxterm));
    }
  }
}

#endif /* GDK_WINDOWING_X11 */

void
matedialog_util_show_dialog (GtkWidget *dialog)
{
  gtk_widget_realize (dialog);
#ifdef GDK_WINDOWING_X11
  g_assert (gtk_widget_get_window(dialog));
  matedialog_util_make_transient (gtk_widget_get_window(dialog));
#endif
  gtk_widget_show (dialog);
}

gboolean 
matedialog_util_timeout_handle (void)
{
  gtk_main_quit();
  exit(MATEDIALOG_TIMEOUT);
  return FALSE;
}
