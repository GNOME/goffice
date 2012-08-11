/*
 * gog-gradient-selector.h :
 *
 * Copyright (C) 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#ifndef GO_GRADIENT_SELECTOR_H
#define GO_GRADIENT_SELECTOR_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

GtkWidget	*go_selector_new_gradient		(GOGradientDirection initial_direction,
							 GOGradientDirection default_direction);
#ifndef GOFFICE_DISABLE_DEPRECATED
GOFFICE_DEPRECATED_FOR(go_selector_new_gradient)
GtkWidget	*go_gradient_selector_new		(GOGradientDirection initial_direction,
							 GOGradientDirection default_direction);
#endif
void 		 go_gradient_selector_set_colors  	(GOSelector *selector,
							 GOColor start,
							 GOColor stop);

G_END_DECLS

#endif /* GO_GRADIENT_SELECTOR_H */
