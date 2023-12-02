/*
 * go-libxml-extras.h :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_LIBXML_EXTRAS_H
#define GO_LIBXML_EXTRAS_H

#include <goffice/goffice.h>
#include <libxml/tree.h>
#include <gsf/gsf-libxml.h>

G_BEGIN_DECLS

xmlDocPtr  go_xml_parse_file    (const char *filename);

xmlChar   *go_xml_node_get_cstr	(xmlNodePtr node, char const *name);
void	   go_xml_node_set_cstr	(xmlNodePtr node, char const *name, char const *val);
gboolean   go_xml_node_get_bool	(xmlNodePtr node, char const *name, gboolean *result);
void       go_xml_node_set_bool	(xmlNodePtr node, char const *name, gboolean val);
gboolean   go_xml_node_get_int	(xmlNodePtr node, char const *name, int *result);
void       go_xml_node_set_int	(xmlNodePtr node, char const *name, int  val);
gboolean   go_xml_node_get_double	(xmlNodePtr node, char const *name, double *result);
void       go_xml_node_set_double	(xmlNodePtr node, char const *name, double  val, int precision);
gboolean   go_xml_node_get_gocolor (xmlNodePtr node, char const *name, GOColor *result);
void	   go_xml_node_set_gocolor (xmlNodePtr node, char const *name, GOColor  val);
gboolean   go_xml_node_get_enum    (xmlNodePtr node, char const *name, GType etype, gint *val);
void       go_xml_node_set_enum    (xmlNodePtr node, char const *name, GType etype, gint val);

xmlNode *go_xml_get_child_by_name	 (xmlNode const *tree, char const *name);
xmlNode *go_xml_get_child_by_name_no_lang (xmlNode const *tree, char const *name);
xmlNode *go_xml_get_child_by_name_by_lang (xmlNode const *tree, char const *name);

void       go_xml_out_add_double (GsfXMLOut *output, char const *id, double d);
#ifdef GOFFICE_WITH_LONG_DOUBLE
void       go_xml_out_add_long_double (GsfXMLOut *output, char const *id, long double ld);
#endif
#ifdef GOFFICE_WITH_DECIMAL64
void       go_xml_out_add_decimal64 (GsfXMLOut *output, char const *id, _Decimal64 d);
#endif
void	   go_xml_out_add_color (GsfXMLOut *out, char const *id, GOColor c);

void       go_xml_in_doc_dispose_on_exit (GsfXMLInDoc **pdoc);

void _go_libxml_extras_shutdown (void);

G_END_DECLS

#endif /* GO_LIBXML_EXTRAS_H */
