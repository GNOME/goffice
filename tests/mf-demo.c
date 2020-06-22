/*
 * mf-demo.c : open WMF file
 *
 *  Copyright (C) 2010 Valek Filippov (frob@gnome.org)
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <goffice/goffice.h>

static void
on_quit (GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_destroy (user_data);
	gtk_main_quit ();
}

static void
my_test (GocCanvas *canvas, GdkEventButton *event, G_GNUC_UNUSED gpointer data)
{
	double ppu=1.;

	ppu = goc_canvas_get_pixels_per_unit (canvas);
	if (1 == event->button) {
		ppu = ppu / 1.5;
		goc_canvas_set_pixels_per_unit (canvas, ppu);
	} else {
		ppu = ppu * 1.5;
		goc_canvas_set_pixels_per_unit (canvas, ppu);
	}
}

static void
open_file (char const *filename, GtkWidget *nbook)
{
	GError *error = NULL;
	char *display_name;
	GocCanvas *canvas;
	GOImage *image;
	GtkWidget *window, *label;

	g_print ("%s\n", filename);

	image = go_image_new_from_file (filename, &error);
	if (error) {
		display_name = g_filename_display_name (filename);
		g_printerr ("%s: Failed to open %s: %s\n",
				g_get_prgname (),
				display_name,
				error->message);
		g_free (display_name);
		return;
	}

	if (image && !GO_IS_EMF (image)) {
		g_object_unref (image);
		return;
	}

	canvas = GOC_CANVAS (go_emf_get_canvas (GO_EMF (image)));
	g_object_unref (image);
	g_signal_connect_swapped (canvas, "button-press-event", G_CALLBACK (my_test), canvas);

	window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (window), GTK_WIDGET (canvas));
	if (g_strrstr (filename, "/") != NULL)
		label = gtk_label_new (g_strrstr (filename, "/") + 1);
	else
		label = gtk_label_new (filename);
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook), window, label);
	gtk_widget_show_all (GTK_WIDGET (window));
}

static void
on_close (GtkMenuItem *menuitem, GtkWidget *nbook)
{
	gtk_widget_destroy (gtk_notebook_get_nth_page (GTK_NOTEBOOK (nbook), gtk_notebook_get_current_page (GTK_NOTEBOOK (nbook))));
}

static void
on_open (GtkMenuItem *menuitem, GtkWidget *nbook)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new ("Open File",
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		open_file (filename, nbook);
		g_free (filename);
	}

	gtk_widget_destroy (dialog);
}

int
main (int argc, char *argv[])
{
	GtkWidget *window, *file_menu, *menu_bar, *file_item, *open_item, *close_item, *quit_item, *grid, *nbook;

#ifndef GOFFICE_WITH_EMF
	g_assert ("Goffice must be built with emf support to make this demo work.");
#endif
	gtk_init (&argc, &argv);
	libgoffice_init ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_resize (GTK_WINDOW (window), 300, 340);
	gtk_window_set_title (GTK_WINDOW (window), "MF demo");
	g_signal_connect (window, "destroy", gtk_main_quit, NULL);

	grid = gtk_grid_new ();
	g_object_set (G_OBJECT (grid), "orientation", GTK_ORIENTATION_VERTICAL, NULL);
	menu_bar = gtk_menu_bar_new ();
	nbook = gtk_notebook_new ();
	g_object_set (G_OBJECT (nbook), "expand", TRUE, "margin", 2, NULL);

	file_menu = gtk_menu_new();
	file_item = gtk_menu_item_new_with_label ("File");
	open_item = gtk_menu_item_new_with_label ("Open");
	close_item = gtk_menu_item_new_with_label ("Close");
	quit_item = gtk_menu_item_new_with_label ("Quit");
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), open_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), close_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), quit_item);

	g_signal_connect (quit_item, "activate", G_CALLBACK (on_quit), window);
	g_signal_connect (close_item, "activate", G_CALLBACK (on_close), nbook);
	g_signal_connect (open_item, "activate", G_CALLBACK (on_open), nbook);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item), file_menu);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item );

	gtk_container_add (GTK_CONTAINER (grid), menu_bar);
	gtk_container_add (GTK_CONTAINER (grid), nbook);
	gtk_container_add (GTK_CONTAINER (window), grid);
	gtk_widget_show_all (GTK_WIDGET (window));

	if (argc > 1)
		open_file (*(argv+1), nbook);

	gtk_main ();

	libgoffice_shutdown ();
	return 0;
}
