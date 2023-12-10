/*
 * gog-error-bar.c:
 *
 * Copyright (C) 2004 Jean Brefort (jean.brefort@ac-dijon.fr)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <gsf/gsf-impl-utils.h>

#include <glib/gi18n-lib.h>
#include <string.h>

#define CC2XML(s) ((const xmlChar *)(s))

/**
 * GogErrorBarType:
 * @GOG_ERROR_BAR_TYPE_NONE: No error bars.
 * @GOG_ERROR_BAR_TYPE_ABSOLUTE: Absolute errors.
 * @GOG_ERROR_BAR_TYPE_RELATIVE: Relative errors.
 * @GOG_ERROR_BAR_TYPE_PERCENT: Relative errors as percent.
 **/

/**
 * GogErrorBarDirection:
 * @GOG_ERROR_BAR_DIRECTION_HORIZONTAL: horizontal (xy plots).
 * @GOG_ERROR_BAR_DIRECTION_VERTICAL: vertical (xy plots).
 * @GOG_ERROR_BAR_DIRECTION_ANGULAR: angular (polar plots).
 * @GOG_ERROR_BAR_DIRECTION_RADIAL: radial (polar plots).
 **/

/**
 * GogErrorBarDisplay:
 * @GOG_ERROR_BAR_DISPLAY_NONE: no display.
 * @GOG_ERROR_BAR_DISPLAY_POSITIVE: display positive deviations.
 * @GOG_ERROR_BAR_DISPLAY_NEGATIVE: display negative deviations.
 * @GOG_ERROR_BAR_DISPLAY_BOTH: display both positive and negative deviations.
 **/

typedef GObjectClass GogErrorBarClass;
static GObjectClass *error_bar_parent_klass;

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/gtk/go-color-selector.h>

typedef struct {
	GogSeries 	   *series;
	GogErrorBar 	   *bar;
	char const 	   *property;
	GogErrorBarDisplay  display;
	GOColor 	    color;
	double 		    width, line_width;
} GogErrorBarEditor;

static const struct {
	char const 		*h_pixbuf;
	char const 		*v_pixbuf;
	char const 		*label;
	GogErrorBarDisplay	 display;
} display_combo_desc[] = {
	{"res:go:graph/bar-none.png",
	 "res:go:graph/bar-none.png",
	 N_("None"),
	 GOG_ERROR_BAR_DISPLAY_NONE},

	{"res:go:graph/bar-hplus.png",
	 "res:go:graph/bar-vplus.png",
	 N_("Positive"),
	 GOG_ERROR_BAR_DISPLAY_POSITIVE},

	{"res:go:graph/bar-hminus.png",
	 "res:go:graph/bar-vminus.png",
	 N_("Negative"),
	 GOG_ERROR_BAR_DISPLAY_NEGATIVE},

	{"res:go:graph/bar-hboth.png",
	 "res:go:graph/bar-vboth.png",
	 N_("Both"),
	 GOG_ERROR_BAR_DISPLAY_BOTH}
};

static void
cb_destroy (G_GNUC_UNUSED GtkWidget *w, GogErrorBarEditor *editor)
{
	if (editor->bar)
		g_object_unref (editor->bar);
	g_free (editor);
}

static void
cb_width_changed (GtkAdjustment *adj, GogErrorBarEditor *editor)
{
	editor->width = gtk_adjustment_get_value (adj);
	if (editor->bar) {
		editor->bar->width = gtk_adjustment_get_value (adj);
		gog_object_request_update (GOG_OBJECT (editor->series));
	}
}

static void
cb_line_width_changed (GtkAdjustment *adj, GogErrorBarEditor *editor)
{
	editor->line_width = gtk_adjustment_get_value (adj);
	if (editor->bar) {
		editor->bar->style->line.width = gtk_adjustment_get_value (adj);
		gog_object_request_update (GOG_OBJECT (editor->series));
	}
}

static void
cb_color_changed (GOSelector *selector,
		  GogErrorBarEditor *editor)
{
	GOColor color = go_color_selector_get_color (selector, NULL);

	editor->color = color;
	if (editor->bar) {
		editor->bar->style->line.color = color;
		gog_object_request_update (GOG_OBJECT (editor->series));
	}
}

static void
cb_display_changed (GtkComboBox *combo, GogErrorBarEditor *editor)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GValue value;

	memset (&value, 0, sizeof (GValue));
       	model = gtk_combo_box_get_model (combo);
	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get_value (model, &iter, 2, &value);

	editor->display = g_value_get_uint (&value);
	if (editor->bar) {
		editor->bar->display = g_value_get_uint (&value);
		gog_object_request_update (GOG_OBJECT (editor->series));
	}

	g_value_unset (&value);
}

static void
cb_type_changed (GtkWidget *w, GogErrorBarEditor *editor)
{
	GtkBuilder *gui = GTK_BUILDER (g_object_get_data (G_OBJECT (w), "gui"));
	gpointer data;
	GogDataset *set;
	GogDataAllocator *dalloc;
	int type = gtk_combo_box_get_active (GTK_COMBO_BOX (w));
	dalloc = GOG_DATA_ALLOCATOR (g_object_get_data (G_OBJECT (w), "allocator"));
	if (type == GOG_ERROR_BAR_TYPE_NONE) {
		set = GOG_DATASET (editor->bar->series);
		gog_dataset_set_dim (set, editor->bar->error_i, NULL, NULL);
		gog_dataset_set_dim (set, editor->bar->error_i + 1, NULL, NULL);
		g_object_set (editor->series, editor->property, NULL, NULL);
		g_object_unref (editor->bar);
		editor->bar = NULL;
		data = g_object_get_data (G_OBJECT (w), "plus");
		if (GTK_IS_WIDGET (data))
			gtk_widget_destroy (GTK_WIDGET(data));
		data = g_object_get_data (G_OBJECT (w), "minus");
		if (GTK_IS_WIDGET (data))
			gtk_widget_destroy (GTK_WIDGET(data));
		g_object_set_data (G_OBJECT (w), "plus", NULL);
		g_object_set_data (G_OBJECT (w), "minus", NULL);
		gtk_widget_hide (go_gtk_builder_get_widget (gui, "values-grid"));
		gtk_widget_hide (go_gtk_builder_get_widget (gui, "style-grid"));
	} else {
		GtkWidget *grid = go_gtk_builder_get_widget (gui, "values-grid");
		if (!editor->bar) {
			editor->bar = g_object_new (GOG_TYPE_ERROR_BAR, NULL);
			editor->bar->style->line.color = editor->color;
			editor->bar->style->line.width = editor->line_width;
			editor->bar->width = editor->width;
			editor->bar->display = editor->display;
			editor->bar->type = type;
			g_object_set (editor->series, editor->property, editor->bar, NULL);
			g_object_unref (editor->bar);
			g_object_get (editor->series, editor->property, &editor->bar, NULL);
		}
		editor->bar->type = type;
		set = GOG_DATASET (editor->bar->series);
		data = g_object_get_data (G_OBJECT (w), "plus");
		if (!data) {
			GtkWidget* al = GTK_WIDGET (gog_data_allocator_editor (dalloc, set,
									       editor->bar->error_i,
									       GOG_DATA_VECTOR));
			gtk_widget_set_hexpand (al, TRUE);
			gtk_grid_attach (GTK_GRID (grid), al, 1, 1, 1, 1);
			g_object_set_data (G_OBJECT (w), "plus", al);
		}
		data = g_object_get_data (G_OBJECT (w), "minus");
		if (!data) {
			GtkWidget* al = GTK_WIDGET (gog_data_allocator_editor (dalloc, set,
									       editor->bar->error_i + 1,
									       GOG_DATA_VECTOR));
			gtk_widget_set_hexpand (al, TRUE);
			gtk_grid_attach (GTK_GRID (grid), al, 1, 2, 1, 1);
			g_object_set_data (G_OBJECT (w), "minus", al);
		}
		gtk_widget_show_all (grid);
		gtk_widget_show_all (go_gtk_builder_get_widget (gui, "style-grid"));
	}
	gog_object_request_update (GOG_OBJECT (editor->series));
}

/**
 * gog_error_bar_prefs:
 * @series: #GogSeries
 * @property: the name of the @series property correspondig to the #GogErrorBar.
 * @direction: #GogErrorBarDirection
 * @dalloc: #GogDataAllocator
 * @cc: #GOCmdContext
 *
 * Returns: (transfer full): the error bar properties #GtkWidget
 **/
gpointer
gog_error_bar_prefs (GogSeries *series,
		     char const *property,
		     GogErrorBarDirection direction,
		     GogDataAllocator *dalloc,
		     GOCmdContext *cc)
{
	GtkBuilder *gui;
	GtkWidget *w, *bar_prefs, *grid;
	GtkWidget *combo;
	GtkListStore *list;
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	GdkPixbuf *pixbuf;
	GogDataset *set;
	GogErrorBarEditor *editor;
	unsigned int i;

	g_return_val_if_fail (GOG_IS_SERIES (series), NULL);

	editor = g_new0 (GogErrorBarEditor, 1);
	editor->series = series;
	editor->property = property;
	g_object_get (series, property, &editor->bar, NULL);
	if (editor->bar) {
		editor->color = editor->bar->style->line.color;
		editor->line_width = editor->bar->style->line.width;
		editor->width = editor->bar->width;
		editor->display = editor->bar->display;
	} else {
		editor->color = GO_COLOR_BLACK;
		editor->line_width = 1.;
		editor->width = 5.;
		editor->display = GOG_ERROR_BAR_DISPLAY_BOTH;
	}
	set = GOG_DATASET (series);

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-error-bar-prefs.ui", GETTEXT_PACKAGE, cc);

	/* Style properties */
	grid = go_gtk_builder_get_widget (gui, "style-grid");

	/* Width */
	w = go_gtk_builder_get_widget (gui, "width");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), editor->width);
	g_signal_connect (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w)),
		"value_changed",
		G_CALLBACK (cb_width_changed), editor);

	/* Line width */
	w = go_gtk_builder_get_widget (gui, "line_width");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), editor->line_width);
	g_signal_connect (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w)),
		"value_changed",
		G_CALLBACK (cb_line_width_changed), editor);

	/* Color */
	w = go_selector_new_color (editor->color, GO_COLOR_BLACK, "error-bar");
	gtk_label_set_mnemonic_widget (
		GTK_LABEL (gtk_builder_get_object (gui, "color_label")), w);
	g_signal_connect (G_OBJECT (w),
		"activate",
		G_CALLBACK (cb_color_changed), editor);
	gtk_widget_set_halign (w, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), w, 1, 4, 1, 1);

	/* Display style */
	list = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_UINT);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list));
	g_object_unref (list);

	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell, "pixbuf", 0, NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell, "text", 1, NULL);

	for (i = 0; i < G_N_ELEMENTS (display_combo_desc); i++) {
		pixbuf = go_gdk_pixbuf_load_from_file (direction == GOG_ERROR_BAR_DIRECTION_HORIZONTAL ?
						  display_combo_desc[i].h_pixbuf :
						  display_combo_desc[i].v_pixbuf);
		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter,
				    0, pixbuf,
				    1, _(display_combo_desc[i].label),
				    2, display_combo_desc[i].display,
				    -1);
		g_object_unref (pixbuf);
		if (editor->display == display_combo_desc[i].display || i == 0)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
	}

	gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET(combo), 1, 1, 1, 1);
	g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (cb_display_changed), editor);

	/* if radial, change the width unit */
	if (direction == GOG_ERROR_BAR_DIRECTION_RADIAL) {
		w = go_gtk_builder_get_widget (gui, "width-label");
		/* Note for translator: the angle unit */
		gtk_label_set_text (GTK_LABEL (w), _("°"));
	}

	/* Category property*/
	w = go_gtk_builder_get_widget (gui, "category-combo");
	gtk_combo_box_set_active (GTK_COMBO_BOX (w), (editor->bar)? (int) editor->bar->type: 0);
	g_object_set_data (G_OBJECT (w), "gui", gui);
	g_object_set_data (G_OBJECT (w), "allocator", dalloc);
	g_signal_connect (G_OBJECT (w), "changed", G_CALLBACK (cb_type_changed), editor);

	/* Value properties */
	bar_prefs = go_gtk_builder_get_widget (gui, "gog-error-bar-prefs");
	g_object_ref (bar_prefs);
	g_signal_connect (bar_prefs, "destroy", G_CALLBACK (cb_destroy), editor);
	gtk_widget_show_all (bar_prefs);

	if (editor->bar) {
		GtkWidget* al = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, editor->bar->error_i, GOG_DATA_VECTOR));
		grid = go_gtk_builder_get_widget (gui, "values-grid");
		gtk_widget_show (al);
		gtk_widget_set_hexpand (al, TRUE);
		gtk_grid_attach (GTK_GRID (bar_prefs), al, 1, 1, 1, 1);
		g_object_set_data (G_OBJECT (w), "plus", al);
		al = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, editor->bar->error_i + 1, GOG_DATA_VECTOR));
		gtk_widget_show (al);
		gtk_widget_set_hexpand (al, TRUE);
		gtk_grid_attach (GTK_GRID (bar_prefs), al, 1, 2, 1, 1);
		g_object_set_data (G_OBJECT (w), "minus", al);
	} else {
		gtk_widget_hide (go_gtk_builder_get_widget (gui, "values-grid"));
		gtk_widget_hide (go_gtk_builder_get_widget (gui, "style-grid"));
	}
	g_signal_connect_swapped (G_OBJECT (bar_prefs), "destroy", G_CALLBACK (g_object_unref), gui);

	return GTK_WIDGET(bar_prefs);
}
#endif

static void
gog_error_bar_init (GogErrorBar* bar)
{
	bar->type = GOG_ERROR_BAR_TYPE_NONE;
	bar->display = GOG_ERROR_BAR_DISPLAY_BOTH;
	bar->width = 5.;
	bar->style = go_style_new ();
	bar->style->interesting_fields = GO_STYLE_LINE;
	bar->style->line.color = GO_COLOR_BLACK;
	bar->style->line.width = 1.;
}

static void
gog_error_bar_finalize (GObject *obj)
{
	GogErrorBar *bar = GOG_ERROR_BAR (obj);
	if (bar->style) {
		g_object_unref (bar->style);
		bar->style = NULL;
	}
	(error_bar_parent_klass->finalize) (obj);
}

static void
gog_error_bar_class_init (GogErrorBarClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	error_bar_parent_klass = g_type_class_peek_parent (klass);

	gobject_klass->finalize		= gog_error_bar_finalize;
}

static void
gog_error_bar_persist_sax_save (GOPersist const *gp, GsfXMLOut *output)
{
	GogErrorBar *bar = GOG_ERROR_BAR (gp);
	char const *str;

	gsf_xml_out_add_cstr_unchecked (output, "type", "GogErrorBar");
	switch (bar->type) {
	case GOG_ERROR_BAR_TYPE_ABSOLUTE: str = "absolute"; break;
	case GOG_ERROR_BAR_TYPE_RELATIVE: str = "relative"; break;
	case GOG_ERROR_BAR_TYPE_PERCENT:  str = "percent"; break;
	default: str = NULL; break;
	}
	if (str != NULL)
		gsf_xml_out_add_cstr_unchecked (output, "error_type", str);

	switch (bar->display) {
	case GOG_ERROR_BAR_DISPLAY_NONE:	str = "none"; break;
	case GOG_ERROR_BAR_DISPLAY_POSITIVE:	str = "positive"; break;
	case GOG_ERROR_BAR_DISPLAY_NEGATIVE:	str = "negative"; break;
	default: str = NULL; break;
	}
	if (str != NULL)
		gsf_xml_out_add_cstr_unchecked (output, "display", str);
	if (bar->width != 5.)
		gsf_xml_out_add_float (output, "width", bar->width, 6);
	if (bar->style->line.width != 1.)
		gsf_xml_out_add_float (output, "line_width", bar->style->line.width, 6);
	if (bar->style->line.color != GO_COLOR_BLACK)
		go_xml_out_add_color (output, "color", bar->style->line.color);
}

static void
gog_error_bar_persist_prep_sax (GOPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	GogErrorBar *bar = GOG_ERROR_BAR (gog_xml_read_state_get_obj (xin));

	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "error_type")) {
			if (!strcmp (attrs[1], "absolute"))
				bar->type = GOG_ERROR_BAR_TYPE_ABSOLUTE;
			else if (!strcmp (attrs[1], "relative"))
				bar->type = GOG_ERROR_BAR_TYPE_RELATIVE;
			else if (!strcmp (attrs[1], "percent"))
				bar->type = GOG_ERROR_BAR_TYPE_PERCENT;
		} else if (0 == strcmp (attrs[0], "display")) {
			if (!strcmp (attrs[1], "none"))
				bar->display = GOG_ERROR_BAR_DISPLAY_NONE;
			else if (!strcmp (attrs[1], "positive"))
				bar->display = GOG_ERROR_BAR_DISPLAY_POSITIVE;
			else if (!strcmp (attrs[1], "negative"))
				bar->display = GOG_ERROR_BAR_DISPLAY_NEGATIVE;
		} else if (0 == strcmp (attrs[0], "width"))
			bar->width = g_strtod (attrs[1], NULL);
		else if (0 == strcmp (attrs[0], "line_width"))
			bar->style->line.width = g_strtod (attrs[1], NULL);
		else if (0 == strcmp (attrs[0], "color"))
			go_color_from_str (attrs[1], &bar->style->line.color);
#if 0
		else
			/* ignore unknown attrs.  they are from the surrounding
			 * layers of xml handler. */
#endif
}

static void
gog_error_bar_persist_init (GOPersistClass *iface)
{
	iface->prep_sax = gog_error_bar_persist_prep_sax;
	iface->sax_save = gog_error_bar_persist_sax_save;
}

GSF_CLASS_FULL (GogErrorBar, gog_error_bar,
		NULL, NULL, gog_error_bar_class_init, NULL,
		gog_error_bar_init, G_TYPE_OBJECT, 0,
		GSF_INTERFACE (gog_error_bar_persist_init, GO_TYPE_PERSIST))


/**
 * gog_error_bar_get_bounds:
 * @bar: A GogErrorBar
 * @index: the index corresponding to the value which error limits are
 * @min: where the minimum value will be stored
 * @max: where the maximum value will be stored
 *
 * If the value correponding to @index is valid, fills min and max with the error values:
 * -> positive_error in @max.
 * -> negative_error in @min.
 * If one of the errors is not valid or not defined, its value is set to -1.0.
 *
 * Returns: %FALSE if the @bar->type is %GOG_ERROR_BAR_TYPE_NONE or if the value is not valid,
 * 	%TRUE otherwise.
 **/
gboolean
gog_error_bar_get_bounds (GogErrorBar const *bar, int index, double *min, double *max)
{
	double value;
	GOData *data;
	GOData *vec;
	int length;

	/* -1 ensures that the bar will not be displayed if the error is not a correct one.
		With a 0 value, it might be, because of rounding errors */
	*min = *max = -1.;

	g_return_val_if_fail (GOG_IS_ERROR_BAR (bar), FALSE);
	if (!gog_series_is_valid (bar->series))
		return FALSE;
	vec = bar->series->values[bar->dim_i].data;
	if (vec == NULL || index < 0)
		return FALSE;
	value = go_data_get_vector_value (vec, index);
	data = bar->series->values[bar->error_i].data;
	length = (GO_IS_DATA (data)) ? go_data_get_vector_size (data) : 0;

	if ((bar->type == GOG_ERROR_BAR_TYPE_NONE) || isnan (value) || !go_finite (value))
		return FALSE;

	if (length == 1)
		*max = go_data_get_vector_value (data, 0);
	else if (length > index)
		*max = go_data_get_vector_value (data, index);

	data = bar->series->values[bar->error_i + 1].data;
	length = (GO_IS_DATA (data))? go_data_get_vector_size (data): 0;
	if (length == 0)
		*min = *max; /* use same values for + and - */
	else if (length == 1)
		*min = go_data_get_vector_value (data, 0);
	else if (length > index)
		*min = go_data_get_vector_value (data, index);

	if (isnan (*min) || !go_finite (*min) || (*min <= 0)) {
		*min = -1.;
	}
	if (isnan (*max) || !go_finite (*max) || (*max <= 0)) {
		*max = -1.;
	}

	switch (bar->type)
	{
	case GOG_ERROR_BAR_TYPE_RELATIVE:
		*min *= fabs (value);
		*max *= fabs (value);
		break;
	case GOG_ERROR_BAR_TYPE_PERCENT:
		*min *= fabs (value) / 100;
		*max *= fabs (value) / 100;
		break;
	default:
		break;
	}
	return TRUE;
}

void
gog_error_bar_get_minmax (const GogErrorBar *bar, double *min, double *max)
{
	double *values;
	int i, imax;
	double tmp_min, tmp_max, plus, minus;

	g_return_if_fail (GOG_IS_ERROR_BAR (bar));

	if (!gog_series_is_valid (bar->series)) {
		*min = DBL_MAX;
		*max = -DBL_MAX;
		return;
	}

	imax = go_data_get_vector_size (bar->series->values[bar->dim_i].data);
	if (imax == 0)
		return;
	go_data_get_bounds (bar->series->values[bar->dim_i].data, min, max);
	values = go_data_get_values (bar->series->values[bar->dim_i].data);

	for (i = 0; i < imax; i++) {
		if  (gog_error_bar_get_bounds (bar, i, &minus, &plus)) {
			tmp_min = values[i] - minus;
			tmp_max = values[i] + plus;
			if (tmp_min < *min)
				*min = tmp_min;
			if (tmp_max > *max)
				*max = tmp_max;
		}
	}
}

/**
 * gog_error_bar_dup:
 * @bar: #GogErrorBar
 *
 * Returns: (transfer full): the duplicated error bar.
 **/
GogErrorBar  *
gog_error_bar_dup		(GogErrorBar const *bar)
{
	GogErrorBar* dbar;

	g_return_val_if_fail (GOG_IS_ERROR_BAR (bar), NULL);

	dbar = g_object_new (GOG_TYPE_ERROR_BAR, NULL);
	dbar->type = bar->type;
	dbar->series = bar->series;
	dbar->dim_i = bar->dim_i;
	dbar->error_i = bar->error_i;
	dbar->display = bar->display;
	dbar->width = bar->width;
	if (dbar->style) g_object_unref (dbar->style);
	dbar->style = go_style_dup (bar->style);
	return dbar;
}

/**
 * gog_error_bar_render:
 * @bar: A GogErrorBar
 * @rend: A GogRenderer
 * @map:  A GogChartMap for the chart
 * @x: x coordinate of the origin of the bar
 * @y: y coordinate of the origin of the bar
 * @plus: distance from the origin to the positive end of the bar
 * @minus: distance from the origin to the negative end of the bar
 * @direction: the #GogErrorBarDirection for the bar.
 *
 * Displays the error bar. If @plus is negative, the positive side of the bar is not displayed,
 * and if @minus is negative, the negative side of the bar is not displayed.
 * x_map and y_map are used to convert coordinates from data space to canvas coordinates.
 * This function must not be called if #gog_error_bar_get_bounds returned FALSE.
 **/
void gog_error_bar_render (const GogErrorBar *bar,
			   GogRenderer *rend,
			   GogChartMap *map,
			   double x, double y,
			   double minus,
			   double plus,
			   GogErrorBarDirection direction)
{
	GOPath *path;
	double x_start, y_start, x_end, y_end;
	double line_width, width;
	gboolean start = plus > .0 && bar->display & GOG_ERROR_BAR_DISPLAY_POSITIVE,
		 end = minus > 0. && bar->display & GOG_ERROR_BAR_DISPLAY_NEGATIVE;
	GogAxisMap *x_map = gog_chart_map_get_axis_map (map, 0),
		   *y_map = gog_chart_map_get_axis_map (map, 1);

	if (!start && !end)
		return;

	switch (direction) {
	case GOG_ERROR_BAR_DIRECTION_HORIZONTAL:
		if (!gog_axis_map_finite (x_map, x) ||
		    !gog_axis_map_finite (y_map, y) ||
		    (start && !gog_axis_map_finite (x_map, x + plus)) ||
		    (end   && !gog_axis_map_finite (x_map, x - minus)))
			return;

		x_start = start ?
			gog_axis_map_to_view (x_map, x + plus) :
			gog_axis_map_to_view (x_map, x);
		x_end =  end ?
			gog_axis_map_to_view (x_map , x - minus) :
			gog_axis_map_to_view (x_map , x);
		y_start = y_end = gog_axis_map_to_view (y_map, y);
		break;
	case GOG_ERROR_BAR_DIRECTION_VERTICAL:
		if (!gog_axis_map_finite (x_map, x) ||
		    !gog_axis_map_finite (y_map, y) ||
		    (start && !gog_axis_map_finite (y_map, y + plus)) ||
		    (end   && !gog_axis_map_finite (y_map, y - minus)))
			return;

		x_start = x_end = gog_axis_map_to_view (x_map ,x);
		y_start = start ?
			gog_axis_map_to_view (y_map, y + plus) :
			gog_axis_map_to_view (y_map, y);
		y_end =  end ?
			gog_axis_map_to_view (y_map, y - minus) :
			gog_axis_map_to_view (y_map, y);
		break;
	case GOG_ERROR_BAR_DIRECTION_RADIAL: {
		GogChartMapPolarData *polar_parms;
		double cx, cy;
		double a, rr, xx, yy, y_min, y_max;
		gboolean cap_min, cap_max;
		/* x = angle, y = radius */
		polar_parms = gog_chart_map_get_polar_parms (map);
		cx = polar_parms->cx;
		cy = polar_parms->cy;
		gog_axis_map_get_bounds (y_map, &y_min, &y_max);
		width = gog_renderer_pt2r (rend, bar->width) / 2.;
		if (start && y + plus > y_max) {
			plus = y_max - y;
			cap_max = FALSE;
		} else
			cap_max = TRUE;
		if (end && y - minus < y_min) {
			minus = y -y_min;
			cap_min = FALSE;
		} else
			cap_min = TRUE;

		gog_chart_map_2D_to_view (map, x, (start ? y + plus : y),
							  &xx, &yy);
		path = go_path_new ();
		if (start && cap_max) {
			rr = hypot (xx - cx, yy - cy);
			a = atan2 (yy - cy, xx - cx);
			go_path_arc (path, cx, cy, rr, rr, a - width * M_PI / 180., a + width * M_PI / 180.);
		}
		go_path_move_to (path, xx, yy);
		gog_chart_map_2D_to_view (map, x, (end ? y - minus : y),
							  &xx, &yy);
		go_path_line_to (path, xx, yy);
		if (end && cap_min) {
			rr = hypot (xx - cx, yy - cy);
			a = atan2 (yy - cy, xx - cx);
			go_path_arc (path, cx, cy, rr, rr, a - width * M_PI / 180., a + width * M_PI / 180.);
		}
		gog_renderer_push_style (rend, bar->style);
		gog_renderer_stroke_serie (rend, path);
		gog_renderer_pop_style (rend);
		go_path_free (path);
		return;
	}
	case GOG_ERROR_BAR_DIRECTION_ANGULAR: {
		GogChartMapPolarData *polar_parms;
		double cx, cy, a0, x0, y0, a1, x1, y1, rr, y_min, y_max;
		polar_parms = gog_chart_map_get_polar_parms (map);
		cx = polar_parms->cx;
		cy = polar_parms->cy;
		gog_axis_map_get_bounds (y_map, &y_min, &y_max);
		if (y < y_min || y > y_max)
			return;
		width = gog_renderer_pt2r (rend, bar->width) / 2.;
		line_width = gog_renderer_pt2r (rend, bar->style->line.width);
		gog_chart_map_2D_to_view (map, (start ? x + plus : x), y,
							  &x0, &y0);
		rr = hypot (x0 - cx, y0 - cy);
		if (rr == 0.)
			return;
		a0 = atan2 (y0 - cy, x0 - cx);
		path = go_path_new ();
		gog_chart_map_2D_to_view (map, (end ? x - minus : x), y,
							  &x1, &y1);
		a1 = atan2 (y1 - cy, x1 - cx);
		go_path_arc (path, cx, cy, rr, rr, a1, a0);
		if ((2. * width) > line_width) {
			double dx, dy;
			dx = width * cos (a0);
			dy = width * sin (a0);
			go_path_move_to (path, x0 - dx, y0 - dy);
			go_path_line_to (path, x0 + dx, y0 + dy);
			dx = width * cos (a1);
			dy = width * sin (a1);
			go_path_move_to (path, x1 - dx, y1 - dy);
			go_path_line_to (path, x1 + dx, y1 + dy);
		}
		gog_renderer_push_style (rend, bar->style);
		gog_renderer_stroke_serie (rend, path);
		gog_renderer_pop_style (rend);
		go_path_free (path);
		return;
	}
	default:
		return; /* should not occur */
	}
	x = gog_axis_map_to_view (x_map, x);
	y = gog_axis_map_to_view (y_map, y);

	path = go_path_new ();
	go_path_move_to (path, x_start, y_start);
	go_path_line_to (path, x_end, y_end);

	if (direction == GOG_ERROR_BAR_DIRECTION_HORIZONTAL) {
		width = gog_renderer_pt2r_y (rend, bar->width) / 2.;
		line_width = gog_renderer_pt2r_x (rend, bar->style->line.width);
	} else {
		width = gog_renderer_pt2r_x (rend, bar->width) / 2.;
		line_width = gog_renderer_pt2r_y (rend, bar->style->line.width);
	}

	if ((2. * width) > line_width) {
		if (direction == GOG_ERROR_BAR_DIRECTION_HORIZONTAL) {
			if (start) {
				go_path_move_to (path, x_start, y - width);
				go_path_line_to (path, x_start, y + width);
			}
			if (end) {
				go_path_move_to (path, x_end, y - width);
				go_path_line_to (path, x_end, y + width);
			}
		} else {
			if (start) {
				go_path_move_to (path, x - width, y_start);
				go_path_line_to (path, x + width, y_start);
			}
			if (end) {
				go_path_move_to (path, x - width, y_end);
				go_path_line_to (path, x + width, y_end);
			}
		}
	}

	gog_renderer_push_style (rend, bar->style);
	gog_renderer_stroke_serie (rend, path);
	gog_renderer_pop_style (rend);
	go_path_free (path);
}

gboolean
gog_error_bar_is_visible (GogErrorBar *bar)
{
	return (bar != NULL) &&
		(bar->type != GOG_ERROR_BAR_TYPE_NONE) &&
		(bar->display != GOG_ERROR_BAR_DISPLAY_NONE);
}
