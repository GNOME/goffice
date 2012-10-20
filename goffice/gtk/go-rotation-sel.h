/*
 * go-rotation-sel.h - Select a text orientation
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
#ifndef _GO_ROTATION_SEL_H_
#define _GO_ROTATION_SEL_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_ROTATION_SEL	(go_rotation_sel_get_type ())
#define GO_ROTATION_SEL(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), GO_TYPE_ROTATION_SEL, GORotationSel))
#define GO_IS_ROTATION_SEL(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GO_TYPE_ROTATION_SEL))

typedef struct _GORotationSel GORotationSel;

GType      go_rotation_sel_get_type (void);
GtkWidget *go_rotation_sel_new      (void);
GtkWidget *go_rotation_sel_new_full (void);
void	   go_rotation_sel_set_rotation (GORotationSel *rs, int degrees);
int	   go_rotation_sel_get_rotation (GORotationSel const *rs);

G_END_DECLS

#endif /* _GO_ROTATION_SEL_H_ */
