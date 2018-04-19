/*
 * go-persist.c :
 *
 * Copyright (C) 2003-2008 Jody Goldberg (jody@gnome.org)
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

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>


/**
 * GOPersistClass:
 * @base: base interface.
 * @prep_sax: loads the persistent object from a #GsfXMLIn.
 * @sax_save: saves the persistent object to a #GsfXMLOut.
 **/

GType
go_persist_get_type (void)
{
	static GType go_persist_type = 0;

	if (!go_persist_type) {
		static GTypeInfo const go_persist_info = {
			sizeof (GOPersistClass),	/* class_size */
			NULL,		/* base_init */
			NULL,		/* base_finalize */
		};

		go_persist_type = g_type_register_static (G_TYPE_INTERFACE,
			"GOPersist", &go_persist_info, 0);
	}

	return go_persist_type;
}

void
go_persist_sax_save (GOPersist const *gp, GsfXMLOut *output)
{
	g_return_if_fail (GO_IS_PERSIST (gp));
	GO_PERSIST_GET_CLASS (gp)->sax_save (gp, output);
}
void
go_persist_prep_sax (GOPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	g_return_if_fail (GO_IS_PERSIST (gp));
	GO_PERSIST_GET_CLASS (gp)->prep_sax (gp, xin, attrs);
}
