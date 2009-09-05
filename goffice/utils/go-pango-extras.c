/*
 * go-pango-extras.c: Various utility routines that should have been in pango.
 *
 * Authors:
 *    Morten Welinder (terra@gnome.org)
 */

#include <goffice/goffice-config.h>
#include "go-pango-extras.h"

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
