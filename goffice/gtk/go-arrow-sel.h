/*
 * go-arrow-sel.h - Selector for GOArrow
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
#ifndef _GO_ARROW_SEL_H_
#define _GO_ARROW_SEL_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_ARROW_SEL	(go_arrow_sel_get_type ())
#define GO_ARROW_SEL(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), GO_TYPE_ARROW_SEL, GOArrowSel))
#define GO_IS_ARROW_SEL(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GO_TYPE_ARROW_SEL))

typedef struct _GOArrowSel GOArrowSel;

GType         go_arrow_sel_get_type (void);
GtkWidget    *go_arrow_sel_new      (void);

void	      go_arrow_sel_set_arrow (GOArrowSel *as, GOArrow const *arrow);
GOArrow const *go_arrow_sel_get_arrow (GOArrowSel const *as);

G_END_DECLS

#endif /* _GO_ARROW_SEL_H_ */
