/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-widget-impl.h :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef GOC_WIDGET_IMPL_H
#define GOC_WIDGET_IMPL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocWidget {
	GocItem base;

	double x, y, w, h;
	GtkWidget *widget;
};

typedef GocItemClass GocWidgetClass;

G_END_DECLS

#endif  /* GOC_WIDGET_IMPL_H */