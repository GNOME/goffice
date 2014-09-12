/*
 * goffice-utils.h:
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

#ifndef GOFFICE_UTILS_H
#define GOFFICE_UTILS_H

#include <glib.h>

G_BEGIN_DECLS

typedef guint32					GOColor;
typedef struct _GOBezierSpline  GOBezierSpline;
typedef struct _GOEditor        GOEditor;
typedef struct _GOFont			GOFont;
typedef struct _GOFontMetrics	GOFontMetrics;
typedef struct _GOPattern		GOPattern;
typedef struct _GOMarker		GOMarker;
typedef struct _GOFormat		GOFormat;
typedef struct _GODateConventions	GODateConventions;
typedef struct _GOImage			GOImage;
typedef struct _GOPixbuf		GOPixbuf;
typedef struct _GOSvg			GOSvg;
typedef struct _GOEmf			GOEmf;
typedef struct _GOSpectre		GOSpectre;
typedef struct _GOPath          GOPath;
typedef struct _GOString        GOString;
typedef struct _GOStyle			GOStyle;
typedef struct _GOStyledObject	GOStyledObject;

/* rename this */
typedef struct _GOMemChunk		GOMemChunk;

typedef const char *(*GOTranslateFunc)(char const *path, gpointer func_data);

typedef enum {
	GO_LINE_NONE,
	GO_LINE_SOLID,
	GO_LINE_S_DOT,
	GO_LINE_S_DASH_DOT,
	GO_LINE_S_DASH_DOT_DOT,
	GO_LINE_DASH_DOT_DOT_DOT,
	GO_LINE_DOT,
	GO_LINE_S_DASH,
	GO_LINE_DASH,
	GO_LINE_LONG_DASH,
	GO_LINE_DASH_DOT,
	GO_LINE_DASH_DOT_DOT,
	GO_LINE_MAX
} GOLineDashType;

typedef enum {
	GO_LINE_INTERPOLATION_LINEAR,
	GO_LINE_INTERPOLATION_SPLINE,
	GO_LINE_INTERPOLATION_CLOSED_SPLINE,
	GO_LINE_INTERPOLATION_CUBIC_SPLINE,
	GO_LINE_INTERPOLATION_PARABOLIC_CUBIC_SPLINE,
	GO_LINE_INTERPOLATION_CUBIC_CUBIC_SPLINE,
	GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE,
	GO_LINE_INTERPOLATION_STEP_START,
	GO_LINE_INTERPOLATION_STEP_END,
	GO_LINE_INTERPOLATION_STEP_CENTER_X,
	GO_LINE_INTERPOLATION_STEP_CENTER_Y,
	GO_LINE_INTERPOLATION_ODF_SPLINE,
	GO_LINE_INTERPOLATION_MAX
} GOLineInterpolation;

typedef enum
{
  GO_ANCHOR_CENTER,
  GO_ANCHOR_NORTH,
  GO_ANCHOR_NORTH_WEST,
  GO_ANCHOR_NORTH_EAST,
  GO_ANCHOR_SOUTH,
  GO_ANCHOR_SOUTH_WEST,
  GO_ANCHOR_SOUTH_EAST,
  GO_ANCHOR_WEST,
  GO_ANCHOR_EAST,
  GO_ANCHOR_BASELINE_CENTER,
  GO_ANCHOR_BASELINE_WEST,
  GO_ANCHOR_BASELINE_EAST,
  GO_ANCHOR_N		= GO_ANCHOR_NORTH,
  GO_ANCHOR_NW		= GO_ANCHOR_NORTH_WEST,
  GO_ANCHOR_NE		= GO_ANCHOR_NORTH_EAST,
  GO_ANCHOR_S		= GO_ANCHOR_SOUTH,
  GO_ANCHOR_SW		= GO_ANCHOR_SOUTH_WEST,
  GO_ANCHOR_SE		= GO_ANCHOR_SOUTH_EAST,
  GO_ANCHOR_W		= GO_ANCHOR_WEST,
  GO_ANCHOR_E		= GO_ANCHOR_EAST,
  GO_ANCHOR_B		= GO_ANCHOR_BASELINE_CENTER,
  GO_ANCHOR_BW		= GO_ANCHOR_BASELINE_WEST,
  GO_ANCHOR_BE		= GO_ANCHOR_BASELINE_EAST
} GOAnchorType;

typedef enum {
	GOD_ANCHOR_DIR_UNKNOWN    = 0xFF,
	GOD_ANCHOR_DIR_UP_LEFT    = 0x00,
	GOD_ANCHOR_DIR_UP_RIGHT   = 0x01,
	GOD_ANCHOR_DIR_DOWN_LEFT  = 0x10,
	GOD_ANCHOR_DIR_DOWN_RIGHT = 0x11,

	GOD_ANCHOR_DIR_NONE_MASK  = 0x00,
	GOD_ANCHOR_DIR_H_MASK	  = 0x01,
	GOD_ANCHOR_DIR_RIGHT	  = 0x01,
	GOD_ANCHOR_DIR_V_MASK	  = 0x10,
	GOD_ANCHOR_DIR_DOWN	  = 0x10
} GODrawingAnchorDir;

typedef struct _GODrawingAnchor {
	int			pos_pts [4];	/* position in points */
	GODrawingAnchorDir	direction;
} GODrawingAnchor;

typedef enum {
	GO_FONT_SCRIPT_SUB	= -1,
	GO_FONT_SCRIPT_STANDARD =  0,
	GO_FONT_SCRIPT_SUPER	=  1
} GOFontScript;

typedef enum {
	GO_RESOURCE_NATIVE,
	GO_RESOURCE_RO,
	GO_RESOURCE_RW,
	GO_RESOURCE_CHILD,
	GO_RESOURCE_EXTERNAL,
	GO_RESOURCE_GENERATED,
	GO_RESOURCE_INVALID
} GoResourceType;

G_END_DECLS

#include <goffice/goffice.h>

#include <goffice/utils/datetime.h>
#include <goffice/utils/go-bezier.h>
#include <goffice/utils/go-cairo.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-editor.h>
#include <goffice/utils/go-file.h>
#include <goffice/utils/go-font.h>
#include <goffice/utils/go-format.h>
#include <goffice/utils/go-geometry.h>
#include <goffice/utils/go-glib-extras.h>
#include <goffice/utils/go-gdk-pixbuf.h>
#include <goffice/utils/go-gradient.h>
#include <goffice/utils/go-image.h>
#include <goffice/utils/go-pixbuf.h>
#include <goffice/utils/go-svg.h>
#include <goffice/utils/go-emf.h>
#include <goffice/utils/go-spectre.h>
#include <goffice/utils/go-libxml-extras.h>
#include <goffice/utils/go-line.h>
#include <goffice/utils/go-locale.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-mml-to-itex.h>
#include <goffice/utils/go-pango-extras.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-pattern.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-rsm.h>
#include <goffice/utils/go-string.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-undo.h>
#include <goffice/utils/go-unit.h>
#include <goffice/utils/go-units.h>
#include <goffice/utils/regutf8.h>

GType go_resource_type_get_type (void);
char *go_uuid (void);

#endif /* GOFFICE_UTILS_H */
