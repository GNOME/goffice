/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-line.h : 
 *
 * Copyright (C) 2004-2006 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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

#ifndef GO_LINE_H
#define GO_LINE_H

#include <goffice/graph/goffice-graph.h>

G_BEGIN_DECLS

typedef enum {
	GO_LINE_NONE,
	GO_LINE_SOLID,
	GO_LINE_S_DOT,
	GO_LINE_S_DASH_DOT,
	GO_LINE_S_DASH_DOT_DOT,
	GO_LINE_DASH_DOT_DOT_DOT,
	GO_LINE_DOT,
	GO_LINE_S_DASH,
	GO_LINE_DASH,
	GO_LINE_LONG_DASH,
	GO_LINE_DASH_DOT,
	GO_LINE_DASH_DOT_DOT,
	GO_LINE_MAX
} GOLineDashType;

typedef enum {
	GO_LINE_INTERPOLATION_LINEAR,
	GO_LINE_INTERPOLATION_SPLINE,
	GO_LINE_INTERPOLATION_STEP_START,
	GO_LINE_INTERPOLATION_STEP_END,
	GO_LINE_INTERPOLATION_STEP_CENTER_X,
	GO_LINE_INTERPOLATION_STEP_CENTER_Y,
	GO_LINE_INTERPOLATION_MAX
} GOLineInterpolation;

typedef struct {
	double		 offset;
	unsigned int	 n_dash;
	double		*dash;
} GOLineDashSequence;

GOLineDashType		 go_line_dash_from_str		(char const *name);
char const 		*go_line_dash_as_str		(GOLineDashType type);
char const 		*go_line_dash_as_label 		(GOLineDashType type);
double			 go_line_dash_get_length	(GOLineDashType type);
GOLineDashSequence	*go_line_dash_get_sequence 	(GOLineDashType type, double scale);
void 		  	 go_line_dash_sequence_free	(GOLineDashSequence *sequence);

GOLineInterpolation	 go_line_interpolation_from_str		(char const *name);
char const 		*go_line_interpolation_as_str		(GOLineInterpolation type);

G_END_DECLS

#endif /* GO_LINE_H */
