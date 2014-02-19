/*
 * go-font-sel.h - Misc GTK+ utilities
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
#ifndef _GO_FONT_SEL_H_
#define _GO_FONT_SEL_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_FONT_SEL	(go_font_sel_get_type ())
#define GO_FONT_SEL(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), GO_TYPE_FONT_SEL, GOFontSel))
#define GO_IS_FONT_SEL(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GO_TYPE_FONT_SEL))

typedef struct _GOFontSel GOFontSel;

GType         go_font_sel_get_type (void);
GtkWidget    *go_font_sel_new      (void);
void	      go_font_sel_set_font (GOFontSel *fs, GOFont const *font);
GOFont const *go_font_sel_get_font (GOFontSel const *fs);
void go_font_sel_editable_enters   (GOFontSel *fs, GtkWindow *dialog);
void go_font_sel_set_sample_text   (GOFontSel *fs, char const *text);

PangoAttrList *go_font_sel_get_sample_attributes (GOFontSel *fs);
void go_font_sel_set_sample_attributes (GOFontSel *fs,
					PangoAttrList *attrs);

void go_font_sel_set_family        (GOFontSel *fs, char const *family);
void go_font_sel_set_size          (GOFontSel *fs, int size);
void go_font_sel_set_style         (GOFontSel *fs,
				    PangoWeight weight, PangoStyle style);
void go_font_sel_set_strikethrough (GOFontSel *fs, gboolean strikethrough);
void go_font_sel_set_color         (GOFontSel *gfs, GOColor c,
				    gboolean is_default);
GOColor go_font_sel_get_color	   (GOFontSel *gfs);
void go_font_sel_set_script        (GOFontSel *fs, GOFontScript script);

void go_font_sel_set_font_desc     (GOFontSel *fs, PangoFontDescription *desc);
PangoFontDescription *go_font_sel_get_font_desc (GOFontSel *fs);

G_END_DECLS

#endif /* _GO_FONT_SEL_H_ */
