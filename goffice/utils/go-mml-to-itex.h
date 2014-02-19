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
#ifndef _GO_MML_TO_ITEX_H_
#define _GO_MML_TO_ITEX_H_

G_BEGIN_DECLS

gboolean go_mathml_to_itex (char const *mml, char **buf, int *length, gboolean *compact, GOCmdContext *gcc);

G_END_DECLS

#endif  /* _GO_MML_TO_ITEX_H_ */
