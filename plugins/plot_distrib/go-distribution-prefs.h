/*
 * go-distribution-prefs.h
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_DISTRIBUTION_PREFS_H
#define GO_DISTRIBUTION_PREFS_H

#include <goffice/app/go-cmd-context.h>
#include <goffice/graph/gog-data-allocator.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget *go_distribution_pref_new   (GObject *obj, GogDataAllocator *dalloc,
				       GOCmdContext *cc);

G_END_DECLS

#endif
