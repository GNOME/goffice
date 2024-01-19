/*
 * go-libxml-extras.c: stuff that should have been in libxml2.
 *
 * Authors:
 *   Daniel Veillard <Daniel.Veillard@w3.org>
 *   Miguel de Icaza <miguel@gnu.org>
 *   Jody Goldberg <jody@gnome.org>
 *   Jukka-Pekka Iivonen <jiivonen@hutcs.cs.hut.fi>
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
#include "go-libxml-extras.h"
#include "go-color.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>

#define CC2XML(s) ((xmlChar const *)(s))
#define CXML2C(s) ((char const *)(s))


/**
 * go_xml_parse_file: (skip)
 * @filename: the locale path to a file to parse.
 *
 * Like xmlParseFile, but faster.  Does not accept compressed files.
 * See http://bugzilla.gnome.org/show_bug.cgi?id=168414
 *
 * Note: this reads the entire file into memory and should therefore
 * not be used for user-supplied files.
 *
 * Returns: (transfer full) (nullable): A libxml2 xmlDocPtr or %NULL.
 **/
xmlDocPtr
go_xml_parse_file (char const *filename)
{
	xmlDocPtr result = NULL;
	gchar *contents;
	gsize length;

	if (g_file_get_contents (filename, &contents, &length, NULL)) {
		result = xmlParseMemory (contents, length);
		g_free (contents);
	}

	return result;
}

/**
 * go_xml_node_get_cstr: (skip)
 * @node: #xmlNodePtr
 * @name: attribute name
 * Get an xmlChar * value for a node carried as an attibute
 * result must be xmlFree
 *
 * Returns: (transfer full) (nullable): the attribute value
 */
xmlChar *
go_xml_node_get_cstr (xmlNodePtr node, char const *name)
{
	if (name != NULL)
		return xmlGetProp (node, CC2XML (name));
	/* in libxml1 <foo/> would return NULL
	 * in libxml2 <foo/> would return ""
	 */
	if (node->xmlChildrenNode != NULL)
		return xmlNodeGetContent (node);
	return NULL;
}
void
go_xml_node_set_cstr (xmlNodePtr node, char const *name, char const *val)
{
	if (name)
		xmlSetProp (node, CC2XML (name), CC2XML (val));
	else
		xmlNodeSetContent (node, CC2XML (val));
}

gboolean
go_xml_node_get_bool (xmlNodePtr node, char const *name, gboolean *val)
{
	xmlChar *buf = go_xml_node_get_cstr (node, name);
	if (buf == NULL)
		return FALSE;

	*val = (!strcmp (CXML2C (buf), "1")
		|| 0 == g_ascii_strcasecmp (CXML2C (buf), "true"));
	xmlFree (buf);
	return TRUE;
}

void
go_xml_node_set_bool (xmlNodePtr node, char const *name, gboolean val)
{
	go_xml_node_set_cstr (node, name, val ? "true" : "false");
}

gboolean
go_xml_node_get_int (xmlNodePtr node, char const *name, int *val)
{
	xmlChar *buf;
	char *end;
	gboolean ok;
	long l;

	buf = go_xml_node_get_cstr (node, name);
	if (buf == NULL)
		return FALSE;

	errno = 0; /* strto(ld) sets errno, but does not clear it.  */
	*val = l = strtol (CXML2C (buf), &end, 10);
	ok = (CXML2C (buf) != end) && *end == 0 && errno != ERANGE && (*val == l);
	xmlFree (buf);

	return ok;
}

void
go_xml_node_set_int (xmlNodePtr node, char const *name, int val)
{
	char str[4 * sizeof (int)];
	sprintf (str, "%d", val);
	go_xml_node_set_cstr (node, name, str);
}

gboolean
go_xml_node_get_double (xmlNodePtr node, char const *name, double *val)
{
	xmlChar *buf;
	char *end;
	gboolean ok;

	buf = go_xml_node_get_cstr (node, name);
	if (buf == NULL)
		return FALSE;

	errno = 0; /* strto(ld) sets errno, but does not clear it.  */
	*val = strtod (CXML2C (buf), &end);
	ok = (CXML2C (buf) != end) && *end == 0 && errno != ERANGE;
	xmlFree (buf);

	return ok;
}

void
go_xml_node_set_double (xmlNodePtr node, char const *name, double val,
			int precision)
{
	char str[101 + DBL_DIG];

	if (precision < 0 || precision > DBL_DIG)
		precision = DBL_DIG;

	if (fabs (val) < 1e9 && fabs (val) > 1e-5)
		g_snprintf (str, 100 + DBL_DIG, "%.*g", precision, val);
	else
		g_snprintf (str, 100 + DBL_DIG, "%f", val);

	go_xml_node_set_cstr (node, name, str);
}


gboolean
go_xml_node_get_gocolor (xmlNodePtr node, char const *name, GOColor *res)
{
	xmlChar *color;
	int r, g, b;

	color = xmlGetProp (node, CC2XML (name));
	if (color == NULL)
		return FALSE;
	if (sscanf (CXML2C (color), "%X:%X:%X", &r, &g, &b) == 3) {
		r >>= 8;
		g >>= 8;
		b >>= 8;
		*res = GO_COLOR_FROM_RGBA (r,g,b,0xff);
		xmlFree (color);
		return TRUE;
	}
	xmlFree (color);
	return FALSE;
}

void
go_xml_node_set_gocolor (xmlNodePtr node, char const *name, GOColor val)
{
	unsigned r, g, b;
	char str[4 * sizeof (val)];

	GO_COLOR_TO_RGB (val, &r, &g, &b);
	sprintf (str, "%X:%X:%X", r, g, b);
	go_xml_node_set_cstr (node, name, str);
}

gboolean
go_xml_node_get_enum (xmlNodePtr node, char const *name, GType etype, gint *val)
{
	GEnumClass *eclass = G_ENUM_CLASS (g_type_class_peek (etype));
	GEnumValue *ev;
	xmlChar *s;
	int i;

	s = xmlGetProp (node, CC2XML (name));
	if (s == NULL)
		return FALSE;

	ev = g_enum_get_value_by_name (eclass, CXML2C (s));
	if (!ev) ev = g_enum_get_value_by_nick (eclass, CXML2C (s));
	if (!ev && go_xml_node_get_int (node, name, &i))
		/* Check that the value is valid.  */
		ev = g_enum_get_value (eclass, i);
	xmlFree (s);
	if (!ev) return FALSE;

	*val = ev->value;
	return TRUE;
}

void
go_xml_node_set_enum (xmlNodePtr node, char const *name, GType etype, gint val)
{
	GEnumClass *eclass = G_ENUM_CLASS (g_type_class_peek (etype));
	GEnumValue *ev = g_enum_get_value (eclass, val);

	if (ev)
		go_xml_node_set_cstr (node, name, ev->value_name);
	else
		g_warning ("Invalid value %d for type %s",
			   val, g_type_name (etype));
}

/*************************************************************************/

/**
 * go_xml_get_child_by_name: (skip)
 * @tree: #xmlNode
 * @name: child name
 *
 * Returns: (transfer none) (nullable): the child with @name as name if any.
 **/
xmlNode *
go_xml_get_child_by_name (xmlNode const *parent, char const *child_name)
{
	xmlNode *child;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (child_name != NULL, NULL);

	for (child = parent->xmlChildrenNode; child != NULL; child = child->next) {
		if (xmlStrcmp (child->name, CC2XML (child_name)) == 0) {
			return child;
		}
	}
	return NULL;
}

/**
 * go_xml_get_child_by_name_no_lang: (skip)
 * @tree: #xmlNode
 * @name: child name
 *
 * Returns: (transfer none) (nullable): the child with @name as name and
 * without "xml:lang" attribute if any.
 **/
xmlNode *
go_xml_get_child_by_name_no_lang (xmlNode const *parent, char const *name)
{
	xmlNodePtr node;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	for (node = parent->xmlChildrenNode; node != NULL; node = node->next) {
		xmlChar *lang;

		if (node->name == NULL || strcmp (CXML2C (node->name), name) != 0) {
			continue;
		}
		lang = xmlGetProp (node, CC2XML ("xml:lang"));
		if (lang == NULL) {
			return node;
		}
		xmlFree (lang);
	}

	return NULL;
}

/**
 * go_xml_get_child_by_name_by_lang: (skip)
 * @tree: #xmlNode
 * @name: child name
 *
 * Returns: (transfer none) (nullable): the child with @name as name and
 * with "xml:lang" attribute corresponding to the preferred language.
 **/
xmlNode *
go_xml_get_child_by_name_by_lang (xmlNode const *parent, gchar const *name)
{
	xmlNodePtr   best_node = NULL, node;
	gint         best_lang_score = INT_MAX;
	char const * const *langs = g_get_language_names ();

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	for (node = parent->xmlChildrenNode; node != NULL; node = node->next) {
		xmlChar *lang;

		if (node->name == NULL || strcmp (CXML2C (node->name), name) != 0)
			continue;

		lang = xmlGetProp (node, CC2XML ("lang"));
		if (lang != NULL) {
			gint i;

			for (i = 0; langs[i] != NULL && i < best_lang_score; i++) {
				if (strcmp (langs[i], CXML2C (lang)) == 0) {
					best_node = node;
					best_lang_score = i;
				}
			}
			xmlFree (lang);
		} else if (best_node == NULL)
			best_node = node;

		if (best_lang_score == 0)
			return best_node;
	}

	return best_node;
}

/**
 * go_xml_out_add_double:
 * @output: destination
 * @id: (allow-none): attribute name
 * @d: value
 *
 * Output a representation of @d that will be read back without loss of
 * precision.
 */
void
go_xml_out_add_double (GsfXMLOut *output, char const *id, double d)
{
	GString *str = g_string_new (NULL);
	go_dtoa (str, "!g", d);
	gsf_xml_out_add_cstr (output, id, str->str);
	g_string_free (str, TRUE);
}

#ifdef GOFFICE_WITH_LONG_DOUBLE
/**
 * go_xml_out_add_long_double:
 * @output: destination
 * @id: (allow-none): attribute name
 * @ld: value
 *
 * Output a representation of @ld that will be read back without loss of
 * precision.
 */
void
go_xml_out_add_long_double (GsfXMLOut *output, char const *id, long double ld)
{
	GString *str = g_string_new (NULL);
	go_dtoa (str, "!Lg", ld);
	gsf_xml_out_add_cstr (output, id, str->str);
	g_string_free (str, TRUE);
}
#endif

#ifdef GOFFICE_WITH_DECIMAL64
/**
 * go_xml_out_add_decimal64:
 * @output: destination
 * @id: (allow-none): attribute name
 * @d: value
 *
 * Output a representation of @d that will be read back without loss of
 * precision.
 */
void
go_xml_out_add_decimal64 (GsfXMLOut *output, char const *id, _Decimal64 d)
{
	GString *str = g_string_new (NULL);
	go_dtoa (str, "!" GO_DECIMAL64_MODIFIER "g", d);
	gsf_xml_out_add_cstr (output, id, str->str);
	g_string_free (str, TRUE);
}
#endif

void
go_xml_out_add_color (GsfXMLOut *output, char const *id, GOColor c)
{
	char *str = go_color_as_str (c);
	gsf_xml_out_add_cstr_unchecked (output, id, str);
	g_free (str);
}

static GSList *xml_in_docs;

/**
 * _go_libxml_extras_shutdown: (skip)
 */
void
_go_libxml_extras_shutdown (void)
{
	GSList *l;

	for (l = xml_in_docs; l; l = l->next) {
		GsfXMLInDoc **pdoc = l->data;
		gsf_xml_in_doc_free (*pdoc);
		*pdoc = NULL;
	}
	g_slist_free (xml_in_docs);
	xml_in_docs = NULL;
}

void
go_xml_in_doc_dispose_on_exit (GsfXMLInDoc **pdoc)
{
	xml_in_docs = g_slist_prepend (xml_in_docs, pdoc);
}
