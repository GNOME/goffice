/*
 * go-rsm.c: Resource manager for Goffice.
 *
 * Copyright (C) 2011 Morten Welinder (terra@gnome.org)
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

#include <goffice/goffice.h>
#include "go-rsm.h"

typedef struct {
  gconstpointer data;
  size_t len;
} GORSMResource;

static gboolean debug;
static GHashTable *rsm;

/**
 * _go_rsm_init: (skip)
 */
void
_go_rsm_init (void)
{
  debug = go_debug_flag ("rsm");
  rsm = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

/**
 * _go_rsm_shutdown: (skip)
 */
void
_go_rsm_shutdown (void)
{
  if (debug)
    g_printerr ("Shutting down with %d resources\n", g_hash_table_size (rsm));
  g_hash_table_destroy (rsm);
  rsm = NULL;
}

void
go_rsm_register_file (const char *id, gconstpointer data, size_t len)
{
  GORSMResource *r;

  g_return_if_fail (id != NULL);
  g_return_if_fail (g_hash_table_lookup (rsm, id) == NULL);

  if (debug)
    g_printerr ("Registering resource [%s]\n", id);
  r = g_new (GORSMResource, 1);
  r->data = data;
  r->len = len;
  g_hash_table_insert (rsm, g_strdup (id), r);
}

void
go_rsm_unregister_file (const char *id)
{
  g_return_if_fail (id != NULL);
  g_return_if_fail (g_hash_table_lookup (rsm, id) != NULL);

  if (debug)
    g_printerr ("Unregistering resource [%s]\n", id);

  g_hash_table_remove (rsm, id);
}

/**
 * go_rsm_lookup:
 * @id: resource name
 * @len: where to store the resource size in bytes
 *
 * Returns: (transfer none) (nullable): the resource or %NULL if not found
 **/
gconstpointer
go_rsm_lookup (const char *id, size_t *len)
{
  GORSMResource *r;

  g_return_val_if_fail (id != NULL, NULL);

  r = g_hash_table_lookup (rsm, id);
  if (!r)
    return NULL;

  if (len)
    *len = r->len;
  return r->data;
}
