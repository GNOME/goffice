/*
 * gog-marker-selector.h :
 *
 * Copyright (C) 2003-2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
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

#ifndef GO_MARKER_SELECTOR_H
#define GO_MARKER_SELECTOR_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

GtkWidget	*go_marker_selector_new		(GOMarkerShape initial_shape,
						 GOMarkerShape default_shape);
void 		 go_marker_selector_set_colors  (GOSelector *selector,
						 GOColor outline,
						 GOColor fill);
void		 go_marker_selector_set_shape   (GOSelector *selector,
						 GOMarkerShape shape);
void		 go_marker_selector_set_auto_fill (GOSelector *selector,
		                 gboolean auto_fill);

G_END_DECLS

#endif /* GO_MARKER_SELECTOR_H */
