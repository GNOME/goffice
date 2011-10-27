/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-pango-extras.c: Various utility routines that should have been in pango.
 *
 * Authors:
 *    Morten Welinder (terra@gnome.org)
 *    Andreas J. Guelzow (aguelzow@pyrshep.ca)
 */

#include <goffice/goffice-config.h>
#include "go-pango-extras.h"
#include "go-glib-extras.h"
#include <string.h>

struct _GOPangoAttrSuperscript {
	PangoAttribute attr;
};
struct _GOPangoAttrSubscript {
	PangoAttribute attr;
};

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
 * @list : #PangoAttrList
 * @start : starting character index
 * @end : last character index
 * @type : #PangoAttrType
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

static gboolean
go_load_pango_attributes_into_buffer_filter (PangoAttribute *attribute,
					  G_GNUC_UNUSED gpointer data)
{
	return ((PANGO_ATTR_FOREGROUND == attribute->klass->type) ||
		(PANGO_ATTR_UNDERLINE == attribute->klass->type) ||
		(PANGO_ATTR_RISE == attribute->klass->type));
}

static gboolean
go_load_pango_attributes_into_buffer_named_filter (PangoAttribute *attribute,
						    G_GNUC_UNUSED gpointer data)
{
	return ((PANGO_ATTR_STYLE == attribute->klass->type) ||
		(PANGO_ATTR_WEIGHT == attribute->klass->type) ||
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
				go_slist_free_custom (attr, (GFreeFunc)pango_attribute_destroy);
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
					case PANGO_ATTR_UNDERLINE:
						g_object_set (G_OBJECT (tag),
							      "underline",
							      ((PangoAttrInt *)attribute)->value,
							      "underline-set", TRUE,
							      NULL);
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
				go_slist_free_custom (attr, (GFreeFunc)pango_attribute_destroy);
			}
		} while (pango_attr_iterator_next (iter));
		pango_attr_iterator_destroy (iter);
		pango_attr_list_unref (our_markup);
	}
}

static gboolean
filter_func (PangoAttribute *attribute, G_GNUC_UNUSED gpointer data)
{
	PangoAttrType type = attribute->klass->type;
	return (type == go_pango_attr_superscript_get_type () ||
		type == go_pango_attr_subscript_get_type ());
}

static void
go_pango_translate_here (PangoAttrIterator *state_iter, 
			 PangoAttrIterator *attr_iter, PangoAttrList *attrs)
{
	double font_scale = 1.;
	double scale = 1.;
	int rise = 0;
	PangoAttribute *pa;
	PangoFontDescription *desc = pango_font_description_new ();
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
		/* If we haven't used a sub or superscript then */
		/* go_pango_attr_*_type may still be PANGO_ATTR_INVALID */
		if (type != PANGO_ATTR_INVALID) {
			if (type == go_pango_attr_superscript_get_type ()) {
				scale *= GO_SUPERSCRIPT_SCALE;
				rise += GO_SUPERSCRIPT_RISE * font_scale;
				font_scale *= GO_SUPERSCRIPT_SCALE;
			} else { /* go_pango_attr_subscript_type */
				scale *= GO_SUBSCRIPT_SCALE;
				rise += GO_SUBSCRIPT_RISE * font_scale;
				font_scale *= GO_SUBSCRIPT_SCALE;
			}
		}	
	}
	go_slist_free_custom (the_attrs, (GFreeFunc)pango_attribute_destroy);

	if (scale != 1.) {
		PangoAttribute *attr = pango_attr_scale_new (scale);
		attr->start_index = range_start;
		attr->end_index = range_end;
		pango_attr_list_insert (attrs, attr);
	}
	if (rise != 0) {
		PangoAttribute *attr = pango_attr_rise_new (rise);
		attr->start_index = range_start;
		attr->end_index = range_end;
		pango_attr_list_insert (attrs, attr);
	}
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
go_pango_attr_subscript_get_type (void)
{
	static PangoAttrType go_pango_attr_subscript_type = PANGO_ATTR_INVALID;

	if(go_pango_attr_subscript_type == PANGO_ATTR_INVALID)
		go_pango_attr_subscript_type =
			pango_attr_type_register ("GOSubscript");

	return go_pango_attr_subscript_type;
}

PangoAttrType 
go_pango_attr_superscript_get_type (void)
{
	static PangoAttrType go_pango_attr_superscript_type = PANGO_ATTR_INVALID;

	if(go_pango_attr_superscript_type == PANGO_ATTR_INVALID)
		go_pango_attr_superscript_type =
			pango_attr_type_register ("GOSuperscript");

	return go_pango_attr_superscript_type;
}


static PangoAttribute *
go_pango_attr_subscript_copy (G_GNUC_UNUSED PangoAttribute const *attr)
{
	return go_pango_attr_subscript_new ();
}

static PangoAttribute *
go_pango_attr_superscript_copy (G_GNUC_UNUSED PangoAttribute const *attr)
{
	return go_pango_attr_superscript_new ();
}

static void
go_pango_attr_destroy (PangoAttribute *attr)
{
	g_free (attr);
}

static gboolean
go_pango_attr_compare (G_GNUC_UNUSED PangoAttribute const *attr1,
		       G_GNUC_UNUSED PangoAttribute const *attr2)
{
	return FALSE;
}

PangoAttribute *
go_pango_attr_subscript_new (void)
{
	GOPangoAttrSubscript *result;

	static PangoAttrClass klass = {
		0,
		go_pango_attr_subscript_copy,
		go_pango_attr_destroy,
		go_pango_attr_compare
	};

	if (!klass.type)
		klass.type = go_pango_attr_subscript_get_type ();

	result = g_new (GOPangoAttrSubscript, 1);
	result->attr.klass = &klass;

	return (PangoAttribute *) result;
}

PangoAttribute *
go_pango_attr_superscript_new (void)
{
	GOPangoAttrSuperscript *result;

	static PangoAttrClass klass = {
		0,
		go_pango_attr_superscript_copy,
		go_pango_attr_destroy,
		go_pango_attr_compare
	};

	if (!klass.type)
		klass.type = go_pango_attr_superscript_get_type ();

	result = g_new (GOPangoAttrSuperscript, 1);
	result->attr.klass = &klass;

	return (PangoAttribute *) result;
}
