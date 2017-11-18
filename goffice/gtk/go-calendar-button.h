/*
 * go-calendar-button.h: A custom GtkAction to chose among a set of images
 *
 * Copyright (C) 2009 Morten Welinder (terra@gnome.org)
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

#ifndef _GO_CALENDAR_BUTTON_H_
#define _GO_CALENDAR_BUTTON_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GO_TYPE_CALENDAR_BUTTON	(go_calendar_button_get_type ())
#define GO_CALENDAR_BUTTON(o) (G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_CALENDAR_BUTTON, GOCalendarButton))
#define GO_IS_CALENDAR_BUTTON(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_CALENDAR_BUTTON))

typedef struct _GOCalendarButton GOCalendarButton;

GType		  go_calendar_button_get_type	 (void);
GtkWidget        *go_calendar_button_new (void);
GtkCalendar      *go_calendar_button_get_calendar (GOCalendarButton *calb);
void              go_calendar_button_set_date (GOCalendarButton *calb,
					       GDate const *date);
gboolean          go_calendar_button_get_date (GOCalendarButton *calb,
					       GDate *date);

G_END_DECLS

#endif /* _GO_CALENDAR_BUTTON_H_ */
