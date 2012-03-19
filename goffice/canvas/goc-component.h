/*
 * goc-component.h :
 *
 * Copyright (C) 2010 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_COMPONENT_H
#define GOC_COMPONENT_H

#include <goffice/goffice.h>
#include <goffice/component/goffice-component.h>

G_BEGIN_DECLS

#define GOC_TYPE_COMPONENT	(goc_component_get_type ())
#define GOC_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_COMPONENT, GocComponent))
#define GOC_IS_COMPONENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_COMPONENT))

GType goc_component_get_type (void);

GOComponent * goc_component_get_object (GocComponent *component);

G_END_DECLS

#endif  /* GOC_COMPONENT_H */
