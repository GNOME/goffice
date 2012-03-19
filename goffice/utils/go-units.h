/*
 * go-units.h :
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
#ifndef GO_UNITS_H
#define GO_UNITS_H

#include <glib.h>

G_BEGIN_DECLS

/* Conversion factors */
/* The following number is the least common multiplier of 254 (1/10mm), 72(pt), 100000, and 576 */
/* This way inch, pt, and mm are all integer multipliers (in fact, a nanometer is.) */
/* (Of course that is only true because we use the lobotomized pt size so that
   1inch is exactly 72pt.)  */
#define GO_PT_PER_IN 72
#define GO_CM_PER_IN 254
#define GO_EMU_PER_IN 914400

#define GO_UN_PER_IN 228600000
#define GO_UN_PER_EMU (GO_UN_PER_IN / GO_EMU_PER_IN)
#define GO_UN_PER_PT (GO_UN_PER_IN / GO_PT_PER_IN)
#define GO_UN_PER_CM (GO_UN_PER_IN / GO_CM_PER_IN)

#define GO_IN_TO_UN(inch)	((inch) * GO_UN_PER_IN)
#define GO_IN_TO_PT(inch)	((inch) * GO_PT_PER_IN)
#define GO_IN_TO_CM(inch)	((inch) * GO_CM_PER_IN / 100)
#define GO_IN_TO_EMU(inch)	((inch) * GO_EMU_PER_IN)

#define GO_UN_TO_IN(unit)	((unit) / GO_UN_PER_IN)
#define GO_UN_TO_PT(unit)	((unit) / GO_UN_PER_PT)
#define GO_UN_TO_CM(unit)	((unit) / GO_UN_PER_CM / 100)
#define GO_UN_TO_EMU(unit)	((unit) / GO_UN_PER_EMU)

#define GO_PT_TO_UN(pt)		((pt) * GO_UN_PER_PT)
#define GO_PT_TO_IN(pt)		((pt) / GO_PT_PER_IN)
#define GO_PT_TO_CM(pt)		((pt) * GO_CM_PER_IN / GO_PT_PER_IN / 100)
#define GO_PT_TO_EMU(pt)	((pt) * GO_EMU_PER_IN / GO_PT_PER_IN)

#define GO_CM_TO_UN(cm)		((cm) * 100 * GO_UN_PER_CM)
#define GO_CM_TO_IN(cm)		((cm) * 100 / GO_CM_PER_IN)
#define GO_CM_TO_PT(cm)		((cm) * 100 * GO_PT_PER_IN / GO_CM_PER_IN)
#define GO_CM_TO_EMU(cm)	((cm) * 100 * GO_PT_PER_IN / GO_EMU_PER_IN)

#define GO_EMU_TO_UN(emu)	((emu) * GO_UN_PER_EMU)
#define GO_EMU_TO_IN(emu)	((emu) / GO_EMU_PER_IN)
#define GO_EMU_TO_PT(emu)	((emu) * GO_PT_PER_IN / GO_EMU_PER_IN)
#define GO_EMU_TO_CM(emu)	((emu) * GO_CM_PER_IN / GO_EMU_PER_IN / 100)

typedef gint64 GODistance;
typedef struct {
	GODistance x;
	GODistance y;
} GOPoint;
typedef struct {
	GODistance top;
	GODistance left;
	GODistance bottom;
	GODistance right;
} GORect;

G_END_DECLS

#endif /* GO_UNITS_H */
