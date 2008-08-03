/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-style.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2006-2007 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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
#include <goffice/app/go-doc.h>
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-styled-object.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-font.h>
#include <goffice/utils/go-file.h>
#include <goffice/utils/go-image.h>
#include <goffice/utils/go-line.h>
#include <goffice/utils/go-marker.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/gtk/go-pixbuf.h>
#include <goffice/gtk/go-font-sel.h>
#include <goffice/gtk/go-palette.h>
#include <goffice/gtk/go-rotation-sel.h>
#include <goffice/gtk/go-color-selector.h>
#include <goffice/gtk/go-gradient-selector.h>
#include <goffice/gtk/go-line-selector.h>
#include <goffice/gtk/go-marker-selector.h>
#include <goffice/gtk/go-pattern-selector.h>
#include <goffice/gtk/go-image-sel.h>

#include <glade/glade-xml.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktable.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtksizegroup.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#endif

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <math.h>

#define HSCALE 100
#define VSCALE 120

typedef GObjectClass GogStyleClass;

static GObjectClass *parent_klass;

/*
 * I would have liked to do this differently and have a tighter binding between theme element and style
 * 	eg gog_style_new (theme_element)
 * However that will not work easily in the context of xls import where we do
 * not know what the type is destined for until later.  This structure melds
 * smoothly with both approaches at the expense of a bit of power.
 */

/*************************************************************************/

#ifdef GOFFICE_WITH_GTK
typedef struct {
	GladeXML  	*gui;
	GladeXML  	*font_gui;
	GogStyle  	*style;
	GogStyle  	*default_style;
	GObject		*object_with_style;
	gboolean   	 enable_edit;
	gulong     	 style_changed_handler;
	struct {
		GtkSizeGroup *size_group;
		GtkWidget *background;
		GtkWidget *background_box;
		GtkWidget *background_label;
		GtkWidget *foreground;
		GtkWidget *foreground_box;
		GtkWidget *foreground_label;
		GtkWidget *notebook;
		GtkWidget *extension_box;
		struct {
			GtkWidget *selector;
			GtkWidget *box;
		} pattern;
		struct {
			GtkWidget *selector;
			GtkWidget *box;
			GtkWidget *brightness;
			GtkWidget *brightness_box;
		} gradient;
		struct {
			GOImage *image;
		} image;
	} fill;
	struct {
		GtkWidget *selector;
	} marker;
	GODoc *doc;
} StylePrefState;

static void
cb_style_changed (GogStyledObject *obj, GogStyle *style, StylePrefState *state)
{
}

static void
set_style (StylePrefState const *state)
{
	if (state->object_with_style != NULL) {
		if (state->style_changed_handler)
			g_signal_handler_block (state->object_with_style, state->style_changed_handler);
		g_object_set (G_OBJECT (state->object_with_style), "style", state->style, NULL);
		if (state->style_changed_handler)
			g_signal_handler_unblock (state->object_with_style, state->style_changed_handler);
	}
}

static GtkWidget *
create_go_combo_color (StylePrefState *state,
		       GOColor initial_color, GOColor automatic_color,
		       GladeXML *gui,
		       char const *group, char const *label_name,
		       GCallback func)
{
	GtkWidget *w;

	w = go_color_selector_new (initial_color, automatic_color, group);
	gtk_label_set_mnemonic_widget (
		GTK_LABEL (glade_xml_get_widget (gui, label_name)), w);
	g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (func), state);
	return w;
}

static void
gog_style_set_image_preview (GOImage *pix, StylePrefState *state)
{
	GdkPixbuf *scaled;
	int width, height;
	char *size;
	GtkWidget *w;

	if (state->fill.image.image != pix) {
		if (state->fill.image.image != NULL)
			g_object_unref (state->fill.image.image);
		state->fill.image.image = pix;
		if (state->fill.image.image != NULL)
			g_object_ref (state->fill.image.image);
	}

	if (pix == NULL)
		return;

	w = glade_xml_get_widget (state->gui, "fill_image_sample");

	scaled = go_pixbuf_intelligent_scale (go_image_get_pixbuf (pix), HSCALE, VSCALE);
	gtk_image_set_from_pixbuf (GTK_IMAGE (w), scaled);
	g_object_unref (scaled);

	w = glade_xml_get_widget (state->gui, "image-size-label");
	g_object_get (pix, "width", &width, "height", &height, NULL);

	size = g_strdup_printf (_("%d x %d"), width, height);
	gtk_label_set_text (GTK_LABEL (w), size);
	g_free (size);
}

/************************************************************************/

static void
cb_outline_dash_type_changed (GOSelector *selector, StylePrefState const *state)
{
	GogStyleLine *line = &state->style->outline;

	line->dash_type = go_selector_get_active (selector, &line->auto_dash);
	set_style (state);
}

static void
cb_outline_size_changed (GtkAdjustment *adj, StylePrefState *state)
{
	GogStyle *style = state->style;

	g_return_if_fail (style != NULL);

	style->outline.width = go_rint (adj->value * 100.) / 100.;
	set_style (state);
}

static void
cb_outline_color_changed (GOSelector *selector,
			  StylePrefState *state)
{
	GogStyle *style = state->style;

	g_return_if_fail (style != NULL);

	style->outline.color = go_color_selector_get_color (selector, 
							    &style->outline.auto_color);
	set_style (state);
}

static void
outline_init (StylePrefState *state, gboolean enable, GogEditor *editor)
{
	GogStyle *style = state->style;
	GogStyle *default_style = state->default_style;
	GtkWidget *w, *table;

	w = glade_xml_get_widget (state->gui, "outline_box");
	if (!enable) {
		gtk_widget_hide (w);
		return;
	}

	gog_editor_register_widget (editor, w);

	table = glade_xml_get_widget (state->gui, "outline_table");

	/* DashType */
	w = go_line_dash_selector_new (style->outline.dash_type,
				       default_style->outline.dash_type);
	gtk_table_attach (GTK_TABLE (table), w, 1, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	g_signal_connect (G_OBJECT (w), "activate", 
			  G_CALLBACK (cb_outline_dash_type_changed), state);
	/* Size */
	w = glade_xml_get_widget (state->gui, "outline_size_spin");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), style->outline.width);
	g_signal_connect (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w)),
		"value_changed",
		G_CALLBACK (cb_outline_size_changed), state);
	/* Color */
	w = create_go_combo_color (state,
		style->outline.color, default_style->outline.color,
		state->gui,
		"outline_color", "outline_color_label",
		G_CALLBACK (cb_outline_color_changed));
	gtk_table_attach (GTK_TABLE (table), w, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show_all (table);
}


/************************************************************************/

static void
cb_line_dash_type_changed (GOSelector *palette, StylePrefState const *state)
{
	GogStyleLine *line = &state->style->line;

	line->dash_type = go_selector_get_active (palette, &line->auto_dash);
	set_style (state);
}

static void
cb_line_size_changed (GtkAdjustment *adj, StylePrefState const *state)
{
	GogStyle *style = state->style;

	g_return_if_fail (style != NULL);

	style->line.width = go_rint (adj->value * 100.) / 100.;
	set_style (state);
}

static void
cb_line_color_changed (GOSelector *selector,
		       StylePrefState *state)
{
	GogStyle *style = state->style;

	g_return_if_fail (style != NULL);

	style->line.color = go_color_selector_get_color (selector, &style->line.auto_color);
	set_style (state);
}

static void
line_init (StylePrefState *state, gboolean enable, GogEditor *editor)
{
	GogStyle *style = state->style;
	GogStyle *default_style = state->default_style;
	GtkWidget *w, *table;

	w = glade_xml_get_widget (state->gui, "line_box");
	if (!enable) {
		gtk_widget_hide (w);
		return;
	}

	gog_editor_register_widget (editor, w);

	table = glade_xml_get_widget (state->gui, "line_table");

	/* DashType */
	w = go_line_dash_selector_new (style->line.dash_type,
				       default_style->line.dash_type);
	gtk_table_attach (GTK_TABLE (table), w, 1, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	g_signal_connect (G_OBJECT (w), "activate",
			  G_CALLBACK (cb_line_dash_type_changed), state);

	/* Size */
	w = glade_xml_get_widget (state->gui, "line_size_spin");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), style->line.width);
	g_signal_connect (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w)),
		"value_changed",
		G_CALLBACK (cb_line_size_changed), state);

	/* Colour */
	w = create_go_combo_color (state,
		style->line.color, default_style->line.color,
		state->gui,
		"line_color", "line_color_label",
		G_CALLBACK (cb_line_color_changed));
	gtk_table_attach (GTK_TABLE (table), w, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show_all (table);
}

static void cb_fill_background_color (GOSelector *selector, StylePrefState *state);
static void cb_fill_foreground_color (GOSelector *selector, StylePrefState *state);

static void
fill_update_selectors (StylePrefState const *state)
{
	GogStyle *style = state->style;

	go_pattern_selector_set_colors (GO_SELECTOR (state->fill.pattern.selector),
				        style->fill.pattern.fore,
					style->fill.pattern.back);
	go_gradient_selector_set_colors (GO_SELECTOR (state->fill.gradient.selector), 
					 style->fill.pattern.back,
					 style->fill.pattern.fore);

	g_signal_handlers_block_by_func (state->fill.background, cb_fill_background_color, 
					 (gpointer) state);
	g_signal_handlers_block_by_func (state->fill.foreground, cb_fill_background_color, 
					 (gpointer) state);
	
	go_color_selector_set_color (GO_SELECTOR (state->fill.background),
				     style->fill.pattern.back);
	go_color_selector_set_color (GO_SELECTOR (state->fill.foreground),
				     style->fill.pattern.fore);
	
	g_signal_handlers_unblock_by_func (state->fill.background, cb_fill_background_color, 
					   (gpointer) state);
	g_signal_handlers_unblock_by_func (state->fill.foreground, cb_fill_background_color, 
					   (gpointer) state);
}

/************************************************************************/

static void
cb_pattern_type_activate (GtkWidget *selector, StylePrefState const *state)
{
	GogStyle *style = state->style;
	gboolean is_auto;

	style->fill.pattern.pattern = go_selector_get_active (GO_SELECTOR (selector), &is_auto);
	set_style (state);
}

static void
fill_pattern_init (StylePrefState *state)
{
	GogStyle *style = state->style;
	GogStyle *default_style = state->default_style;
	GtkWidget *selector;
	GtkWidget *label;
       
	state->fill.pattern.selector = selector = 
		go_pattern_selector_new (style->fill.pattern.pattern,
					 default_style->fill.pattern.pattern);
	go_pattern_selector_set_colors (GO_SELECTOR (selector), style->fill.pattern.fore,
					style->fill.pattern.back);

	label = glade_xml_get_widget (state->gui, "fill_pattern_label");
	gtk_size_group_add_widget (state->fill.size_group, label);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), selector);
	state->fill.pattern.box = glade_xml_get_widget (state->gui, "fill_pattern_box");
	gtk_box_pack_start (GTK_BOX (state->fill.pattern.box), selector, FALSE, FALSE, 0);

	g_signal_connect (G_OBJECT (selector), "activate",
			  G_CALLBACK (cb_pattern_type_activate), state);
	gtk_widget_show (selector);
}

/************************************************************************/

static void
cb_brightness_changed (GtkRange *range, StylePrefState *state)
{
	GogStyle *style = state->style;

	gog_style_set_fill_brightness (style, gtk_range_get_value (range));
	set_style (state);
	fill_update_selectors (state);
}

static void
cb_gradient_type_changed (GOSelector *selector, StylePrefState const *state)
{
	GogStyle *style = state->style;
	style->fill.gradient.dir = go_selector_get_active (selector, NULL);
	set_style (state);
}

static void
fill_gradient_init (StylePrefState *state)
{
	GogStyle *style = state->style;
	GtkWidget *selector;
	GtkWidget *brightness;
	GtkWidget *label;

	state->fill.gradient.selector = selector = 
		go_gradient_selector_new (style->fill.gradient.dir,
					  style->fill.gradient.dir);
	go_gradient_selector_set_colors (GO_SELECTOR (selector), 
					 style->fill.pattern.back,
					 style->fill.pattern.fore);
	label = glade_xml_get_widget (state->gui, "fill_gradient_label");
	gtk_size_group_add_widget (state->fill.size_group, label);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), selector);
	state->fill.gradient.box = glade_xml_get_widget (state->gui, "fill_gradient_box");
	gtk_box_pack_start (GTK_BOX (state->fill.gradient.box), selector, FALSE, FALSE, 0);

	state->fill.gradient.brightness = brightness =
		glade_xml_get_widget (state->gui, "fill_brightness_scale");
	label = glade_xml_get_widget (state->gui, "fill_brightness_label");
	gtk_size_group_add_widget (state->fill.size_group, label);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), brightness);
	state->fill.gradient.brightness_box = glade_xml_get_widget (state->gui, "fill_brightness_box");
	
	g_signal_connect (selector, "activate",
			  G_CALLBACK (cb_gradient_type_changed), state);
	g_signal_connect (brightness, "value_changed", 
			   G_CALLBACK (cb_brightness_changed), state);
	gtk_widget_show (selector);
	gtk_widget_show (brightness);
}

/************************************************************************/

static void
cb_fill_background_color (GOSelector *selector, StylePrefState *state)
{
	GogStyle *style = state->style;

	style->fill.pattern.back = go_color_selector_get_color (selector, 
								&style->fill.auto_back);
	set_style (state);
	fill_update_selectors (state);
}

static void
cb_fill_foreground_color (GOSelector *selector, StylePrefState *state)
{
	GogStyle *style = state->style;

	style->fill.pattern.fore = go_color_selector_get_color (selector,
								&style->fill.auto_fore);
	style->fill.gradient.brightness = -1;
	set_style (state);
	fill_update_selectors (state);
}

static void
fill_color_init (StylePrefState *state)
{
	GogStyle *style = state->style;
	GogStyle *default_style = state->default_style;
	GtkWidget *w;

	state->fill.background = w = create_go_combo_color (state,
		style->fill.pattern.back,
		default_style->fill.pattern.back,
		state->gui, "pattern_background", "fill_background_label",
		G_CALLBACK (cb_fill_background_color));
	state->fill.background_box = glade_xml_get_widget (state->gui, "fill_background_box");
	gtk_box_pack_start (GTK_BOX (state->fill.background_box), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	state->fill.foreground = w = create_go_combo_color (state,
		style->fill.pattern.fore,
		default_style->fill.pattern.fore,
		state->gui, "pattern_foreground", "fill_foreground_label",
		G_CALLBACK (cb_fill_foreground_color));
	state->fill.foreground_box = glade_xml_get_widget (state->gui, "fill_foreground_box");
	gtk_box_pack_start (GTK_BOX (state->fill.foreground_box), w, FALSE, FALSE, 0);
	gtk_widget_show (w);

	state->fill.foreground_label = glade_xml_get_widget (state->gui, "fill_foreground_label");
	gtk_size_group_add_widget (state->fill.size_group, state->fill.foreground_label);
	state->fill.background_label = glade_xml_get_widget (state->gui, "fill_background_label");
	gtk_size_group_add_widget (state->fill.size_group, state->fill.background_label);
}
/************************************************************************/

static void
cb_image_select (GtkWidget *cc, StylePrefState *state)
{
	GogStyle *style = state->style;
	GtkWidget *w;

	g_return_if_fail (style != NULL);
	g_return_if_fail (GOG_FILL_STYLE_IMAGE == style->fill.type);

	w = go_image_sel_new (state->doc, NULL, &style->fill.image.image);
	gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (gtk_widget_get_toplevel (cc)));
	gtk_dialog_run (GTK_DIALOG (w));

	gog_style_set_image_preview (style->fill.image.image, state);
	set_style (state);
}

static void
cb_image_style_changed (GtkWidget *w, StylePrefState *state)
{
	GogStyle *style = state->style;
	g_return_if_fail (style != NULL);
	g_return_if_fail (GOG_FILL_STYLE_IMAGE == style->fill.type);
	style->fill.image.type = gtk_combo_box_get_active (GTK_COMBO_BOX (w));
	set_style (state);
}

static void
fill_image_init (StylePrefState *state)
{
	GtkWidget *w, *sample, *type;
	GogStyle *style = state->style;

	w = glade_xml_get_widget (state->gui, "fill_image_select_picture");
	g_signal_connect (G_OBJECT (w),
		"clicked",
		G_CALLBACK (cb_image_select), state);

	sample = glade_xml_get_widget (state->gui, "fill_image_sample");
	gtk_widget_set_size_request (sample, HSCALE + 10, VSCALE + 10);
	type   = glade_xml_get_widget (state->gui, "fill_image_fit");

	state->fill.image.image = NULL;

	if (GOG_FILL_STYLE_IMAGE == style->fill.type) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (type),
			style->fill.image.type);
		gog_style_set_image_preview (style->fill.image.image, state);
		state->fill.image.image = style->fill.image.image;
		if (state->fill.image.image)
			g_object_ref (state->fill.image.image);
	} else
		gtk_combo_box_set_active (GTK_COMBO_BOX (type), 0);
	g_signal_connect (G_OBJECT (type),
		"changed",
		G_CALLBACK (cb_image_style_changed), state);
}

/************************************************************************/

typedef enum {
	FILL_TYPE_NONE,
	FILL_TYPE_PATTERN,
	FILL_TYPE_GRADIENT_BICOLOR,
	FILL_TYPE_GRADIENT_UNICOLOR,
	FILL_TYPE_IMAGE
} FillType;

static struct {
	GogFillStyle 	type;
	int		page;
	gboolean	show_pattern;
	gboolean	show_gradient;
	gboolean	show_brightness;
} fill_infos[] = {
	{GOG_FILL_STYLE_NONE,		0, FALSE, FALSE, FALSE},
	{GOG_FILL_STYLE_PATTERN,	1, TRUE,  FALSE, FALSE},
	{GOG_FILL_STYLE_GRADIENT,	1, FALSE, TRUE,  FALSE},
	{GOG_FILL_STYLE_GRADIENT,	1, FALSE, TRUE,  TRUE},
	{GOG_FILL_STYLE_IMAGE,		2, FALSE, FALSE, FALSE}
};

static void
fill_update_visibilies (FillType type, StylePrefState *state)
{
	g_object_set (state->fill.pattern.box, "visible" , fill_infos[type].show_pattern, NULL);
	g_object_set (state->fill.gradient.box, "visible", fill_infos[type].show_gradient, NULL);
	g_object_set (state->fill.gradient.brightness_box, "visible",
		      fill_infos[type].show_brightness, NULL);
	g_object_set (state->fill.foreground_box, "visible",
		      !fill_infos[type].show_brightness, NULL);
	g_object_set (state->fill.extension_box, "visible", type != FILL_TYPE_NONE, NULL);

	if (fill_infos[type].show_gradient) {
		gtk_label_set_text (GTK_LABEL (state->fill.foreground_label), _("Start:"));
		gtk_label_set_text (GTK_LABEL (state->fill.background_label), _("End:"));
	} else {
		gtk_label_set_text (GTK_LABEL (state->fill.foreground_label), _("Foreground:"));
		gtk_label_set_text (GTK_LABEL (state->fill.background_label), _("Background:"));
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (state->fill.notebook), fill_infos[type].page);
}

static void
cb_fill_type_changed (GtkWidget *menu, StylePrefState *state)
{
	int index;

	index = CLAMP (gtk_combo_box_get_active (GTK_COMBO_BOX (menu)), 0, (int) G_N_ELEMENTS (fill_infos) - 1);

	state->style->fill.type = fill_infos[index].type;
	state->style->fill.auto_type = FALSE;
	set_style (state);

	fill_update_visibilies (index, state);
}

static void
fill_init (StylePrefState *state, gboolean enable, GogEditor *editor)
{
	GtkWidget *w;
	FillType type;

	if (!enable) {
		gtk_widget_hide (glade_xml_get_widget (state->gui, "fill_box"));
		return;
	}

	state->fill.extension_box = glade_xml_get_widget (state->gui, "fill_extension_box");
	gog_editor_register_widget (editor, state->fill.extension_box);

	state->fill.size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	fill_color_init (state);
	fill_pattern_init (state);
	fill_gradient_init (state);
	if (state->doc != NULL)
		fill_image_init (state);
	fill_update_selectors (state);

	state->fill.notebook = glade_xml_get_widget (state->gui, "fill_notebook");

	switch (state->style->fill.type) {
		case GOG_FILL_STYLE_PATTERN:
			type = FILL_TYPE_PATTERN;
			break;
		case GOG_FILL_STYLE_GRADIENT:
			if (state->style->fill.gradient.brightness >= 0)
				type = FILL_TYPE_GRADIENT_UNICOLOR;
			else
				type = FILL_TYPE_GRADIENT_BICOLOR;
			break;
		case GOG_FILL_STYLE_IMAGE:
			if (state->doc != NULL) {
				type = FILL_TYPE_IMAGE;
				break;
			} else
				state->style->fill.type = GOG_FILL_STYLE_NONE;
		case GOG_FILL_STYLE_NONE:
		default:
			type = FILL_TYPE_NONE;
			break;
	}
	fill_update_visibilies (type, state);

	w = glade_xml_get_widget (state->gui, "fill_type_menu");
	if (state->doc == NULL)
		gtk_combo_box_remove_text (GTK_COMBO_BOX (w), FILL_TYPE_IMAGE);
	gtk_combo_box_set_active (GTK_COMBO_BOX (w), type);
	g_signal_connect (G_OBJECT (w),
		"changed",
		G_CALLBACK (cb_fill_type_changed), state);

	w = glade_xml_get_widget (state->gui, "fill_box");
	gtk_widget_show (GTK_WIDGET (w));
}

/************************************************************************/

static void
cb_marker_shape_changed (GOSelector *selector, StylePrefState const *state)
{
	GogStyle *style = state->style;
	GOMarkerShape shape;
	gboolean is_auto;

	shape = go_selector_get_active (selector, &is_auto);
	go_marker_set_shape (style->marker.mark, shape);
	style->marker.auto_shape = is_auto;
	set_style (state);
}

static void
cb_marker_outline_color_changed (GOSelector *selector,
				 StylePrefState *state)
{
	GogStyle *style = state->style;
	GOColor color;
	gboolean is_auto;

	color = go_color_selector_get_color (selector, &is_auto);
	go_marker_set_outline_color (style->marker.mark, color);
	style->marker.auto_outline_color = is_auto;
	set_style (state);
	
	go_marker_selector_set_colors (GO_SELECTOR (state->marker.selector), 
				       color,
				       go_marker_get_fill_color (style->marker.mark));
}

static void
cb_marker_fill_color_changed (GOSelector *selector,
			      StylePrefState *state)
{
	GogStyle *style = state->style;
	GOColor color;
	gboolean is_auto;

	color = go_color_selector_get_color (selector, &is_auto);
	go_marker_set_fill_color (style->marker.mark, color);
	style->marker.auto_fill_color = is_auto;
	set_style (state);

	go_marker_selector_set_colors (GO_SELECTOR (state->marker.selector), 
				       go_marker_get_outline_color (style->marker.mark),
				       color);
}

static void
cb_marker_size_changed (GtkAdjustment *adj, StylePrefState *state)
{
	go_marker_set_size (state->style->marker.mark, adj->value);
	set_style (state);
}

static void gog_style_pref_state_free (StylePrefState *state);

static void
marker_init (StylePrefState *state, gboolean enable, GogEditor *editor, GOCmdContext *cc)
{
	GogStyle *style = state->style;
	GogStyle *default_style = state->default_style;
	GtkWidget *table, *w;
	GtkWidget *selector;
	GladeXML *gui;

	if (!enable)
		return;

	gui = go_libglade_new ("gog-style-prefs.glade", "gog_style_marker_prefs", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	table = glade_xml_get_widget (gui, "marker_table");

	state->marker.selector = selector = 
		go_marker_selector_new (go_marker_get_shape (style->marker.mark),
					go_marker_get_shape (state->default_style->marker.mark));
	if ((style->interesting_fields & GOG_STYLE_MARKER_NO_COLOR )== 0)
		go_marker_selector_set_colors (GO_SELECTOR (selector), 
					       go_marker_get_outline_color (style->marker.mark),
					       go_marker_get_fill_color (style->marker.mark));
	else
		go_marker_selector_set_colors (GO_SELECTOR (selector), 
					       RGBA_BLUE, RGBA_BLUE);
	w = glade_xml_get_widget (gui, "marker_shape_label");
	gtk_label_set_mnemonic_widget (GTK_LABEL (w), selector);
	gtk_table_attach (GTK_TABLE (table), selector, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	g_signal_connect (G_OBJECT (selector), "activate", 
			  G_CALLBACK (cb_marker_shape_changed), state);
	gtk_widget_show (selector);

	if ((style->interesting_fields & GOG_STYLE_MARKER_NO_COLOR ) == 0)
		w = create_go_combo_color (state,
			go_marker_get_fill_color (style->marker.mark),
			go_marker_get_fill_color (default_style->marker.mark),
			gui, "pattern_background", "marker_fill_label",
			G_CALLBACK (cb_marker_fill_color_changed));
	else {
		w = create_go_combo_color (state,
			RGBA_BLUE, RGBA_BLUE,
			gui, "pattern_background", "marker_fill_label",
			G_CALLBACK (cb_marker_fill_color_changed));
		gtk_widget_set_sensitive (w, FALSE);
	}
	gtk_table_attach (GTK_TABLE (table), w, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	
	if ((style->interesting_fields & GOG_STYLE_MARKER_NO_COLOR ) == 0)
		w = create_go_combo_color (state,
			go_marker_get_outline_color (style->marker.mark),
			go_marker_get_outline_color (default_style->marker.mark),
			gui, "pattern_foreground", "marker_outline_label",
			G_CALLBACK (cb_marker_outline_color_changed));
	else {
		w = create_go_combo_color (state,
			RGBA_BLUE, RGBA_BLUE,
			gui, "pattern_background", "marker_fill_label",
			G_CALLBACK (cb_marker_outline_color_changed));
		gtk_widget_set_sensitive (w, FALSE);
	}
	gtk_table_attach (GTK_TABLE (table), w, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

	w = glade_xml_get_widget (gui, "marker_size_spin");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
		go_marker_get_size (style->marker.mark));
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_marker_size_changed), state);

	gtk_widget_show_all (table);

	w = glade_xml_get_widget (gui, "gog_style_marker_prefs");

	gog_editor_add_page (editor, w, _("Markers"));
	g_object_unref (gui);
	if (state->gui == NULL)
		g_object_set_data_full (G_OBJECT (w),
			"state", state, (GDestroyNotify) gog_style_pref_state_free);
}

/************************************************************************/

static void
cb_font_changed (GOFontSel *fs, PangoAttrList *list,
		 StylePrefState *state)
{
	PangoAttrIterator *iter = pango_attr_list_get_iterator (list);
	PangoFontDescription *desc = pango_font_description_new ();
	GSList *extra_attrs;
	const GOFont *font;
	pango_attr_iterator_get_font (iter, desc, NULL, &extra_attrs);
	font = go_font_new_by_desc (desc);
/* FIXME FIXME FIXME we should do something for extra attributes */
	g_slist_foreach (extra_attrs, (GFunc)pango_attribute_destroy, NULL);
	g_slist_free (extra_attrs);
	pango_attr_iterator_destroy (iter);
	gog_style_set_font (state->style, font);
	set_style (state);
}

static void
cb_font_color_changed (GOSelector *selector,
		       StylePrefState *state)
{
	GogStyle *style = state->style;
	
	style->font.color = go_color_selector_get_color (selector, NULL);
	set_style (state);
}

static void
font_init (StylePrefState *state, guint32 enable, GogEditor *editor, GOCmdContext *cc)
{
	GogStyle *style = state->style;
	GtkWidget *w, *box;
	GladeXML *gui;

	if (!enable)
		return;

	g_return_if_fail (style->font.font != NULL);

	gui = go_libglade_new ("gog-style-prefs.glade", "gog_style_font_prefs", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	state->font_gui = gui;

	w = create_go_combo_color (state, style->font.color, style->font.color,
				   gui, "pattern_foreground", "font_color_label",
				   G_CALLBACK (cb_font_color_changed));
	box = glade_xml_get_widget (gui, "color_box");
	gtk_box_pack_start (GTK_BOX (box), w, TRUE, TRUE, 0);
	gtk_widget_show (w);
				    
	w = go_font_sel_new ();
	go_font_sel_set_font (GO_FONT_SEL (w), style->font.font);
	g_signal_connect (G_OBJECT (w), "font_changed", 
			  G_CALLBACK (cb_font_changed), state);
	gtk_widget_show (w);

 	box = glade_xml_get_widget (gui, "gog_style_font_prefs");
	gtk_box_pack_end (GTK_BOX (box), w, TRUE, TRUE, 0);
	
	gog_editor_add_page (editor, box, _("Font"));
}

/************************************************************************/

static void
cb_angle_changed (GORotationSel *grs, int angle, StylePrefState *state)
{
	gog_style_set_text_angle (state->style, angle);
	set_style (state);
}

static void
text_layout_init (StylePrefState *state, guint32 enable, GogEditor *editor, GOCmdContext *cc)
{
	GogStyle *style = state->style;
	GtkWidget *w, *box;

	if (!enable)
		return;

	w = go_rotation_sel_new ();
	go_rotation_sel_set_rotation (GO_ROTATION_SEL (w),
		style->text_layout.angle);
	g_signal_connect (G_OBJECT (w), "rotation-changed",
		G_CALLBACK (cb_angle_changed), state);
	box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start_defaults (GTK_BOX (box), w);
	gtk_container_set_border_width (GTK_CONTAINER (box), 12);
	gog_editor_add_page (editor, box, _("Text"));
}

/************************************************************************/

static void
cb_parent_is_gone (StylePrefState *state, GObject *where_the_object_was)
{
	state->style_changed_handler = 0;
	state->object_with_style = NULL;
}

static void
gog_style_pref_state_free (StylePrefState *state)
{
	if (state->style_changed_handler) {
		g_signal_handler_disconnect (state->object_with_style,
			state->style_changed_handler);
		g_object_weak_unref (G_OBJECT (state->object_with_style),
			(GWeakNotify) cb_parent_is_gone, state);
	}
	g_object_unref (state->style);
	g_object_unref (state->default_style);
	if (state->gui)
		g_object_unref (state->gui);
	if (state->doc)
		g_object_unref (state->doc);
	if (state->font_gui != NULL)
		g_object_unref (state->font_gui);
	if (state->fill.image.image != NULL)
		g_object_unref (state->fill.image.image);
	g_free (state);
}

void
gog_style_populate_editor (GogStyle *style,
			   GogEditor *editor,
			   GogStyle *default_style,
			   GOCmdContext *cc,
			   GObject	*object_with_style,
			   gboolean   watch_for_external_change)
{
	GogStyleFlag enable;
	GtkWidget *w;
	GladeXML *gui;
	StylePrefState *state;
	GogGraph *graph;

	g_return_if_fail (style != NULL);
	g_return_if_fail (default_style != NULL);

	enable = style->interesting_fields;

	g_object_ref (style);
	g_object_ref (default_style);

	state = g_new0 (StylePrefState, 1);
	state->gui = NULL;
	state->font_gui = NULL;
	state->style = style;
	state->default_style = default_style;
	state->object_with_style = object_with_style;
	state->enable_edit = FALSE;

	graph = gog_object_get_graph (GOG_OBJECT (object_with_style));
	g_object_get (graph, "document", &state->doc, NULL);

	if ((enable & (GOG_STYLE_OUTLINE | GOG_STYLE_LINE | GOG_STYLE_FILL)) != 0) {
		gui = go_libglade_new ("gog-style-prefs.glade", "gog_style_prefs", GETTEXT_PACKAGE, cc);
		if (gui == NULL) {
			g_free (state);
			return;
		}
		state->gui = gui;
		w = glade_xml_get_widget (gui, "gog_style_prefs");
		g_object_set_data_full (G_OBJECT (w),
			"state", state, (GDestroyNotify) gog_style_pref_state_free);
		gog_editor_add_page (editor, w, _("Style"));

		outline_init 	 (state, enable & GOG_STYLE_OUTLINE, editor);
		line_init    	 (state, enable & GOG_STYLE_LINE, editor);
		fill_init    	 (state, enable & GOG_STYLE_FILL, editor);
	}
	marker_init  	 (state, enable & GOG_STYLE_MARKER, editor, cc);
	font_init    	 (state, enable & GOG_STYLE_FONT, editor, cc);
	text_layout_init (state, enable & GOG_STYLE_TEXT_LAYOUT, editor, cc);

	state->enable_edit = TRUE;

	if (object_with_style != NULL && watch_for_external_change) {
		state->style_changed_handler = g_signal_connect (G_OBJECT (object_with_style),
			"style-changed",
			G_CALLBACK (cb_style_changed), state);
		g_object_weak_ref (G_OBJECT (object_with_style),
			(GWeakNotify) cb_parent_is_gone, state);
	}
}

gpointer
gog_style_get_editor (GogStyle *style,
		  GogStyle *default_style,
		  GOCmdContext *cc,
		  GObject *object_with_style)
{
	GtkWidget *notebook;
	GogEditor *editor = gog_editor_new ();

	gog_style_populate_editor (style, editor, default_style, cc, 
				   object_with_style, FALSE);

	notebook = gog_editor_get_notebook (editor);
	gog_editor_free (editor);
	gtk_widget_show (notebook);
	return notebook;
}
#endif

/*****************************************************************************/

GogStyle *
gog_style_new (void)
{
	return g_object_new (GOG_STYLE_TYPE, NULL);
}

/**
 * gog_style_dup:
 * @style : a source #GogStyle
 *
 * Duplicates @style.
 *
 * return value: a new #GogStyle
 **/
GogStyle *
gog_style_dup (GogStyle const *src)
{
	GogStyle *dst;

	g_return_val_if_fail (IS_GOG_STYLE (src), NULL);

	dst = gog_style_new ();
	gog_style_assign (dst, src);
	return dst;
}

void
gog_style_assign (GogStyle *dst, GogStyle const *src)
{
	if (src == dst)
		return;

	g_return_if_fail (IS_GOG_STYLE (src));
	g_return_if_fail (IS_GOG_STYLE (dst));

	if (GOG_FILL_STYLE_IMAGE == src->fill.type &&
	    src->fill.image.image != NULL)
		g_object_ref (src->fill.image.image);
	if (GOG_FILL_STYLE_IMAGE == dst->fill.type) {
		if (dst->fill.image.image != NULL)
			g_object_unref (dst->fill.image.image);
	}

	if (src->font.font != NULL)
		go_font_ref (src->font.font);
	if (dst->font.font != NULL)
		go_font_unref (dst->font.font);

	dst->outline = src->outline;
	dst->fill    = src->fill;
	dst->line    = src->line;
	if (dst->marker.mark)
		g_object_unref (dst->marker.mark);
	dst->marker = src->marker;
	dst->marker.mark = go_marker_dup (src->marker.mark);
	dst->font    = src->font;

	if (GOG_FILL_STYLE_IMAGE == dst->fill.type && src->fill.image.image)
		dst->fill.image.image = g_object_ref (src->fill.image.image);

	dst->text_layout = src->text_layout;

	dst->interesting_fields = src->interesting_fields;
	dst->disable_theming = src->disable_theming;
}

/**
 * gog_style_apply_theme :
 * @dst : #GogStyle
 * @src :  #GogStyle
 *
 * Merge the attributes from @src onto the elements of @dst that were not user
 * assigned (is_auto)
 **/
void
gog_style_apply_theme (GogStyle *dst, GogStyle const *src)
{
	if (src == dst)
		return;

	g_return_if_fail (IS_GOG_STYLE (src));
	g_return_if_fail (IS_GOG_STYLE (dst));

	if (dst->outline.auto_dash)
		dst->outline.dash_type = src->outline.dash_type;
	if (dst->outline.auto_color)
		dst->outline.color = src->outline.color;
	if (dst->fill.auto_type)
		dst->fill.type = src->fill.type;
	if (dst->fill.auto_fore)
		dst->fill.pattern.fore = src->fill.pattern.fore;
	if (dst->fill.auto_back)
		dst->fill.pattern.back = src->fill.pattern.back;
	if (dst->line.auto_dash)
		dst->line.dash_type = src->line.dash_type;
	if (dst->line.auto_color)
		dst->line.color = src->line.color;
	if (dst->marker.auto_shape)
		go_marker_set_shape (dst->marker.mark,
			go_marker_get_shape (src->marker.mark));
	if (dst->marker.auto_outline_color)
		go_marker_set_outline_color (dst->marker.mark,
			go_marker_get_outline_color (src->marker.mark));
	if (dst->marker.auto_fill_color)
		go_marker_set_fill_color (dst->marker.mark,
			go_marker_get_fill_color (src->marker.mark));
	if (dst->text_layout.auto_angle)
		dst->text_layout.angle = src->text_layout.angle;

#if 0
	/* Fonts are not themed until we have some sort of auto mechanism
	 * stronger than 'auto_size' */
	if (src->font.font != NULL)
		go_font_ref (src->font.font);
	if (dst->font.font != NULL)
		go_font_unref (dst->font.font);
	dst->font = src->font;
#endif
}

static void
gog_style_finalize (GObject *obj)
{
	GogStyle *style = GOG_STYLE (obj);

	if (GOG_FILL_STYLE_IMAGE == style->fill.type &&
	    style->fill.image.image != NULL)
		g_object_unref (style->fill.image.image);

	if (style->font.font != NULL) {
		go_font_unref (style->font.font);
		style->font.font = NULL;
	}

	if (style->marker.mark != NULL) {
		g_object_unref (style->marker.mark);
		style->marker.mark = NULL;
	}

	(parent_klass->finalize) (obj);
}

static void
gog_style_class_init (GogStyleClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)klass;
	parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->finalize	= gog_style_finalize;
}

static void
gog_style_init (GogStyle *style)
{
	style->interesting_fields = GOG_STYLE_ALL;
	style->disable_theming = 0;
	gog_style_force_auto (style);
	style->line.dash_type = GO_LINE_SOLID;
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.width = 0;
	style->fill.type = GOG_FILL_STYLE_NONE;
	style->fill.gradient.brightness = -1.;
	go_pattern_set_solid (&style->fill.pattern, RGBA_BLACK);
	style->font.font = go_font_new_by_index (0);
	style->font.color = RGBA_BLACK;
	style->text_layout.angle = 0.0;
}

static struct {
	GogFillStyle fstyle;
	char const *name;
} fill_names[] = {
	{ GOG_FILL_STYLE_NONE,     "none" },
	{ GOG_FILL_STYLE_PATTERN,  "pattern" },
	{ GOG_FILL_STYLE_GRADIENT, "gradient" },
	{ GOG_FILL_STYLE_IMAGE,    "image" }
};

static struct {
	GogImageType fstyle;
	char const *name;
} image_tiling_names[] = {
	{ GOG_IMAGE_CENTERED,     "centered" },
	{ GOG_IMAGE_STRETCHED,    "stretched" },
	{ GOG_IMAGE_WALLPAPER,    "wallpaper" },
};

static gboolean
bool_prop (xmlNode *node, char const *name, gboolean *res)
{
	char *str = xmlGetProp (node, name);
	if (str != NULL) {
		*res = g_ascii_tolower (*str) == 't' ||
			g_ascii_tolower (*str) == 'y' ||
			strtol (str, NULL, 0);
		xmlFree (str);
		return TRUE;
	}
	return FALSE;
}

static gboolean
bool_sax_prop (char const *name, char const *id, char const *val, gboolean *res)
{
	if (0 == strcmp (name, id)) {
		*res = g_ascii_tolower (*val) == 't' ||
			g_ascii_tolower (*val) == 'y' ||
			strtol (val, NULL, 0);
		return TRUE;
	}
	return FALSE;
}


static GogFillStyle
str_as_fill_style (char const *name)
{
	unsigned i;
	for (i = 0; i < G_N_ELEMENTS (fill_names); i++)
		if (strcmp (fill_names[i].name, name) == 0)
			return fill_names[i].fstyle;
	return GOG_FILL_STYLE_PATTERN;
}

static char const *
fill_style_as_str (GogFillStyle fstyle)
{
	unsigned i;
	for (i = 0; i < G_N_ELEMENTS (fill_names); i++)
		if (fill_names[i].fstyle == fstyle)
			return fill_names[i].name;
	return "pattern";
}

static GogImageType
str_as_image_tiling (char const *name)
{
	unsigned i;
	for (i = 0; i < G_N_ELEMENTS (image_tiling_names); i++)
		if (strcmp (image_tiling_names[i].name, name) == 0)
			return image_tiling_names[i].fstyle;
	return GOG_IMAGE_STRETCHED;
}

static char const *
image_tiling_as_str (GogImageType fstyle)
{
	unsigned i;
	for (i = 0; i < G_N_ELEMENTS (image_tiling_names); i++)
		if (image_tiling_names[i].fstyle == fstyle)
			return image_tiling_names[i].name;
	return "stretched";
}

static void
gog_style_line_load (xmlNode *node, GogStyleLine *line)
{
	char *str;
	gboolean tmp;

	str = xmlGetProp (node, "dash");
	if (str != NULL) {
		line->dash_type = go_line_dash_from_str (str);
		xmlFree (str);
	}
	if (bool_prop (node, "auto-dash", &tmp))
		line->auto_dash = tmp;
	str = xmlGetProp (node, "width");
	if (str != NULL) {
		line->width = g_strtod (str, NULL);
		/* For compatibility with older graphs, when dash_type
		 * didn't exist */
		if (line->width < 0.) {
			line->width = 0.;
			line->dash_type = GO_LINE_NONE;
		}
		xmlFree (str);
	}
	str = xmlGetProp (node, "color");
	if (str != NULL) {
		go_color_from_str (str, &line->color);
		xmlFree (str);
	}
	if (bool_prop (node, "auto-color", &tmp))
		line->auto_color = tmp;
}

static void
gog_style_line_sax_save (GsfXMLOut *output, char const *name,
			 GogStyleLine const *line)
{
	gsf_xml_out_start_element (output, name);
	gsf_xml_out_add_cstr_unchecked (output, "dash",
		go_line_dash_as_str (line->dash_type));
	gsf_xml_out_add_bool (output, "auto-dash", line->auto_dash);
	gsf_xml_out_add_float (output, "width", line->width, 6);
	go_xml_out_add_color (output, "color", line->color);
	gsf_xml_out_add_bool (output, "auto-color", line->auto_color);
	gsf_xml_out_end_element (output);
}

static void
gog_style_gradient_sax_save (GsfXMLOut *output, GogStyle const *style)
{
	gsf_xml_out_start_element (output, "gradient");
	gsf_xml_out_add_cstr_unchecked (output, "direction",
		go_gradient_dir_as_str (style->fill.gradient.dir));
	go_xml_out_add_color (output, "start-color",
		style->fill.pattern.back);
	if (style->fill.gradient.brightness >= 0.)
		gsf_xml_out_add_float (output, "brightness",
			style->fill.gradient.brightness, 6);
	else 
		go_xml_out_add_color (output, "end-color",
			style->fill.pattern.fore);
	gsf_xml_out_end_element (output);
}

static void
gog_style_fill_sax_save (GsfXMLOut *output, GogStyle const *style)
{
	gsf_xml_out_start_element (output, "fill");
	gsf_xml_out_add_cstr_unchecked (output, "type",
		    fill_style_as_str (style->fill.type));
	gsf_xml_out_add_bool (output, "auto-type",
		style->fill.auto_type);
	gsf_xml_out_add_bool (output, "is-auto",
		style->fill.auto_back);
	gsf_xml_out_add_bool (output, "auto-fore",
		style->fill.auto_fore);

	switch (style->fill.type) {
	case GOG_FILL_STYLE_NONE: break;
	case GOG_FILL_STYLE_PATTERN:
		gsf_xml_out_start_element (output, "pattern");
		gsf_xml_out_add_cstr_unchecked (output, "type",
			go_pattern_as_str (style->fill.pattern.pattern));
		go_xml_out_add_color (output, "fore",
			style->fill.pattern.fore);
		go_xml_out_add_color (output, "back",
			style->fill.pattern.back);
		gsf_xml_out_end_element (output);
		break;

	case GOG_FILL_STYLE_GRADIENT:
		gog_style_gradient_sax_save (output, style);
		break;
	case GOG_FILL_STYLE_IMAGE:
		gsf_xml_out_start_element (output, "image");
		/* FIXME : type is not a good characterization */
		gsf_xml_out_add_cstr_unchecked (output, "type",
			image_tiling_as_str (style->fill.image.type));
		gsf_xml_out_add_cstr (output, "name", go_image_get_name (style->fill.image.image));
		go_doc_save_image ((GODoc *) g_object_get_data (G_OBJECT (gsf_xml_out_get_output (output)), "document"), go_image_get_name (style->fill.image.image));
		gsf_xml_out_end_element (output);
		break;
	default:
		break;
	}
	gsf_xml_out_end_element (output);
}

static void
gog_style_gradient_load (xmlNode *node, GogStyle *style)
{
	char    *str = xmlGetProp (node, "direction");
	if (str != NULL) {
		style->fill.gradient.dir
			= go_gradient_dir_from_str (str);
		xmlFree (str);
	}
	str = xmlGetProp (node, "start-color");
	if (str != NULL) {
		go_color_from_str (str, &style->fill.pattern.back);
		xmlFree (str);
	}
	str = xmlGetProp (node, "brightness");
	if (str != NULL) {
		gog_style_set_fill_brightness (style, g_strtod (str, NULL));
		xmlFree (str);
	} else {
		str = xmlGetProp (node, "end-color");
		if (str != NULL) {
			go_color_from_str (str, &style->fill.pattern.fore);
			xmlFree (str);
		}
	}
}

static void
gog_style_image_load (xmlNode *node, GogStyle *style)
{
	char *str = xmlGetProp (node, "type");
	if (str != NULL) {
		style->fill.image.type = str_as_image_tiling (str);
		xmlFree (str);
	}
	/* TODO: load the pixels */
}

static void
gog_style_fill_load (xmlNode *node, GogStyle *style)
{
	xmlNode *ptr;
	gboolean tmp;
	char    *str = xmlGetProp (node, "type");

	if (str == NULL)
		return;
	style->fill.type = str_as_fill_style (str);
	xmlFree (str);

	style->fill.auto_type = FALSE;

	if (bool_prop (node, "auto-type", &tmp))
		style->fill.auto_type = tmp;
	if (bool_prop (node, "is-auto", &tmp))
		style->fill.auto_back = tmp;
	if (bool_prop (node, "auto-fore", &tmp))
		style->fill.auto_fore = tmp;

	switch (style->fill.type) {
	case GOG_FILL_STYLE_PATTERN:
		for (ptr = node->xmlChildrenNode ;
		     ptr != NULL ; ptr = ptr->next) {
			if (xmlIsBlankNode (ptr) || ptr->name == NULL)
				continue;
			if (strcmp (ptr->name, "pattern") == 0) {
				str = xmlGetProp (ptr, "type");
				if (str != NULL) {
					style->fill.pattern.pattern
						= go_pattern_from_str (str);
					xmlFree (str);
				}
				str = xmlGetProp (ptr, "fore");
				if (str != NULL) {
					go_color_from_str (str, &style->fill.pattern.fore);
					xmlFree (str);
				}
				str = xmlGetProp (ptr, "back");
				if (str != NULL) {
					go_color_from_str (str, &style->fill.pattern.back);
					xmlFree (str);
				}
			}
		}
		break;
	case GOG_FILL_STYLE_GRADIENT:
		for (ptr = node->xmlChildrenNode ;
		     ptr != NULL ; ptr = ptr->next) {
			if (xmlIsBlankNode (ptr) || ptr->name == NULL)
				continue;
			if (strcmp (ptr->name, "gradient") == 0)
				gog_style_gradient_load (ptr, style);
		}
		break;
	case GOG_FILL_STYLE_IMAGE:
		for (ptr = node->xmlChildrenNode ;
		     ptr != NULL ; ptr = ptr->next) {
			if (xmlIsBlankNode (ptr) || ptr->name == NULL)
				continue;
			if (strcmp (ptr->name, "image") == 0) {
				gog_style_image_load (ptr, style);
			}
		}
		break;
	default:
		break;
	}
}

static void
gog_style_marker_load (xmlNode *node, GogStyle *style)
{
	char *str;
	GOColor c;
	GOMarker *marker = go_marker_dup (style->marker.mark);

	str = xmlGetProp (node, "shape");
	if (str != NULL) {
		style->marker.auto_shape = TRUE;
		bool_prop (node, "auto-shape", &style->marker.auto_shape);
		go_marker_set_shape (marker, go_marker_shape_from_str (str));
		xmlFree (str);
	}
	str = xmlGetProp (node, "outline-color");
	if (str != NULL) {
		style->marker.auto_outline_color = TRUE;
		bool_prop (node, "auto-outline", &style->marker.auto_outline_color);
		if (go_color_from_str (str, &c))
			go_marker_set_outline_color (marker, c);
		xmlFree (str);
	}
	str = xmlGetProp (node, "fill-color");
	if (str != NULL) {
		style->marker.auto_fill_color = TRUE;
		bool_prop (node, "auto-fill", &style->marker.auto_fill_color);
		if (go_color_from_str (str, &c))
			go_marker_set_fill_color (marker, c);
		xmlFree (str);
	}
	str = xmlGetProp (node, "size");
	if (str != NULL) {
		go_marker_set_size (marker, g_strtod (str, NULL));
		xmlFree (str);
	}
	gog_style_set_marker (style, marker);
}

static void
gog_style_marker_sax_save (GsfXMLOut *output, GogStyle const *style)
{
	gsf_xml_out_start_element (output, "marker");
	gsf_xml_out_add_bool (output, "auto-shape", style->marker.auto_shape);
	gsf_xml_out_add_cstr (output, "shape",
		go_marker_shape_as_str (go_marker_get_shape (style->marker.mark)));
	gsf_xml_out_add_bool (output, "auto-outline", 
		style->marker.auto_outline_color);
	go_xml_out_add_color (output, "outline-color",
		go_marker_get_outline_color (style->marker.mark));
	gsf_xml_out_add_bool (output, "auto-fill", style->marker.auto_fill_color);
	go_xml_out_add_color (output, "fill-color",
		go_marker_get_fill_color (style->marker.mark));
	gsf_xml_out_add_int (output, "size",
		go_marker_get_size (style->marker.mark));
	gsf_xml_out_end_element (output);
}

static void
gog_style_font_load (xmlNode *node, GogStyle *style)
{
	char *str;
	gboolean tmp;

	str = xmlGetProp (node, "color");
	if (str != NULL) {
		go_color_from_str (str, &style->font.color);
		xmlFree (str);
	}
	str = xmlGetProp (node, "font");
	if (str != NULL) {
		PangoFontDescription *desc;

		desc = pango_font_description_from_string (str);
		if (desc != NULL)
			gog_style_set_font_desc (style, desc);
		xmlFree (str);
	}
	if (bool_prop (node, "auto-scale", &tmp))
		style->font.auto_scale = tmp;
}

static void
gog_style_font_sax_save (GsfXMLOut *output, GogStyle const *style)
{
	char *str;
	gsf_xml_out_start_element (output, "font");
	go_xml_out_add_color (output, "color", style->font.color);
	str = go_font_as_str (style->font.font);
	gsf_xml_out_add_cstr (output, "font", str);
	g_free (str);
	gsf_xml_out_add_bool (output, "auto-scale", style->font.auto_scale);
	gsf_xml_out_end_element (output);
}

static void
gog_style_text_layout_load (xmlNode *node, GogStyle *style)
{
	char *str;

	str = xmlGetProp (node, "angle");
	if (str != NULL) {
		gog_style_set_text_angle (style, g_strtod (str, NULL));
		xmlFree (str);
	}
}

static void
gog_style_text_layout_sax_save (GsfXMLOut *output, GogStyle const *style)
{
	gsf_xml_out_start_element (output, "text_layout");
	if (!style->text_layout.auto_angle)
		gsf_xml_out_add_float (output, "angle", style->text_layout.angle, 6);
	gsf_xml_out_end_element (output);
}

static gboolean
gog_style_persist_dom_load (GogPersist *gp, xmlNode *node)
{
	GogStyle *style = GOG_STYLE (gp);
	xmlNode *ptr;

	/* while reloading no need to reapply settings */
	for (ptr = node->xmlChildrenNode ; ptr != NULL ; ptr = ptr->next) {
		if (xmlIsBlankNode (ptr) || ptr->name == NULL)
			continue;
		if (strcmp (ptr->name, "outline") == 0)
			gog_style_line_load (ptr, &style->outline);
		else if (strcmp (ptr->name, "line") == 0)
			gog_style_line_load (ptr, &style->line);
		else if (strcmp (ptr->name, "fill") == 0)
			gog_style_fill_load (ptr, style);
		else if (strcmp (ptr->name, "marker") == 0)
			gog_style_marker_load (ptr, style);
		else if (strcmp (ptr->name, "font") == 0)
			gog_style_font_load (ptr, style);
		else if (strcmp (ptr->name, "text_layout") == 0)
			gog_style_text_layout_load (ptr, style);
	}
	return TRUE;
}

static void
gog_style_sax_load_line (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	GogStyleLine *line = xin->node->user_data.v_int ? &style->outline : &style->line;

	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "dash"))
			line->dash_type = go_line_dash_from_str (attrs[1]);
		else if (bool_sax_prop ("auto-dash", attrs[0], attrs[1], &line->auto_dash))
			;
		else if (0 == strcmp (attrs[0], "width")) {
			line->width = g_strtod (attrs[1], NULL);
			/* For compatibility with older graphs, when dash_type
			 * didn't exist */
			if (line->width < 0.) {
				line->width = 0.;
				line->dash_type = GO_LINE_NONE;
			}
		} else if (0 == strcmp (attrs[0], "color"))
			go_color_from_str (attrs[1], &line->color);
		else if (bool_sax_prop ("auto-color", attrs[0], attrs[1], &line->auto_color))
			;
}

static void
gog_style_sax_load_fill_pattern (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	g_return_if_fail (style->fill.type == GOG_FILL_STYLE_PATTERN);
	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "type"))
			style->fill.pattern.pattern = go_pattern_from_str (attrs[1]);
		else if (0 == strcmp (attrs[0], "fore"))
			go_color_from_str (attrs[1], &style->fill.pattern.fore);
		else if (0 == strcmp (attrs[0], "back"))
			go_color_from_str (attrs[1], &style->fill.pattern.back);
}

static void
gog_style_sax_load_fill_gradient (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	g_return_if_fail (style->fill.type == GOG_FILL_STYLE_GRADIENT);
	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "direction"))
			style->fill.gradient.dir = go_gradient_dir_from_str (attrs[1]);
		else if (0 == strcmp (attrs[0], "start-color"))
			go_color_from_str (attrs[1], &style->fill.pattern.back);
		else if (0 == strcmp (attrs[0], "end-color"))
			go_color_from_str (attrs[1], &style->fill.pattern.fore);
		else if (0 == strcmp (attrs[0], "brightness"))
			gog_style_set_fill_brightness (style, g_strtod (attrs[1], NULL));
}

static void
gog_style_sax_load_fill_image (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	GODoc *doc = (GODoc *) g_object_get_data (G_OBJECT (gsf_xml_in_get_input (xin)), "document");
	g_return_if_fail (style->fill.type == GOG_FILL_STYLE_IMAGE);
	g_return_if_fail (IS_GO_DOC (doc));
	/* TODO: load the pixels */
	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "type")) {
			style->fill.image.type = str_as_image_tiling (attrs[1]);
		} else if (0 == strcmp (attrs[0], "name"))
			style->fill.image.image = g_object_ref (go_doc_image_fetch (doc, attrs[1]));
}

static void
gog_style_sax_load_fill (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	style->fill.auto_type = FALSE;
	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "type"))
			style->fill.type = str_as_fill_style (attrs[1]);
		else if (bool_sax_prop ("auto-type", attrs[0], attrs[1], &style->fill.auto_type))
			;
		else if (bool_sax_prop ("is-auto", attrs[0], attrs[1], &style->fill.auto_back))
			;
		else if (bool_sax_prop ("auto-fore", attrs[0], attrs[1], &style->fill.auto_fore))
			;
}
static void
gog_style_sax_load_marker (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	GOMarker *marker = go_marker_dup (style->marker.mark);
	GOColor c;

	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (bool_sax_prop ("auto-shape", attrs[0], attrs[1], &style->marker.auto_shape))
			;
		else if (0 == strcmp (attrs[0], "shape"))
			go_marker_set_shape (marker, go_marker_shape_from_str (attrs[1]));
		else if (bool_sax_prop ("auto-outline", attrs[0], attrs[1], &style->marker.auto_outline_color))
			;
		else if (0 == strcmp (attrs[0], "outline-color")) {
			if (go_color_from_str (attrs[1], &c))
				go_marker_set_outline_color (marker, c);
		} else if (bool_sax_prop ("auto-fill", attrs[0], attrs[1], &style->marker.auto_fill_color))
			;
		else if (0 == strcmp (attrs[0], "fill-color")) {
			if (go_color_from_str (attrs[1], &c))
				go_marker_set_fill_color (marker, c);
		} else if (0 == strcmp (attrs[0], "size"))
			go_marker_set_size (marker, g_strtod (attrs[1], NULL));

	gog_style_set_marker (style, marker);
}

static void
gog_style_sax_load_font (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "color"))
			go_color_from_str (attrs[1], &style->font.color);
		else if (0 == strcmp (attrs[0], "font")) {
			PangoFontDescription *desc = pango_font_description_from_string (attrs[1]);
			if (desc != NULL)
				gog_style_set_font_desc (style, desc);
		} else if (bool_sax_prop ("auto-scale", attrs[0], attrs[1], &style->font.auto_scale))
			;
}
static void
gog_style_sax_load_text_layout (GsfXMLIn *xin, xmlChar const **attrs)
{
	GogStyle *style = GOG_STYLE (xin->user_state);
	for (; attrs != NULL && attrs[0] && attrs[1] ; attrs += 2)
		if (0 == strcmp (attrs[0], "angle"))
			gog_style_set_text_angle (style, g_strtod (attrs[1], NULL));
}

static void
gog_style_persist_sax_save (GogPersist const *gp, GsfXMLOut *output)
{
	GogStyle const *style = GOG_STYLE (gp);

	gsf_xml_out_add_cstr_unchecked (output, "type",
		G_OBJECT_TYPE_NAME (style));

	if (style->interesting_fields & GOG_STYLE_OUTLINE)
		gog_style_line_sax_save (output, "outline", &style->outline);
	if (style->interesting_fields & GOG_STYLE_LINE)
		gog_style_line_sax_save (output, "line", &style->line);
	if (style->interesting_fields & GOG_STYLE_FILL)
		gog_style_fill_sax_save (output, style);
	if (style->interesting_fields & GOG_STYLE_MARKER)
		gog_style_marker_sax_save (output, style);
	if (style->interesting_fields & GOG_STYLE_FONT)
		gog_style_font_sax_save (output, style);
	if (style->interesting_fields & GOG_STYLE_TEXT_LAYOUT)
		gog_style_text_layout_sax_save (output, style);
}

static void
gog_style_persist_prep_sax (GogPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	static GsfXMLInNode const dtd[] = {
		GSF_XML_IN_NODE 	(STYLE, STYLE, 
					 -1, "GogObject", 
					 FALSE, NULL, NULL),
		GSF_XML_IN_NODE_FULL 	(STYLE, STYLE_LINE, 
					 -1, "line", 
					 GSF_XML_NO_CONTENT, FALSE, FALSE, 
					 &gog_style_sax_load_line, NULL, 0),
		GSF_XML_IN_NODE_FULL 	(STYLE, STYLE_OUTLINE,	
					 -1, "outline", 
					 GSF_XML_NO_CONTENT, FALSE, FALSE, 
					 &gog_style_sax_load_line, NULL, 1),
		GSF_XML_IN_NODE 	(STYLE, STYLE_FILL, 
					 -1, "fill", 
					 GSF_XML_NO_CONTENT, 
					 &gog_style_sax_load_fill, NULL),
		GSF_XML_IN_NODE 	(STYLE_FILL, FILL_PATTERN,  
					 -1, "pattern", 
					 GSF_XML_NO_CONTENT, 
					 &gog_style_sax_load_fill_pattern, NULL),
		GSF_XML_IN_NODE 	(STYLE_FILL, FILL_GRADIENT, 
					 -1, "gradient", 
					 GSF_XML_NO_CONTENT, 
					 &gog_style_sax_load_fill_gradient, NULL),
		GSF_XML_IN_NODE 	(STYLE_FILL, FILL_IMAGE, 
					 -1, "image", 
					 GSF_XML_NO_CONTENT, 
					 &gog_style_sax_load_fill_image, NULL),
		GSF_XML_IN_NODE 	(STYLE, STYLE_MARKER,	
					 -1, "marker", 
					 GSF_XML_NO_CONTENT, 
					 &gog_style_sax_load_marker, NULL),
		GSF_XML_IN_NODE 	(STYLE, STYLE_FONT,
			 		 -1, "font", 
					 GSF_XML_NO_CONTENT, 
					 &gog_style_sax_load_font, NULL),
		GSF_XML_IN_NODE 	(STYLE, STYLE_ALIGNMENT,	
					 -1, "text_layout", 
					 GSF_XML_NO_CONTENT, 
					 &gog_style_sax_load_text_layout, NULL),
		GSF_XML_IN_NODE_END
	};
	static GsfXMLInDoc *doc = NULL;
	
	if (NULL == doc)
		doc = gsf_xml_in_doc_new (dtd, NULL);
	gsf_xml_in_push_state (xin, doc, gp, NULL, attrs);
}

static void
gog_style_persist_init (GogPersistClass *iface)
{
	iface->dom_load = gog_style_persist_dom_load;
	iface->prep_sax = gog_style_persist_prep_sax;
	iface->sax_save = gog_style_persist_sax_save;
}

GSF_CLASS_FULL (GogStyle, gog_style,
		NULL, NULL, gog_style_class_init, NULL,
		gog_style_init, G_TYPE_OBJECT, 0,
		GSF_INTERFACE (gog_style_persist_init, GOG_PERSIST_TYPE))

gboolean
gog_style_is_different_size (GogStyle const *a, GogStyle const *b)
{
	if (a == NULL || b == NULL)
		return TRUE;
	return	a->outline.dash_type != b->outline.dash_type ||
		a->outline.width != b->outline.width ||
		a->line.width != b->line.width ||
		a->fill.type != b->fill.type ||
		a->text_layout.angle != b->text_layout.angle ||
		!go_font_eq (a->font.font, b->font.font);
}

gboolean
gog_style_is_marker_visible (GogStyle const *style)
{
	g_return_val_if_fail (IS_GOG_STYLE (style), FALSE);

	/* FIXME FIXME FIXME TODO : make this smarter */
	return (style->interesting_fields & GOG_STYLE_MARKER) &&
		go_marker_get_shape (style->marker.mark) != GO_MARKER_NONE;
}

gboolean
gog_style_is_outline_visible (GogStyle const *style)
{
	g_return_val_if_fail (IS_GOG_STYLE (style), FALSE);

	/* FIXME FIXME FIXME make this smarter */
	return UINT_RGBA_A (style->outline.color) > 0 && 
		style->outline.dash_type != GO_LINE_NONE;
}

gboolean
gog_style_is_line_visible (GogStyle const *style)
{
	g_return_val_if_fail (IS_GOG_STYLE (style), FALSE);

	/* FIXME FIXME FIXME TODO : make this smarter */
	return UINT_RGBA_A (style->line.color) > 0 && 
		style->line.dash_type != GO_LINE_NONE;
}

gboolean 
gog_style_is_fill_visible (const GogStyle *style)
{
	g_return_val_if_fail (IS_GOG_STYLE (style), FALSE);

	return (style->fill.type != GOG_FILL_STYLE_NONE);
}

void
gog_style_force_auto (GogStyle *style)
{
	g_return_if_fail (IS_GOG_STYLE (style));

	if (style->marker.mark != NULL)
		g_object_unref (G_OBJECT (style->marker.mark));
	style->marker.mark = go_marker_new ();
	style->marker.auto_shape =
	style->marker.auto_outline_color =
	style->marker.auto_fill_color =
	style->outline.auto_dash =
	style->outline.auto_color =
	style->line.auto_dash =
	style->line.auto_color =
	style->fill.auto_type =
	style->fill.auto_fore =
	style->fill.auto_back =
	style->font.auto_scale =
	style->text_layout.auto_angle = TRUE;
}

/**
 * gog_style_set_marker :
 * @style : #GogStyle
 * @marker : #GOMarker
 *
 * Absorb a reference to @marker and assign it to @style.
 **/
void
gog_style_set_marker (GogStyle *style, GOMarker *marker)
{
	g_return_if_fail (IS_GOG_STYLE (style));
	g_return_if_fail (IS_GO_MARKER (marker));

	if (style->marker.mark != marker) {
		if (style->marker.mark != NULL)
			g_object_unref (style->marker.mark);
		style->marker.mark = marker;
	}
}

/**
 * gog_style_get_marker :
 * @style : #GogStyle
 *
 * Accessor for @style::marker, without referencing it. 
 *
 * return value: the style #GOMarker.
 **/
GOMarker const *
gog_style_get_marker (GogStyle *style) 
{
	g_return_val_if_fail (IS_GOG_STYLE (style), NULL);

	return style->marker.mark;
}

void
gog_style_set_font_desc (GogStyle *style, PangoFontDescription *desc)
{
	GOFont const *font;

	g_return_if_fail (IS_GOG_STYLE (style));

	font = go_font_new_by_desc (desc);
	if (font != NULL) {
		go_font_unref (style->font.font);
		style->font.font = font;
	}
}

void
gog_style_set_font (GogStyle *style, GOFont const *font)
{
	g_return_if_fail (IS_GOG_STYLE (style));

	if (font != NULL) {
		go_font_unref (style->font.font);
		style->font.font = font;
	}
}

void
gog_style_set_fill_brightness (GogStyle *style, float brightness)
{
	g_return_if_fail (IS_GOG_STYLE (style));
	g_return_if_fail (style->fill.type == GOG_FILL_STYLE_GRADIENT);

	brightness = CLAMP (brightness, 0, 100.0);

	style->fill.gradient.brightness = brightness;
	style->fill.pattern.fore = (brightness < 50.)
		? UINT_INTERPOLATE(style->fill.pattern.back, RGBA_WHITE, 1. - brightness / 50.)
		: UINT_INTERPOLATE(style->fill.pattern.back, RGBA_BLACK, brightness / 50. - 1.);
}

/**
 * gog_style_set_text_angle:
 * @style : #GogStyle
 * @angle : text rotation in degrees
 *
 * Set text rotation angle in degrees. Valid values are in the range
 * [-180.0° , 180.0°].
 **/
void
gog_style_set_text_angle (GogStyle *style, double angle)
{
	g_return_if_fail (IS_GOG_STYLE (style));

	style->text_layout.angle = CLAMP (angle, -180.0, 180.0);
	style->text_layout.auto_angle = FALSE;
}

/**
 * gog_style_create_cairo_pattern:
 * @style : #GogStyle
 * @cr: a cairo context
 *
 * Create a cairo_patern_t using the current style settings for filling.
 * A pattern will be created only if the style has the corresponding field
 * and if it is not set to a none constant.
 *
 * return value: the pattern or NULL if it could not be created.
 **/
cairo_pattern_t *
gog_style_create_cairo_pattern (GogStyle const *style, cairo_t *cr)
{
	cairo_pattern_t *cr_pattern;
	cairo_matrix_t cr_matrix;
	double x[3], y[3];
	int w, h;

	static struct { unsigned x0i, y0i, x1i, y1i; } const grad_i[GO_GRADIENT_MAX] = {
		{0, 0, 0, 1},
		{0, 1, 0, 0},
		{0, 0, 0, 2},
		{0, 2, 0, 1},
		{0, 0, 1, 0},
		{1, 0, 0, 0},
		{0, 0, 2, 0},
		{2, 0, 1, 0},
		{0, 0, 1, 1},
		{1, 1, 0, 0},
		{0, 0, 2, 2},
		{2, 2, 1, 1},
		{1, 0, 0, 1},
		{0, 1, 1, 0},
		{1, 0, 2, 2},
		{2, 2, 0, 1}
	};

	g_return_val_if_fail (IS_GOG_STYLE (style), NULL);

	if (style->fill.type == GOG_FILL_STYLE_NONE)
		return NULL;

	cairo_fill_extents (cr, &x[0], &y[0], &x[1], &y[1]);
	if (go_sub_epsilon (fabs (x[0] - x[1])) <=0.0 ||
	    go_sub_epsilon (fabs (y[0] - y[1])) <=0.0)
		return NULL;

	switch (style->fill.type) {
		case GOG_FILL_STYLE_PATTERN:
			return go_pattern_create_cairo_pattern (&style->fill.pattern, cr);

		case GOG_FILL_STYLE_GRADIENT:
			x[2] = (x[1] - x[0]) / 2.0 + x[0];
			y[2] = (y[1] - y[0]) / 2.0 + y[0];
			cr_pattern = cairo_pattern_create_linear (
				x[grad_i[style->fill.gradient.dir].x0i],
				y[grad_i[style->fill.gradient.dir].y0i],
				x[grad_i[style->fill.gradient.dir].x1i],
				y[grad_i[style->fill.gradient.dir].y1i]);
			cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REFLECT);
			cairo_pattern_add_color_stop_rgba (cr_pattern, 0,
				GO_COLOR_TO_CAIRO (style->fill.pattern.back));
			cairo_pattern_add_color_stop_rgba (cr_pattern, 1,
				GO_COLOR_TO_CAIRO (style->fill.pattern.fore));
			return cr_pattern;

		case GOG_FILL_STYLE_IMAGE:
			if (style->fill.image.image == NULL)
				return cairo_pattern_create_rgba (1, 1, 1, 1);

			cr_pattern = go_image_create_cairo_pattern (style->fill.image.image);
			if (cr_pattern == NULL) {
				/* don't reference anymore an invalid image */
				((GogStyle *) style)->fill.image.image = NULL;
				return cairo_pattern_create_rgba (1, 1, 1, 1);
			}
			g_object_get (style->fill.image.image, "width", &w, "height", &h, NULL);
			cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REPEAT);
			switch (style->fill.image.type) {
				case GOG_IMAGE_CENTERED:
					cairo_matrix_init_translate (&cr_matrix,
								     -(x[1] - x[0] - w) / 2 - x[0],
								     -(y[1] - y[0] - h) / 2 - y[0]);
					cairo_pattern_set_matrix (cr_pattern, &cr_matrix);
					break;
				case GOG_IMAGE_STRETCHED:
					cairo_matrix_init_scale (&cr_matrix,
								 w / (x[1] - x[0]),
								 h / (y[1] - y[0]));
					cairo_matrix_translate (&cr_matrix, -x[0], -y[0]);
					cairo_pattern_set_matrix (cr_pattern, &cr_matrix);
					break;
				case GOG_IMAGE_WALLPAPER:
					break;
			}
			return cr_pattern;

		case GOG_FILL_STYLE_NONE:
			return NULL;
	}

	return NULL;
}
