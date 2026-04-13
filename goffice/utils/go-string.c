/*
 * go-string.c: ref counted shared strings with richtext and phonetic support
 *
 * Copyright (C) 2008 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2007-2008 Morten Welinder (terra@gnome.org)
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
#include <gsf/gsf-utils.h>
#include <string.h>

/**
 * SECTION: go-string
 * @short_description: Reference-Counted String Support
 *
 * GOString implements a reference-counted string object.
 */

// We use a 32-bit length.  Limit the length of strings a bit more than
// that to reduce the chance of problems.
#define GO_STRING_MAXLEN 0x7ffffffe


/**
 * GOString:
 * @str: the embeded UTF-8 string
 *
 * GOString is a structure containing a string.
 **/
typedef struct {
	GOString base;

	// On-demand filled:
	const char *collate_str;
	const char *casefold_str;
	const char *collate_casefold_str;

	guint32 hash, ref_count, length;
	guint qrich : 1;
} GOStringImpl;


typedef struct {
	GOStringImpl base;
	// Note: the base has collate_str etc. that we stictly
	// speaking don't need.

	GOStringImpl *baseptr;

	PangoAttrList *markup;
	// something, something, phonetic
} GOStringRichImpl;


#define GO_STRING_LEN(s)	(((GOStringImpl const *)(s))->length)

/* Collection of unique strings
 * : GOStringImpl.base.hash -> GOStringImpl * */
static GHashTable *go_strings_base;

static GHashTable *go_strings_rich;


static inline GOStringImpl *
go_string_impl_new (char const *str, guint32 hash, guint32 length)
{
	GOStringImpl *res = g_slice_new0 (GOStringImpl);
	res->base.str = str;
	res->hash = hash;
	res->length = length;
	res->ref_count = 1;
	g_hash_table_replace (go_strings_base, res, res);
	return res;
}

// Length-limited hash function
static guint32
go_str_hash_n (const char *str, size_t n)
{
	// Same spirit as g_str_hash
	const unsigned char *p = (const unsigned char *)str;
	guint32 h = 5381;

	for (; n > 0 && *p != '\0'; p++, n--)
		h = (h << 5) + h + *p;

	return h;
}

/**
 * go_string_new_len:
 * @str: (nullable): string
 * @len: guint32
 *
 * GOString duplicates @str if no string already exists.
 *
 * Returns: (transfer full) (nullable): a #GOString containing @str, or
 * %NULL if @str is %NULL
 **/
GOString *
go_string_new_len (char const *str, guint32 len)
{
	GOStringImpl key, *res;

	if (NULL == str)
		return NULL;

	// Brutally truncate
	len = MIN (len, GO_STRING_MAXLEN);

	key.base.str = str;
	key.length = len;
	key.hash = go_str_hash_n (str, len);
	res = g_hash_table_lookup (go_strings_base, &key);
	if (NULL == res) {
		char *s = g_malloc (len + 1);
		memcpy (s, str, len);
		s[len] = 0;
		GOStringImpl *res = go_string_impl_new (s, key.hash, key.length);
		return &res->base;
	} else
		return go_string_ref (&res->base);
}

/**
 * go_string_new_nocopy_len:
 * @str: (transfer full) (nullable): string
 * @len: guint32
 *
 * Create a #GOString from @str with a length on @len bytes.  This function
 * takes ownership of @str which must be allocated to be at least @len + 1
 * bytes.
 *
 * Returns: (transfer full) (nullable): a #GOString containing @str
 **/
GOString *
go_string_new_nocopy_len (char *str, guint32 len)
{
	GOStringImpl key, *res;

	if (NULL == str)
		return NULL;

	// Brutally truncate
	len = MIN (len, GO_STRING_MAXLEN);

	key.base.str = str;
	key.length   = len;
	key.hash     = go_str_hash_n (str, len);
	res = g_hash_table_lookup (go_strings_base, &key);
	if (NULL == res) {
		// Taking ownership of str.
		str[len] = 0;  // We need to terminate.
		res = go_string_impl_new (str, key.hash, key.length);
		return &res->base;
	}

	g_return_val_if_fail (str != res->base.str, NULL);
	g_free (str);

	return go_string_ref (&res->base);
}

/**
 * go_string_new:
 * @str: (nullable): string
 *
 * GOString duplicates @str if no string already exists.
 *
 * Returns: (transfer full) (nullable): a #GOString containing @str, or
 * %NULL if @str is %NULL
 **/
GOString *
go_string_new (char const *str)
{
	return str ? go_string_new_len (str, strlen (str)) : NULL;
}

/**
 * go_string_new_nocopy:
 * @str: (nullable): string
 *
 * GOString takes ownership of @str
 *
 * Returns: (transfer full) (nullable): a #GOString containing @str, or
 * %NULL if @str is %NULL
 **/
GOString *
go_string_new_nocopy (char *str)
{
	return str ? go_string_new_nocopy_len (str, strlen (str)) : NULL;
}

static GOString *
do_rich_setup (GOString *gstr,
	       PangoAttrList *markup,
	       GOStringPhonetic *phonetic)
{
	if (!gstr) {
		if (NULL != markup) pango_attr_list_unref (markup);
		return NULL;
	}

	GOStringImpl *gstri = (GOStringImpl *)gstr;
	GOStringRichImpl *res = g_slice_new0 (GOStringRichImpl);
	g_hash_table_insert (go_strings_rich, res, gstr);
	res->base.base.str = gstr->str;  // We don't own this string
	res->base.hash = gstri->hash;
	res->base.length = gstri->length;
	res->base.ref_count = 1;
	res->base.qrich = 1;
	res->baseptr = gstri;
	res->markup = markup;
	return &res->base.base;
}

/**
 * go_string_new_rich:
 * @str: (nullable): string.
 * @byte_len: byte length; -1 means to call strlen.
 * @markup: (transfer full) (nullable): optional markup.
 * @phonetic: (transfer full) (nullable): optional list of phonetic extensions.
 *
 * Returns: (transfer full): a string.
 **/
GOString *
go_string_new_rich (char const *str,
		    int byte_len,
		    PangoAttrList *markup,
		    GOStringPhonetic *phonetic)
{
	size_t len = byte_len < 0 ? strlen (str) : (size_t)byte_len;
	GOString *base = go_string_new_len (str, len);
	return do_rich_setup (base, markup, phonetic);
}

/**
 * go_string_new_rich_nocopy:
 * @str: (transfer full) (nullable): string
 * @byte_len: byte length; -1 means to call strlen.
 * @markup: (transfer full) (nullable): optional markup.
 * @phonetic: (transfer full) (nullable): optional list of phonetic extensions.
 *
 * Returns: (transfer full): a string.
 **/
GOString *
go_string_new_rich_nocopy (char *str,
			   int byte_len,
			   PangoAttrList *markup,
			   GOStringPhonetic *phonetic)
{
	size_t len = byte_len < 0 ? strlen (str) : (size_t)byte_len;
	GOString *base = go_string_new_nocopy_len (str, len);
	return do_rich_setup (base, markup, phonetic);
}

GOString *
go_string_ref (GOString *gstr)
{
	if (NULL != gstr)
		((GOStringImpl *)gstr)->ref_count++;
	return gstr;
}

void
go_string_unref (GOString *gstr)
{
	GOStringImpl *impl = (GOStringImpl *)gstr;
	if (NULL == gstr)
		return;

	g_return_if_fail (impl->ref_count > 0);

	if ((--(impl->ref_count)) > 0)
		return;

	g_free ((gpointer)(impl->collate_str));
	g_free ((gpointer)(impl->casefold_str));
	g_free ((gpointer)(impl->collate_casefold_str));

	if (impl->qrich) {
		GOStringRichImpl *rich = (GOStringRichImpl *)impl;
		go_string_unref (&rich->baseptr->base);
		pango_attr_list_unref (rich->markup);
		g_hash_table_remove (go_strings_rich, rich);
		g_slice_free (GOStringRichImpl, rich);
	} else {
		g_hash_table_remove (go_strings_base, gstr);
		g_free ((gpointer)(gstr->str));
		g_slice_free (GOStringImpl, impl);
	}
}

unsigned int
go_string_get_ref_count (GOString const *gstr)
{
	return gstr ? ((GOStringImpl const *)gstr)->ref_count : 0;
}

guint32
go_string_hash (gconstpointer gstr)
{
	return gstr ? ((GOStringImpl const *)gstr)->hash : 0;
}

int
go_string_cmp (gconstpointer gstr_a, gconstpointer gstr_b)
{
	return (gstr_a == gstr_b)
		? 0
		: strcmp (go_string_get_collation (gstr_a),
			  go_string_get_collation (gstr_b));
}

int
go_string_cmp_ignorecase (gconstpointer gstr_a, gconstpointer gstr_b)
{
	return (gstr_a == gstr_b)
		? 0
		: strcmp (go_string_get_casefolded_collate (gstr_a),
			  go_string_get_casefolded_collate (gstr_b));
}

gboolean
go_string_equal (gconstpointer gstr_a, gconstpointer gstr_b)
{
	GOString const *a = gstr_a;
	GOString const *b = gstr_b;
	return a == b || (NULL != a && NULL != b && a->str == b->str);
}

char const *
go_string_get_collation (GOString const *gstr)
{
	GOStringImpl *gstri = (GOStringImpl *)gstr;

	if (NULL == gstr)
		return "";

	if (gstri->qrich)
		gstri = ((GOStringRichImpl *)gstri)->baseptr;

	if (gstri->collate_str == NULL)
		gstri->collate_str = g_utf8_collate_key (gstr->str, GO_STRING_LEN (gstr));

	return gstri->collate_str;
}

char const *
go_string_get_casefold (GOString const *gstr)
{
	GOStringImpl *gstri = (GOStringImpl *)gstr;

	if (NULL == gstr)
		return "";

	if (gstri->qrich)
		gstri = ((GOStringRichImpl *)gstri)->baseptr;

	if (gstri->casefold_str == NULL)
		gstri->casefold_str = g_utf8_casefold (gstr->str, GO_STRING_LEN (gstr));

	return gstri->casefold_str;
}

char const *
go_string_get_casefolded_collate (GOString const *gstr)
{
	GOStringImpl *gstri = (GOStringImpl *)gstr;

	if (NULL == gstr)
		return "";

	if (gstri->qrich)
		gstri = ((GOStringRichImpl *)gstri)->baseptr;

	if (gstri->collate_casefold_str == NULL)
		gstri->collate_casefold_str =
			g_utf8_collate_key (go_string_get_casefold (gstr), -1);

	return gstri->collate_casefold_str;
}

static GOString *go_string_ERROR_val = NULL;

/**
 * go_string_ERROR:
 *
 * A convenience for g_return_val_if_fail to share one error string without adding a
 * reference to functions that do not add references to the result
 *
 * Returns: (transfer none): A string saying 'ERROR'
 **/
GOString *
go_string_ERROR (void)
{
	return go_string_ERROR_val;
}

/*******************************************************************************/

/* Internal comparison routine that actually does a strcmp, rather than
 * assuming that only str == str are equal  */
static gboolean
go_string_equal_internal (gconstpointer gstr_a, gconstpointer gstr_b)
{
	GOStringImpl const *a = gstr_a;
	GOStringImpl const *b = gstr_b;
	return a == b ||
		((a->hash == b->hash) &&
		 (GO_STRING_LEN (a) == GO_STRING_LEN (b)) &&
		 0 == memcmp (a->base.str, b->base.str, GO_STRING_LEN (a)));
}

/**
 * _go_string_init: (skip)
 */
void
_go_string_init (void)
{
	go_strings_base = g_hash_table_new (go_string_hash, go_string_equal_internal);
	go_strings_rich = g_hash_table_new (g_direct_hash, g_direct_equal);

	/* Do it here, so we always have it.  */
	go_string_ERROR_val = go_string_new ("<ERROR>");
}

static gboolean
cb_string_pool_leak (G_GNUC_UNUSED gpointer key,
		     gpointer value,
		     G_GNUC_UNUSED gpointer user)
{
	GOString const *gstr = value;
	g_printerr ("Leaking string [%s] with ref_count=%d.\n",
		    gstr->str, go_string_get_ref_count (gstr));
	return TRUE;
}

/**
 * _go_string_shutdown: (skip)
 */
void
_go_string_shutdown (void)
{
	go_string_unref (go_string_ERROR_val);
	go_string_ERROR_val = NULL;

	g_hash_table_foreach_remove (go_strings_base,
				     cb_string_pool_leak,
				     NULL);
	g_hash_table_destroy (go_strings_base);
	go_strings_base = NULL;

	int nrich = g_hash_table_size (go_strings_rich);
	if (nrich)
		g_printerr ("Leaking %d rich strings\n", nrich);
	g_hash_table_destroy (go_strings_rich);
	go_strings_rich = NULL;
}

static void
cb_collect_strings (G_GNUC_UNUSED gpointer key, gpointer str,
		    gpointer user_data)
{
	GSList **pstrs = user_data;
	*pstrs = g_slist_prepend (*pstrs, str);
}

static gint
cb_by_refcount_str (gconstpointer a_, gconstpointer b_)
{
	GOStringImpl const *a = a_;
	GOStringImpl const *b = b_;

	if (a->ref_count == b->ref_count)
		return strcmp (a->base.str, b->base.str);
	return (a->ref_count - b->ref_count);
}

/**
 * _go_string_dump:
 *
 * Internal debugging utility to dump summary information about the
 * string table.
 **/
void
_go_string_dump (void)
{
	GSList *strs = NULL;
	GSList *l;
	int refs = 0;
	int count;
	size_t len = 0;

	g_hash_table_foreach (go_strings_base, cb_collect_strings, &strs);
	strs = g_slist_sort (strs, cb_by_refcount_str);
	count = g_slist_length (strs);
	for (l = strs; l; l = l->next) {
		GOStringImpl const *s = l->data;
		refs += s->ref_count;
		len  += GO_STRING_LEN (s);
	}

	for (l = g_slist_nth (strs, MAX (0, count - 100)); l; l = l->next) {
		GOStringImpl const *s = l->data;
		g_print ("%8d \"%s\"\n", s->ref_count, s->base.str);
	}
	g_print ("String table contains %d different strings.\n", count);
	g_print ("String table contains a total of %zd characters.\n", len);
	g_print ("String table contains a total of %d refs.\n", refs);
	g_slist_free (strs);
}

/**
 * go_string_foreach_base:
 * @callback: (scope call): callback
 * @data: user data
 *
 * Iterates through the strings data base and apply @callback to each.
 **/
void
go_string_foreach_base (GHFunc callback, gpointer data)
{
	g_hash_table_foreach (go_strings_base, callback, data);
}

static void
value_transform_gostring_string (GValue const *src_val,
				 GValue       *dest)
{
	GOString *src_str = src_val->data[0].v_pointer;
	dest->data[0].v_pointer = src_str
		? g_strndup (src_str->str, GO_STRING_LEN (src_str))
		: NULL;
}

/**
 * go_string_get_type:
 *
 * Register #GOString as a type for #GValue
 *
 * Returns: #GType
 **/
GType
go_string_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOString",
			 (GBoxedCopyFunc)go_string_ref,
			 (GBoxedFreeFunc)go_string_unref);
		g_value_register_transform_func (t, G_TYPE_STRING, value_transform_gostring_string);
	}
	return t;
}

/**
 * go_string_get_len:
 * @gstr: string.
 **/
guint32
go_string_get_len (GOString const *gstr)
{
	return GO_STRING_LEN (gstr);
}


/**
 * go_string_get_markup:
 * @gstr: string.
 *
 * Returns: (transfer none) (nullable): markup from @gstr.
 **/
PangoAttrList *
go_string_get_markup (GOString const *gstr)
{
	GOStringImpl *impl = (GOStringImpl *)gstr;
	return impl->qrich
		? ((GOStringRichImpl *)impl)->markup
		: NULL;
}

/**
 * go_string_get_phonetic: (skip)
 * @gstr: #GOString.
 *
 * Returns: (transfer none) (nullable): the phonetic data.
 **/
GOStringPhonetic *
go_string_get_phonetic (GOString const *gstr)
{
	return NULL;
}


/**
 * go_string_equal_ignorecase:
 * @gstr_a: string.
 * @gstr_b: string.
 *
 * Returns: %TRUE if the two strings are equal when ignoring letter case.
 **/
gboolean
go_string_equal_ignorecase (gconstpointer gstr_a, gconstpointer gstr_b)
{
	return (0 == go_string_cmp_ignorecase (gstr_a, gstr_b));
}


/**
 * go_string_equal_rich:
 * @gstr_a: string.
 * @gstr_b: string.
 **/
gboolean
go_string_equal_rich (gconstpointer gstr_a, gconstpointer gstr_b)
{
	/* TODO  */

	return go_string_equal (gstr_a, gstr_b);
}

static gboolean
find_shape_attr (PangoAttribute *attribute, G_GNUC_UNUSED gpointer data)
{
	return (attribute->klass->type == PANGO_ATTR_SHAPE);
}

/**
 * go_string_trim:
 * @gstr: (transfer full): string.
 * @internal: Trim multiple consequtive internal spaces.
 *
 * Returns: (transfer full): @gstr
 **/
GOString *
go_string_trim (GOString *gstr, gboolean internal)
{
	/*TODO: handle phonetics */
	GOStringImpl *impl = (GOStringImpl *)gstr;
	char *text = NULL;
	char const *ctext;
	PangoAttrList *attrs;
	char const *t;
	int cnt, len;

	if (!impl->qrich)
		return gstr;

	attrs = go_string_get_markup (gstr);
	t = ctext = text = g_strdup (gstr->str);
	if (attrs != NULL)
		attrs = pango_attr_list_copy (attrs);
	while (*t != 0 && *t == ' ')
		t++;
	cnt = t - text;
	if (cnt > 0) {
		len = strlen (t);
		memmove (text, t, len + 1);
		go_pango_attr_list_erase (attrs, 0, cnt);
	} else
		len = strlen(ctext);
	t = ctext + len - 1;
	while (t > ctext && *t == ' ')
		t--;
	cnt = ((t - ctext) + 1);
	if (len > cnt) {
		text[cnt] = '\0';
		go_pango_attr_list_erase (attrs, cnt, len - cnt);
	}

	if (internal) {
		PangoAttrList *at = pango_attr_list_filter (attrs, find_shape_attr, NULL);
		char *tt = text;
		if (at) pango_attr_list_unref (at);
		while (NULL != (tt = strchr (tt, ' '))) {
			if (tt[1] == ' ') {
				go_pango_attr_list_erase (attrs, tt - text, 1);
				memmove (tt + 1, tt + 2, strlen(tt + 2) + 1);
				continue;
			}
			tt++;
		}
	}

	go_string_unref (gstr);

	return go_string_new_rich_nocopy (text, -1, attrs, NULL);
}
