/*
 * go-pattern-selector.h
 *
 * Copyright (C) 2006-2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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

#ifndef GO_PATTERN_SELECTOR_H
#define GO_PATTERN_SELECTOR_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

GtkWidget 	*go_pattern_selector_new	(GOPatternType initial_type,
						 GOPatternType default_type);
void 		 go_pattern_selector_set_colors (GOSelector *selector,
						 GOColor foreground,
						 GOColor background);

G_END_DECLS

#endif /* GO_PATTERN_SELECTOR_H */
