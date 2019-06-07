/*
 * A math editor widget.
 *
 * Copyright 2014 by Jean Brefort (jean.brefort@normalesup.org)
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

#include <glib/gi18n-lib.h>

struct _GoMathEditor {
	GtkBin		base;
	GtkTextBuffer *itex;
	GtkDrawingArea *preview;
	GtkToggleButton *inline_btn;
};

typedef struct {
	GtkBinClass base;

	/* signals */
	void (* itex_changed) (GoMathEditor *gme);
	void (* inline_changed) (GoMathEditor *gme);
} GoMathEditorClass;

enum {
	ITEX_CHANGED,
	INLINE_CHANGED,
	LAST_SIGNAL
};

static guint gme_signals[LAST_SIGNAL] = { 0 };

static void
go_math_editor_class_init (GtkWidgetClass *klass)
{

	gme_signals[ITEX_CHANGED] =
		g_signal_new (
			"itex-changed",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GoMathEditorClass, itex_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
	gme_signals[INLINE_CHANGED] =
		g_signal_new (
			"inline-changed",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GoMathEditorClass, inline_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
}

static void
inline_toggled_cb (G_GNUC_UNUSED GtkToggleButton *button, GoMathEditor *gme)
{
	g_signal_emit (gme, gme_signals[INLINE_CHANGED], 0);
}

static void
itex_changed_cb (G_GNUC_UNUSED GtkTextBuffer *buffer, GoMathEditor *gme)
{
	g_signal_emit (gme, gme_signals[ITEX_CHANGED], 0);
}

static void
go_math_editor_init (GoMathEditor *gme)
{
	GtkWidget *w, *scroll;
	GtkGrid *grid = GTK_GRID (gtk_grid_new ());
	gtk_container_add (GTK_CONTAINER (gme), (GtkWidget *) grid);
	g_object_set ((GObject *) grid, "border-width", 12, "row-spacing", 6, "column-spacing", 12, NULL);
	w = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (w), GTK_WRAP_WORD_CHAR);
	gme->itex = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
	g_signal_connect (gme->itex, "changed", G_CALLBACK (itex_changed_cb), gme);
	scroll = gtk_scrolled_window_new (NULL, NULL);
	g_object_set (scroll,
	              "shadow_type", GTK_SHADOW_ETCHED_IN,
	              "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
	              "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
	              "expand", TRUE,
	              NULL);
	gtk_container_add (GTK_CONTAINER (scroll), w);
	gtk_grid_attach (grid, scroll, 0, 0, 1, 1);
	w = gtk_check_button_new_with_mnemonic (_("_Compact mode"));
	gme->inline_btn = GTK_TOGGLE_BUTTON (w);
	gtk_grid_attach (grid, w, 0, 2, 1, 1);
	g_signal_connect (w, "toggled", G_CALLBACK (inline_toggled_cb), gme);
	gtk_widget_show_all ((GtkWidget *) gme);
}

GSF_CLASS (GoMathEditor, go_math_editor,
	go_math_editor_class_init, go_math_editor_init,
	GTK_TYPE_BIN)

GtkWidget *
go_math_editor_new (void)
{
	return g_object_new (GO_TYPE_MATH_EDITOR, NULL);
}

gboolean
go_math_editor_get_inline (GoMathEditor const *gme)
{
	g_return_val_if_fail (GO_IS_MATH_EDITOR (gme), FALSE);
	return gtk_toggle_button_get_active (gme->inline_btn);
}

char *
go_math_editor_get_itex (GoMathEditor const *gme)
{
	GtkTextIter start;
	GtkTextIter end;

	g_return_val_if_fail (GO_IS_MATH_EDITOR (gme), NULL);
	gtk_text_buffer_get_bounds (gme->itex, &start, &end);
	return gtk_text_buffer_get_text (gme->itex, &start, &end, TRUE);
}

char *
go_math_editor_get_mathml (GoMathEditor const *gme)
{
	g_return_val_if_fail (GO_IS_MATH_EDITOR (gme), NULL);
	return NULL; // FIXME
}

void go_math_editor_set_inline (GoMathEditor *gme, gboolean mode)
{
	g_return_if_fail (GO_IS_MATH_EDITOR (gme));
	gtk_toggle_button_set_active (gme->inline_btn, mode);
}

void
go_math_editor_set_itex (GoMathEditor *gme, char const *text)
{
	g_return_if_fail (GO_IS_MATH_EDITOR (gme));
	gtk_text_buffer_set_text (gme->itex, (text != NULL)? text: "", -1);
}

void
go_math_editor_set_mathml (GoMathEditor *gme, char const *text)
{
	g_return_if_fail (GO_IS_MATH_EDITOR (gme));
}
