/*
 * goffice-gtk.h - Misc GTK+ utilities
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */
#ifndef _GOFFICE_GTK_H_
#define _GOFFICE_GTK_H_

#include <gtk/gtk.h>
#include <goffice/goffice.h>

#include <goffice/gtk/go-palette.h>
#include <goffice/gtk/go-selector.h>

#include <goffice/gtk/go-arrow-sel.h>
#include <goffice/gtk/go-3d-rotation-sel.h>
#include <goffice/gtk/go-action-combo-color.h>
#include <goffice/gtk/go-action-combo-pixmaps.h>
#include <goffice/gtk/go-action-combo-stack.h>
#include <goffice/gtk/go-action-combo-text.h>
#include <goffice/gtk/go-calendar-button.h>
#include <goffice/gtk/go-charmap-sel.h>
#include <goffice/gtk/go-color-group.h>
#include <goffice/gtk/go-color-palette.h>
#include <goffice/gtk/go-color-selector.h>
#include <goffice/gtk/go-combo-box.h>
#include <goffice/gtk/go-combo-color.h>
#include <goffice/gtk/go-combo-pixmaps.h>
#include <goffice/gtk/go-font-sel.h>
#include <goffice/gtk/go-font-sel-dialog.h>
#include <goffice/gtk/go-format-sel.h>
#include <goffice/gtk/go-gradient-selector.h>
#include <goffice/gtk/go-graph-widget.h>
#include <goffice/gtk/go-image-sel.h>
#include <goffice/gtk/go-line-selector.h>
#include <goffice/gtk/go-locale-sel.h>
#include <goffice/gtk/go-marker-selector.h>
#include <goffice/gtk/go-optionmenu.h>
#include <goffice/gtk/go-pattern-selector.h>
#include <goffice/gtk/go-rotation-sel.h>
#include <goffice/gtk/go-math-editor.h>


G_BEGIN_DECLS

void       _go_gtk_shutdown (void);

// Add goffice-specific css provider
void       _go_gtk_widget_add_css_provider (GtkWidget *w);

GtkStyleContext *go_style_context_from_selector (GtkStyleContext *parent,
						 const char      *selector);

gboolean _go_gtk_new_theming (void);

void	   go_gtk_editable_enters (GtkWindow *window, GtkWidget *w);

GtkBuilder *go_gtk_builder_load_internal (char const *uifile,
					 char const *domain, GOCmdContext *gcc);
GtkBuilder *go_gtk_builder_load (char const *uifile,
				char const *domain, GOCmdContext *gcc);
#ifndef GOFFICE_DISABLE_DEPRECATED
GOFFICE_DEPRECATED_FOR(go_gtk_builder_load)
GtkBuilder *go_gtk_builder_new (char const *uifile,
				char const *domain, GOCmdContext *gcc);
#endif
gulong	   go_gtk_builder_signal_connect (GtkBuilder *gui,
					  gchar const *instance_name,
					  gchar const *detailed_signal,
					  GCallback c_handler,
					  gpointer data);
gulong	   go_gtk_builder_signal_connect_swapped (GtkBuilder *gui,
					    gchar const *instance_name,
					    gchar const *detailed_signal,
					    GCallback c_handler,
					    gpointer data);
GtkWidget *go_gtk_builder_get_widget (GtkBuilder *gui,
				      char const *widget_name);
GtkComboBox *go_gtk_builder_combo_box_init_text (GtkBuilder *gui,
						 char const *widget_name);
void	   go_gtk_combo_box_append_text (GtkComboBox *combo, char const *str);
void	   go_gtk_combo_box_remove_text (GtkComboBox *combo, int position);

int	   go_gtk_builder_group_value (GtkBuilder *gui,
				       char const * const group[]);

gint       go_gtk_dialog_run		(GtkDialog *dialog, GtkWindow *parent);
GtkWidget *go_gtk_dialog_add_button	(GtkDialog *dialog, char const *text,
					 char const *stock_id,
					 gint response_id);
void       go_gtk_notice_dialog		(GtkWindow *parent, GtkMessageType type,
					 const gchar *format, ...) G_GNUC_PRINTF (3, 4);
void       go_gtk_notice_nonmodal_dialog (GtkWindow *parent, GtkWidget **ref,
					  GtkMessageType type,
					  const gchar *format, ...) G_GNUC_PRINTF (4, 5);
gboolean   go_gtk_query_yes_no		(GtkWindow *toplevel, gboolean default_answer,
					 const gchar *format, ...) G_GNUC_PRINTF (3, 4);

GtkWidget *go_gtk_button_build_with_stock (char const *text,
					 char const *stock_id);
#ifndef GOFFICE_DISABLE_DEPRECATED
GOFFICE_DEPRECATED_FOR(go_gtk_button_build_with_stock)
GtkWidget *go_gtk_button_new_with_stock (char const *text,
					 char const *stock_id);
#endif
void	   go_gtk_widget_replace	(GtkWidget *victim,
					 GtkWidget *replacement);
void       go_gtk_grid_remove_row       (GtkGrid *grid, int row);
void	   go_gtk_widget_disable_focus	(GtkWidget *w);
void       go_gtk_window_set_transient  (GtkWindow *toplevel, GtkWindow *window);
void	   go_gtk_help_button_init	(GtkWidget *w, char const *data_dir,
					 char const *app, char const *link);
void       go_gtk_nonmodal_dialog	(GtkWindow *toplevel, GtkWindow *dialog);
gboolean   go_gtk_file_sel_dialog	(GtkWindow *toplevel, GtkWidget *w);
char	  *go_gtk_select_image		(GtkWindow *toplevel, const char *initial);
char	  *go_gtk_select_image_with_extra_widget (GtkWindow *toplevel, char const *initial, GtkWidget *extra);
char      *go_gui_get_image_save_info 	(GtkWindow *toplevel,
					 GSList *supported_formats,
					 GOImageFormat *ret_format,
					 double *resolution);

gboolean   go_gtk_url_is_writeable	(GtkWindow *parent, char const *uri,
					 gboolean overwrite_by_default);

void	   go_atk_setup_label	 	(GtkWidget *label, GtkWidget *target);

void       go_dialog_guess_alternative_button_order (GtkDialog *dialog);

void 	   go_menu_position_below (GtkMenu *menu, gint *x, gint *y,
				   gint *push_in, gpointer user_data);

GError	  *go_gtk_url_show (gchar const *url, GdkScreen *screen);

GdkPixbuf *go_gtk_widget_render_icon_pixbuf (GtkWidget   *widget,
					     const gchar *icon_name,
					     GtkIconSize  size);


G_END_DECLS

#endif /* _GOFFICE_GTK_H_ */
