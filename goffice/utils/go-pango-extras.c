/*
 * go-pango-extras.c: Various utility routines that should have been in pango.
 *
 * Authors:
 *    Morten Welinder (terra@gnome.org)
 *    Andreas J. Guelzow (aguelzow@pyrshep.ca)
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
#include "go-pango-extras.h"
#include "go-glib-extras.h"
#include <string.h>

struct cb_splice {
	guint pos, len;
	PangoAttrList *result;
};

static gboolean
cb_splice (PangoAttribute *attr, gpointer _data)
{
	struct cb_splice *data = _data;

	if (attr->start_index >= data->pos) {
		PangoAttribute *new_attr = pango_attribute_copy (attr);
		new_attr->start_index += data->len;
		new_attr->end_index += data->len;
		pango_attr_list_insert (data->result, new_attr);
	} else if (attr->end_index <= data->pos) {
		PangoAttribute *new_attr = pango_attribute_copy (attr);
		pango_attr_list_insert (data->result, new_attr);
	} else {
		PangoAttribute *new_attr = pango_attribute_copy (attr);
		new_attr->end_index = data->pos;
		pango_attr_list_insert (data->result, new_attr);

		new_attr = pango_attribute_copy (attr);
		new_attr->start_index = data->pos + data->len;
		new_attr->end_index += data->len;
		pango_attr_list_insert (data->result, new_attr);
	}

	return FALSE;
}

static gboolean
cb_splice_true (G_GNUC_UNUSED PangoAttribute *attr, G_GNUC_UNUSED gpointer data)
{
	return TRUE;
}

/**
 * go_pango_attr_list_open_hole:
 * @tape: An attribute list
 * @pos: a text position in bytes
 * @len: length of segment in bytes
 *
 * This function opens up a blank segment of attributes.  This is what to
 * call after inserting a segment into the text described by the attributes.
 */
void
go_pango_attr_list_open_hole (PangoAttrList *tape, guint pos, guint len)
{
	/* Clean out the tape.  */
	PangoAttrList *tape2 =
		pango_attr_list_filter (tape, cb_splice_true, NULL);

	if (tape2) {
		struct cb_splice data;
		data.result = tape;
		data.pos = pos;
		data.len = len;

		(void)pango_attr_list_filter (tape2, cb_splice, &data);
		pango_attr_list_unref (tape2);
	}
}

struct cb_erase {
	unsigned start_pos, end_pos, len; /* in bytes not chars */
};


/*
 *
 * |       +-------------------+            The attribute
 *
 *                                +----+    (1)
 *                  +------------------+    (2)
 *                  +------+                (3)
 *   +--+                                   (4)
 *   +--------------+                       (5)
 *   +---------------------------------+    (6)
 *
 */
static gboolean
cb_delete_filter (PangoAttribute *a, struct cb_erase *change)
{
	if (change->start_pos >= a->end_index)
		return FALSE;  /* (1) */

	if (change->start_pos <= a->start_index) {
		if (change->end_pos >= a->end_index)
			return TRUE; /* (6) */

		a->end_index -= change->len;
		if (change->end_pos >= a->start_index)
			a->start_index = change->start_pos; /* (5) */
		else
			a->start_index -= change->len; /* (4) */
	} else {
		if (change->end_pos >= a->end_index)
			a->end_index = change->start_pos;  /* (2) */
		else
			a->end_index -= change->len; /* (3) */
	}

	return FALSE;
}

/**
 * go_pango_attr_list_erase:
 * @attrs: An attribute list
 * @pos: a text position in bytes
 * @len: length of segment in bytes
 *
 * This function erases a segment of attributes.  This is what to call
 * after deleting a segment from the text described by the attributes.
 */
void
go_pango_attr_list_erase (PangoAttrList *attrs, guint pos, guint len)
{
	PangoAttrList *gunk;
	struct cb_erase data;

	if (attrs == NULL)
		return;

	data.start_pos = pos;
	data.end_pos = pos + len;
	data.len = len;

	gunk = pango_attr_list_filter (attrs,
				       (PangoAttrFilterFunc)cb_delete_filter,
				       &data);
	if (gunk != NULL)
		pango_attr_list_unref (gunk);
}



struct cb_unset {
	PangoAttrList *list;
	guint start_index, end_index;
};

static gboolean
cb_unset1 (PangoAttribute *attr, gpointer _data)
{
	const PangoAttrType *ptype = _data;
	return *ptype == PANGO_ATTR_INVALID || *ptype == attr->klass->type;
}

static gboolean
cb_unset2 (PangoAttribute *attr, gpointer _data)
{
	struct cb_unset *data = _data;

	/* All inside cleared area.  */
	if (attr->start_index >= data->start_index &&
	    attr->end_index <= data->end_index)
		return FALSE;

	attr = pango_attribute_copy (attr);

	if (attr->end_index > data->start_index && attr->end_index <= data->end_index)
		/* Ends inside cleared area.  */
		attr->end_index = data->start_index;
	else if (attr->start_index >= data->start_index && attr->start_index < data->end_index)
		/* Starts inside cleared area.  */
		attr->start_index = data->end_index;
	else if (attr->start_index < data->start_index && attr->end_index > data->end_index) {
		/* Starts before, ends after.  */
		PangoAttribute *attr2 = pango_attribute_copy (attr);
		attr2->start_index = data->end_index;
		pango_attr_list_insert (data->list, attr2);
		attr->end_index = data->start_index;
	}

	pango_attr_list_insert (data->list, attr);

	return FALSE;
}

/**
 * go_pango_attr_list_unset:
 * @list: #PangoAttrList
 * @start: starting character index
 * @end: last character index
 * @type: #PangoAttrType
 *
 * See http://bugzilla.gnome.org/show_bug.cgi?id=163679
 **/
void
go_pango_attr_list_unset (PangoAttrList  *list,
			  gint            start,
			  gint            end,
			  PangoAttrType   type)
{
	PangoAttrList *matches;
	struct cb_unset data;

	g_return_if_fail (list != NULL);

	if (start >= end || end < 0)
		return;

	/*
	 * Pull out the attribute subset we even care about.  Notice that this
	 * is all-or-nothing for a given type so any reordering will not
	 * matter.
	 */
	matches = pango_attr_list_filter (list, cb_unset1, &type);
	if (!matches)
		return;

	/* Get rid of whole segments and adjust end_index of rest.  */
	data.list = list;
	data.start_index = start;
	data.end_index = end;
	(void)pango_attr_list_filter (matches, cb_unset2, &data);

	pango_attr_list_unref (matches);
}

static gboolean
cb_empty (G_GNUC_UNUSED PangoAttribute *attr, gpointer data)
{
	gboolean *pempty = data;
	*pempty = FALSE;
	return FALSE;
}

gboolean
go_pango_attr_list_is_empty (const PangoAttrList *attrs)
{
	gboolean empty = TRUE;
	if (attrs)
		(void)pango_attr_list_filter ((PangoAttrList *)attrs,
					      cb_empty, &empty);
	return empty;
}

#ifdef GOFFICE_WITH_GTK
static gboolean
go_load_pango_attributes_into_buffer_filter (PangoAttribute *attribute,
					  G_GNUC_UNUSED gpointer data)
{
	return ((PANGO_ATTR_FOREGROUND == attribute->klass->type) ||
		(PANGO_ATTR_RISE == attribute->klass->type));
}

static gboolean
go_load_pango_attributes_into_buffer_named_filter (PangoAttribute *attribute,
						    G_GNUC_UNUSED gpointer data)
{
	return ((PANGO_ATTR_STYLE == attribute->klass->type) ||
		(PANGO_ATTR_WEIGHT == attribute->klass->type) ||
		(PANGO_ATTR_UNDERLINE == attribute->klass->type) ||
		(PANGO_ATTR_STRIKETHROUGH == attribute->klass->type));
}

void
go_create_std_tags_for_buffer (GtkTextBuffer *buffer)
{
	gtk_text_buffer_create_tag (buffer, "PANGO_STYLE_NORMAL", "style", PANGO_STYLE_NORMAL,
				    "style-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_STYLE_ITALIC", "style", PANGO_STYLE_ITALIC,
				    "style-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_STRIKETHROUGH_TRUE", "strikethrough", TRUE,
				    "strikethrough-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_STRIKETHROUGH_FALSE", "strikethrough", FALSE,
				    "strikethrough-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_THIN", "weight", 100,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_ULTRALIGHT", "weight", PANGO_WEIGHT_ULTRALIGHT,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_LIGHT", "weight", PANGO_WEIGHT_LIGHT,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_BOOK", "weight", 380,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_NORMAL", "weight", PANGO_WEIGHT_NORMAL,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_MEDIUM", "weight", 500,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_SEMIBOLD", "weight", PANGO_WEIGHT_SEMIBOLD,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_BOLD", "weight", PANGO_WEIGHT_BOLD,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_ULTRABOLD", "weight", PANGO_WEIGHT_ULTRABOLD,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_HEAVY", "weight", PANGO_WEIGHT_HEAVY,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_WEIGHT_ULTRAHEAVY", "weight", 1000,
				    "weight-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_UNDERLINE_NONE", "underline", PANGO_UNDERLINE_NONE,
				    "underline-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_UNDERLINE_SINGLE", "underline", PANGO_UNDERLINE_SINGLE,
				    "underline-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_UNDERLINE_DOUBLE", "underline", PANGO_UNDERLINE_DOUBLE,
				    "underline-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_UNDERLINE_LOW", "underline", PANGO_UNDERLINE_LOW,
				    "underline-set", TRUE, NULL);
	gtk_text_buffer_create_tag (buffer, "PANGO_UNDERLINE_ERROR", "underline", PANGO_UNDERLINE_ERROR,
				    "underline-set", TRUE, NULL);
}

static gint
go_load_pango_byte_to_char (gchar const *str, gint byte)
{
	if (byte >= (gint)strlen (str))
		return g_utf8_strlen (str, -1);
	return g_utf8_pointer_to_offset (str, g_utf8_prev_char (str + byte + 1));
}

void
go_load_pango_attributes_into_buffer (PangoAttrList  *markup, GtkTextBuffer *buffer, gchar const *str)
{
	PangoAttrIterator * iter;
	PangoAttrList  *copied_markup;
	PangoAttrList  *our_markup;

	if (markup == NULL)
		return;

/* For some styles we create named tags. The names are taken from the Pango enums */

	copied_markup = pango_attr_list_copy (markup);
	our_markup = pango_attr_list_filter (copied_markup,
					     go_load_pango_attributes_into_buffer_named_filter,
					     NULL);
	pango_attr_list_unref (copied_markup);
	if (our_markup != NULL) {
		iter = pango_attr_list_get_iterator (our_markup);

		do {
			GSList *attr = pango_attr_iterator_get_attrs (iter);
			if (attr != NULL) {
				GSList *ptr;
				gint start, end;
				GtkTextIter start_iter, end_iter;
				char const *name;

				pango_attr_iterator_range (iter, &start, &end);
				start = go_load_pango_byte_to_char (str, start);
				end = go_load_pango_byte_to_char (str, end);
				gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, start);
				gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, end);

				for (ptr = attr; ptr != NULL; ptr = ptr->next) {
					PangoAttribute *attribute = ptr->data;
					GtkTextTag *tag;
					int val;

					switch (attribute->klass->type) {
					case PANGO_ATTR_STYLE:
						name = (((PangoAttrInt *)attribute)->value
							== PANGO_STYLE_NORMAL)
							? "PANGO_STYLE_NORMAL" :
							"PANGO_STYLE_ITALIC";
						tag = gtk_text_tag_table_lookup
							(gtk_text_buffer_get_tag_table (buffer),
							 name);
						gtk_text_buffer_apply_tag (buffer, tag,
									   &start_iter, &end_iter);
						break;
					case PANGO_ATTR_STRIKETHROUGH:
						name = (((PangoAttrInt *)attribute)->value) ?
							"PANGO_STRIKETHROUGH_TRUE" :
							"PANGO_STRIKETHROUGH_FALSE";
						tag = gtk_text_tag_table_lookup
							(gtk_text_buffer_get_tag_table (buffer),
							 name);
						gtk_text_buffer_apply_tag (buffer, tag,
									   &start_iter, &end_iter);
						break;
					case PANGO_ATTR_UNDERLINE:
						val = ((PangoAttrInt *)attribute)->value;
						if (val == PANGO_UNDERLINE_NONE)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_UNDERLINE_NONE",
											   &start_iter, &end_iter);
						else if (val == PANGO_UNDERLINE_SINGLE)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_UNDERLINE_SINGLE",
											   &start_iter, &end_iter);
						else if (val == PANGO_UNDERLINE_DOUBLE)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_UNDERLINE_DOUBLE",
											   &start_iter, &end_iter);
						else if (val == PANGO_UNDERLINE_LOW)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_UNDERLINE_LOW",
											   &start_iter, &end_iter);
						else if (val == PANGO_UNDERLINE_ERROR)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_UNDERLINE_ERROR",
											   &start_iter, &end_iter);
						break;
					case PANGO_ATTR_WEIGHT:
						val = ((PangoAttrInt *)attribute)->value;
						if (val < (PANGO_WEIGHT_THIN + PANGO_WEIGHT_ULTRALIGHT)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_THIN",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_ULTRALIGHT",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_BOOK)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_LIGHT",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_BOOK + PANGO_WEIGHT_NORMAL)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_BOOK",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_MEDIUM)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_NORMAL",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_MEDIUM + PANGO_WEIGHT_SEMIBOLD)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_MEDIUM",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_SEMIBOLD + PANGO_WEIGHT_BOLD)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_SEMIBOLD",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_BOLD",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_ULTRABOLD + PANGO_WEIGHT_HEAVY)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_ULTRABOLD",
											   &start_iter, &end_iter);
						else if (val < (PANGO_WEIGHT_HEAVY + PANGO_WEIGHT_ULTRAHEAVY)/2)
							gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_HEAVY",
											   &start_iter, &end_iter);
						else gtk_text_buffer_apply_tag_by_name (buffer,"PANGO_WEIGHT_ULTRAHEAVY",
											&start_iter, &end_iter);
						break;
					default:
						break;
					}
				}
				g_slist_free_full (attr, (GDestroyNotify)pango_attribute_destroy);
			}
		} while (pango_attr_iterator_next (iter));
		pango_attr_iterator_destroy (iter);
		pango_attr_list_unref (our_markup);
	}

/* For other styles (that are not at true/false type styles) we use unnamed styles */

	copied_markup = pango_attr_list_copy (markup);
	our_markup = pango_attr_list_filter (copied_markup,
					     go_load_pango_attributes_into_buffer_filter,
					     NULL);
	pango_attr_list_unref (copied_markup);
	if (our_markup != NULL) {
		iter = pango_attr_list_get_iterator (our_markup);

		do {
			GSList *attr = pango_attr_iterator_get_attrs (iter);
			if (attr != NULL) {
				char *string;
				GSList *ptr;
				gint start, end;
				GtkTextIter start_iter, end_iter;
				GtkTextTag *tag = gtk_text_buffer_create_tag (buffer, NULL, NULL);
				for (ptr = attr; ptr != NULL; ptr = ptr->next) {
					PangoAttribute *attribute = ptr->data;
					switch (attribute->klass->type) {
					case PANGO_ATTR_FOREGROUND:
						string = pango_color_to_string
							(&((PangoAttrColor *)attribute)->color);
						g_object_set (G_OBJECT (tag),
							      "foreground", string,
							      "foreground-set", TRUE,
							      NULL);
						g_free (string);
						break;
					case PANGO_ATTR_RISE:
						g_object_set (G_OBJECT (tag),
							      "rise",
							      ((PangoAttrInt *)attribute)->value,
							      "rise-set", TRUE,
							      NULL);
						break;
					default:
						break;
					}
				}
				pango_attr_iterator_range (iter, &start, &end);
				start = go_load_pango_byte_to_char (str, start);
				end = go_load_pango_byte_to_char (str, end);
				gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, start);
				gtk_text_buffer_get_iter_at_offset (buffer, &end_iter, end);
				gtk_text_buffer_apply_tag (buffer, tag, &start_iter, &end_iter);
				g_slist_free_full (attr, (GDestroyNotify)pango_attribute_destroy);
			}
		} while (pango_attr_iterator_next (iter));
		pango_attr_iterator_destroy (iter);
		pango_attr_list_unref (our_markup);
	}
}
#endif

static gboolean
filter_func (PangoAttribute *attribute, G_GNUC_UNUSED gpointer data)
{
	PangoAttrType type = attribute->klass->type;
	return (type == go_pango_attr_superscript_get_attr_type () ||
		type == go_pango_attr_subscript_get_attr_type ());
}

static void
go_pango_translate_here (PangoAttrIterator *state_iter,
			 PangoAttrIterator *attr_iter, PangoAttrList *attrs)
{
	double font_scale = 1.;
	double scale = 1.;
	int rise = 0;
	PangoAttribute *pa;
	PangoFontDescription *desc;
	GSList *the_attrs, *l;
	gint range_start, range_end;

	pango_attr_iterator_range (attr_iter, &range_start, &range_end);

	if (range_start == range_end)
		return;

	desc = pango_font_description_new ();
	pango_attr_iterator_get_font (state_iter, desc, NULL, NULL);
	font_scale = pango_font_description_get_size (desc)/
		(double)PANGO_SCALE/10.;
	pango_font_description_free (desc);

	pa = pango_attr_iterator_get
		(state_iter, PANGO_ATTR_SCALE);
	if (pa)
		scale = ((PangoAttrFloat *)pa)->value;
	pa = pango_attr_iterator_get
		(state_iter, PANGO_ATTR_RISE);
	if (pa)
		rise = ((PangoAttrInt *)pa)->value;

	/* We should probably figured out the default font size used */
	/* rather than assuming it is 10 pts * scale */
	if (font_scale == 0)
		font_scale = scale;

	the_attrs = pango_attr_iterator_get_attrs (attr_iter);
	for (l = the_attrs; l != NULL; l = l->next) {
		PangoAttribute *attribute = l->data;
		PangoAttrType type = attribute->klass->type;
		if (type == go_pango_attr_superscript_get_attr_type ()) {
			GOPangoAttrSuperscript *attr = (GOPangoAttrSuperscript *)attribute;
			if (attr->val) {
				scale *= GO_SUPERSCRIPT_SCALE;
				rise += GO_SUPERSCRIPT_RISE * font_scale;
				font_scale *= GO_SUPERSCRIPT_SCALE;
			}
		} else { /* go_pango_attr_subscript_type */
			GOPangoAttrSubscript *attr = (GOPangoAttrSubscript *)attribute;
			if (attr->val) {
				scale *= GO_SUBSCRIPT_SCALE;
				rise += GO_SUBSCRIPT_RISE * font_scale;
				font_scale *= GO_SUBSCRIPT_SCALE;
			}
		}
	}

	if (the_attrs != NULL) {
		PangoAttribute *attr = pango_attr_scale_new (scale);
		attr->start_index = range_start;
		attr->end_index = range_end;
		pango_attr_list_insert (attrs, attr);
		attr = pango_attr_rise_new (rise);
		attr->start_index = range_start;
		attr->end_index = range_end;
		pango_attr_list_insert (attrs, attr);
	}
	g_slist_free_full (the_attrs, (GDestroyNotify)pango_attribute_destroy);
}


PangoAttrList *
go_pango_translate_attributes (PangoAttrList *attrs)
{
	PangoAttrList *n_attrs, *filtered;

	if (attrs == NULL)
		return NULL;

	n_attrs = pango_attr_list_copy (attrs);
	filtered = pango_attr_list_filter (n_attrs, filter_func, NULL);

	if (filtered == NULL) {
		pango_attr_list_unref (n_attrs);
		return attrs;
	} else {
		PangoAttrIterator *iter, *f_iter;
		f_iter = pango_attr_list_get_iterator (filtered);
		do {
			gint f_range_start, f_range_end;
			gint range_start, range_end;
			/* We need to restart everytime since we are changing n_attrs */
			iter = pango_attr_list_get_iterator (n_attrs);
			pango_attr_iterator_range (f_iter, &f_range_start,
						   &f_range_end);
			pango_attr_iterator_range (iter, &range_start,
						   &range_end);
			while (range_end <= f_range_start) {
				if (!pango_attr_iterator_next (iter))
					break;
				pango_attr_iterator_range (iter, &range_start,
						   &range_end);
			}
			/* Now range_end > f_range_start >= range_start */
			go_pango_translate_here (iter, f_iter, n_attrs);
			pango_attr_iterator_destroy (iter);
		} while (pango_attr_iterator_next (f_iter));
		pango_attr_iterator_destroy (f_iter);
	}
	pango_attr_list_unref (filtered);
	return n_attrs;
}

void
go_pango_translate_layout (PangoLayout *layout)
{
	PangoAttrList *attrs, *n_attrs;

	g_return_if_fail (layout != NULL);

	attrs = pango_layout_get_attributes (layout);
	n_attrs = go_pango_translate_attributes (attrs);
	if (attrs != n_attrs) {
		pango_layout_set_attributes (layout, n_attrs);
		pango_attr_list_unref (n_attrs);
	}

}

PangoAttrType
go_pango_attr_subscript_get_attr_type (void)
{
	static PangoAttrType go_pango_attr_subscript_type = PANGO_ATTR_INVALID;

	if(go_pango_attr_subscript_type == PANGO_ATTR_INVALID)
		go_pango_attr_subscript_type =
			pango_attr_type_register ("GOSubscript");

	return go_pango_attr_subscript_type;
}

PangoAttrType
go_pango_attr_subscript_get_type (void)
{
	return go_pango_attr_subscript_get_attr_type ();
}

PangoAttrType
go_pango_attr_superscript_get_attr_type (void)
{
	static PangoAttrType go_pango_attr_superscript_type = PANGO_ATTR_INVALID;

	if(go_pango_attr_superscript_type == PANGO_ATTR_INVALID)
		go_pango_attr_superscript_type =
			pango_attr_type_register ("GOSuperscript");

	return go_pango_attr_superscript_type;
}

PangoAttrType
go_pango_attr_superscript_get_type (void)
{
	return go_pango_attr_superscript_get_attr_type ();
}

static PangoAttribute *
go_pango_attr_subscript_copy (PangoAttribute const *attr)
{
	GOPangoAttrSubscript *at = (GOPangoAttrSubscript *)attr;
	return go_pango_attr_subscript_new (at->val);
}

static PangoAttribute *
go_pango_attr_superscript_copy (PangoAttribute const *attr)
{
	GOPangoAttrSuperscript *at = (GOPangoAttrSuperscript *)attr;
	return go_pango_attr_superscript_new (at->val);
}

static void
go_pango_attr_destroy (PangoAttribute *attr)
{
	g_free (attr);
}

static gboolean
go_pango_attr_superscript_compare (PangoAttribute const *attr1,
				   PangoAttribute const *attr2)
{
	GOPangoAttrSuperscript *at1 = (GOPangoAttrSuperscript *)attr1;
	GOPangoAttrSuperscript *at2 = (GOPangoAttrSuperscript *)attr2;
	return (at1->val == at2->val);
}

static gboolean
go_pango_attr_subscript_compare (PangoAttribute const *attr1,
				 PangoAttribute const *attr2)
{
	GOPangoAttrSubscript *at1 = (GOPangoAttrSubscript *)attr1;
	GOPangoAttrSubscript *at2 = (GOPangoAttrSubscript *)attr2;
	return (at1->val == at2->val);
}

/**
 * go_pango_attr_subscript_new: (skip)
 * @val: %TRUE if the characters must be lowered.
 *
 * Returns: (transfer full): a subscript attribute.
 **/
PangoAttribute *
go_pango_attr_subscript_new (gboolean val)
{
	GOPangoAttrSubscript *result;

	static PangoAttrClass klass = {
		0,
		go_pango_attr_subscript_copy,
		go_pango_attr_destroy,
		go_pango_attr_subscript_compare
	};

	if (!klass.type)
		klass.type = go_pango_attr_subscript_get_attr_type ();

	result = g_new (GOPangoAttrSubscript, 1);
	result->attr.klass = &klass;
	result->val = val;

	return (PangoAttribute *) result;
}

/**
 * go_pango_attr_superscript_new: (skip)
 * @val: %TRUE if the characters must be raised.
 *
 * Returns: (transfer full): a superscript attribute.
 **/
PangoAttribute *
go_pango_attr_superscript_new (gboolean val)
{
	GOPangoAttrSuperscript *result;

	static PangoAttrClass klass = {
		0,
		go_pango_attr_superscript_copy,
		go_pango_attr_destroy,
		go_pango_attr_superscript_compare
	};

	if (!klass.type)
		klass.type = go_pango_attr_superscript_get_attr_type ();

	result = g_new (GOPangoAttrSuperscript, 1);
	result->attr.klass = &klass;
	result->val = val;

	return (PangoAttribute *) result;
}

static int
go_pango_attr_as_markup_string (PangoAttribute *a, GString *gstr)
{
	int spans = 0;

	switch (a->klass->type) {
	case PANGO_ATTR_FONT_DESC :
		{
			char *str = pango_font_description_to_string
				(((PangoAttrFontDesc *)a)->desc);
			spans += 1;
			g_string_append_printf (gstr, "<span font_desc=\"%s\">", str);
			g_free (str);
		}
		break;
	case PANGO_ATTR_FAMILY :
		spans += 1;
		g_string_append_printf (gstr, "<span font_family=\"%s\">",
					((PangoAttrString *)a)->value);
		break;
	case PANGO_ATTR_ABSOLUTE_SIZE :
	case PANGO_ATTR_SIZE :
		spans += 1;
		g_string_append_printf (gstr, "<span font_size=\"%i\">",
					((PangoAttrSize *)a)->size);
		break;
	case PANGO_ATTR_RISE:
		spans += 1;
		g_string_append_printf (gstr, "<span rise=\"%i\">",
					((PangoAttrInt *)a)->value);
		break;
	case PANGO_ATTR_STYLE :
		spans += 1;
		switch (((PangoAttrInt *)a)->value) {
		case PANGO_STYLE_ITALIC:
			g_string_append (gstr, "<span font_style=\"italic\">");
			break;
		case PANGO_STYLE_OBLIQUE:
			g_string_append (gstr, "<span font_style=\"oblique\">");
			break;
		case PANGO_STYLE_NORMAL:
		default:
			g_string_append (gstr, "<span font_style=\"normal\">");
			break;
		}
		break;
	case PANGO_ATTR_WEIGHT :
		spans += 1;
		g_string_append_printf (gstr, "<span font_weight=\"%i\">",
					((PangoAttrInt *)a)->value);
	break;
	case PANGO_ATTR_STRIKETHROUGH :
		spans += 1;
		if (((PangoAttrInt *)a)->value)
			g_string_append (gstr, "<span strikethrough=\"true\">");
		else
			g_string_append (gstr, "<span strikethrough=\"false\">");
		break;
	case PANGO_ATTR_UNDERLINE :
		spans += 1;
		switch (((PangoAttrInt *)a)->value) {
		case PANGO_UNDERLINE_SINGLE:
			g_string_append (gstr, "<span underline=\"single\">");
			break;
		case PANGO_UNDERLINE_DOUBLE:
			g_string_append (gstr, "<span underline=\"double\">");
			break;
		case PANGO_UNDERLINE_LOW:
			g_string_append (gstr, "<span underline=\"low\">");
			break;
		case PANGO_UNDERLINE_ERROR:
			g_string_append (gstr, "<span underline=\"error\">");
			break;
		case PANGO_UNDERLINE_NONE:
		default:
			g_string_append (gstr, "<span underline=\"none\">");
			break;
		}
		break;
	case PANGO_ATTR_LANGUAGE :
		spans += 1;
		g_string_append_printf (gstr, "<span lang=\"%s\">",
					pango_language_to_string (((PangoAttrLanguage *)a)->value));
		break;
	case PANGO_ATTR_VARIANT :
		spans += 1;
		if (((PangoAttrInt *)a)->value == PANGO_VARIANT_NORMAL)
			g_string_append (gstr, "<span font_variant=\"normal\">");
		else
			g_string_append (gstr, "<span font_variant=\"smallcaps\">");
		break;
	case PANGO_ATTR_LETTER_SPACING :
		spans += 1;
		g_string_append_printf (gstr, "<span letter_spacing=\"%i\">",
					((PangoAttrInt *)a)->value);
		break;
	case PANGO_ATTR_FALLBACK :
		spans += 1;
		if (((PangoAttrInt *)a)->value)
			g_string_append (gstr, "<span fallback=\"true\">");
		else
			g_string_append (gstr, "<span fallback=\"false\">");
		break;
	case PANGO_ATTR_STRETCH :
		spans += 1;
		switch (((PangoAttrInt *)a)->value) {
		case PANGO_STRETCH_ULTRA_CONDENSED:
			g_string_append (gstr, "<span font_stretch=\"ultracondensed\">");
			break;
		case PANGO_STRETCH_EXTRA_CONDENSED:
			g_string_append (gstr, "<span font_stretch=\"extracondensed\">");
			break;
		case PANGO_STRETCH_CONDENSED:
			g_string_append (gstr, "<span font_stretch=\"condensed\">");
			break;
		case PANGO_STRETCH_SEMI_CONDENSED:
			g_string_append (gstr, "<span font_stretch=\"semicondensed\">");
			break;
		case PANGO_STRETCH_SEMI_EXPANDED:
			g_string_append (gstr, "<span font_stretch=\"semiexpanded\">");
			break;
		case PANGO_STRETCH_EXPANDED:
			g_string_append (gstr, "<span font_stretch=\"expanded\">");
			break;
		case PANGO_STRETCH_EXTRA_EXPANDED:
			g_string_append (gstr, "<span font_stretch=\"extraexpanded\">");
			break;
		case PANGO_STRETCH_ULTRA_EXPANDED:
			g_string_append (gstr, "<span font_stretch=\"ultraexpanded\">");
			break;
		case PANGO_STRETCH_NORMAL:
		default:
			g_string_append (gstr, "<span font_stretch=\"normal\">");
			break;
		}
		break;
	case PANGO_ATTR_GRAVITY :
		spans += 1;
		switch (((PangoAttrInt *)a)->value) {
		case PANGO_GRAVITY_SOUTH:
			g_string_append (gstr, "<span gravity=\"south\">");
			break;
		case PANGO_GRAVITY_EAST:
			g_string_append (gstr, "<span gravity=\"east\">");
			break;
		case PANGO_GRAVITY_NORTH:
			g_string_append (gstr, "<span gravity=\"north\">");
			break;
		case PANGO_GRAVITY_WEST:
			g_string_append (gstr, "<span gravity=\"west\">");
			break;
		case PANGO_GRAVITY_AUTO:
		default:
			g_string_append (gstr, "<span gravity=\"auto\">");
			break;
		}
		break;
	case PANGO_ATTR_GRAVITY_HINT :
		spans += 1;
		switch (((PangoAttrInt *)a)->value) {
		case PANGO_GRAVITY_HINT_LINE:
			g_string_append (gstr, "<span gravity_hint=\"line\">");
			break;
		case PANGO_GRAVITY_HINT_STRONG:
			g_string_append (gstr, "<span gravity_hint=\"strong\">");
			break;
		case PANGO_GRAVITY_HINT_NATURAL:
		default:
			g_string_append (gstr, "<span gravity_hint=\"natural\">");
			break;
		}
		break;

	case PANGO_ATTR_FOREGROUND :
		{
			PangoColor *color = &((PangoAttrColor *)a)->color;
			spans += 1;
			g_string_append_printf (gstr, "<span foreground=\"#%02X%02X%02X\">",
						color->red, color->green, color->blue);
		}
		break;
	case PANGO_ATTR_BACKGROUND :
		{
			PangoColor *color = &((PangoAttrColor *)a)->color;
			spans += 1;
			g_string_append_printf (gstr, "<span background=\"#%02X%02X%02X\">",
						color->red, color->green, color->blue);
		}
		break;
	case PANGO_ATTR_UNDERLINE_COLOR :
		{
			PangoColor *color = &((PangoAttrColor *)a)->color;
			spans += 1;
			g_string_append_printf (gstr, "<span underline_color=\"#%02X%02X%02X\">",
						color->red, color->green, color->blue);
		}
		break;
	case PANGO_ATTR_STRIKETHROUGH_COLOR :
		{
			PangoColor *color = &((PangoAttrColor *)a)->color;
			spans += 1;
			g_string_append_printf (gstr, "<span strikethrough_color=\"#%02X%02X%02X\">",
						color->red, color->green, color->blue);
		}
		break;

	case PANGO_ATTR_SCALE :
	case PANGO_ATTR_SHAPE :
	default :
		break; /* ignored */
	}

	return spans;
}

char *
go_pango_attrs_to_markup (PangoAttrList *attrs, char const *text)
{
	PangoAttrIterator * iter;
	int handled = 0;
	int from, to;
	int len;
	GString *gstr;

	if (text == NULL)
		return NULL;
	if (attrs == NULL || go_pango_attr_list_is_empty (attrs))
		return g_strdup (text);

	len = strlen (text);
	gstr = g_string_sized_new (len + 1);

	iter = pango_attr_list_get_iterator (attrs);
	do {
		GSList *list, *l;
		int spans = 0;

		pango_attr_iterator_range (iter, &from, &to);
		to = (to > len) ? len : to;       /* Since "to" can be really big! */
		from = (from > len) ? len : from; /* Since "from" can also be really big! */
		if (from > handled)
			g_string_append_len (gstr, text + handled, from - handled);
		list = pango_attr_iterator_get_attrs (iter);
		for (l = list; l != NULL; l = l->next)
			spans += go_pango_attr_as_markup_string (l->data, gstr);
		g_slist_free (list);
		if (to > from)
			g_string_append_len (gstr, text + from, to - from);
		while (spans-- > 0)
			g_string_append (gstr, "</span>");
		handled = to;
	} while (pango_attr_iterator_next (iter));

	pango_attr_iterator_destroy (iter);

	return g_string_free (gstr, FALSE);
}


/**
 * go_pango_measure_string:
 * @context: #PangoContext
 * @font_desc: #PangoFontDescription
 * @str: The text to measure.
 *
 * A utility function to measure text.
 *
 * Returns: the pixel length of @str according to @context.
 **/
int
go_pango_measure_string (PangoContext *context,
			 PangoFontDescription const *font_desc,
			 char const *str)
{
	PangoLayout *layout = pango_layout_new (context);
	int width;

	pango_layout_set_text (layout, str, -1);
	pango_layout_set_font_description (layout, font_desc);
	pango_layout_get_pixel_size (layout, &width, NULL);

	g_object_unref (layout);

	return width;
}
