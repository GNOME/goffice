/*
 * go-glib-extras.c:  Various utility routines that should have been in glib.
 *
 * Authors:
 *    Miguel de Icaza (miguel@gnu.org)
 *    Jukka-Pekka Iivonen (iivonen@iki.fi)
 *    Zbigniew Chyla (cyba@gnome.pl)
 *    Morten Welinder (terra@gnome.org)
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
#include "go-glib-extras.h"
#include "go-locale.h"
#include <goffice/app/go-cmd-context.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <libxml/encoding.h>

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/**
 * go_hash_keys:
 * @hash: #GHashTable
 *
 * Collects an unordered list of the keys in @hash.
 *
 * Returns: (element-type void) (transfer container): a list which the
 * caller needs to free. The content has not additional references added
 *
 * Note: consider using g_hash_table_get_keys instead.
 **/
GSList *
go_hash_keys (GHashTable *hash)
{
	GHashTableIter hiter;
	GSList *accum = NULL;
	gpointer key;

	g_hash_table_iter_init (&hiter, hash);
	while (g_hash_table_iter_next (&hiter, &key, NULL))
		accum = g_slist_prepend (accum, key);

	return accum;
}

/***************************************************************************/

/**
 * go_ptr_array_insert:
 * @array: (element-type void): a #GPtrArray
 * @value: the pointer to insert
 * @index: where to insert @value
 *
 * Inserts a pointer inside an existing array.
 *
 * Note: consider using g_ptr_array_insert instead, but watch out of the
 * argument order.
 **/
void
go_ptr_array_insert (GPtrArray *array, gpointer value, int index)
{
	g_ptr_array_insert (array, index, value);
}

/**
 * go_slist_create:
 * @item1: (allow-none) (transfer none): first item
 * @...: (transfer none): %NULL terminated list of additional items
 *
 * Creates a GList from NULL-terminated list of arguments.
 * As the arguments are just copied to the list, the caller owns them.
 * Returns: (element-type void) (transfer container): created list.
 **/
GSList *
go_slist_create (gconstpointer item1, ...)
{
	va_list args;
	GSList *list = NULL;
	gpointer item;

	va_start (args, item1);
	for (item = (gpointer)item1; item; item = va_arg (args, gpointer)) {
		list = g_slist_prepend (list, item);
	}
	va_end (args);

	return g_slist_reverse (list);
}

/**
 * go_slist_map:
 * @list: (element-type void): list of some items
 * @map_func: (scope call): mapping function
 *
 * The ownership of the list elements depends on map_func.
 * Returns: (element-type void) (transfer container): the mapped list
 *
 * Note: consider using g_slist_copy_deep instead.
 **/
GSList *
go_slist_map (GSList const *list, GOMapFunc map_func)
{
	return g_slist_copy_deep ((GSList *)list, (GCopyFunc)map_func, NULL);
}

/**
 * go_list_index_custom:
 * @list: (element-type void): #GList
 * @data: element for which the index is searched for
 * @cmp_func: (scope call): #GCompareFunc
 *
 * Searched for @data in @list and return the corresponding index or -1 if not
 * found.
 *
 * Returns: the data index in the list.
 **/
gint
go_list_index_custom (GList *list, gpointer data, GCompareFunc cmp_func)
{
	GList *l;
	gint i;

	for (l = list, i = 0; l != NULL; l = l->next, i++) {
		if (cmp_func (l->data, data) == 0) {
			return i;
		}
	}

	return -1;
}

/**
 * go_strsplit_to_slist:
 * @str: String to split
 * @delimiter: Token delimiter
 *
 * Splits up string into tokens at delim and returns a string list.
 *
 * Returns: (element-type utf8) (transfer full): string list which you should
 * free after use using function g_slist_free_full(), using g_free as second
 * argument.
 **/
GSList *
go_strsplit_to_slist (gchar const *string, gchar delimiter)
{
	gchar **token_v;
	GSList *string_list = NULL;
	char buf[2] = { '\0', '\0' };
	gint i;

	buf[0] = delimiter;
	token_v = g_strsplit (string, buf, 0);
	if (token_v != NULL) {
		for (i = 0; token_v[i] != NULL; i++) {
			string_list = g_slist_prepend (string_list, token_v[i]);
		}
		string_list = g_slist_reverse (string_list);
		g_free (token_v);
	}

	return string_list;
}

gint
go_utf8_collate_casefold (const char *a, const char *b)
{
	char *a2 = g_utf8_casefold (a, -1);
	char *b2 = g_utf8_casefold (b, -1);
	int res = g_utf8_collate (a2, b2);
	g_free (a2);
	g_free (b2);
	return res;
}

gint
go_ascii_strcase_equal (gconstpointer v1, gconstpointer v2)
{
	return g_ascii_strcasecmp ((char const *) v1, (char const *)v2) == 0;
}

/* a char* hash function from ASU */
guint
go_ascii_strcase_hash (gconstpointer v)
{
	unsigned const char *s = (unsigned const char *)v;
	unsigned const char *p;
	guint h = 0, g;

	for(p = s; *p != '\0'; p += 1) {
		h = ( h << 4 ) + g_ascii_tolower (*p);
		if ( ( g = h & 0xf0000000 ) ) {
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}

	return h /* % M */;
}

/* ------------------------------------------------------------------------- */

/*
 * Escapes all backslashes and quotes in a string. It is based on glib's
 * g_strescape.
 *
 * Also adds quotes around the result.
 */
void
go_strescape (GString *target, char const *string)
{
	g_string_append_c (target, '"');
	/* This loop should be UTF-8 safe.  */
	for (; *string; string++) {
		switch (*string) {
		case '"':
		case '\\':
			g_string_append_c (target, '\\');
		default:
			g_string_append_c (target, *string);
		}
	}
	g_string_append_c (target, '"');
}

/*
 * The reverse operation of go_strescape.  Returns a pointer to the
 * first char after the string on success or %NULL on failure.
 *
 * First character of the string should be an ASCII character used
 * for quoting.
 */
const char *
go_strunescape (GString *target, const char *string)
{
	char quote = *string++;
	size_t oldlen = target->len;

	/* This should be UTF-8 safe as long as quote is ASCII.  */
	while (*string != quote) {
		if (*string == 0)
			goto error;
		else if (*string == '\\') {
			string++;
			if (*string == 0)
				goto error;
		}

		g_string_append_c (target, *string);
		string++;
	}

	return ++string;

 error:
	g_string_truncate (target, oldlen);
	return NULL;
}

void
go_string_append_gstring (GString *target, const GString *source)
{
	g_string_append_len (target, source->str, source->len);
}

void
go_string_append_c_n (GString *target, char c, gsize n)
{
	gsize len = target->len;
	g_string_set_size (target, len + n);
	memset (target->str + len, c, n);
}

void
go_string_replace (GString *target,
		   gsize pos, gssize oldlen,
		   const char *txt, gssize newlen)
{
	gsize cplen;

	g_return_if_fail (target != NULL);
	g_return_if_fail (pos >= 0);
	g_return_if_fail (pos <= target->len);

	if (oldlen < 0)
		oldlen = target->len - pos;
	if (newlen < 0)
		newlen = strlen (txt);

	cplen = MIN (oldlen, newlen);
	memcpy (target->str + pos, txt, cplen);

	pos += cplen;
	oldlen -= cplen;
	txt += cplen;
	newlen -= cplen;

	/*
	 * At least one of oldlen and newlen is zero now.  We could call
	 * both erase and insert unconditionally, but erase does not appear
	 * to handle zero length efficiently.
	 */

	if (oldlen > 0)
		g_string_erase (target, pos, oldlen);
	else if (newlen > 0)
		g_string_insert_len (target, pos, txt, newlen);
}

int
go_unichar_issign (gunichar uc)
{
	switch (uc) {
	case '+':
	case 0x207a: /* Superscript plus */
	case 0x208a: /* Subscript plus */
	case 0x2795: /* Unicode heavy plus */
	case 0xfb29: /* Hebrew plus */
	case 0xfe62: /* Unicode small plus */
	case 0xff0b: /* Variant of '+' for CJK */
		return +1;
	case '-':
	/* case 0x2052: /\* Commercial minus *\/ */
	case 0x207b: /* Superscript minus */
	case 0x208b: /* Subscript minus */
	case 0x2212: /* Unicode minus */
	case 0x2796: /* Unicode heavy minus */
	case 0x8ca0: /* Traditional Chinese minus */
	case 0x8d1f: /* Simplified Chinese minus */
	case 0xfe63: /* Unicode small minus */
	case 0xff0d: /* Variant of '-' for CJK */
		return -1;
	default:
		return 0;
	}
}

/* ------------------------------------------------------------------------- */

/**
 * go_utf8_strcapital:
 * @p: pointer to UTF-8 string
 * @len: length in bytes, or -1.
 *
 * Similar to g_utf8_strup and g_utf8_strup, except that this function
 * creates a string "Very Much Like: This, One".
 *
 * Return value: newly allocated string.
 **/
char *
go_utf8_strcapital (const char *p, gssize len)
{
	const char *pend = (len < 0 ? NULL : p + len);
	GString *res = g_string_sized_new (len < 0 ? 1 : len + 1);
	gboolean up = TRUE;

	/*
	 * This does a simple character-by-character mapping and probably
	 * is not linguistically correct.
	 */

	for (; (len < 0 || p < pend) && *p; p = g_utf8_next_char (p)) {
		gunichar c = g_utf8_get_char (p);

		if (g_unichar_isalpha (c)) {
			if (up ? g_unichar_isupper (c) : g_unichar_islower (c))
				/* Correct case -- keep the char.  */
				g_string_append_unichar (res, c);
			else {
				char *tmp = up
					? g_utf8_strup (p, 1)
					: g_utf8_strdown (p, 1);
				g_string_append (res, tmp);
				g_free (tmp);
			}
			up = FALSE;
		} else {
			g_string_append_unichar (res, c);
			up = TRUE;
		}
	}

	return g_string_free (res, FALSE);
}

/* ------------------------------------------------------------------------- */

#undef DEBUG_CHUNK_ALLOCATOR

typedef struct _go_mem_chunk_freeblock go_mem_chunk_freeblock;
typedef struct _go_mem_chunk_block go_mem_chunk_block;

struct _go_mem_chunk_freeblock {
	go_mem_chunk_freeblock *next;
};

struct _go_mem_chunk_block {
	gpointer data;
	int freecount, nonalloccount;
	go_mem_chunk_freeblock *freelist;
#ifdef DEBUG_CHUNK_ALLOCATOR
	int id;
#endif
};

struct _GOMemChunk {
	char *name;
	size_t atom_size, user_atom_size, chunk_size, alignment;
	int atoms_per_block;

	/* List of all blocks.  */
	GSList *blocklist;

	/* List of blocks that are not full.  */
	GList *freeblocks;

#ifdef DEBUG_CHUNK_ALLOCATOR
	int blockid;
#endif
	unsigned ref_count;
};


GOMemChunk *
go_mem_chunk_new (char const *name, gsize user_atom_size, gsize chunk_size)
{
	int atoms_per_block;
	GOMemChunk *res;
	size_t user_alignment, alignment, atom_size;
	size_t maxalign = 1 + ((sizeof (void *) - 1) |
			       (sizeof (long) - 1) |
			       (sizeof (double) - 1));

	/*
	 * The alignment that the caller can expect is 2^(lowest_bit_in_size).
	 * The fancy bit math computes this.  Think it over.
	 *
	 * We don't go lower than pointer-size, so this always comes out as
	 * 4 or 8.  Sometimes, when user_atom_size is a multiple of 8, this
	 * alignment is bigger than really needed, but we don't know if the
	 * structure has elements with 8-byte alignment.  In those cases we
	 * waste memory.
	 */
	user_alignment = ((user_atom_size ^ (user_atom_size - 1)) + 1) / 2;
	alignment = MIN (MAX (user_alignment, sizeof (go_mem_chunk_block *)), maxalign);
	atom_size = alignment + MAX (user_atom_size, sizeof (go_mem_chunk_freeblock));
	atoms_per_block = MAX (1, chunk_size / atom_size);
	chunk_size = atoms_per_block * atom_size;

#ifdef DEBUG_CHUNK_ALLOCATOR
	g_print ("Created %s with alignment=%d, atom_size=%d (%d), chunk_size=%d.\n",
		 name, alignment, atom_size, user_atom_size,
		 chunk_size);
#endif

	res = g_new (GOMemChunk, 1);
	res->alignment = alignment;
	res->name = g_strdup (name);
	res->user_atom_size = user_atom_size;
	res->atom_size = atom_size;
	res->chunk_size = chunk_size;
	res->atoms_per_block = atoms_per_block;
	res->blocklist = NULL;
	res->freeblocks = NULL;
#ifdef DEBUG_CHUNK_ALLOCATOR
	res->blockid = 0;
#endif
	res->ref_count = 1;

	return res;
}

void
go_mem_chunk_destroy (GOMemChunk *chunk, gboolean expect_leaks)
{
	GSList *l;

	g_return_if_fail (chunk != NULL);

	if (chunk->ref_count-- > 1)
		return;
#ifdef DEBUG_CHUNK_ALLOCATOR
	g_print ("Destroying %s.\n", chunk->name);
#endif
	/*
	 * Since this routine frees all memory allocated for the pool,
	 * it is sometimes convenient not to free at all.  For such
	 * cases, don't report leaks.
	 */
	if (!expect_leaks) {
		GSList *l;
		int leaked = 0;

		for (l = chunk->blocklist; l; l = l->next) {
			go_mem_chunk_block *block = l->data;
			leaked += chunk->atoms_per_block - (block->freecount + block->nonalloccount);
		}
		if (leaked) {
			g_warning ("Leaked %d nodes from %s.",
				   leaked, chunk->name);
		}
	}

	for (l = chunk->blocklist; l; l = l->next) {
		go_mem_chunk_block *block = l->data;
		g_free (block->data);
		g_free (block);
	}
	g_slist_free (chunk->blocklist);
	g_list_free (chunk->freeblocks);
	g_free (chunk->name);
	g_free (chunk);
}

static GOMemChunk *
go_mem_chunk_ref (GOMemChunk *chunk)
{
	chunk->ref_count++;
	return chunk;
}

GType
go_mem_chunk_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOMemChunk",
			 (GBoxedCopyFunc)go_mem_chunk_ref,
			 (GBoxedFreeFunc)go_mem_chunk_destroy);
	}
	return t;
}

/**
 * go_mem_chunk_alloc:
 * @chunk: #GOMemChunk
 *
 * Returns: (transfer full): an unused memory block
 **/
gpointer
go_mem_chunk_alloc (GOMemChunk *chunk)
{
	go_mem_chunk_block *block;
	char *res;

	/* First try the freelist.  */
	if (chunk->freeblocks) {
		go_mem_chunk_freeblock *res;

		block = chunk->freeblocks->data;
		res = block->freelist;
		if (res) {
			block->freelist = res->next;

			block->freecount--;
			if (block->freecount == 0 && block->nonalloccount == 0) {
				/* Block turned full -- remove it from freeblocks.  */
				chunk->freeblocks = g_list_delete_link (chunk->freeblocks,
									chunk->freeblocks);
			}
			return res;
		}
		/*
		 * If we get here, the block has free space that was never
		 * allocated.
		 */
	} else {
		block = g_new (go_mem_chunk_block, 1);
#ifdef DEBUG_CHUNK_ALLOCATOR
		block->id = chunk->blockid++;
		g_print ("Allocating new chunk %d for %s.\n", block->id, chunk->name);
#endif
		block->nonalloccount = chunk->atoms_per_block;
		block->freecount = 0;
		block->data = g_malloc (chunk->chunk_size);
		block->freelist = NULL;

		chunk->blocklist = g_slist_prepend (chunk->blocklist, block);
		chunk->freeblocks = g_list_prepend (chunk->freeblocks, block);
	}

	res = (char *)block->data +
		(chunk->atoms_per_block - block->nonalloccount--) * chunk->atom_size;
	*((go_mem_chunk_block **)res) = block;

	if (block->nonalloccount == 0 && block->freecount == 0) {
		/* Block turned full -- remove it from freeblocks.  */
		chunk->freeblocks = g_list_delete_link (chunk->freeblocks, chunk->freeblocks);
	}

	return res + chunk->alignment;
}

/**
 * go_mem_chunk_alloc0:
 * @chunk: #GOMemChunk
 *
 * Returns: (transfer full): an unused memory block filled with 0
 **/
gpointer
go_mem_chunk_alloc0 (GOMemChunk *chunk)
{
	gpointer res = go_mem_chunk_alloc (chunk);
	memset (res, 0, chunk->user_atom_size);
	return res;
}

/**
 * go_mem_chunk_free:
 * @chunk: #GOMemChunk
 * @mem: (transfer full): item to release
 *
 * Returns the given item to the pool.
 **/
void
go_mem_chunk_free (GOMemChunk *chunk, gpointer mem)
{
	go_mem_chunk_freeblock *fb = mem;
	go_mem_chunk_block *block =
		*((go_mem_chunk_block **)((char *)mem - chunk->alignment));

#if 0
	/*
	 * This is useful when the exact location of a leak needs to be
	 * pin-pointed.
	 */
	memset (mem, 0, chunk->user_atom_size);
#endif

	fb->next = block->freelist;
	block->freelist = fb;
	block->freecount++;

	if (block->freecount == 1 && block->nonalloccount == 0) {
		/* Block turned non-full.  */
		chunk->freeblocks = g_list_prepend (chunk->freeblocks, block);
	} else if (block->freecount == chunk->atoms_per_block) {
		/* Block turned all-free.  */

#ifdef DEBUG_CHUNK_ALLOCATOR
		g_print ("Releasing chunk %d for %s.\n", block->id, chunk->name);
#endif
		/*
		 * FIXME -- this could be faster if we rolled our own lists.
		 * Hopefully, however, (a) the number of blocks is small,
		 * and (b) the freed block might be near the beginning ("top")
		 * of the stacks.
		 */
		chunk->blocklist = g_slist_remove (chunk->blocklist, block);
		chunk->freeblocks = g_list_remove (chunk->freeblocks, block);

		g_free (block->data);
		g_free (block);
	}
}

/**
 * go_mem_chunk_foreach_leak:
 * @chunk: #GOMemChunk
 * @cb: (scope call): callback
 * @user: user data for @cb
 *
 * Loop over all non-freed memory in the chunk.  It's safe to allocate or free
 * from the chunk in the callback.
 */
void
go_mem_chunk_foreach_leak (GOMemChunk *chunk, GFunc cb, gpointer user)
{
	GSList *l, *leaks = NULL;

	for (l = chunk->blocklist; l; l = l->next) {
		go_mem_chunk_block *block = l->data;
		if (chunk->atoms_per_block - (block->freecount + block->nonalloccount) > 0) {
			char *freed = g_new0 (char, chunk->atoms_per_block);
			go_mem_chunk_freeblock *fb = block->freelist;
			int i;

			while (fb) {
				char *atom = (char *)fb - chunk->alignment;
				int no = (atom - (char *)block->data) / chunk->atom_size;
				freed[no] = 1;
				fb = fb->next;
			}

			for (i = chunk->atoms_per_block - block->nonalloccount - 1; i >= 0; i--) {
				if (!freed[i]) {
					char *atom = (char *)block->data + i * chunk->atom_size;
					leaks = g_slist_prepend (leaks, atom + chunk->alignment);
				}
			}
			g_free (freed);
		}
	}

	g_slist_foreach (leaks, cb, user);
	g_slist_free (leaks);
}

int
go_str_compare (void const *x, void const *y)
{
	if (x == y)
		return 0;

	if (x == NULL || y == NULL)
		return x ? -1 : 1;

	return strcmp (x, y);
}


const char *
go_guess_encoding (const char *raw, gsize len, const char *user_guess,
		   GString **utf8_str, guint *truncated)
{
	int try;
	gboolean debug = go_debug_flag ("encoding");

	g_return_val_if_fail (raw != NULL, NULL);

	for (try = 1; 1; try++) {
		char const *guess = NULL;
		GError *error = NULL;
		char *utf8_data;
		gsize bytes_written = 0;
		gsize bytes_read = 0;

		switch (try) {
		case 1: guess = user_guess; break;
		case 2: {
			xmlCharEncoding enc =
				xmlDetectCharEncoding ((const unsigned char*)raw, len);
			switch (enc) {
			case XML_CHAR_ENCODING_ERROR:
			case XML_CHAR_ENCODING_NONE:
				break;
			case XML_CHAR_ENCODING_UTF16LE:
				/* Default would give "UTF-16".  */
				guess = "UTF-16LE";
				break;
			case XML_CHAR_ENCODING_UTF16BE:
				/* Default would give "UTF-16".  */
				guess = "UTF-16BE";
				break;
			default:
				guess = xmlGetCharEncodingName (enc);
			}
			break;
		}
		case 3: guess = "ASCII"; break;
		case 4: guess = "UTF-8"; break;
		case 5: g_get_charset (&guess); break;
		case 6: guess = "ISO-8859-1"; break;
		default: return NULL;
		}

		if (!guess)
			continue;

		if (debug)
			g_printerr ("Trying %s as encoding using method %d.\n",
				    guess, try);

		utf8_data = g_convert (raw, len, "UTF-8", guess,
				       &bytes_read, &bytes_written, &error);
		if (!error) {
			/*
			 * We can actually fail this test when guess is UTF-8,
			 * see #401588.
			 */
			if (!g_utf8_validate (utf8_data, -1, NULL)) {
				g_free (utf8_data);
				continue;
			}
			if (debug)
				g_printerr ("Guessed %s as encoding.\n", guess);
			if (utf8_str)
				*utf8_str = g_string_new_len
					(utf8_data, bytes_written);
			g_free (utf8_data);
			if (truncated)
				*truncated = len - bytes_read;
			return guess;
		}

		g_error_free (error);
	}
}

static char *go_real_name = NULL;

/**
 * go_get_real_name:
 *
 * Returns: (transfer none): a UTF-8 encoded string with the current user name.
 **/
char const *
go_get_real_name (void)
{
	if (go_real_name == NULL) {
		char const *name = getenv ("NAME");
		if (name == NULL)
			name = g_get_real_name ();
		if (name == NULL)
			name = g_get_user_name ();
		if (name != NULL) {
			GString *converted_name = NULL;
			(void) go_guess_encoding (name, strlen (name),
						  NULL, &converted_name, NULL);
			if (converted_name)
				go_real_name = g_string_free (converted_name, FALSE);
		}
		if (go_real_name == NULL)
			go_real_name = g_strdup ("unknown");
	}
	return go_real_name;
}

/**
 * go_destroy_password:
 * @passwd: (transfer none): The buffer to clear
 *
 * Overwrite a string holding a password.  This is a separate routine to
 * ensure that the compiler does not try to outsmart us.
 *
 * Note: this does not free the memory.
 **/
void
go_destroy_password (char *passwd)
{
	memset (passwd, 0, strlen (passwd));
}


/**
 * go_memdup:
 * @mem: (nullable): Memory to copy
 * @byte_size: size of memory block to copy
 *
 * Like g_memdup or g_memdup2.  This function is meant to ease transition
 * to g_memdup2 without having to require very new glib.
 **/
gpointer
go_memdup (gconstpointer mem, gsize byte_size)
{
	if (mem && byte_size != 0) {
		gpointer new_mem = g_malloc (byte_size);
		memcpy (new_mem, mem, byte_size);
		return new_mem;
	} else
		return NULL;
}

/**
 * go_memdup_n:
 * @mem: (nullable): Memory to copy
 * @n_blocks: Number of blocks to copy.
 * @block_size: Number of bytes per blocks.
 *
 * Like go_memdup (@mem, @n_blocks * @block_size), but with overflow check.
 * Like a potential future g_memdup_n.
 **/
gpointer
go_memdup_n (gconstpointer mem, gsize n_blocks, gsize block_size)
{
	if (mem && n_blocks > 0 && block_size > 0) {
		gpointer new_mem = g_malloc_n (n_blocks, block_size);
		memcpy (new_mem, mem, n_blocks * block_size);
		return new_mem;
	} else
		return NULL;
}       



/**
 * go_object_toggle:
 * @object: #GObject
 * @property_name: name
 *
 * Toggle a boolean object property.
 **/
void
go_object_toggle (gpointer object, const gchar *property_name)
{
	gboolean value = FALSE;
	GParamSpec *pspec;

	g_return_if_fail (G_IS_OBJECT (object));
	g_return_if_fail (property_name != NULL);

	pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property_name);
	if (!pspec ||
	    !G_IS_PARAM_SPEC_BOOLEAN (pspec) ||
	    ((pspec->flags & (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)) !=
	     G_PARAM_READWRITE)) {
		g_warning ("%s: object class `%s' has no boolean property named `%s' that can be both read and written.",
			   G_STRFUNC,
			   G_OBJECT_TYPE_NAME (object),
			   property_name);
		return;
	}

	/* And now, the actual action.  */
	g_object_get (object, property_name, &value, NULL);
	g_object_set (object, property_name, !value, NULL);
}


gboolean
go_object_set_property (GObject *obj, const char *property_name,
			const char *user_prop_name, const char *value,
			GError **err,
			const char *error_template)
{
	GParamSpec *pspec;

	if (err) *err = NULL;

	g_return_val_if_fail (G_IS_OBJECT (obj), TRUE);
	g_return_val_if_fail (property_name != NULL, TRUE);
	g_return_val_if_fail (user_prop_name != NULL, TRUE);
	g_return_val_if_fail (value != NULL, TRUE);

	pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj),
					      property_name);
	g_return_val_if_fail (pspec != NULL, TRUE);

	if (G_IS_PARAM_SPEC_STRING (pspec)) {
		g_object_set (obj, property_name, value, NULL);
		return FALSE;
	}

	if (G_IS_PARAM_SPEC_BOOLEAN (pspec)) {
		gboolean b;

		if (go_utf8_collate_casefold (value, go_locale_boolean_name (TRUE)) == 0 ||
		    go_utf8_collate_casefold (value, _("yes")) == 0 ||
		    g_ascii_strcasecmp (value, "TRUE") == 0 ||
		    g_ascii_strcasecmp (value, "yes") == 0 ||
		    strcmp (value, "1") == 0)
			b = TRUE;
		else if (go_utf8_collate_casefold (value, go_locale_boolean_name (FALSE)) == 0 ||
		    go_utf8_collate_casefold (value, _("no")) == 0 ||
		    g_ascii_strcasecmp (value, "FALSE") == 0 ||
		    g_ascii_strcasecmp (value, "no") == 0 ||
		    strcmp (value, "0") == 0)
			b = FALSE;
		else
			goto error;

		g_object_set (obj, property_name, b, NULL);
		return FALSE;
	}

	if (G_IS_PARAM_SPEC_ENUM (pspec)) {
		GEnumClass *eclass = ((GParamSpecEnum *)pspec)->enum_class;
		GEnumValue *ev;

		ev = g_enum_get_value_by_name (eclass, value);
		if (!ev) ev = g_enum_get_value_by_nick (eclass, value);

		if (!ev)
			goto error;

		g_object_set (obj, property_name, ev->value, NULL);
		return FALSE;
	}

	error:
		if (err)
			*err = g_error_new (go_error_invalid (), 0,
					    error_template,
					    user_prop_name,
					    value);
		return TRUE;
}




/**
 * go_object_properties_collect:
 * @obj: #GObject
 *
 * Collect all rw properties and their values.
 * Returns: (element-type void) (transfer container): the list of collected
 * properties as #GParamSpec and values as #GValue.
 */
GSList *
go_object_properties_collect (GObject *obj)
{
	GSList *res = NULL;
	guint n;
	GParamSpec **pspecs =
		g_object_class_list_properties (G_OBJECT_GET_CLASS (obj),
						&n);

	while (n--) {
		GParamSpec *pspec = pspecs[n];
		if ((pspec->flags & (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)) ==
		    G_PARAM_READWRITE) {
			GValue *value = g_new0 (GValue, 1);
			g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));
			g_object_get_property (obj, pspec->name, value);
			res = g_slist_prepend (res, value);
			res = g_slist_prepend (res, pspec);
		}
	}

	g_free (pspecs);
	return res;
}

/**
 * go_object_properties_apply:
 * @obj: #GObject
 * @props: (element-type void): the list of properties and their values to
 * apply
 * @changed_only: whether to restrict calls to g_object_set_property() to
 * properties with changed values.
 *
 * Sets a list of properties for @obj. The list needs to be a list of
 * alternating #GParamSpec and #GValue.
 **/
void
go_object_properties_apply (GObject *obj, GSList *props, gboolean changed_only)
{
	GValue current = { 0, };

	for (; props; props = props->next->next) {
		GParamSpec *pspec = props->data;
		const GValue *value = props->next->data;
		gboolean doit;

		if (changed_only) {
			g_value_init (&current,
				      G_PARAM_SPEC_VALUE_TYPE (pspec));
			g_object_get_property (obj, pspec->name, &current);
			doit = g_param_values_cmp (pspec, &current, value);
#if 0
			g_print ("%2d:  %-20s  old: [%s]   new: [%s]\n",
				 g_param_values_cmp (pspec, &current, value),
				 g_param_spec_get_name (pspec),
				 g_strdup_value_contents (value),
				 g_strdup_value_contents (&current));
#endif
			g_value_unset (&current);
		} else
			doit = TRUE;

		if (doit)
			g_object_set_property (obj, pspec->name, value);
	}
}

/**
 * go_object_properties_free:
 * @props: (element-type void): the list of properties and their values to
 * unset
 *
 * Unsets the values in the list which needs to be a list of alternating
 * #GParamSpec and #GValue.
 **/
void
go_object_properties_free (GSList *props)
{
	GSList *l;

	for (l = props; l; l = l->next->next) {
		GValue *value = l->next->data;
		g_value_unset (value);
		g_free (value);
	}

	g_slist_free (props);
}


/**
 * go_parse_key_value:
 * @options: Options string.
 * @err: Reference to store GError if parsing fails.
 * @handler: (scope call): Handler to call for each key-value pair.
 * @user: user pointer.
 */
gboolean
go_parse_key_value (const char *options,
		    GError **err,
		    gboolean (*handler) (const char *name,
					 const char *value,
					 GError **err,
					 gpointer user),
		    gpointer user)
{
	GString *sname = g_string_new (NULL);
	GString *svalue = g_string_new (NULL);
	gboolean res = FALSE;

	if (err) *err = NULL;

	while (1) {
		const char *p;

		g_string_truncate (sname, 0);
		g_string_truncate (svalue, 0);

		while (g_unichar_isspace (g_utf8_get_char (options)))
			options = g_utf8_next_char (options);

		if (*options == 0)
			break;

		if (*options == '"' || *options == '\'') {
			options = go_strunescape (sname, options);
			if (!options)
				goto open_string;
		} else {
			p = options;
			while (strchr ("-!_.,:;|/$%#@~", *options) ||
			       g_unichar_isalnum (g_utf8_get_char (options)))
				options = g_utf8_next_char (options);
			g_string_append_len (sname, p, options - p);
			if (p == options)
				goto syntax;
		}

		while (g_unichar_isspace (g_utf8_get_char (options)))
			options = g_utf8_next_char (options);
		if (*options != '=')
			goto syntax;
		options++;
		while (g_unichar_isspace (g_utf8_get_char (options)))
			options = g_utf8_next_char (options);

		if (*options == '"' || *options == '\'') {
			options = go_strunescape (svalue, options);
			if (!options)
				goto open_string;
		} else {
			p = options;
			while (*options && !
			       g_unichar_isspace (g_utf8_get_char (options)))
				options = g_utf8_next_char (options);
			g_string_append_len (svalue, p, options - p);
		}

		if (handler (sname->str, svalue->str, err, user)) {
			res = TRUE;
			break;
		}
	}

done:
	g_string_free (sname, TRUE);
	g_string_free (svalue, TRUE);

	return res;

open_string:
	if (err)
		*err = g_error_new (go_error_invalid (), 0,
				    _("Quoted string not terminated"));
	res = TRUE;
	goto done;

syntax:
	if (err)
		*err = g_error_new (go_error_invalid (), 0,
				    _("Syntax error"));
	res = TRUE;
	goto done;
}

gboolean
go_debug_flag (const char *flag)
{
	GDebugKey key;
	key.key = (char *)flag;
	key.value = 1;

	return g_parse_debug_string (g_getenv ("GO_DEBUG"), &key, 1) != 0;
}

static GHashTable *finalize_hash;

static void
cb_finalized (gpointer data, GObject *victim)
{
	g_hash_table_remove (finalize_hash, victim);
}

void
go_debug_check_finalized (gpointer obj, const char *id)
{
	g_return_if_fail (G_IS_OBJECT (obj));

	if (!finalize_hash)
		finalize_hash = g_hash_table_new_full
			(g_direct_hash, g_direct_equal,
			 NULL, (GDestroyNotify)g_free);
	g_hash_table_replace (finalize_hash, obj, g_strdup (id));
	g_object_weak_ref (obj, cb_finalized, NULL);
}

/**
 * _go_glib_extras_shutdown: (skip)
 */
void
_go_glib_extras_shutdown (void)
{
	g_free (go_real_name);
	go_real_name = NULL;
	if (finalize_hash) {
		GHashTableIter hiter;
		gpointer key, value;
		g_hash_table_iter_init (&hiter, finalize_hash);
		while (g_hash_table_iter_next (&hiter, &key, &value)) {
			const char *name = value;
			g_printerr ("%s \"%s\" at %p not finalized.\n",
				    G_OBJECT_TYPE_NAME (key), name, key);
		}
		g_hash_table_destroy (finalize_hash);
		finalize_hash = NULL;
	}
}
