/*
 * gog-data-set.c : A Utility interface for managing GOData as attributes
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

/**
 * GogDatasetElement:
 * @data: the #GOData
 * @set: the owner data set.
 * @dim_i: the dimension iside the dataset.
 **/

/**
 * GogDatasetClass:
 * @base: base class
 * @get_elem: gets i-th element.
 * @set_dim: sets the data for i-th element.
 * @dims: gest first and last valid elements indices.
 * @dim_changed: called when an element has changed.
 **/

GType
gog_dataset_get_type (void)
{
	static GType gog_dataset_type = 0;

	if (!gog_dataset_type) {
		static GTypeInfo const gog_dataset_info = {
			sizeof (GogDatasetClass),	/* class_size */
			NULL,		/* base_init */
			NULL,		/* base_finalize */
		};

		gog_dataset_type = g_type_register_static (G_TYPE_INTERFACE,
			"GogDataset", &gog_dataset_info, 0);
	}

	return gog_dataset_type;
}

/**
 * gog_dataset_dims:
 * @set: #GogDataset
 * @first: inclusive
 * @last: _inclusive_
 *
 * FIXME ?? Fix what ??
 * Stores the first and last valid indicises to get/set dim
 * in @first and @last.
 **/
void
gog_dataset_dims (GogDataset const *set, int *first, int *last)
{
	GogDatasetClass *klass;
	g_return_if_fail (set);
	klass = GOG_DATASET_GET_CLASS (set);
	g_return_if_fail (klass != NULL);
	g_return_if_fail (first != NULL);
	g_return_if_fail (last != NULL);
	(klass->dims) (set, first, last);
}

/**
 * gog_dataset_get_dim:
 * @set: #GogDataset
 * @dim_i:
 *
 * Returns: (transfer none): the GOData associated with dimension @dim_i.  Does NOT add a
 * 	reference.  or %NULL on failure.
 **/
GOData *
gog_dataset_get_dim (GogDataset const *set, int dim_i)
{
	GogDatasetElement *elem;
	g_return_val_if_fail (set, NULL);
	elem = gog_dataset_get_elem (set, dim_i);
	if (NULL == elem)
		return NULL;
	return elem->data;
}

/**
 * gog_dataset_set_dim:
 * @set: #GogDataset
 * @dim_i:  < 0 gets the name
 * @val: (transfer full): #GOData
 * @err: #GError
 *
 * Absorbs a ref to @val if it is non-%NULL
 **/
void
gog_dataset_set_dim (GogDataset *set, int dim_i, GOData *val, GError **err)
{
	GogDatasetClass *klass;
	int first, last;

	g_return_if_fail (set != NULL || val == NULL || GO_IS_DATA (val));

	if (set == NULL || !GOG_IS_DATASET (set)) {
		g_warning ("gog_dataset_set_dim called with invalid GogDataset");
		goto done;
	}

	gog_dataset_dims (set, &first, &last);
	if (dim_i < first || dim_i > last) {
		g_warning ("gog_dataset_set_dim called with invalid index (%d)", dim_i);
		goto done;
	}
	klass = GOG_DATASET_GET_CLASS (set);

	/* short circuit */
	if (val != gog_dataset_get_dim (set, dim_i)) {
		gog_dataset_set_dim_internal (set, dim_i, val,
			gog_object_get_graph (GOG_OBJECT (set)));

		if (klass->set_dim)
			(klass->set_dim) (set, dim_i, val, err);
		if (klass->dim_changed)
			(klass->dim_changed) (set, dim_i);
	}

done :
	/* absorb ref to orig, simplifies life cycle easier for new GODatas */
	if (val != NULL)
		g_object_unref (val);
}

/**
 * gog_dataset_get_elem: (skip)
 * @set: #GogDataset
 * @dim_i:
 *
 * Returns: the GODataset associated with dimension @dim_i.
 **/
GogDatasetElement *
gog_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogDatasetClass *klass = GOG_DATASET_GET_CLASS (set);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_elem) (set, dim_i);
}

static void
cb_dataset_dim_changed (GOData *data, GogDatasetElement *elem)
{
	GogDatasetClass *klass = GOG_DATASET_GET_CLASS (elem->set);

	g_return_if_fail (klass != NULL);
	if (klass->dim_changed)
		(klass->dim_changed) (elem->set, elem->dim_i);
}

/**
 * gog_dataset_set_dim_internal:
 * @set: #GogDataset
 * @dim_i: the index
 * @val: #GOData
 * @graph: #GogGraph
 *
 * an internal routine to handle signal setup and teardown
 **/
void
gog_dataset_set_dim_internal (GogDataset *set, int dim_i,
			      GOData *val, GogGraph *graph)
{
	GogDatasetElement *elem = gog_dataset_get_elem (set, dim_i);

	g_return_if_fail (elem != NULL);

	if (graph != NULL) {
		if (val == elem->data)
			return;
		if (val != NULL)
			val = gog_graph_ref_data (graph, val);
		if (elem->handler != 0) {
			g_signal_handler_disconnect (G_OBJECT (elem->data),
				elem->handler);
			elem->handler = 0;
			gog_graph_unref_data (graph, elem->data);
		}
		if (val != NULL)
			elem->handler = g_signal_connect (
				G_OBJECT (val), "changed",
				G_CALLBACK (cb_dataset_dim_changed), elem);
	} else {
		if (val != NULL)
			g_object_ref (val);
		if (elem->data != NULL)
			g_object_unref (elem->data);
	}
	elem->data  = val;
	elem->set   = set;
	elem->dim_i = dim_i;
	gog_object_request_update (GOG_OBJECT (set));
}

void
gog_dataset_finalize (GogDataset *set)
{
	GogGraph *graph = gog_object_get_graph (GOG_OBJECT (set));
	int first, last;

	gog_dataset_dims (set, &first, &last);
	while (first <= last)
		gog_dataset_set_dim_internal (set, first++, NULL, graph);
}

void
gog_dataset_parent_changed (GogDataset *set, gboolean was_set)
{
	GogGraph *graph = gog_object_get_graph (GOG_OBJECT (set));
	GogDatasetElement *elem;
	GOData *dat;
	int i, last;

	for (gog_dataset_dims (set, &i, &last); i <= last ; i++) {
		elem = gog_dataset_get_elem (set, i);
		if (elem == NULL || elem->data == NULL)
			continue;
		dat = elem->data;
		if (!was_set) {
			g_object_ref (dat);
			gog_dataset_set_dim_internal (set, i, NULL, graph);
			elem->data = dat;
		} else if (elem->handler == 0) {
			elem->data = NULL; /* disable the short circuit */
			gog_dataset_set_dim_internal (set, i, dat, graph);
			g_object_unref (dat);
		}
	}
	if (was_set)
		gog_object_request_update (GOG_OBJECT (set));
}

void
gog_dataset_dup_to_simple (GogDataset const *src, GogDataset *dst)
{
	gint n, last;
	GOData *src_dat, *dst_dat;

	gog_dataset_dims (src, &n, &last);

	for ( ; n <= last ; n++) {
		unsigned int n_dimensions;

		src_dat = gog_dataset_get_dim (src, n);
		if (src_dat == NULL)
			continue;
		dst_dat = NULL;

		n_dimensions = go_data_get_n_dimensions (src_dat);

		/* for scalar and vector data, try to transform to values first
		if we find non finite, use strings */

		switch (n_dimensions) {
			case 0:
				{
					char *str;
					char *end;
					double d;

					str = go_data_get_scalar_string (src_dat);
					d =  g_strtod (str, &end);

					dst_dat = (*end == 0) ?
						go_data_scalar_val_new (d):
						go_data_scalar_str_new (g_strdup (str), TRUE);

					g_free (str);
				}
				break;
			case 1:
				{
					double *d;
					int i, n;
					gboolean as_values = TRUE;

					d = go_data_get_values (src_dat);
					n = go_data_get_vector_size (src_dat);

					for (i = 0; i < n; i++)
						if (!go_finite (d[i])) {
							as_values = FALSE;
							break;
						}

					if (as_values)
						/* we don't need to duplicate, since this is used only for
						   short lived objects */
						dst_dat = go_data_vector_val_new (d, n, NULL);
					else {
						char **str = g_new (char*, n + 1);
						str[n] = NULL;
						for (i = 0; i < n; i++)
							str[i] = go_data_get_vector_string (src_dat, i);

						dst_dat = go_data_vector_str_new ((char const* const*) str,
										  n, g_free);
					}
				}
				break;
			case 2:
				{
					/* only values are supported so don't care */
					GODataMatrixSize size;

					go_data_get_matrix_size (src_dat, &size.rows, &size.columns);

					dst_dat = go_data_matrix_val_new (go_data_get_values (src_dat),
									  size.rows, size.columns, NULL);
				}
				break;
			default:
				g_warning ("[GogDataSet::dup_to_simple] Source with invalid number of dimensions (%d)",
					   n_dimensions);
		}
		gog_dataset_set_dim (dst, n, dst_dat, NULL);
	}
}
