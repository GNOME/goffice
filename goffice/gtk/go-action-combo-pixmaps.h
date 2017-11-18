/*
 * go-action-combo-pixmaps.h: A custom GtkAction to chose among a set of images
 *
 * Copyright (C) 2004 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 **/

#ifndef _GO_ACTION_COMBO_PIXMAPS_H_
#define _GO_ACTION_COMBO_PIXMAPS_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_ACTION_COMBO_PIXMAPS  (go_action_combo_pixmaps_get_type ())
#define GO_ACTION_COMBO_PIXMAPS(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_ACTION_COMBO_PIXMAPS, GOActionComboPixmaps))
#define GO_IS_ACTION_COMBO_PIXMAPS(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_ACTION_COMBO_PIXMAPS))

typedef struct _GOActionComboPixmaps	 GOActionComboPixmaps;
typedef struct {
	char const *untranslated_tooltip;
	char const *stock_id;
	int id;
} GOActionComboPixmapsElement;

GType	go_action_combo_pixmaps_get_type (void);
GOActionComboPixmaps *
	go_action_combo_pixmaps_new (char const *name,
				     GOActionComboPixmapsElement const *elements,
				     int ncols, int nrows);
int	 go_action_combo_pixmaps_get_selected (GOActionComboPixmaps *action, int *indx);
gboolean go_action_combo_pixmaps_select_id    (GOActionComboPixmaps *action, int id);

G_END_DECLS

#endif  /* _GO_ACTION_COMBO_PIXMAPS_H_ */
