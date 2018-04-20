/*
 * go-color-palette.c - A color selector palette
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 * This code was extracted from widget-color-combo.c
 *   written by Miguel de Icaza (miguel@kernel.org) and
 *   Dom Lachowicz (dominicl@seas.upenn.edu). The extracted
 *   code was re-packaged into a separate object by
 *   Michael Levy (mlevy@genoscope.cns.fr)
 *   And later revised and polished by
 *   Almer S. Tigelaar (almer@gnome.org)
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

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/utils/go-marshalers.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#include <string.h>

struct _GOColorPalette {
	GtkBox	base;

	GOColorGroup *group;
	GOColor	      selection, default_color;
	gboolean      current_is_custom;
	gboolean      current_is_default;
	gboolean      allow_alpha;

	/* only for custom colours */
	GtkWidget   *swatches[GO_COLOR_GROUP_HISTORY_SIZE];

	/* The table with our default color names */
	GONamedColor const *default_set;
};

typedef struct {
	GtkBoxClass base;

	/* Signals emited by this widget */
	void (*color_changed) (GOColorPalette *pal, GOColor color,
			       gboolean is_custom, gboolean by_user, gboolean is_default);
	void (*display_custom_dialog) (GOColorPalette *pal, GtkWidget *dialog);
} GOColorPaletteClass;

#define COLOR_PREVIEW_WIDTH 12
#define COLOR_PREVIEW_HEIGHT 12

#define CCW_KEY "GOColorPalette::ccw"

enum {
	COLOR_CHANGED,
	DISPLAY_CUSTOM_DIALOG,
	LAST_SIGNAL
};

static GONamedColor const default_color_set [] = {
	/* xgettext: See https://bugzilla.gnome.org/attachment.cgi?id=222905 */
	{ GO_COLOR_FROM_RGBA (0x00, 0x00, 0x00, 0xff), N_("black")},
	{ GO_COLOR_FROM_RGBA (0x99, 0x33, 0x00, 0xff), N_("light brown")},
	{ GO_COLOR_FROM_RGBA (0x33, 0x33, 0x00, 0xff), N_("brown gold")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x33, 0x00, 0xff), N_("dark green #2")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x33, 0x66, 0xff), N_("navy")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x00, 0x80, 0xff), N_("dark blue")},
	{ GO_COLOR_FROM_RGBA (0x33, 0x33, 0x99, 0xff), N_("purple #2")},
	{ GO_COLOR_FROM_RGBA (0x33, 0x33, 0x33, 0xff), N_("very dark gray")},

	{ GO_COLOR_FROM_RGBA (0x80, 0x00, 0x00, 0xff), N_("dark red")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0x66, 0x00, 0xff), N_("red-orange")},
	{ GO_COLOR_FROM_RGBA (0x80, 0x80, 0x00, 0xff), N_("gold")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x80, 0x00, 0xff), N_("dark green")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x80, 0x80, 0xff), N_("dull blue")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x00, 0xFF, 0xff), N_("blue")},
	{ GO_COLOR_FROM_RGBA (0x66, 0x66, 0x99, 0xff), N_("dull purple")},
	{ GO_COLOR_FROM_RGBA (0x80, 0x80, 0x80, 0xff), N_("dark gray")},

	{ GO_COLOR_FROM_RGBA (0xFF, 0x00, 0x00, 0xff), N_("red")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0x99, 0x00, 0xff), N_("orange")},
	{ GO_COLOR_FROM_RGBA (0x99, 0xCC, 0x00, 0xff), N_("lime")},
	{ GO_COLOR_FROM_RGBA (0x33, 0x99, 0x66, 0xff), N_("dull green")},
	{ GO_COLOR_FROM_RGBA (0x33, 0xCC, 0xCC, 0xff), N_("dull blue #2")},
	{ GO_COLOR_FROM_RGBA (0x33, 0x66, 0xFF, 0xff), N_("sky blue #2")},
	{ GO_COLOR_FROM_RGBA (0x80, 0x00, 0x80, 0xff), N_("purple")},
	{ GO_COLOR_FROM_RGBA (0x96, 0x96, 0x96, 0xff), N_("gray")},

	{ GO_COLOR_FROM_RGBA (0xFF, 0x00, 0xFF, 0xff), N_("magenta")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0xCC, 0x00, 0xff), N_("bright orange")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0xFF, 0x00, 0xff), N_("yellow")},
	{ GO_COLOR_FROM_RGBA (0x00, 0xFF, 0x00, 0xff), N_("green")},
	{ GO_COLOR_FROM_RGBA (0x00, 0xFF, 0xFF, 0xff), N_("cyan")},
	{ GO_COLOR_FROM_RGBA (0x00, 0xCC, 0xFF, 0xff), N_("bright blue")},
	{ GO_COLOR_FROM_RGBA (0x99, 0x33, 0x66, 0xff), N_("red purple")},
	{ GO_COLOR_FROM_RGBA (0xC0, 0xC0, 0xC0, 0xff), N_("light gray")},

	{ GO_COLOR_FROM_RGBA (0xFF, 0x99, 0xCC, 0xff), N_("pink")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0xCC, 0x99, 0xff), N_("light orange")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0xFF, 0x99, 0xff), N_("light yellow")},
	{ GO_COLOR_FROM_RGBA (0xCC, 0xFF, 0xCC, 0xff), N_("light green")},
	{ GO_COLOR_FROM_RGBA (0xCC, 0xFF, 0xFF, 0xff), N_("light cyan")},
	{ GO_COLOR_FROM_RGBA (0x99, 0xCC, 0xFF, 0xff), N_("light blue")},
	{ GO_COLOR_FROM_RGBA (0xCC, 0x99, 0xFF, 0xff), N_("light purple")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0xFF, 0xFF, 0xff), N_("white")},

	{ 0, NULL},

	/* Disable these for now, they are mostly repeats */
	{ GO_COLOR_FROM_RGBA (0x99, 0x99, 0xFF, 0xff), N_("purplish blue")},
	{ GO_COLOR_FROM_RGBA (0x99, 0x33, 0x66, 0xff), N_("red purple")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0xFF, 0xCC, 0xff), N_("light yellow")},
	{ GO_COLOR_FROM_RGBA (0xCC, 0xFF, 0xFF, 0xff), N_("light blue")},
	{ GO_COLOR_FROM_RGBA (0x66, 0x00, 0x66, 0xff), N_("dark purple")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0x80, 0x80, 0xff), N_("pink")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x66, 0xCC, 0xff), N_("sky blue")},
	{ GO_COLOR_FROM_RGBA (0xCC, 0xCC, 0xFF, 0xff), N_("light purple")},

	{ GO_COLOR_FROM_RGBA (0x00, 0x00, 0x80, 0xff), N_("dark blue")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0x00, 0xFF, 0xff), N_("magenta")},
	{ GO_COLOR_FROM_RGBA (0xFF, 0xFF, 0x00, 0xff), N_("yellow")},
	{ GO_COLOR_FROM_RGBA (0x00, 0xFF, 0xFF, 0xff), N_("cyan")},
	{ GO_COLOR_FROM_RGBA (0x80, 0x00, 0x80, 0xff), N_("purple")},
	{ GO_COLOR_FROM_RGBA (0x80, 0x00, 0x00, 0xff), N_("dark red")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x80, 0x80, 0xff), N_("dull blue")},
	{ GO_COLOR_FROM_RGBA (0x00, 0x00, 0xFF, 0xff), N_("blue")},

	{ 0, NULL},
};

gboolean
go_color_palette_query (int n, GONamedColor *color)
{
	if (n < 0 || n >= (int)G_N_ELEMENTS (default_color_set) - 1)
		return FALSE;

	color->name = _(default_color_set[n].name);
	color->color = default_color_set[n].color;
	return TRUE;
}



const GONamedColor *
_go_color_palette_default_color_set (void)
{
	return default_color_set;
}

static guint go_color_palette_signals [LAST_SIGNAL] = { 0, };

static GObjectClass *go_color_palette_parent_class;

static GtkWidget *
create_color_sel (GtkWidget *parent_widget, GObject *action_proxy,
		  GOColor c, GCallback handler, gboolean allow_alpha)
{
	const char *title =
		g_object_get_data (G_OBJECT (action_proxy), "title");
	GtkWidget *toplevel = gtk_widget_get_toplevel (parent_widget);
	GtkWidget *dialog = gtk_dialog_new_with_buttons
		(title,
		 (gtk_widget_is_toplevel (toplevel)
		  ? GTK_WINDOW (toplevel)
		  : NULL),
		 GTK_DIALOG_DESTROY_WITH_PARENT,
		 GTK_STOCK_OK, GTK_RESPONSE_OK,
		 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		 NULL);
	GtkWidget *ccw = gtk_color_chooser_widget_new ();
	GtkWidget *dca = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	GdkRGBA gdk;

	g_object_set_data (G_OBJECT (dialog), CCW_KEY, ccw);

	gtk_container_add (GTK_CONTAINER (dca), ccw);

	go_color_to_gdk_rgba (c, &gdk);

	g_object_set (G_OBJECT (ccw),
		      "use-alpha", allow_alpha,
		      "rgba", &gdk,
		      "show-editor", TRUE,
		      NULL);

	g_signal_connect_object (dialog,
		"response", handler, action_proxy, 0);

	/* require an explicit show _after_ the custom-dialog signal fires */
	return dialog;
}

static gboolean
handle_color_sel (GtkDialog *dialog,
		  gint response_id, GOColor *res)
{
	if (response_id == GTK_RESPONSE_OK) {
		GtkColorChooser *chooser = g_object_get_data (G_OBJECT (dialog), CCW_KEY);
		GdkRGBA gdk;
		gtk_color_chooser_get_rgba (chooser, &gdk);
		*res = GO_COLOR_FROM_GDK_RGBA (gdk);
	}
	/* destroy _before_ we emit */
	gtk_widget_destroy (GTK_WIDGET (dialog));
	return response_id == GTK_RESPONSE_OK;
}

static void
go_color_palette_finalize (GObject *object)
{
	GOColorPalette *pal = GO_COLOR_PALETTE (object);

	go_color_palette_set_group (pal, NULL);

	(*go_color_palette_parent_class->finalize) (object);
}

static void
go_color_palette_class_init (GObjectClass *gobject_class)
{
	gobject_class->finalize = go_color_palette_finalize;

	go_color_palette_parent_class = g_type_class_peek_parent (gobject_class);

	go_color_palette_signals [COLOR_CHANGED] =
		g_signal_new ("color_changed",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOColorPaletteClass, color_changed),
			      NULL, NULL,
			      go__VOID__INT_BOOLEAN_BOOLEAN_BOOLEAN,
			      G_TYPE_NONE, 4, G_TYPE_INT,
			      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	go_color_palette_signals [DISPLAY_CUSTOM_DIALOG] =
		g_signal_new ("display-custom-dialog",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOColorPaletteClass, display_custom_dialog),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, G_TYPE_OBJECT);

#ifdef HAVE_GTK_WIDGET_CLASS_SET_CSS_NAME
	gtk_widget_class_set_css_name ((GtkWidgetClass *)gobject_class, "colorpalette");
#endif
}

static void
go_color_palette_init (GObject *obj)
{
	g_object_set (obj, "orientation", GTK_ORIENTATION_VERTICAL, NULL);
	_go_gtk_widget_add_css_provider (GTK_WIDGET (obj));
}

GSF_CLASS (GOColorPalette, go_color_palette,
	   go_color_palette_class_init, go_color_palette_init,
	   GTK_TYPE_BOX)

/*
 * Find out if a color is in the default palette (not in the custom colors!)
 *
 * Utility function
 */
static gboolean
color_in_palette (GONamedColor const *set, GOColor color)
{
	int i;

	for (i = 0; set[i].name != NULL; i++)
		if (color == set[i].color)
			return TRUE;
	return FALSE;
}

static void
set_color (GOColorPalette *pal, GOColor color, gboolean is_custom,
	   gboolean by_user, gboolean is_default)
{
	if (is_default)
		color = pal->default_color;
	if (!color_in_palette (pal->default_set, color))
		go_color_group_add_color (pal->group, color);
	pal->selection = color;
	pal->current_is_custom = is_custom;
	pal->current_is_default = is_default;
	g_signal_emit (pal, go_color_palette_signals [COLOR_CHANGED], 0,
		       color, is_custom, by_user, is_default);
}

static void
cb_history_changed (GOColorPalette *pal)
{
	int i;
	GOColorGroup *group = pal->group;

	for (i = 0 ; i < GO_COLOR_GROUP_HISTORY_SIZE ; i++)
		g_object_set_data (G_OBJECT (pal->swatches[i]),
				   "color",
				   GUINT_TO_POINTER (group->history[i]));
}

static gboolean
cb_default_release_event (GtkWidget *button, GdkEventButton *event, GOColorPalette *pal)
{
	set_color (pal, pal->default_color, FALSE, TRUE, TRUE);
	return TRUE;
}

static void
swatch_activated (GOColorPalette *pal, GtkBin *button)
{
	GList *tmp = gtk_container_get_children (GTK_CONTAINER (gtk_bin_get_child (button)));
	GtkWidget *swatch = (tmp != NULL) ? tmp->data : NULL;
	GOColor color;

	g_list_free (tmp);

	g_return_if_fail (swatch != NULL);

	color = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (swatch),
						     "color"));
	set_color (pal, color, FALSE, TRUE, FALSE);
}

static gboolean
cb_swatch_release_event (GtkBin *button, GdkEventButton *event, GOColorPalette *pal)
{
/* FIXME FIXME FIXME TODO do I want to check for which button ? */
	swatch_activated (pal, button);
	return TRUE;
}

static gboolean
cb_swatch_key_press (GtkBin *button, GdkEventKey *event, GOColorPalette *pal)
{
	if (event->keyval == GDK_KEY_Return ||
	    event->keyval == GDK_KEY_KP_Enter ||
	    event->keyval == GDK_KEY_space) {
		swatch_activated (pal, button);
		return TRUE;
	} else
		return FALSE;
}

static gboolean
draw_color_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	GtkAllocation allocation;
	GOColor color = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
							     "color"));
	gtk_widget_get_allocation (widget, &allocation);
	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (color));
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill (cr);
	return TRUE;
}

/*
 * Create the individual color buttons
 *
 * Utility function
 */
static GtkWidget *
go_color_palette_button_new (GOColorPalette *pal, GtkGrid *grid,
			     GONamedColor const *color_name,
			     gint col, gint row)
{
	GtkWidget *button, *swatch, *box;

	swatch = gtk_drawing_area_new ();
	g_signal_connect (G_OBJECT (swatch), "draw", G_CALLBACK (draw_color_cb),
	                  NULL);
	g_object_set_data (G_OBJECT (swatch), "color",
			   GUINT_TO_POINTER (color_name->color));

	/* Wrap inside a vbox with a border so that we can see the focus indicator */
	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (swatch), TRUE, TRUE, 0);

	button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (button), box);
	gtk_widget_set_tooltip_text (button, _(color_name->name));

	gtk_grid_attach (grid, button, col, row, 1, 1);

	g_object_connect (button,
		"signal::button_release_event", G_CALLBACK (cb_swatch_release_event), pal,
		"signal::key_press_event", G_CALLBACK (cb_swatch_key_press), pal,
		NULL);

	if (!_go_gtk_new_theming ()) {
		gtk_widget_set_size_request (swatch, COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT);
		gtk_widget_set_halign (swatch, GTK_ALIGN_FILL);
		gtk_widget_set_valign (swatch, GTK_ALIGN_FILL);
		gtk_container_set_border_width (GTK_CONTAINER (box), 2);
	}

	return swatch;
}

static void
cb_combo_custom_response (GtkDialog *dialog,
			  gint response_id, GOColorPalette *pal)
{
	GOColor c;
	if (handle_color_sel (dialog, response_id, &c))
		set_color (pal, c, TRUE, TRUE, FALSE);
}

static void
cb_combo_custom_clicked (GtkWidget *button, GOColorPalette *pal)
{
	GtkWidget *dialog = create_color_sel
		(button,
		 G_OBJECT (pal), pal->selection,
		 G_CALLBACK (cb_combo_custom_response),
		 pal->allow_alpha);
	g_signal_emit (pal, go_color_palette_signals [DISPLAY_CUSTOM_DIALOG], 0,
		dialog);
	gtk_widget_show_all (dialog);
}

void
go_color_palette_set_title (GOColorPalette *pal, char const *title)
{
	g_object_set_data_full (G_OBJECT (pal), "title",
		g_strdup (title), g_free);
}

/**
 * go_color_palette_set_group:
 * @p: #GOColorPalette
 * @cg: #GOColorGroup
 *
 **/
void
go_color_palette_set_group (GOColorPalette *p, GOColorGroup *cg)
{
	if (p->group == cg)
		return;

	if (p->group) {
		g_signal_handlers_disconnect_by_func (
			G_OBJECT (p->group),
			G_CALLBACK (cb_history_changed), p);
		g_object_unref (p->group);
		p->group = NULL;
	}
	if (cg != NULL) {
		g_object_ref (cg);
		p->group = cg;
		g_signal_connect_swapped (G_OBJECT (cg),
			"history-changed",
			G_CALLBACK (cb_history_changed), p);
	}
}

static GtkWidget *
go_color_palette_setup (GOColorPalette *pal,
			char const *no_color_label,
			int cols, int rows,
			GONamedColor const *color_names)
{
	GtkWidget	*w, *grid;
	int pos, row, col = 0;

	grid = gtk_grid_new ();

	if (no_color_label != NULL) {
		w = gtk_button_new_with_label (no_color_label);
		gtk_widget_set_hexpand (w, TRUE);
		gtk_grid_attach (GTK_GRID (grid), w, 0, 0, cols, 1);
		g_signal_connect (w,
			"button_release_event",
			G_CALLBACK (cb_default_release_event), pal);
	}

	for (row = 0; row < rows; row++)
		for (col = 0; col < cols; col++) {
			pos = row * cols + col;
			if (color_names [pos].name == NULL)
				goto custom_colors;
			go_color_palette_button_new ( pal,
				GTK_GRID (grid),
				&(color_names [pos]), col, row + 1);
		}

custom_colors :
	if (col > 0)
		row++;
	for (col = 0; col < cols && col < GO_COLOR_GROUP_HISTORY_SIZE; col++) {
		GONamedColor color_name = { 0, N_("custom") };
		color_name.color = pal->group->history [col];
		pal->swatches[col] = go_color_palette_button_new (pal,
			GTK_GRID (grid),
			&color_name, col, row + 1);
	}

	w = go_gtk_button_build_with_stock (_("Custom color..."),
		GTK_STOCK_SELECT_COLOR);
	gtk_button_set_alignment (GTK_BUTTON (w), 0., .5);
	gtk_widget_set_hexpand (w, TRUE);
	gtk_grid_attach (GTK_GRID (grid), w, 0, row + 2, cols, 1);
	g_signal_connect (G_OBJECT (w),
		"clicked",
		G_CALLBACK (cb_combo_custom_clicked), pal);

	return grid;
}

void
go_color_palette_set_color_to_default (GOColorPalette *pal)
{
	set_color (pal, pal->default_color, FALSE, TRUE, TRUE);
}

void
go_color_palette_set_current_color (GOColorPalette *pal, GOColor color)
{
	set_color (pal, color,
		   color_in_palette (pal->default_set, color),
		   FALSE, FALSE);
}

GOColor
go_color_palette_get_current_color (GOColorPalette *pal,
				    gboolean *is_default, gboolean *is_custom)
{
	if (is_default != NULL)
		*is_default = pal->current_is_default;
	if (is_custom != NULL)
		*is_custom = pal->current_is_custom;
	return pal->selection;
}

void
go_color_palette_set_allow_alpha (GOColorPalette *pal, gboolean allow_alpha)
{
	pal->allow_alpha = allow_alpha;
}

/**
 * go_color_palette_new:
 * @no_color_label: color label
 * @default_color: #GOColor
 * @cg: #GOColorGroup
 *
 * Returns: A new palette
 **/
GtkWidget *
go_color_palette_new (char const *no_color_label,
		      GOColor default_color,
		      GOColorGroup *cg)
{
	GOColorPalette *pal;
	int const cols = 8;
	int const rows = 6;
	GONamedColor const *color_names = default_color_set;

	pal = g_object_new (GO_TYPE_COLOR_PALETTE, NULL);

	pal->default_set   = color_names;
	pal->default_color = default_color;
	pal->selection	   = default_color;
	pal->current_is_custom  = FALSE;
	pal->current_is_default = TRUE;
	go_color_palette_set_group (pal, cg);

	gtk_container_add (GTK_CONTAINER (pal),
		go_color_palette_setup (pal, no_color_label, cols, rows,
				     pal->default_set));
	return GTK_WIDGET (pal);
}


/***********************************************************************/

typedef struct {
	GtkMenu base;
	gboolean allow_alpha;
	GOColor  selection, default_color;
} GOMenuColor;

typedef struct {
	GtkMenuClass	base;
	void (* color_changed) (GOMenuColor *menu, GOColor color,
				gboolean custom, gboolean by_user, gboolean is_default);
	void (*display_custom_dialog) (GOColorPalette *pal, GtkWidget *dialog);
} GOMenuColorClass;

static guint go_menu_color_signals [LAST_SIGNAL] = { 0, };

static void
go_menu_color_class_init (GObjectClass *gobject_class)
{
	go_menu_color_signals [COLOR_CHANGED] =
		g_signal_new ("color_changed",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOMenuColorClass, color_changed),
			      NULL, NULL,
			      go__VOID__INT_BOOLEAN_BOOLEAN_BOOLEAN,
			      G_TYPE_NONE, 4, G_TYPE_INT,
			      G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	go_menu_color_signals [DISPLAY_CUSTOM_DIALOG] =
		g_signal_new ("display-custom-dialog",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOColorPaletteClass, display_custom_dialog),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, G_TYPE_OBJECT);
}

static GSF_CLASS (GOMenuColor, go_menu_color,
		  go_menu_color_class_init, NULL,
		  GTK_TYPE_MENU)

static void
cb_menu_item_toggle_size_request (GtkWidget *item, gint *requitision)
{
	*requitision = 1;
}

static GtkWidget *
make_colored_menu_item (char const *label, GOColor c)
{
	GtkWidget *button;
	GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
		COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT);
	gdk_pixbuf_fill (pixbuf, c);

	if (label && 0 == strcmp (label, " ")) {
		/* color buttons are created with a label of " " */
		button = gtk_menu_item_new ();
		gtk_container_add (GTK_CONTAINER (button),
			gtk_image_new_from_pixbuf (pixbuf));
	} else {
		button = gtk_image_menu_item_new_with_label (label);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (button),
			gtk_image_new_from_pixbuf (pixbuf));
	}
	g_object_unref (pixbuf);
	gtk_widget_show_all (button);

	g_object_set_data (G_OBJECT (button), "go_color", GINT_TO_POINTER (c));

	/* Workaround for bug http://bugzilla.gnome.org/show_bug.cgi?id=585421 */
	g_signal_connect (button, "toggle-size-request", G_CALLBACK (cb_menu_item_toggle_size_request), NULL);

	return button;
}

static void
cb_menu_default_activate (GtkWidget *button, GOMenuColor *menu)
{
	menu->selection = menu->default_color;
	g_signal_emit (menu, go_menu_color_signals [COLOR_CHANGED], 0,
		       menu->selection, FALSE, TRUE, TRUE);
}

static void
cb_menu_color_activate (GtkWidget *button, GOMenuColor *menu)
{
	GOColor color = GPOINTER_TO_INT (
		g_object_get_data (G_OBJECT (button), "go_color"));
	menu->selection = color;
	g_signal_emit (menu, go_menu_color_signals [COLOR_CHANGED], 0,
		       color, FALSE, TRUE, FALSE);
}
static void
cb_menu_custom_response (GtkDialog *dialog,
			 gint response_id, GOMenuColor *menu)
{
	GOColor c;
	if (handle_color_sel (dialog, response_id, &c)) {
		menu->selection = c;
		g_signal_emit (menu, go_menu_color_signals [COLOR_CHANGED], 0,
			c, TRUE, TRUE, FALSE);
	}
}

static void
cb_menu_custom_activate (GtkWidget *button, GOMenuColor *menu)
{
	GtkWidget *dialog = create_color_sel
		(button,
		 G_OBJECT (menu), menu->selection,
		 G_CALLBACK (cb_menu_custom_response),
		 menu->allow_alpha);
	g_signal_emit (menu, go_menu_color_signals[DISPLAY_CUSTOM_DIALOG], 0,
		dialog);
	gtk_widget_show_all (dialog);
}

/**
 * go_color_palette_make_menu:
 * @no_color_label: color label
 * @default_color: #GOColor
 * @cg: #GOColorGroup
 * @custom_dialog_title: optional string
 * @current_color: #GOColor
 *
 * Returns: (transfer full): a submenu with a palette of colours.  Caller is responsible for
 * 	creating an item to point to the submenu.
 **/
GtkWidget *
go_color_palette_make_menu (char const *no_color_label,
			    GOColor default_color,
			    GOColorGroup *cg,
			    char const *custom_dialog_title,
			    GOColor current_color)
{
	int cols = 8;
	int rows = 6;
	int col, row, pos, table_row = 0;
	GONamedColor const *color_names = default_color_set;
        GtkWidget *w, *submenu;

	submenu = g_object_new (go_menu_color_get_type (), NULL);

	if (no_color_label != NULL) {
		w = make_colored_menu_item (no_color_label, default_color);
		gtk_menu_attach (GTK_MENU (submenu), w, 0, cols, 0, 1);
		g_signal_connect (G_OBJECT (w),
			"activate",
			G_CALLBACK (cb_menu_default_activate), submenu);
		table_row++;
	}
	for (row = 0; row < rows; row++, table_row++) {
		for (col = 0; col < cols; col++) {
			pos = row * cols + col;
			if (color_names [pos].name == NULL)
				goto custom_colors;
			w = make_colored_menu_item (" ",
				color_names [pos].color);
			gtk_widget_set_tooltip_text (w, _(color_names[pos].name));
			gtk_menu_attach (GTK_MENU (submenu), w,
				col, col+1, table_row, table_row+1);
			g_signal_connect (G_OBJECT (w),
				"activate",
				G_CALLBACK (cb_menu_color_activate), submenu);
		}
	}

custom_colors :
	if (col > 0)
		row++;
	for (col = 0; col < cols && col < GO_COLOR_GROUP_HISTORY_SIZE; col++) {
		w = make_colored_menu_item (" ", cg->history[col]);
		gtk_menu_attach (GTK_MENU (submenu), w,
			col, col+1, table_row, table_row+1);
		g_signal_connect (G_OBJECT (w),
			"activate",
			G_CALLBACK (cb_menu_color_activate), submenu);
	}
	w = gtk_image_menu_item_new_with_label (_("Custom color..."));
	/* Workaround for bug http://bugzilla.gnome.org/show_bug.cgi?id=585421 */
	/* We can't have an image in one of the gtk_menu_item, it would lead to an
	   ugly item spacing. */
	/* gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (w),*/
	/* 	gtk_image_new_from_stock (GTK_STOCK_SELECT_COLOR, GTK_ICON_SIZE_MENU));*/
	gtk_widget_show_all (w);
	gtk_menu_attach (GTK_MENU (submenu), w, 0, cols, row + 2, row + 3);
	g_signal_connect (G_OBJECT (w),
		"activate",
		G_CALLBACK (cb_menu_custom_activate), submenu);

	((GOMenuColor *)submenu)->selection = current_color;
	((GOMenuColor *)submenu)->default_color = default_color;
	g_object_set_data_full (G_OBJECT (submenu), "title",
		g_strdup (custom_dialog_title), g_free);

	gtk_widget_show (submenu);

	return submenu;
}
