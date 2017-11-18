/*
 * go-action-combo-color.h: A custom GtkAction to handle color selection
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

#ifndef _GO_ACTION_COMBO_COLOR_H_
#define _GO_ACTION_COMBO_COLOR_H_

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_ACTION_COMBO_COLOR  (go_action_combo_color ())
#define GO_ACTION_COMBO_COLOR(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_ACTION_COMBO_COLOR, GOActionComboColor))
#define GO_IS_ACTION_COMBO_COLOR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_ACTION_COMBO_COLOR))

typedef struct _GOActionComboColor	 GOActionComboColor;

GType	 go_action_combo_color_get_type	  (void);
GOActionComboColor *
	 go_action_combo_color_new	  (char const  *action_name,
					   char const  *stock_id,
					   char const  *default_color_label,
					   GOColor	default_color,
					   gpointer	group_key);
void 	go_action_combo_color_set_group (GOActionComboColor *a, gpointer group_key);
GOColor go_action_combo_color_get_color (GOActionComboColor *a, gboolean *is_default);
void    go_action_combo_color_set_color (GOActionComboColor *a, GOColor color);
void    go_action_combo_color_set_allow_alpha (GOActionComboColor *a, gboolean allow_alpha);
G_END_DECLS

#endif  /* _GO_ACTION_COMBO_COLOR_H_ */
