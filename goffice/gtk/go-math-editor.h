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
#ifndef _GO_MATH_EDITOR_H_
#define _GO_MATH_EDITOR_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_MATH_EDITOR	(go_math_editor_get_type ())
#define GO_MATH_EDITOR(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), GO_TYPE_MATH_EDITOR, GoMathEditor))
#define GO_IS_MATH_EDITOR(obj)  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GO_TYPE_MATH_EDITOR))

typedef struct _GoMathEditor GoMathEditor;

GType         go_math_editor_get_type (void);
GtkWidget    *go_math_editor_new      (void);

gboolean go_math_editor_get_inline (GoMathEditor const *gme);
char *go_math_editor_get_itex (GoMathEditor const *gme);
char *go_math_editor_get_mathml (GoMathEditor const *gme);
void go_math_editor_set_inline (GoMathEditor *gme, gboolean mode);
void go_math_editor_set_itex (GoMathEditor *gme, char const *text);
void go_math_editor_set_mathml (GoMathEditor *gme, char const *text);

G_END_DECLS

#endif /* _GO_MATH_EDITOR_H_ */
