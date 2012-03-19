/*
 * shapes-demo.c :
 *
 * Copyright (C) 2003-2005 Jean Brefort (jean.brefort@normalesup.org)
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


static void parse_line (GocCanvas *canvas, gchar *entry);
static void open_file (GocCanvas* canvas, char* filename);

static void
open_file (GocCanvas* canvas, char* filename)
{
	FILE	*fd;
	char	*s;
	size_t	n = 64;

	s = (char *) malloc (n + 1);

	if (NULL == (fd = fopen (filename, "r"))) {
		g_print ("The file can't be opened!\n");
		return;
	}

	while (-1 != getline (&s, &n, fd)) {
		parse_line (canvas, s);
	}
	free (s);
}

static void
on_open(GocCanvas *canvas)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new ("Open File",
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		open_file (canvas, filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}



static void
my_test (GocCanvas *canvas, GdkEventButton *event, G_GNUC_UNUSED gpointer data)
{
	GocItem *item;
	double x,y;
	GOStyle *style;
	double ppu=1.;

	g_print ("# %.0f %.0f Button: %d. ", event->x, event->y, event->button);

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return;

	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	item = goc_canvas_get_item_at (canvas, x, y);
	if (item) {
		style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
		if (style->line.color == GO_COLOR_FROM_RGBA (0, 0, 0, 128)) {
			style->line.color = GO_COLOR_FROM_RGBA (255, 0, 0, 128);
		} else {
			style->line.color = GO_COLOR_FROM_RGBA (0, 0, 0, 128);
		}
		goc_item_invalidate (item);
		g_print ("Hit!\n");
	} else {
		ppu = goc_canvas_get_pixels_per_unit (canvas);
		if (2 == event->button) {
			ppu *= 1.5;
			goc_canvas_set_pixels_per_unit (canvas, ppu);
		} else if (3 == event->button) {
			ppu /= 1.5;
			goc_canvas_set_pixels_per_unit (canvas, ppu);
		}
		g_print ("\n");
	}
}

static void
parse_line (GocCanvas *canvas, gchar *entry)
{
	gchar		**v = NULL;
	GocItem		*item;
	GocGroup	*group;
	GOStyle		*style;
	GOArrow		*arr;
	guint		i,j;
	int		cmd = -1;
	GocPoints *points;
	GocIntArray *array;

	/* check for "comment" */
	if (g_str_has_prefix (entry, "#"))
		return;

	g_strstrip (entry);
	v = g_strsplit_set (entry, " ", -1);

	if (v != NULL) {
		if (g_ascii_strcasecmp (v[0], "ARC") == 0) {
			cmd = 0;
		} else if (g_ascii_strcasecmp (v[0], "CHORD") == 0) {
			cmd = 1;
		} else if (g_ascii_strcasecmp (v[0], "PIE") == 0) {
			cmd = 2;
		} else if (g_ascii_strcasecmp (v[0], "LINE") == 0) {
			cmd = 3;
		} else if (g_ascii_strcasecmp (v[0], "RECT") == 0) {
			cmd = 4;
		} else if (g_ascii_strcasecmp (v[0], "OVAL") == 0) {
			cmd = 5;
		} else if (g_ascii_strcasecmp (v[0], "POLYL") == 0) {
			cmd = 6;
		} else if (g_ascii_strcasecmp (v[0], "POLYG") == 0) {
			cmd = 7;
		} else if (g_ascii_strcasecmp (v[0], "PPOLYG") == 0) {
			cmd = 8;
		} else if (g_ascii_strcasecmp (v[0], "RRECT") == 0) {
			cmd = 9;
		} else if (g_ascii_strcasecmp (v[0], "STROKE") == 0) {
			cmd = 20;
		} else if (g_ascii_strcasecmp (v[0], "FILL") == 0) {
			cmd = 21;
		} else if (g_ascii_strcasecmp (v[0], "ARROW") == 0) {
			cmd = 22;
		} else if (g_ascii_strcasecmp (v[0], "RTL") == 0) {
			cmd = 100;
		} else if (g_ascii_strcasecmp (v[0], "SCROLL") == 0) {
			cmd = 101;
		} else if (g_ascii_strcasecmp (v[0], "DEL") == 0) {
			cmd = 102;
		} else if (g_ascii_strcasecmp (v[0], "FILE") == 0) {
			cmd = 1000;
		}
	}

	switch (cmd) {
	case -1:
		break;
	case 0:
	case 1:
	case 2: /* ARC / CHORD / PIE */
		if (g_strv_length (v) > 6) {
			item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_ARC,
				"xc", (double) atoi (v[1]), "yc", (double) atoi (v[2]), "xr", (double) atoi (v[3]), "yr", (double) atoi (v[4]),
				"ang1", (double) atoi (v[5]) * M_PI / 180., "ang2", (double) atoi (v[6]) * M_PI / 180., "type", cmd, NULL);
			if (g_strv_length (v) > 7) {
				goc_item_set (item, "rotation", (double) atoi (v[7]) * M_PI/ 180., NULL);
			}
		}
		break;
	case 3: /* LINE */
		if (g_strv_length (v) > 4) {
			item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_LINE,
				"x0", (double) atoi (v[1]), "y0", (double) atoi (v[2]), "x1", (double) atoi (v[3]), "y1", (double) atoi (v[4]), NULL);
		}
		break;
	case 4: /* RECTANGLE */
		if (g_strv_length (v) > 4) {
			item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_RECTANGLE,
				"x", (double) atoi (v[1]), "y", (double) atoi (v[2]), "width", (double) atoi (v[3]), "height", (double) atoi (v[4]), NULL);
			if( g_strv_length (v) > 5) {
				goc_item_set (item, "rotation", (double) atoi (v[5]) * M_PI / 180., NULL);
			}
		}
		break;
	case 5: /* ELLIPSE */
		if (g_strv_length(v) > 4) {
			item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_ELLIPSE,
				"x", (double) atoi (v[1]), "y", (double) atoi (v[2]), "width", (double) atoi (v[3]), "height", (double) atoi (v[4]), NULL);
			if(g_strv_length (v) > 5) {
				goc_item_set (item,"rotation",(double) atoi (v[5]) * M_PI / 180., NULL);
			}
		}
		break;
	case 6: /* POLY */
	case 7:
		if (g_strv_length(v) > 2) {
			points = goc_points_new ((g_strv_length(v) - 1) / 2);
			for (i=0; i < g_strv_length (v) / 2; i++) {
				points->points[i].x = atoi (v[i * 2 + 1]);
				points->points[i].y = atoi (v[i * 2 + 2]);
			}
			if (cmd == 6)
				item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_POLYLINE, "points", points, NULL);
			else
				item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_POLYGON, "points", points, "fill-rule", 0, NULL);
		}
		break;
	case 8: /* PPOLYG num_of_poly num_size1 num_size2 ... num_sizeN-1 points  */
		if (g_strv_length(v) > 2)
			if (atoi(v[1]) < 2)
				break;
			array = goc_int_array_new (atoi(v[1]));
			for (j = 0; j < (guint) atoi(v[1]); j++)
				array->vals[j] = atoi(v[j + 2]);
			j = atoi(v[1]);
			points = goc_points_new ((g_strv_length(v) - j - 2) / 2);
			for (i=0; i < (g_strv_length (v) - j - 2) / 2; i++) {
				points->points[i].x = atoi (v[i * 2 + 1 + j + 1]);
				points->points[i].y = atoi (v[i * 2 + 2 + j + 1]);
			}
			item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_POLYGON, "points", points, "sizes", array, NULL);
		break;
	case 9: /* RRECT */
			if (g_strv_length (v) > 8) {
				item = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_RECTANGLE,
					"x", (double) atoi (v[1]), "y", (double) atoi (v[2]), "width", (double) atoi (v[3]), "height", (double) atoi (v[4]),
					"rx", (double) atoi (v[5]), "ry", (double) atoi (v[6]), "rotation", (double) atoi (v[7]) * M_PI / 180., "type", atoi (v[8]), NULL);
			}
			break;
	case 20: /* STROKE */
		group = (GocGroup*) goc_canvas_get_root (canvas);
		item = g_list_nth_data (group->children, atoi (v[1]));
		if (item != NULL && g_strv_length (v) > 2) {
			style = go_style_dup (go_styled_object_get_style (GO_STYLED_OBJECT (item)));
			style->line.width = atoi (v[2]);
			if (g_ascii_strcasecmp (v[2], "DASH") == 0) {
				if(g_strv_length (v) > 3)
					style->line.dash_type = atoi (v[3]);
				else
					style->line.dash_type = 0;
			} else if (g_strv_length(v) > 6) {
				style->line.color = GO_COLOR_FROM_RGBA (atoi (v[3]), atoi (v[4]), atoi (v[5]), atoi (v[6]));
				style->line.auto_color = FALSE;
			}
			goc_item_set (item, "style", style, NULL);
			g_object_unref (style);
		}
		break;
	case 21: /* FILL */
		group = (GocGroup*) goc_canvas_get_root (canvas);
		item = g_list_nth_data (group->children, atoi (v[1]));
		if (item != NULL && g_strv_length (v) > 2) {
			style = go_style_dup (go_styled_object_get_style (GO_STYLED_OBJECT (item)));
			if (g_ascii_strcasecmp (v[2], "NONE") == 0) {
				style->fill.type = GO_STYLE_FILL_NONE;
			} else if (g_ascii_strcasecmp (v[2], "WIND") == 0) {
				goc_item_set ( item, "fill-rule", atoi(v[3]), NULL);
			} else if (g_strv_length (v) > 5) {
				style->fill.type = GO_STYLE_FILL_PATTERN;
				style-> fill.pattern.back = GO_COLOR_FROM_RGBA (atoi (v[2]), atoi (v[3]), atoi (v[4]), atoi (v[5]));
				style->fill.auto_back = FALSE;
			}
			goc_item_set (item, "style", style, NULL);
			g_object_unref (style);
		}
		break;
	case 22: /* ARROW */
		group = (GocGroup*) goc_canvas_get_root (canvas);
		item = g_list_nth_data (group->children, atoi (v[1]));
		if (item != NULL) {
			if(g_strv_length (v) > 6) {
				arr = g_new0 (GOArrow, 1);
				if(arr) {
					if (atoi (v[2]) == 0)
						go_arrow_init_kite (arr, atoi (v[3]), atoi (v[4]), atoi (v[5]));
					else
						go_arrow_init_oval (arr, atoi (v[3]), atoi (v[4]));
					if (atoi (v[6]) == 0)
						goc_item_set (item, "start-arrow", arr, NULL);
					else
						goc_item_set (item, "end-arrow", arr, NULL);
				}
			}
		}
		break;
	case 100: /* RTL */
		if (g_strv_length (v) > 1)
			goc_canvas_set_direction (canvas, atoi (v[1]));
		break;
	case 101: /* SCROLL */
		if (g_strv_length (v) > 2)
			goc_canvas_scroll_to (canvas, atoi (v[1]), atoi (v[2]));
		break;
	case 102: /* DELETE */
		if (v[1] != NULL && atoi (v[1]) > 0) { /* crash with 0, why? */
			group = (GocGroup*) goc_canvas_get_root (canvas);
			item = g_list_nth_data (group->children, atoi (v[1]));
			if (item != NULL)
				g_object_unref (item);
		}
		break;
	case 1000: /* FILE */
		if (v[1] != NULL) {
			open_file (canvas, v[1]);
		} else {
			on_open (canvas);
		}

		break;
	default:
		break;
	}

	if (v != NULL)
		g_strfreev (v);
}


static void
enter_callback (GocCanvas* canvas, GtkWidget *entry, G_GNUC_UNUSED gpointer data)
{
	char *entry_text;
	const gchar *clean = "";

	entry_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	if (0 != gtk_entry_get_text_length (GTK_ENTRY (entry))) {
		g_print ("%s\n", entry_text);
		parse_line (canvas, entry_text);
		gtk_entry_set_text (GTK_ENTRY (entry), clean);
	}
	g_free (entry_text);
}

int
main (int argc, char *argv[])
{
	GtkWidget *window, *grid, *widget;
	GocCanvas *canvas;


	gtk_init (&argc, &argv);
	/* Initialize libgoffice */
	libgoffice_init ();
	/* Initialize plugins manager */

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_resize (GTK_WINDOW (window), 300, 340);
	gtk_window_set_title (GTK_WINDOW (window), "shapes demo");
	g_signal_connect (window, "destroy", gtk_main_quit, NULL);

	grid = gtk_grid_new ();
	g_object_set (G_OBJECT (grid), "orientation", GTK_ORIENTATION_VERTICAL, NULL);
	canvas = g_object_new (GOC_TYPE_CANVAS, "expand", TRUE, NULL);
	g_signal_connect_swapped (canvas, "button-press-event", G_CALLBACK (my_test), canvas);
	gtk_container_add (GTK_CONTAINER (grid), GTK_WIDGET(canvas));

	widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_container_add (GTK_CONTAINER (grid), widget);


	widget = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (widget), 80);
	g_signal_connect_swapped (G_OBJECT (widget), "activate", G_CALLBACK (enter_callback), canvas);

	gtk_container_add (GTK_CONTAINER (grid), widget);

	gtk_container_add (GTK_CONTAINER (window), grid);
	gtk_widget_show_all (GTK_WIDGET (window));
	if (argc > 1)
		open_file (canvas, *(argv+1));

	gtk_main ();

	/* Clean libgoffice stuff */
	libgoffice_shutdown ();
	return 0;
}
