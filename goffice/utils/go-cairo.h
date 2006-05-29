/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-cairo.h
 *
 * Copyright (C) 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#ifndef GO_CAIRO_H
#define GO_CAIRO_H

typedef struct _GOCairo GOCairo;

#include <goffice/utils/go-path.h>
#include <goffice/graph/gog-style.h>
#include <cairo.h>

#define IS_GO_CAIRO(x) ((x) != NULL)

GOCairo *go_cairo_new		(cairo_t *cairo, double scale);
void go_cairo_free 		(GOCairo *gcairo);

void go_cairo_push_style	(GOCairo *gcairo, GogStyle const *style);
void go_cairo_pop_style		(GOCairo *gcairo);

void go_cairo_push_clip 	(GOCairo *gcairo, GOPath const *path);
void go_cairo_pop_clip  	(GOCairo *gcairo);

void go_cairo_stroke		(GOCairo *gcairo, GOPath const *path);
void go_cairo_fill		(GOCairo *gcairo, GOPath const *path);
void go_cairo_draw_shape	(GOCairo *gcairo, GOPath const *path);
void go_cairo_draw_markers	(GOCairo *gcairo, GOPath const *path);
void go_cairo_draw_text		(GOCairo *gcairo, double x, double y, char const *text, GtkAnchorType anchor);

void go_cairo_get_text_OBR	(GOCairo *gcairo, char const *text, GOGeometryOBR *obr);
void go_cairo_get_text_AABR	(GOCairo *gcairo, char const *text, GOGeometryAABR *aabr);

double go_cairo_line_size	(GOCairo const *gcairo, double width);
double go_cairo_pt2r_x		(GOCairo const *gcairo, double d);
double go_cairo_pt2r_y		(GOCairo const *gcairo, double d);
double go_cairo_pt2r		(GOCairo const *gcairo, double d);

#endif 
