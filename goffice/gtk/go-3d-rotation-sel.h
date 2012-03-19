/*
 * go-3d-rotation-sel.h - Select a rotation angles
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
#ifndef _GO_3D_ROTATION_SEL_H_
#define _GO_3D_ROTATION_SEL_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_3D_ROTATION_SEL	(go_3d_rotation_sel_get_type ())
#define GO_3D_ROTATION_SEL(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                 GO_TYPE_3D_ROTATION_SEL, GO3DRotationSel))
#define GO_IS_3D_ROTATION_SEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                    GO_TYPE_3D_ROTATION_SEL))

typedef struct _GO3DRotationSel GO3DRotationSel;

GType      go_3d_rotation_sel_get_type (void);
GtkWidget *go_3d_rotation_sel_new (void);
void       go_3d_rotation_sel_set_matrix (GO3DRotationSel *rs, GOMatrix3x3 *mat);
void       go_3d_rotation_sel_set_fov (GO3DRotationSel *rs, double fov);
void       go_3d_rotation_sel_get_matrix (GO3DRotationSel const *rs, GOMatrix3x3 *mat);
double     go_3d_rotation_sel_get_psi (GO3DRotationSel const *rs);
double     go_3d_rotation_sel_get_theta (GO3DRotationSel const *rs);
double     go_3d_rotation_sel_get_phi (GO3DRotationSel const *rs);
double     go_3d_rotation_sel_get_fov (GO3DRotationSel const *rs);

G_END_DECLS

#endif /* _GO_3D_ROTATION_SEL_H_ */
