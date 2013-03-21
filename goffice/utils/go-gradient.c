/*
 * go-gradient.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/go-combo-pixmaps.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#endif

#include <glib/gi18n-lib.h>
#include <string.h>

/**
 * GOGradientDirection:
 * @GO_GRADIENT_N_TO_S: top to bottom.
 * @GO_GRADIENT_S_TO_N: bottom to top.
 * @GO_GRADIENT_N_TO_S_MIRRORED: top to bottom, mirrored.
 * @GO_GRADIENT_S_TO_N_MIRRORED: bottom to top, mirrored.
 * @GO_GRADIENT_W_TO_E: left to right.
 * @GO_GRADIENT_E_TO_W: right to left.
 * @GO_GRADIENT_W_TO_E_MIRRORED: left to right, mirrored.
 * @GO_GRADIENT_E_TO_W_MIRRORED: right to left, mirrored.
 * @GO_GRADIENT_NW_TO_SE: top left to bottom right.
 * @GO_GRADIENT_SE_TO_NW: bottom right to top left.
 * @GO_GRADIENT_NW_TO_SE_MIRRORED: top left to bottom right, mirrored.
 * @GO_GRADIENT_SE_TO_NW_MIRRORED: bottom right to top left, mirrored.
 * @GO_GRADIENT_NE_TO_SW: top right to bottom left.
 * @GO_GRADIENT_SW_TO_NE: bottom left to top right.
 * @GO_GRADIENT_SW_TO_NE_MIRRORED: top right to bottom left, mirrored.
 * @GO_GRADIENT_NE_TO_SW_MIRRORED: bottom left to top right, mirrored.
 * @GO_GRADIENT_MAX: maximum value, should not occur.
} ;
 **/

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
	return (unsigned)dir >= GO_GRADIENT_MAX
		? "gradient"
		: grad_dir_names[dir];
}
