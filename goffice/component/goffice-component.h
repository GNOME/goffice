/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goffice-component.h :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef GOFFICE_COMPONENT_H
#define GOFFICE_COMPONENT_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GOComponent GOComponent;
typedef struct _GOComponentType GOComponentType;

#include <goffice/component/go-component.h>
#include <goffice/component/go-component-factory.h>
G_END_DECLS

#endif	/* GOFFICE_COMPONENT_H */
