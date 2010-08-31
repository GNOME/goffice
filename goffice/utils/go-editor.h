/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-editor.h :
 *
 * Copyright (C) 2003-2009 Jody Goldberg (jody@gnome.org)
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

#ifndef GO_EDITOR_H
#define GO_EDITOR_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	char const	*label;		/* label for notebook page */
	gpointer 	 widget;	/* GtkWidget* */
} GOEditorPage;

struct _GOEditor {
	unsigned	*store_page;		/* pointer to a place for storing last edited page */
	GSList		*pages;			/* GOEditorPage */
	GData		*registered_widgets;
};

GOEditor	*go_editor_new 		  (void);
void 		 go_editor_free 		  (GOEditor *editor);
void		 go_editor_add_page 		  (GOEditor *editor, gpointer widget, char const *label);
void 		 go_editor_set_store_page  	  (GOEditor *editor, unsigned *store_page);
#ifdef GOFFICE_WITH_GTK
void 		 go_editor_register_widget 	  (GOEditor *editor, GtkWidget *widget);
GtkWidget  	*go_editor_get_registered_widget (GOEditor *editor, char const *name);
GtkWidget 	*go_editor_get_notebook 	  (GOEditor *editor);
GtkWidget 	*go_editor_get_page      	  (GOEditor *editor, char const *name);
#endif

G_END_DECLS

#endif

