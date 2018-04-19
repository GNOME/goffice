/*
 * mathml to ITEX converter.
 *
 * Copyright 2014 by Jean Brefort (jean.brefort@normalesup.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/goffice-priv.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <string.h>

gboolean
go_mathml_to_itex (char const *mml, char **buf, int *length, gboolean *compact, GOCmdContext *gcc)
{
    static xsltStylesheet *sheet = NULL;
    xmlDocPtr doc, res;
 	int ret, len, start, end;
	char *itex;

	if (mml == NULL || *mml == 0)
        /* Nothing has failed, but we have nothing to do anyway */
		return FALSE;
    if (sheet == NULL) {
		char *path = g_build_filename (go_sys_data_dir (), "mmlitex/mmlitex.xsl", NULL);
			
		sheet = xsltParseStylesheetFile (path);
		if (!sheet) {
			if (gcc)
		        go_cmd_context_error_import (gcc, "MathML to ITeX: parsing stylesheet failed");
			return FALSE;
		}
    }
	
    doc = xmlParseDoc((xmlChar const *) mml);
    if (!doc) {
		if (gcc)
    		go_cmd_context_error_import (gcc, "MathML toI TeX: parsing MathML document failed");
		return FALSE;
    }	
	
    res = xsltApplyStylesheet(sheet, doc, NULL);
    if (!res) {
		if (gcc)
    		go_cmd_context_error_import (gcc, "MathML to ITeX: applying stylesheet failed");
		xmlFreeDoc(doc);
		return FALSE;
    }

    ret = xsltSaveResultToString ((xmlChar **) &itex, &len, res, sheet) != 0;
	/* Remove the extra charaters are start or end */
	if (compact)
		*compact = FALSE; /* the default */
	if (!strncmp (itex + 1, "\\[", 2) && !strcmp (itex + len - 2, "\\]")) {
		start = 3;
		end = len - 3;
		while ((itex[start] == '\n' || itex[start] == '\t') && start < len)
			start++;
		while ((itex[end] == '\n' || itex[end] == '\t') && start < len)
			end--;
		end++;
	} else if (itex[1] == '$' && itex[len - 1] == '$') {
		if (compact)
			*compact = TRUE;
		start = 2;
		end = len - 2;
		while ((itex[start] == '\n' || itex[start] == '\t') && start < len)
			start++;
		while ((itex[end] == '\n' || itex[end] == '\t') && start < len)
			end--;
		end++;
	} else {
		start = 0;
		end = len;
	}
	end -= start;
	if (buf) {
		*buf = g_malloc (end + 1);
		memcpy (*buf, itex + start, end);
		(*buf)[end] = 0;
	}
	if (length)
		*length = len;
	xmlFree (itex);
	xmlFreeDoc(res);
	xmlFreeDoc(doc);
	return ret >= 0;
}
