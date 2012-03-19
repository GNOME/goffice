/*
 * go-component-mime-dialog.c :
 *
 * Copyright (C) 2010 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_COMPONENT_MIME_DIALOG_H
#define GO_COMPONENT_MIME_DIALOG_H

#include <glib-object.h>
#include <goffice/goffice.h>
#include <goffice/component/goffice-component.h>

G_BEGIN_DECLS

#define GO_TYPE_COMPONENT_MIME_DIALOG	(go_component_mime_dialog_get_type ())
#define GO_COMPONENT_MIME_DIALOG(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_COMPONENT_MIME_DIALOG, GOComponentMimeDialog))
#define GO_IS_COMPONENT_MIME_DIALOG(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_COMPONENT_MIME_DIALOG))

GType	  go_component_mime_dialog_get_type (void);
GtkWidget  *go_component_mime_dialog_new (void);
char const *go_component_mime_dialog_get_mime_type (GOComponentMimeDialog *dlg);

G_END_DECLS

#endif  /* GO_COMPONENT_MIME_DIALOG_H */
