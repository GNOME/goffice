/*
 * gog-axis-line-impl.h :
 *
 * Copyright (C) 2005 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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

#ifndef GOG_AXIS_LINE_IMPL_H
#define GOG_AXIS_LINE_IMPL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct  {
		gboolean tick_in, tick_out;
		int size_pts;
} GogAxisTickProperties;

struct _GogAxisBase {
	GogStyledObject	 base;

	GogChart	*chart;
	GogAxis		*axis;

	GogAxisPosition    position;
	unsigned int 	   crossed_axis_id;
	GogDatasetElement  cross_location;

	int		   padding;

	GogAxisTickProperties major, minor;
	gboolean major_tick_labeled;
};

typedef GogStyledObjectClass GogAxisBaseClass;

typedef struct {
	GogView		base;

	double		x_start, y_start;
	double		x_stop, y_stop;
} GogAxisBaseView;

typedef GogViewClass	GogAxisBaseViewClass;

#define GOG_TYPE_AXIS_BASE_VIEW		(gog_axis_base_view_get_type ())
#define GOG_AXIS_BASE_VIEW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_AXIS_BASE_VIEW, GogAxisBaseView))
#define GOG_IS_AXIS_BASE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_AXIS_BASE_VIEW))

GType gog_axis_base_view_get_type (void);
void gog_axis_base_view_label_position_request (GogView *view,
			                        GogViewAllocation const *bbox,
						GogViewAllocation *pos);

G_END_DECLS

#endif /*GOG_AXIS_LINE_IMPL_H*/

