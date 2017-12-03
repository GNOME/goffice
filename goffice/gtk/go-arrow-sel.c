/*
 * An arrow selector widget.
 *
 * Copyright 2015 by Morten Welinder (terra@gnome.org)
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

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


struct _GOArrowSel {
	GtkBin base;
	GtkBuilder *gui;
	GOArrow arrow;

	GtkWidget *type_selector;
	GtkSpinButton *spin_a, *spin_b, *spin_c;
	GtkWidget *preview;
};

typedef struct {
	GtkBinClass parent_class;
} GOArrowSelClass;

enum {
	PROP_0,
	PROP_ARROW
};

static GObjectClass *as_parent_class;

static void
set_actives (GOArrowSel *as)
{
	static guint8 active_sliders[3] = { 0, 7, 3 };

	gtk_widget_set_sensitive (GTK_WIDGET (as->spin_a),
				  (active_sliders[as->arrow.typ] & 1) != 0);
	gtk_widget_set_sensitive (GTK_WIDGET (as->spin_b),
				  (active_sliders[as->arrow.typ] & 2) != 0);
	gtk_widget_set_sensitive (GTK_WIDGET (as->spin_c),
				  (active_sliders[as->arrow.typ] & 4) != 0);
}

static void
cb_changed (GOArrowSel *as)
{
	GOArrow arr = as->arrow;
	int idx;

	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (as->type_selector));
	if (idx >= 0)
		arr.typ = idx;
	arr.a = gtk_spin_button_get_value (as->spin_a);
	arr.b = gtk_spin_button_get_value (as->spin_b);
	arr.c = gtk_spin_button_get_value (as->spin_c);

	go_arrow_sel_set_arrow (as, &arr);
}

static gboolean
cb_draw_arrow (GtkWidget *widget, cairo_t *cr, GOArrowSel *as)
{
	guint width = gtk_widget_get_allocated_width (widget);
	guint height = gtk_widget_get_allocated_height (widget);
	double dx, dy;
	double x = width / 2;
	double y1 = height / 4, y2 = height * 3 / 4;

	cairo_save (cr);
	cairo_translate (cr, x, y2);
	/* cairo_scale (cr, 2, 2); */
	/* cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->line.color)); */
	go_arrow_draw (&as->arrow, cr, &dx, &dy, 0);
	cairo_restore (cr);

	cairo_move_to (cr, x, y1);
	cairo_line_to (cr, x + dx, y2 + dy);
	cairo_stroke (cr);

	return FALSE;
}

static GObject*
go_arrow_sel_constructor (GType type,
			  guint n_construct_properties,
			  GObjectConstructParam *construct_params)
{
	GtkWidget *arrowsel;
	GOArrowSel *as = (GOArrowSel *)
		(as_parent_class->constructor (type,
						n_construct_properties,
						construct_params));

	if (!as)
		return NULL;

	as->gui = go_gtk_builder_load_internal ("res:go:gtk/go-arrow-sel.ui", GETTEXT_PACKAGE, NULL);
	as->spin_a = GTK_SPIN_BUTTON (go_gtk_builder_get_widget (as->gui, "spin_a"));
	g_signal_connect_swapped (as->spin_a, "value-changed", G_CALLBACK (cb_changed), as);
	as->spin_b = GTK_SPIN_BUTTON (go_gtk_builder_get_widget (as->gui, "spin_b"));
	g_signal_connect_swapped (as->spin_b, "value-changed", G_CALLBACK (cb_changed), as);
	as->spin_c = GTK_SPIN_BUTTON (go_gtk_builder_get_widget (as->gui, "spin_c"));
	g_signal_connect_swapped (as->spin_c, "value-changed", G_CALLBACK (cb_changed), as);

	as->type_selector = go_gtk_builder_get_widget (as->gui, "type-selector");
	gtk_combo_box_set_active (GTK_COMBO_BOX (as->type_selector), GO_ARROW_NONE);
	set_actives (as);
	g_signal_connect_swapped (as->type_selector, "changed", G_CALLBACK (cb_changed), as);

	as->preview = go_gtk_builder_get_widget (as->gui, "preview");
	g_signal_connect (G_OBJECT (as->preview), "draw",
			  G_CALLBACK (cb_draw_arrow), as);

	arrowsel = go_gtk_builder_get_widget (as->gui, "arrow-selector");
	gtk_widget_show_all (arrowsel);
	gtk_container_add (GTK_CONTAINER (as), arrowsel);

	return (GObject *)as;
}

static void
go_arrow_sel_dispose (GObject *obj)
{
	GOArrowSel *as = GO_ARROW_SEL (obj);
	g_clear_object (&as->gui);
	as_parent_class->dispose (obj);
}

static void
go_arrow_sel_get_property (GObject         *object,
			   guint            prop_id,
			   GValue          *value,
			   GParamSpec      *pspec)
{
	GOArrowSel *as = GO_ARROW_SEL (object);

	switch (prop_id) {
	case PROP_ARROW:
		g_value_set_boxed (value, &as->arrow);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
go_arrow_sel_set_property (GObject         *object,
			   guint            prop_id,
			   const GValue    *value,
			   GParamSpec      *pspec)
{
	GOArrowSel *as = GO_ARROW_SEL (object);

	switch (prop_id) {
	case PROP_ARROW:
		go_arrow_sel_set_arrow (as, g_value_peek_pointer (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
go_arrow_sel_init (GObject *object)
{
	GOArrowSel *as = GO_ARROW_SEL (object);
	go_arrow_clear (&as->arrow);
}

static void
go_arrow_sel_class_init (GObjectClass *klass)
{
	klass->constructor = go_arrow_sel_constructor;
	klass->dispose = go_arrow_sel_dispose;
	klass->get_property = go_arrow_sel_get_property;
	klass->set_property = go_arrow_sel_set_property;

	as_parent_class = g_type_class_peek_parent (klass);

	g_object_class_install_property
		(klass, PROP_ARROW,
                 g_param_spec_boxed ("arrow",
				     _("Arrow"),
				     _("The currently selected arrow"),
				     GO_ARROW_TYPE,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));

#ifdef HAVE_GTK_WIDGET_CLASS_SET_CSS_NAME
	gtk_widget_class_set_css_name ((GtkWidgetClass *)klass, "arrowselector");
#endif
}

GSF_CLASS (GOArrowSel, go_arrow_sel,
	   go_arrow_sel_class_init, go_arrow_sel_init, GTK_TYPE_BIN)
#if 0
;
#endif


GtkWidget *
go_arrow_sel_new (void)
{
	return g_object_new (GO_TYPE_ARROW_SEL, NULL);
}


GOArrow const *
go_arrow_sel_get_arrow (GOArrowSel const *as)
{
	g_return_val_if_fail (GO_IS_ARROW_SEL (as), NULL);

	return &as->arrow;
}

void
go_arrow_sel_set_arrow (GOArrowSel *as, GOArrow const *arrow)
{
	g_return_if_fail (GO_IS_ARROW_SEL (as));
	g_return_if_fail (arrow != NULL);

	if (go_arrow_equal (arrow, &as->arrow))
		return;

	g_object_freeze_notify (G_OBJECT (as));
	as->arrow = *arrow;
	g_object_notify (G_OBJECT (as), "arrow");
	gtk_combo_box_set_active (GTK_COMBO_BOX (as->type_selector), arrow->typ);
	gtk_spin_button_set_value (as->spin_a, arrow->a);
	gtk_spin_button_set_value (as->spin_b, arrow->b);
	gtk_spin_button_set_value (as->spin_c, arrow->c);
	set_actives (as);
	g_object_thaw_notify (G_OBJECT (as));

	gtk_widget_queue_draw (as->preview);
}
