/*
 * gog-equation.h :
 *
 * Copyright (C) 2008 Emmanuel Pacaud <emmanuel@gnome.org>
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

#ifndef GOG_EQUATION_H
#define GOG_EQUATION_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GOG_TYPE_EQUATION		(gog_equation_get_type ())
#define GOG_EQUATION(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_EQUATION, GogEquation))
#define GOG_IS_EQUATION(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_EQUATION))
#define GOG_EQUATION_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_EQUATION, GogEquationClass))

GType gog_equation_get_type (void);

G_END_DECLS

#endif /* GOG_EQUATION_H */

