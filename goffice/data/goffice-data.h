/*
 * goffice-data.h:
 *
 * Copyright (C) 2005 Jody Goldberg (jody@gnome.org)
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

#ifndef GOFFICE_DATA_H
#define GOFFICE_DATA_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GOData		 GOData;

typedef struct _GODataScalar	 GODataScalar;
typedef struct _GODataVector	 GODataVector;
typedef struct _GODataMatrix	 GODataMatrix;
typedef struct {
	int rows;	/* negative if dirty, includes missing values */
	int columns;	/* negative if dirty, includes missing values */
} GODataMatrixSize;

G_END_DECLS

#include <goffice/data/go-data.h>
#include <goffice/data/go-data-impl.h>
#include <goffice/data/go-data-simple.h>

#endif /* GOFFICE_DATA_H */
