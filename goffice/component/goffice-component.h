/*
 * goffice-component.h :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GOFFICE_COMPONENT_H
#define GOFFICE_COMPONENT_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GOComponent GOComponent;
typedef struct _GOComponentType GOComponentType;
typedef struct _GOComponentMimeDialog GOComponentMimeDialog;

G_END_DECLS

#include <goffice/component/go-component.h>
#include <goffice/component/go-component-factory.h>
#include <goffice/component/go-component-mime-dialog.h>

#endif	/* GOFFICE_COMPONENT_H */
