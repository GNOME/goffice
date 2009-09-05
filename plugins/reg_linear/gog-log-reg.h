/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-log-reg.h :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_LOG_REG_H
#define GOG_LOG_REG_H

#include <gog-lin-reg.h>

G_BEGIN_DECLS

typedef GogLinRegCurve GogLogRegCurve;
typedef GogLinRegCurveClass GogLogRegCurveClass;

#define GOG_TYPE_LOG_REG_CURVE	(gog_log_reg_curve_get_type ())
#define GOG_LOG_REG_CURVE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LOG_REG_CURVE, GogLogRegCurve))
#define GOG_IS_LOG_REG_CURVE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LOG_REG_CURVE))

GType gog_log_reg_curve_get_type (void);
void  gog_log_reg_curve_register_type (GTypeModule *module);

G_END_DECLS

#endif	/* GOG_LOG_REG_H */
