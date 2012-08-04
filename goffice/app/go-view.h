/*
 * go-view.h :  A GOffice base view
 *
 * Copyright (C) 2012 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#ifndef GO_VIEW_H
#define GO_VIEW_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GoView {
	GObject base;
};

#define GO_TYPE_VIEW    (go_view_get_type ())
#define GO_VIEW(o)	    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_VIEW, GoView))
#define GO_IS_VIEW(o)	    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_VIEW))

GType go_view_get_type (void);

G_END_DECLS

#endif /* GO_VIEW_H */
