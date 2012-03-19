/*
 * go-demo.c :
 *
 * Copyright (C) 2003-2005 Jean Brefort (jean.brefort@normalesup.org)
 * Copyright (C) 2008 Sun Microsystems, Inc.  All rights reserved.
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

#include <glib.h>
#include <strings.h>
#include <gtk/gtk.h>

#include <goffice/goffice.h>
#include <goffice/app/go-plugin.h>
#include <goffice/app/go-plugin-loader-module.h>
#include <goffice/data/go-data-simple.h>
#include <goffice/graph/gog-data-set.h>
#include <goffice/graph/gog-label.h>
#include <goffice/graph/gog-object.h>
#include <goffice/graph/gog-plot.h>
#include <goffice/graph/gog-series.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/gtk/go-graph-widget.h>

typedef struct _GoDemoPrivate GODemoPrivate;
struct _GoDemoPrivate {
	GtkWidget *toplevel;
	GtkBuilder  *xml;

	GtkAction *menu_item_quit;
	GtkWidget *btn_regen;
	GtkWidget *notebook_charts;
	GtkWidget *notebook_data;

	/* Legend data widgets */
	GtkEntry* entry_legend[6];

	/* Value data widgets */
	GtkSpinButton* spb_value[6];

	/* Index data widgets */
	GtkSpinButton* spb_index[6];

	/* Angel data widgets */
	GtkSpinButton* spb_angle[6];

	/* Starts data widgets */
	GtkSpinButton* spb_start[6];

	/* Matrix data widgets */
	GtkSpinButton* spb_matrix[20];
};

static char const * const legends[] = {"first", "second",
					"third", "fourth",
					"fifth", "sixth"};
static double values[6] = {10., 20., 30., 40., 50., 60.};
static double indexs[6] = {1, 2, 3, 4, 5, 6};
static double angles[6] = {0, 30, 80, 180, 250, 300};
static double starts[6] = {1., 2., 3., 4., 5., 6.};
static double matrix[20] = {10., 20., 30., 40., 50., 60., 70., 80., 90., 100.,
			  110., 120., 130., 140., 150., 160., 170., 180., 190., 200.};

static void generate_all_charts (GtkNotebook *notebook);

static void
on_quit (GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_destroy (user_data);
	gtk_main_quit ();
}

static void
btn_regen_clicked (GtkButton *button, gpointer user_data)
{
	GODemoPrivate *priv = (GODemoPrivate *)user_data;
	int i;
	/* get data from widgets */
	for (i = 0; i < 6; i++) {
		values[i] = gtk_spin_button_get_value (priv->spb_value[i]);
		indexs[i] = gtk_spin_button_get_value (priv->spb_index[i]);
		angles[i] = gtk_spin_button_get_value (priv->spb_angle[i]);
		starts[i] = gtk_spin_button_get_value (priv->spb_start[i]);
	}
	for (i = 0; i < 20; i++) {
		matrix[i] = gtk_spin_button_get_value (priv->spb_matrix[i]);
	}

	/* Re-generate all charts based on new data */
	generate_all_charts (GTK_NOTEBOOK (priv->notebook_charts));
}

static GogPlot *
setup_page (GtkNotebook *notebook, const gchar *service_id)
{
	GtkWidget *child, *w;
	GogChart *chart;
	GogGraph *graph;
	GogLabel *label;
	GogPlot *plot;
	GOData *data;
	GOStyle *style;
	PangoFontDescription *desc;

	child = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_notebook_append_page (notebook, child, gtk_label_new (service_id));
	/* Create a graph widget and add it to the GtkVBox */
	w = go_graph_widget_new (NULL);
	gtk_box_pack_end (GTK_BOX (child), w, TRUE, TRUE, 0);
	/* Get the embedded graph */
	graph = go_graph_widget_get_graph (GO_GRAPH_WIDGET (w));
	/* Add a title */
	label = (GogLabel *) g_object_new (GOG_TYPE_LABEL, NULL);
	data = go_data_scalar_str_new (service_id, FALSE);
	gog_dataset_set_dim (GOG_DATASET (label), 0, data, NULL);
	gog_object_add_by_name (GOG_OBJECT (graph), "Title", GOG_OBJECT (label));
	/* Change the title font */
	style = go_styled_object_get_style (GO_STYLED_OBJECT (label));
	desc = pango_font_description_from_string ("Sans bold 16");
	go_style_set_font_desc (style, desc);
	/* Get the chart created by the widget initialization */
	chart = go_graph_widget_get_chart (GO_GRAPH_WIDGET (w));
	/* Create a plot and add it to the chart */
	plot = (GogPlot *) gog_plot_new_by_name (service_id);
	gog_object_add_by_name (GOG_OBJECT (chart), "Plot", GOG_OBJECT (plot));
	/* Add a legend to the chart */
	gog_object_add_by_name (GOG_OBJECT (chart), "Legend", NULL);

	return plot;
}

static void
insert_1_5d_data (GogPlot *plot)
{
	GSList *list;
	GogSeries *series;
	GOData *data;
	GError *error;

	/* Create a series for the plot and populate it with some simple data */
	list = (GSList *)gog_plot_get_series (plot);
	if (g_slist_length (list) == 1)
		series = g_slist_nth_data (list, 0);
	else
		series = gog_plot_new_series (plot);

	data = go_data_vector_str_new (legends, 6, NULL);
	gog_series_set_dim (series, 0, data, &error);
	data = go_data_vector_val_new (values, 6, NULL);
	gog_series_set_dim (series, 1, data, &error);
}

static void
insert_polar_data (GogPlot *plot)
{
	GSList *list;
	GogSeries *series;
	GOData *data;
	GError *error;

	/* Create a series for the plot and populate it with some simple data */
	list = (GSList *)gog_plot_get_series (plot);
	if (g_slist_length (list) == 1)
		series = g_slist_nth_data (list, 0);
	else
		series = gog_plot_new_series (plot);

	data = go_data_vector_val_new (angles, 6, NULL);
	gog_series_set_dim (series, 0, data, &error);
	data = go_data_vector_val_new (values, 6, NULL);
	gog_series_set_dim (series, 1, data, &error);
}

static void
insert_box_data (GogPlot *plot)
{
	GSList *list;
	GogSeries *series;
	GOData *data;
	GError *error;

	/* Create a series for the plot and populate it with some simple data */
	list = (GSList *)gog_plot_get_series (plot);
	if (g_slist_length (list) == 1)
		series = g_slist_nth_data (list, 0);
	else
		series = gog_plot_new_series (plot);

	data = go_data_vector_val_new (values, 6, NULL);
	gog_series_set_dim (series, 0, data, &error);
}

static void
insert_histogram_data (GogPlot *plot)
{
	GSList *list;
	GogSeries *series;
	GOData *data;
	GError *error;

	/* Create a series for the plot and populate it with some simple data */
	list = (GSList *)gog_plot_get_series (plot);
	if (g_slist_length (list) == 1)
		series = g_slist_nth_data (list, 0);
	else
		series = gog_plot_new_series (plot);

	data = go_data_vector_val_new (indexs, 6, NULL);
	gog_series_set_dim (series, 0, data, &error);
	data = go_data_vector_val_new (values, 5, NULL);
	gog_series_set_dim (series, 1, data, &error);
}

static void
insert_xy_data (GogPlot *plot)
{
	insert_histogram_data (plot);
}

static void
insert_bubble_data (GogPlot *plot)
{
	GSList *list;
	GogSeries *series;
	GOData *data;
	GError *error;

	/* Create a series for the plot and populate it with some simple data */
	list = (GSList *)gog_plot_get_series (plot);
	if (g_slist_length (list) == 1)
		series = g_slist_nth_data (list, 0);
	else
		series = gog_plot_new_series (plot);

	data = go_data_vector_val_new (indexs, 6, NULL);
	gog_series_set_dim (series, 0, data, &error);
	data = go_data_vector_val_new (indexs, 6, NULL);
	gog_series_set_dim (series, 1, data, &error);
	data = go_data_vector_val_new (values, 6, NULL);
	gog_series_set_dim (series, 2, data, &error);
}

static void
insert_dropbar_data (GogPlot *plot)
{
	GSList *list;
	GogSeries *series;
	GOData *data;
	GError *error;

	/* Create a series for the plot and populate it with some simple data */
	list = (GSList *)gog_plot_get_series (plot);
	if (g_slist_length (list) == 1)
		series = g_slist_nth_data (list, 0);
	else
		series = gog_plot_new_series (plot);

	data = go_data_vector_str_new (legends, 6, NULL);
	gog_series_set_dim (series, 0, data, &error);
	data = go_data_vector_val_new (starts, 6, NULL);
	gog_series_set_dim (series, 1, data, &error);
	data = go_data_vector_val_new (values, 6, NULL);
	gog_series_set_dim (series, 2, data, &error);
}

static void
insert_xyz_data (GogPlot *plot)
{
	GSList *list;
	GogSeries *series;
	GOData *data;
	GError *error;

	/* Create a series for the plot and populate it with some simple data */
	list = (GSList *)gog_plot_get_series (plot);
	if (g_slist_length (list) == 1)
		series = g_slist_nth_data (list, 0);
	else
		series = gog_plot_new_series (plot);

	data = go_data_vector_val_new (indexs, 6, NULL);
	gog_series_set_dim (series, 0, data, &error);
	data = go_data_vector_val_new (indexs, 6, NULL);
	gog_series_set_dim (series, 1, data, &error);
	data = go_data_matrix_val_new (matrix, 4, 5, NULL);
	gog_series_set_dim (series, 2, data, &error);
}

static void
insert_minmax_data (GogPlot *plot)
{
	insert_dropbar_data (plot);
}

static void
init_data_widgets (GODemoPrivate *priv)
{
	int i;
	char *wname = NULL;

	for (i =0; i< 6; i++) {
		/* init legend widgets */
		g_free (wname);
		wname = g_strdup_printf ("entry_legend_%d", i+1);
		priv->entry_legend[i] = GTK_ENTRY (gtk_builder_get_object (
					priv->xml, wname));
		gtk_entry_set_text (priv->entry_legend[i], legends[i]);
		/* init value widgets */
		g_free (wname);
		wname = g_strdup_printf ("spb_value_%d", i+1);
		priv->spb_value[i] = GTK_SPIN_BUTTON (gtk_builder_get_object (
					priv->xml, wname));
		gtk_spin_button_set_value (priv->spb_value[i], values[i]);
		g_signal_connect(priv->spb_value[i], "value-changed",
				G_CALLBACK(btn_regen_clicked), priv);
		/* init index widgets */
		g_free (wname);
		wname = g_strdup_printf ("spb_index_%d", i+1);
		priv->spb_index[i] = GTK_SPIN_BUTTON (gtk_builder_get_object (
					priv->xml, wname));
		gtk_spin_button_set_value (priv->spb_index[i], indexs[i]);
		g_signal_connect (priv->spb_index[i], "value-changed",
				G_CALLBACK(btn_regen_clicked), priv);
		/* init angle widgets */
		g_free (wname);
		wname = g_strdup_printf ("spb_angle_%d", i+1);
		priv->spb_angle[i] = GTK_SPIN_BUTTON (gtk_builder_get_object (
					priv->xml, wname));
		gtk_spin_button_set_value (priv->spb_angle[i], angles[i]);
		g_signal_connect (priv->spb_angle[i], "value-changed",
				G_CALLBACK(btn_regen_clicked), priv);
		/* init start widgets */
		g_free (wname);
		wname = g_strdup_printf ("spb_start_%d", i+1);
		priv->spb_start[i] = GTK_SPIN_BUTTON (gtk_builder_get_object (
					priv->xml, wname));
		gtk_spin_button_set_value (priv->spb_start[i], starts[i]);
		g_signal_connect (priv->spb_start[i], "value-changed",
				G_CALLBACK(btn_regen_clicked), priv);
	}

	for (i =0; i <20; i++) {
		/* init start widgets */
		g_free (wname);
		wname = g_strdup_printf ("spb_matrix_%d", i+1);
		priv->spb_matrix[i] = GTK_SPIN_BUTTON (gtk_builder_get_object (
					priv->xml, wname));
		gtk_spin_button_set_value (priv->spb_matrix[i], matrix[i]);
		g_signal_connect (priv->spb_matrix[i], "value-changed",
				G_CALLBACK(btn_regen_clicked), priv);
	}

}

static void
generate_all_charts (GtkNotebook *notebook)
{
	GogPlot *plot = NULL;
	int i;
	gint current_page;

	current_page = gtk_notebook_get_current_page (notebook);

	/* clean up old pages */
	for (i = gtk_notebook_get_n_pages (notebook) - 1; i >= 0; i--) {
		gtk_notebook_remove_page (notebook, i);
	}

	plot = setup_page (notebook, "GogPiePlot");
	insert_1_5d_data (plot);
	plot = setup_page (notebook, "GogRingPlot");
	insert_1_5d_data (plot);

	plot = setup_page (notebook, "GogRadarPlot");
	insert_1_5d_data (plot);
	plot = setup_page (notebook, "GogRadarAreaPlot");
	insert_1_5d_data (plot);
	plot = setup_page (notebook, "GogPolarPlot");
	insert_polar_data (plot);

	plot = setup_page (notebook, "GogBoxPlot");
	insert_box_data (plot);
	plot = setup_page (notebook, "GogHistogramPlot");
	insert_histogram_data (plot);

	plot = setup_page (notebook, "GogXYPlot");
	insert_xy_data (plot);
	plot = setup_page (notebook, "GogBubblePlot");
	insert_bubble_data (plot);
	plot = setup_page (notebook, "GogXYColorPlot");
	insert_bubble_data (plot);

	plot = setup_page (notebook, "GogLinePlot");
	insert_1_5d_data (plot);
	plot = setup_page (notebook, "GogAreaPlot");
	insert_1_5d_data (plot);
	plot = setup_page (notebook, "GogBarColPlot");
	insert_1_5d_data (plot);
	plot = setup_page (notebook, "GogDropBarPlot");
	insert_dropbar_data (plot);
	plot = setup_page (notebook, "GogMinMaxPlot");
	insert_minmax_data (plot);

	plot = setup_page (notebook, "GogContourPlot");
	insert_xyz_data (plot);
	plot = setup_page (notebook, "GogSurfacePlot");
	insert_xyz_data (plot);


	/* These charts are included because they are for compatibility
	   with the weird excel, Jean suggest to drop them. See bug
	   http://bugzilla.gnome.org/show_bug.cgi?id=547408 for more detail.
	*/
	/*
	plot = setup_page (notebook, "XLContourPlot");
	plot = setup_page (notebook, "XLSurfacePlot");
	*/

	/* FIXME: These charts are included for they have critical warning */
	/*
	plot = setup_page (notebook, "GogLogFitCurve");

	plot = setup_page (notebook, "GogLinRegCurve");
	plot = setup_page (notebook, "GogExpRegCurve");
	plot = setup_page (notebook, "GogPowerRegCurve");
	plot = setup_page (notebook, "GogLogRegCurve");
	plot = setup_page (notebook, "GogPolynomRegCurve");

	plot = setup_page (notebook, "GogMovingAvg");
	plot = setup_page (notebook, "GogExpSmooth");
	*/
	gtk_widget_show_all (GTK_WIDGET (notebook));
	if (current_page != -1)
		gtk_notebook_set_current_page (notebook, current_page);
}

int
main (int argc, char *argv[])
{
	GtkWidget *window;
	GODemoPrivate *priv;
	const gchar *ui_file;

	gtk_init (&argc, &argv);
	/* Initialize libgoffice */
	libgoffice_init ();
	/* Initialize plugins manager */
	go_plugins_init (NULL, NULL, NULL, NULL, TRUE, GO_TYPE_PLUGIN_LOADER_MODULE);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_resize (GTK_WINDOW (window), 800, 800);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (window), "GOffice demo");
	g_signal_connect (window, "destroy", gtk_main_quit, NULL);

	priv = g_new0 (GODemoPrivate, 1);
#define GO_DEMO_UI  "go-demo.ui"
	ui_file = "./" GO_DEMO_UI;
	if (!g_file_test (ui_file, G_FILE_TEST_EXISTS))
		ui_file = "../" GO_DEMO_UI;
	if (!g_file_test (ui_file, G_FILE_TEST_EXISTS)) {
		g_warning ("Unable to load file %s", GO_DEMO_UI);
		return 0;
	}
	priv->xml = gtk_builder_new ();
	gtk_builder_add_from_file (priv->xml, ui_file, NULL);
#undef GO_DEMO_GLADE

	priv->toplevel = go_gtk_builder_get_widget (priv->xml, "toplevel");
	gtk_container_add (GTK_CONTAINER (window), priv->toplevel);

	priv->menu_item_quit = GTK_ACTION (gtk_builder_get_object (priv->xml, "menu_item_quit"));
	g_signal_connect (priv->menu_item_quit, "activate",
			G_CALLBACK (on_quit), window);

	priv->btn_regen = go_gtk_builder_get_widget (priv->xml, "btn_regen");
	g_signal_connect (priv->btn_regen, "clicked",
			G_CALLBACK (btn_regen_clicked), priv);

	priv->notebook_data = go_gtk_builder_get_widget (priv->xml, "notebook_data");
	init_data_widgets (priv);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook_data), 1);

	priv->notebook_charts = go_gtk_builder_get_widget (priv->xml, "notebook_charts");
	generate_all_charts (GTK_NOTEBOOK (priv->notebook_charts));

	gtk_widget_show_all (GTK_WIDGET (window));

	gtk_main ();

	g_object_unref (priv->xml);
	g_free (priv);

	/* Clean libgoffice stuff */
	libgoffice_shutdown ();
	return 0;
}
