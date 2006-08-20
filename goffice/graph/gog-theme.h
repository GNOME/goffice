/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-theme.h : 
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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
#ifndef GOG_THEME_H
#define GOG_THEME_H

#include <goffice/graph/goffice-graph.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GOG_THEME_TYPE	(gog_theme_get_type ())
#define GOG_THEME(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_THEME_TYPE, GogTheme))
#define IS_GOG_THEME(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_THEME_TYPE))

GType gog_theme_get_type (void);

GogTheme   *gog_theme_new        	(char const *name);
GogTheme   *gog_theme_new_from_file     (char const *name, char const *file);
char const *gog_theme_get_name 		(GogTheme const *theme);
void 	    gog_theme_fillin_style    	(GogTheme const *theme, GogStyle *style,
				         GogObject const *obj, int ind,
				         gboolean complete_overwrite);

void	    gog_theme_registry_add		(GogTheme *theme, gboolean is_default);
GogTheme   *gog_theme_registry_lookup 		(char const *name);
GSList	   *gog_theme_registry_get_theme_names	(void);
	
/* private */
void gog_themes_init	 (void);
void gog_themes_shutdown (void);

G_END_DECLS

#endif /* GOG_THEME_H */
