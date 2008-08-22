/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-gradient.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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

#include <goffice/goffice-config.h>
#include "go-gradient.h"
#include "go-color.h"

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/go-combo-pixmaps.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#endif

#include <glib/gi18n-lib.h>
#include <string.h>


static char const *grad_dir_names[] = {
	"n-s",
	"s-n",
	"n-s-mirrored",
	"s-n-mirrored",
	"w-e",
	"e-w",
	"w-e-mirrored",
	"e-w-mirrored",
	"nw-se",
	"se-nw",
	"nw-se-mirrored",
	"se-nw-mirrored",
	"ne-sw",
	"sw-ne",
	"sw-ne-mirrored",
	"ne-sw-mirrored"
};

GOGradientDirection
go_gradient_dir_from_str (char const *name)
{
	unsigned i;
	for (i = 0; i < GO_GRADIENT_MAX; i++)
		if (strcmp (grad_dir_names[i], name) == 0)
			return i;
	return GO_GRADIENT_N_TO_S;
}

char const *
go_gradient_dir_as_str (GOGradientDirection dir)
{
	return (dir < 0 || dir >= GO_GRADIENT_MAX) ? "gradient"
		: grad_dir_names[dir];
}
