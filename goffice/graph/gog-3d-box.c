/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-3d-box.h :
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/graph/gog-3d-box.h>
#include <goffice/math/go-math.h>
#include <gsf/gsf-impl-utils.h>

typedef GogObjectClass Gog3DBoxClass;

static void
gog_3d_box_class_init (Gog3DBoxClass *klass)
{
}

static void
gog_3d_box_init (Gog3DBox *box)
{
	box->fov = 10. / 180. * M_PI;
	box->psi = 70. / 180. * M_PI;
	box->theta = 10. / 180. * M_PI;
	box->phi = -90. / 180. * M_PI;
	go_matrix3x3_from_euler (&box->mat, box->psi, box->theta, box->phi);
}

GSF_CLASS (Gog3DBox, gog_3d_box,
	   gog_3d_box_class_init, gog_3d_box_init,
	   GOG_OBJECT_TYPE)
