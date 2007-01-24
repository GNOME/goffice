/*
 * go-locale.h:
 *
 * Copyright (C) 2007 Morten Welinder (terra@gnome.org)
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_LOCALE_H
#define GO_LOCALE_H

#include <glib.h>
#include <locale.h>

G_BEGIN_DECLS

void	       go_set_untranslated_bools  (void);
char const *   go_setlocale               (int category, char const *val);
GString const *go_format_get_currency     (gboolean *precedes, gboolean *space_sep);
gboolean       go_format_month_before_day (void);
char           go_format_get_arg_sep      (void);
char           go_format_get_col_sep      (void);
char           go_format_get_row_sep      (void);
GString const *go_format_get_thousand     (void);
GString const *go_format_get_decimal      (void);
char const *   go_format_boolean          (gboolean b);

G_END_DECLS

#endif /* GO_LOCALE_H */
