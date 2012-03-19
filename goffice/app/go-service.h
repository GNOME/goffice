/*
 * go-service.h : A GOffice plugin
 *
 * Copyright (C) 2004 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_SERVICE_H
#define GO_SERVICE_H

#include <goffice/app/goffice-app.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GO_TYPE_SERVICE         (go_service_get_type ())
#define GO_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_SERVICE, GOService))
#define GO_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_SERVICE))

GType     go_service_get_type   (void);
GOPlugin *go_service_get_plugin (GOService const *service);

#define GO_TYPE_SERVICE_SIMPLE	(go_service_simple_get_type ())
#define GO_SERVICE_SIMPLE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_SERVICE_SIMPLE, GOServiceSimple))
#define GO_IS_SERVICE_SIMPLE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_SERVICE_SIMPLE))
typedef struct _GOServiceSimple GOServiceSimple;
GType go_service_simple_get_type (void);

G_END_DECLS

#endif /* GO_SERVICE_H */
