/*
 * gog-object-xml.h :
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
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
#ifndef GOG_OBJECT_XML_H
#define GOG_OBJECT_XML_H

#include <goffice/goffice.h>
#include <libxml/tree.h>
#include <gsf/gsf-libxml.h>

G_BEGIN_DECLS

void	   gog_object_set_arg	   (char const *name, char const *val, GogObject *obj);
void	   gog_object_write_xml_sax(GogObject const *obj, GsfXMLOut *output, gpointer user);

typedef void (*GogObjectSaxHandler)(GogObject *obj, gpointer user_data);
void	   gog_object_sax_push_parser (GsfXMLIn *xin, xmlChar const **attrs,
				       GogObjectSaxHandler handler,
				       gpointer user_unserialize,
				       gpointer user_data);
GogObject *gog_xml_read_state_get_obj (GsfXMLIn *xin);
GogObject *gog_object_new_from_input (GsfInput *input,
                                      gpointer user_unserialize);


G_END_DECLS

#endif /* GOG_OBJECT_XML_H */
