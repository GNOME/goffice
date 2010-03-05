/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
#include <goffice/gtk/go-gtk-compat.h>



static void
change_shape (GocItem *goi)
{
	GocPoints *points = goc_points_new (4);
	
	points->points[0].x = 135;
	points->points[0].y = 150;
	points->points[1].x = 114;
	points->points[1].y = 160;
	points->points[2].x = 110;
	points->points[2].y = 180;
	points->points[3].x = 200;
	points->points[3].y = 200;
	
	goc_item_set (goi, "points", points, NULL);

}

//
static void
my_test (GocCanvas *canvas, GdkEventButton *event, G_GNUC_UNUSED gpointer data) {
	GocItem *item;
	double x,y;
	GOStyle *style;
	
	g_print ("%.0f %.0f Button: %d\n", event->x, event->y, event->button);

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return;
		
	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 + (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 + event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	item = goc_canvas_get_item_at (canvas, x, y);
	if (item) {
		style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
		if (style->line.color == GO_COLOR_BLACK) {
			style->line.color = GO_COLOR_RED;
		} else {
			style->line.color = GO_COLOR_BLACK;
		}
		goc_item_invalidate (item);
	}
	goc_canvas_scroll_to (canvas, 10, -10);
}

static void
create_shape (GocCanvas *canvas)
{
	GocItem *goi;
	double x = 180, y = 120, w = 50, h = 20;

	goi = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_RECTANGLE, NULL);
	goc_item_set (goi, "width", w, "height", h, "x", x, "y", y, NULL);
	

}


static void
on_quit (GtkObject *object)
{
	gtk_object_destroy (object);
	gtk_main_quit ();
}

int
main (int argc, char *argv[])
{
	GtkWidget *window, *box, *hbox, *widget;
	GocCanvas *canvas;
	GOStyle *style;
	/* GError *error; */
	GocItem *goi, *goi2;
	double xc = 80.,yc = 150., xr = 30.,yr = 60.,ang1=M_PI*0.45,ang2 = M_PI*1.75;
	GocPoints *points = goc_points_new (5);
	
	points->points[0].x = xc + xr * cos (atan2 (xr / yr * sin (ang1), cos (ang1)));
	points->points[0].y = yc - yr * sin (atan2 (xr / yr * sin (ang1), cos (ang1)));
	points->points[1].x = 10;
	points->points[1].y = 40;
	points->points[2].x = 23;
	points->points[2].y = 25;
	points->points[3].x = 120;
	points->points[3].y = 150;
	points->points[4].x = xc + xr * cos (atan2 (xr / yr * sin (ang2), cos (ang2)));
	points->points[4].y = yc - yr * sin (atan2 (xr / yr * sin (ang2), cos (ang2)));

	g_print("Arc: %.0f %.0f %.0f %.0f\n", points->points[0].x, points->points[0].y, points->points[4].x, points->points[4].y);

	gtk_init (&argc, &argv);
	/* Initialize libgoffice */
	libgoffice_init ();
	/* Initialize plugins manager */

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_resize (GTK_WINDOW (window), 300, 340);
	gtk_window_set_title (GTK_WINDOW (window), "shapes demo");
	g_signal_connect (window, "destroy", gtk_main_quit, NULL);

	box = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	canvas =g_object_new (GOC_TYPE_CANVAS, NULL);
	g_signal_connect_swapped (canvas, "button-press-event", G_CALLBACK (my_test), canvas);

	goi = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_POLYLINE, NULL);
	goc_item_set (goi, "points", points, NULL);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (goi));
	style->line.width = 1;
	style->line.color = GO_COLOR_BLACK;

	goi2 = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_ARC,
	 "xc", xc, "yc", yc, "xr",xr, "yr", yr,
	 "ang1", ang1, "ang2", ang2, NULL);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (goi2));
	style->line.width = 3;
	style->line.color = GO_COLOR_YELLOW;

	goi2 = goc_item_new (goc_canvas_get_root(canvas), GOC_TYPE_ARC,
	 "xc", xc, "yc", yc, "xr", xr, "yr", yr,
	 "ang1", ang1, "ang2", ang2, "rotation", M_PI / 4., NULL);

	widget = gtk_button_new_from_stock (GTK_STOCK_QUIT);
	g_signal_connect_swapped (widget, "clicked", G_CALLBACK (on_quit), window);
	gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	widget = gtk_button_new_from_stock (GTK_STOCK_NEW);
	g_signal_connect_swapped (widget, "clicked", G_CALLBACK (create_shape), canvas);
	gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	widget = gtk_button_new_from_stock (GTK_STOCK_OPEN);
	g_signal_connect_swapped (widget, "clicked", G_CALLBACK (change_shape), goi);
	gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (box), hbox, FALSE, FALSE, 0);

	widget = gtk_hseparator_new ();
	gtk_box_pack_end (GTK_BOX (box), widget, FALSE, FALSE, 2);

	gtk_box_pack_end (GTK_BOX (box), GTK_WIDGET(canvas), TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (window), box);
	gtk_widget_show_all (GTK_WIDGET (window));

	widget = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

	gtk_main ();

	/* Clean libgoffice stuff */
	libgoffice_shutdown ();
	return 0;
}
