/*
 * go-action-combo-stack.h: A custom GtkAction to handle undo/redo style combos
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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

#ifndef _GO_ACTION_COMBO_STACK_H_
#define _GO_ACTION_COMBO_STACK_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define GO_TYPE_ACTION_COMBO_STACK  (go_action_combo_stack_get_type ())
#define GO_ACTION_COMBO_STACK(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_ACTION_COMBO_STACK, GOActionComboStack))
#define GO_IS_ACTION_COMBO_STACK(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_ACTION_COMBO_STACK))

typedef struct _GOActionComboStack	GOActionComboStack;

GType	 go_action_combo_stack_get_type  (void);
void	 go_action_combo_stack_push      (GOActionComboStack *act,
					  char const *str, gpointer key);
void	 go_action_combo_stack_pop	 (GOActionComboStack *act, unsigned n);
void	 go_action_combo_stack_truncate  (GOActionComboStack *act, unsigned n);
gpointer go_action_combo_stack_selection (GOActionComboStack const *a);

G_END_DECLS

#endif  /* _GO_ACTION_COMBO_STACK_H_ */
