/*
 * gog-series-labels.h
 *
 * Copyright (C) 2011 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef GOG_SERIES_LABELS_H
#define GOG_SERIES_LABELS_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct  {
	GOString *str;
	int legend_pos;
	GogObject *point;
} GogSeriesLabelElt;
GType gog_series_label_elt_get_type (void);

struct _GogDataLabel {
	GogOutlinedObject base;

	/* private */
	unsigned int index;
	GogSeriesLabelsPos position;
	GogSeriesLabelsPos default_pos;
	unsigned int allowed_pos;
	unsigned int offset; /* position offset in pixels */
	char *format;
	char *separator;
	GogDatasetElement custom_label[2];
	GogSeriesLabelElt element;
	gboolean supports_percent;
};

#define GOG_TYPE_DATA_LABEL		(gog_data_label_get_type ())
#define GOG_DATA_LABEL(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_DATA_LABEL, GogDataLabel))
#define GOG_IS_DATA_LABEL(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_DATA_LABEL))

GType gog_data_label_get_type (void);

void gog_data_label_set_allowed_position (GogDataLabel *lbl, unsigned allowed);
void gog_data_label_set_position (GogDataLabel *lbl, GogSeriesLabelsPos pos);
void gog_data_label_set_default_position (GogDataLabel *lbl, GogSeriesLabelsPos pos);
GogSeriesLabelsPos gog_data_label_get_position (GogDataLabel const *lbl);
GogSeriesLabelElt const *gog_data_label_get_element (GogDataLabel const *lbl);

struct _GogSeriesLabels {
	GogOutlinedObject base;

	/* private */
	GogSeriesLabelsPos position;
	GogSeriesLabelsPos default_pos;
	unsigned int allowed_pos;
	unsigned int offset; /* position offset in pixels */
	char *format;
	char *separator;
	GogDatasetElement custom_labels[2];
	unsigned int n_elts;
	GogSeriesLabelElt *elements;
	GList *overrides;
	gboolean supports_percent;
};

#define GOG_TYPE_SERIES_LABELS		(gog_series_labels_get_type ())
#define GOG_SERIES_LABELS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_SERIES_LABELS, GogSeriesLabels))
#define GOG_IS_SERIES_LABELS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_SERIES_LABELS))

GType gog_series_labels_get_type (void);

void gog_series_labels_set_allowed_position (GogSeriesLabels *lbls, unsigned allowed);
void gog_series_labels_set_position (GogSeriesLabels *lbls, GogSeriesLabelsPos pos);
void gog_series_labels_set_default_position (GogSeriesLabels *lbls, GogSeriesLabelsPos pos);
GogSeriesLabelsPos gog_series_labels_get_position (GogSeriesLabels const *lbls);
GogSeriesLabelElt const *gog_series_labels_scalar_get_element (GogSeriesLabels const *lbls);
GogSeriesLabelElt const *gog_series_labels_vector_get_element (GogSeriesLabels const *lbls, unsigned n);

G_END_DECLS

#endif  /* GOG_SERIES_LABELS_H */
