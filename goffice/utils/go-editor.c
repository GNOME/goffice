/*
 * go-editor.c :
 *
 * Copyright (C) 2003-2009 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
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

/*****************************************************************************/

#include <goffice/goffice-config.h>
#include <goffice/utils/go-editor.h>
#include <string.h>


typedef struct {
	char const	*label;		/* label for notebook page */
	gpointer 	 widget;	/* GtkWidget* */
	gpointer scrolled;		/* GtkScrolledWindow embedding the widget.*/
} GOEditorPage;

struct _GOEditor {
	unsigned	*store_page;		/* pointer to a place for storing last edited page */
	GSList		*pages;			/* GOEditorPage */
	GData		*registered_widgets;
	unsigned     ref_count;
	gboolean	 use_scrolled;
};

/**
 * GOEditor:
 *
 * Embeds a notebook containing all the pages added to the editor.
 **/

/**
 * go_editor_new:
 *
 * Returns: (transfer full): a new GOEditor object, which is used to store a collection of
 * 	property edition widgets (pages). The returned object must be freed
 * 	using @go_editor_free.
 **/
GOEditor *
go_editor_new (void)
{
	GOEditor *editor = g_new (GOEditor, 1);

	editor->store_page = NULL;
	editor->pages = NULL;
	g_datalist_init (&editor->registered_widgets);
	editor->ref_count = 1;
	editor->use_scrolled = FALSE;

	return editor;
}

static void
page_free (GOEditorPage *page)
{
	if (page->scrolled)
		g_object_unref (page->scrolled);
	g_free (page);
}

/**
 * go_editor_free:
 * @editor: (nullable): a #GOEditor
 *
 * Frees a GOEditor object.
 **/

void
go_editor_free (GOEditor *editor)
{
	if (editor == NULL || editor->ref_count-- >1)
		return;
	g_slist_foreach (editor->pages, (GFunc) page_free, NULL);
	g_slist_free (editor->pages);
	g_datalist_clear (&editor->registered_widgets);

	g_free (editor);
}

static GOEditor *
go_editor_ref (GOEditor *editor)
{
	editor->ref_count++;
	return editor;
}

GType
go_editor_get_type (void)
{
	static GType t = 0;

	if (t == 0)
		t = g_boxed_type_register_static ("GOEditor",
			 (GBoxedCopyFunc)go_editor_ref,
			 (GBoxedFreeFunc)go_editor_free);
	return t;
}

/**
 * go_editor_add_page:
 * @editor: a #GOEditor
 * @widget: property edition widget
 * @label: a label identifying the widget
 *
 * Adds a page to @editor.
 */

void
go_editor_add_page (GOEditor *editor, gpointer widget, char const *label)
{
	GOEditorPage *page;

	g_return_if_fail (editor != NULL);
	page = g_new0 (GOEditorPage, 1);

	page->widget = widget;
#ifdef GOFFICE_WITH_GTK
	if (editor->use_scrolled) {
		page->scrolled = g_object_ref (gtk_scrolled_window_new (NULL, NULL));
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (page->scrolled),
			                                   widget);
	} else
#endif
		page->scrolled = g_object_ref (widget);
	page->label = label;

	editor->pages = g_slist_prepend (editor->pages, page);
}

/**
 * go_editor_set_store_page:
 * @editor: a #GOEditor
 * @store_page: placeholder for the last selected page
 *
 * Sets a placeholder for storing the last active editor page.
 **/

void
go_editor_set_store_page (GOEditor *editor, unsigned *store_page)
{
	g_return_if_fail (editor != NULL);

	editor->store_page = store_page;
}

/**
 * go_editor_set_use_scrolled_window:
 * @editor: a #GOEditor
 * @use_scrolled: boolean
 *
 * Sets whether each page of the editor should be displayed inside a scrolled
 * window.
 **/
void
go_editor_set_use_scrolled_window (GOEditor *editor, gboolean use_scrolled)
{
	g_return_if_fail (editor != NULL);

	editor->use_scrolled = use_scrolled;
}

#ifdef GOFFICE_WITH_GTK

/**
 * go_editor_register_widget:
 * @editor: a #GOEditor
 * @widget: a #GtkWidget
 *
 * Registers a widget that then can be retrieved later using
 * @go_editor_get_registered_widget. The main use of this function is to
 * provide the ability to extend a page.
 **/
void
go_editor_register_widget (GOEditor *editor, GtkWidget *widget)
{
	g_return_if_fail (editor != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	g_datalist_set_data (&editor->registered_widgets, gtk_buildable_get_name (GTK_BUILDABLE (widget)), widget);
}

/**
 * go_editor_get_registered_widget:
 * @editor: a #GOEditor
 * @name: the name of the registered widget
 *
 * Returns: (transfer none): a widget previously registered using @go_editor_register_widget.
 **/
GtkWidget *
go_editor_get_registered_widget (GOEditor *editor, char const *name)
{
	g_return_val_if_fail (editor != NULL, NULL);

	return g_datalist_get_data (&editor->registered_widgets, name);
}

static void
cb_switch_page (G_GNUC_UNUSED GtkNotebook *n, G_GNUC_UNUSED GtkWidget *p,
		guint page_num, guint *store_page)
{
		*store_page = page_num;
}

/**
 * go_editor_get_notebook:
 * @editor: a #GOEditor
 *
 * Returns: (transfer full): a GtkNotebook from the widget collection stored in @editor.
 **/
GtkWidget *
go_editor_get_notebook (GOEditor *editor)
{
	GtkWidget *notebook;
	GOEditorPage *page;
	GSList *ptr;
	unsigned page_count = 0;

	notebook = gtk_notebook_new ();
	if (editor->pages != NULL) {
		for (ptr = editor->pages; ptr != NULL; ptr = ptr->next) {
			page = (GOEditorPage *) ptr->data;
			gtk_notebook_prepend_page (GTK_NOTEBOOK (notebook),
						   GTK_WIDGET (page->scrolled),
						   gtk_label_new (page->label));
			gtk_widget_show (page->scrolled);
			page_count ++;
		}
	} else {
		/* Display a blank page */
		GtkWidget *label =  gtk_label_new (NULL);
		gtk_notebook_prepend_page (GTK_NOTEBOOK (notebook),
					   label, NULL);
		gtk_widget_show (label);
		page_count = 1;
	}

	if (editor->store_page != NULL) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), *editor->store_page);
		g_signal_connect (G_OBJECT (notebook),
				  "switch_page",
				  G_CALLBACK (cb_switch_page), editor->store_page);
	} else
		gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

	return notebook;
}

/**
 * go_editor_get_page:
 * @editor: #GOEditor
 * @name: page name
 *
 * Returns: (transfer none) (nullable): the page with @name as name if any
 **/
GtkWidget *
go_editor_get_page (GOEditor *editor, char const *name)
{
	GSList *ptr;
	GOEditorPage *page;
	for (ptr = editor->pages; ptr; ptr = ptr->next) {
		page = (GOEditorPage *) ptr->data;
		if (strcmp (page->label, name))
			return page->widget;
	}
	return NULL;
}

#endif
