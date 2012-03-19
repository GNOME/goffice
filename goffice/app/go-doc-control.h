/*
 * go-doc.h :  A GOffice document
 *
 * Copyright (C) 2004 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_DOC_CONTROL_H
#define GO_DOC_CONTROL_H

#include <goffice/app/goffice-app.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GO_TYPE_DOC_CONTROL	    (go_doc_control_get_type ())
#define GO_DOC_CONTROL(o)	    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DOC_CONTROL, GODocControl))
#define GO_IS_DOC_CONTROL(o)	    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DOC_CONTROL))

typedef enum {
	GO_DOC_CONTROL_STATE_NORMAL = 0,
	GO_DOC_CONTROL_STATE_FULLSCREEN,
	GO_DOC_CONTROL_STATE_MAX
} GODocControlState;

GType go_doc_control_get_type (void);

GODoc *go_doc_control_get_doc (GODocControl *dc);
void   go_doc_control_set_doc (GODocControl *dc, GODoc *doc);

G_END_DECLS

#endif /* GO_DOC_CONTROL_H */
