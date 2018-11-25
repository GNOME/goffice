/*
 * gog-series-impl.h :
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

#ifndef GO_SERIES_IMPL_H
#define GO_SERIES_IMPL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	GogStyledObject base;

	unsigned int index;
} GogSeriesElement;

typedef struct {
	GogStyledObjectClass base;

	/* Virtuals */
	gpointer (*gse_populate_editor) (GogObject *gobj,
					 GOEditor *editor,
					 GOCmdContext *cc);
} GogSeriesElementClass;

#define GOG_SERIES_ELEMENT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_SERIES_ELEMENT, GogSeriesElementClass))

typedef enum {
	GOG_SERIES_REQUIRED,  /* it must be there */
	GOG_SERIES_SUGGESTED, /* allocator will fill it in, but use need not */
	GOG_SERIES_OPTIONAL,
	GOG_SERIES_ERRORS
} GogSeriesPriority;

#define GOG_SERIES_ACCEPT_TREND_LINE	1

struct _GogSeriesDimDesc {
	char const *name;
	GogSeriesPriority	priority;
	gboolean		is_shared;
	GogDimType		val_type;
	GogMSDimType		ms_type;
};

struct _GogSeriesDesc {
	unsigned int style_fields;
	unsigned int num_dim;
	GogSeriesDimDesc const *dim;
};

struct _GogSeries {
	GogStyledObject base;

	int index;
	unsigned manual_index : 1;
	unsigned is_valid     : 1;
	unsigned needs_recalc : 1;
	unsigned acceptable_children : 1;

	GogPlot	  	  *plot;
	GogDatasetElement *values;
	gboolean	   has_legend;
	unsigned int   	   num_elements;
	GogSeriesFillType  fill_type;
	GList		  *overrides;  /* GogSeriesElement (individual points) */

	GOLineInterpolation	interpolation;
	gboolean interpolation_skip_invalid;
	/* data related to data labels */
	GogSeriesLabelsPos default_pos;
	unsigned int allowed_pos; /* if 0, no data labels can be addded */
};

typedef struct {
	GogStyledObjectClass base;

	gboolean		 has_interpolation;
	gboolean		 has_fill_type;

	GogSeriesFillType const	*valid_fill_type_list;

	GType			 series_element_type;

	/* Virtuals */
	void        (*dim_changed) (GogSeries *series, int dim_i);
	unsigned    (*get_xy_data) (GogSeries const *series,
	                            double const **x, double const **y);
	GogDataset *(*get_interpolation_params) (GogSeries const *series);

} GogSeriesClass;

#define GOG_SERIES_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_SERIES, GogSeriesClass))
#define GOG_IS_SERIES_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_SERIES))
#define GOG_SERIES_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_SERIES, GogSeriesClass))

/* protected */
void gog_series_check_validity   (GogSeries *series);
GogSeriesElement *gog_series_get_element (GogSeries const *series, int index);

G_END_DECLS

#endif /* GO_SERIES_IMPL_H */
