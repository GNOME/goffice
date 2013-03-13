/*
 * go-font-sel-dialog.h - font selector dialog
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
#ifndef _GO_FONT_SEL_DIALOG_H_
#define _GO_FONT_SEL_DIALOG_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_FONT_SEL_DIALOG	(go_font_sel_dialog_get_type ())
#define GO_FONT_SEL_DIALOG(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), GO_TYPE_FONT_SEL_DIALOG, GOFontSelDialog))
#define GO_IS_FONT_SEL_DIALOG(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GO_TYPE_FONT_SEL_DIALOG))

typedef struct GOFontSelDialog_ GOFontSelDialog;

GType         go_font_sel_dialog_get_type (void);
GtkWidget    *go_font_sel_dialog_new      (void);

G_END_DECLS

#endif /* _GO_FONT_SEL_DIALOG_H_ */
