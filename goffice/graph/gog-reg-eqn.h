/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-reg-eqn.h :  
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef GOG_REG_EQN_H
#define GOG_REG_EQN_H

struct  _GogRegEqn {
	GogStyledObject	base;
	gboolean show_eq, show_r2;
	int hpos, vpos; /* horizontal and vertical position of text 0=top or left
						100 = bottom or right */
};

#define GOG_REG_EQN_TYPE		(gog_reg_eqn_get_type ())
#define GOG_REG_EQN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_REG_EQN_TYPE, GogRegEqn))
#define IS_GOG_REG_EQN(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_REG_EQN_TYPE))

GType gog_reg_eqn_get_type (void);


#endif /* GOG_REG_EQN_H */
