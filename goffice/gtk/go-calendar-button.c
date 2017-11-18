/*
 * go-calendar-button.c: A custom GtkAction to chose among a set of images
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

#include <goffice/goffice-config.h>
#include "go-calendar-button.h"
#include "go-combo-box.h"
#include <gsf/gsf-impl-utils.h>

struct _GOCalendarButton {
	GOComboBox base;
	GtkCalendar *calendar;
};

typedef struct {
	GOComboBoxClass base;
	void (* changed) (GOCalendarButton *);
} GOCalendarButtonClass;

enum {
	CHANGED,
	LAST_SIGNAL
};

static guint go_calendar_button_signals[LAST_SIGNAL] = { 0, };

static void
cb_calendar_changed (GtkCalendar *cal, GOCalendarButton *calb)
{
	g_signal_emit (G_OBJECT (calb), go_calendar_button_signals[CHANGED], 0);
}

static void
go_calendar_button_init (GOCalendarButton *calb)
{
	GtkWidget *w = gtk_calendar_new ();
	calb->calendar = GTK_CALENDAR (w);
	go_combo_box_construct (GO_COMBO_BOX (calb),
				NULL,
				w, w);

	g_signal_connect (G_OBJECT (w), "month_changed",
			  G_CALLBACK (cb_calendar_changed),
			  calb);
	g_signal_connect (G_OBJECT (w), "day_selected",
			  G_CALLBACK (cb_calendar_changed),
			  calb);
}

static void
go_calendar_button_class_init (GObjectClass *gobject_class)
{
	go_calendar_button_signals[CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GOCalendarButtonClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

GSF_CLASS (GOCalendarButton, go_calendar_button,
	   go_calendar_button_class_init, go_calendar_button_init,
	   GO_TYPE_COMBO_BOX)

GtkWidget *
go_calendar_button_new (void)
{
	return g_object_new (GO_TYPE_CALENDAR_BUTTON, NULL);
}

/**
 * go_calendar_button_get_calendar:
 * @calb: #GOCalendarButton
 *
 * Returns: (transfer none): the embedded calendar.
 **/
GtkCalendar *
go_calendar_button_get_calendar (GOCalendarButton *calb)
{
	g_return_val_if_fail (GO_IS_CALENDAR_BUTTON (calb), NULL);
	return calb->calendar;
}

gboolean
go_calendar_button_get_date (GOCalendarButton *calb, GDate *date)
{
	GtkCalendar *cal;
	int d, m, y;

	g_return_val_if_fail (GO_IS_CALENDAR_BUTTON (calb), FALSE);
	g_return_val_if_fail (date != NULL, FALSE);

	cal = go_calendar_button_get_calendar (calb);
	gtk_calendar_get_date (cal, &y, &m, &d);
	m += 1;  /* Zero-based.  */

	g_date_clear (date, 1);
  	if (g_date_valid_dmy (d, m, y))
		g_date_set_dmy (date, d, m, y);

	return g_date_valid (date);
}

void
go_calendar_button_set_date (GOCalendarButton *calb, GDate const *date)
{
	GtkCalendar *cal;
	GDate old_date;

	g_return_if_fail (GO_IS_CALENDAR_BUTTON (calb));
	g_return_if_fail (g_date_valid (date));

	if (go_calendar_button_get_date (calb, &old_date) &&
	    g_date_compare (date, &old_date) == 0)
		return;

	cal = go_calendar_button_get_calendar (calb);
	gtk_calendar_select_month (cal,
				   g_date_get_month (date) - 1,
				   g_date_get_year (date));
	gtk_calendar_select_day (cal, g_date_get_day (date));
}
