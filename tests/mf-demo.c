/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * mf-demo.c : open WMF file
 *
 *  Copyright (C) 2010 Valek Filippov (frob@gnome.org)
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <gtk/gtk.h>
#include <goffice/goffice.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-input-stdio.h>

typedef struct {
	guint8 b;
	guint8 g;
	guint8 r;
	guint8 a;
} Color;

typedef struct {
	double VPx;
	double VPy;
	double VPOx;
	double VPOy;
	double Wx;
	double Wy;
	double x;
	double y;
	double curx;
	double cury;
	gint curfg; 
	gint curbg; 
	gint curfnt;
	gint curpal;
	gint curreg;
	guint txtalign;
	guint bkmode;
	guint pfm;
	Color bkclr;
	Color txtclr;
} DC; 

typedef struct {
	guint type; /* 1 pen, 2 brush, 3 font, 4 region, 5 palette */
	gpointer values;
} MFobj;

typedef struct {
	guint16 style;
	gint16 width;
	gint16 height;
	Color clr;
	guint16 flag;
} Pen;

typedef struct {
	guint width;
	guint height;
	gpointer data;
} brushdata;

typedef struct {
	guint16 style;
	Color clr;
	guint16 hatch;
	brushdata bdata;
} Brush;

typedef struct {
	double b;
	double r;
	double t;
	double l;
	double xs;
	double ys;
	double xe;
	double ye;
} Arc;


typedef struct {
	gpointer name;
	gint16 size;
	gint16 weight;
	gint16 escape;
	gint16 orient;
	gint italic;
	gint under;
	gint strike;
	gchar *charset;
} Font;

typedef struct {
	int maxobj;
	guint curobj;
	guint curdc;
	GHashTable *mfobjs;
	GArray *dcs;
	double zoom;
	double w;
	double h;
	int type;
} Page;

void mr0 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr1 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr2 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr3 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr4 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr5 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr6 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr7 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr8 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr9 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr10 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr11 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr12 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr13 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr14 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr15 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr16 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr17 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr18 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr19 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr20 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr21 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr22 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr23 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr24 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr25 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr26 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr27 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr28 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr29 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr30 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr31 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr32 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr33 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr34 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr35 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr36 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr37 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr38 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr39 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr40 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr41 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr42 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr43 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr44 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr45 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr46 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr47 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr48 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr49 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr50 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr51 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr52 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr53 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr54 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr55 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr56 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr57 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr58 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr59 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr60 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr61 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr62 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr63 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr64 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr65 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr66 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr67 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr68 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);
void mr69 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);

void fill (Page* pg, GocItem* item);
void stroke (Page* pg, GocItem* item);
void init_dc (DC* dc);
void init_page (Page* pg);
void set_font (Page* pg, GocItem* item);
GHashTable* init_recs (void);
GHashTable* init_esc (void);
void mr_convcoord (double* x, double* y, Page* pg);
void mr_arc (GsfInput* input, Page* pg, GocCanvas* canvas, int type);
void mr_rect (GsfInput* input, Page* pg, GocCanvas* canvas, int type);
void mr_poly (GsfInput* input, Page* pg, GocCanvas* canvas, int type);
int find_obj (Page* pg);
void read_color (GsfInput* input, Color* clr);
void read_point (GsfInput* input, double* y, double* x);
void set_anchor (Page* pg, GtkAnchorType* anchor);
void set_align (GsfInput* input, Page* pg, double* x, double* y);
void set_text (Page* pg, GocCanvas* canvas, char* txt, int len, GtkAnchorType* anchor, double* x, double* y);
char* symbol_to_utf (char* txt);
char* mtextra_to_utf (char* txt);

void
init_dc (DC *dc)
{
	dc->VPx = 1;
	dc->VPy = 1;
	dc-> VPOx = 0;
	dc->VPOy = 0;
	dc->Wx = 1.;
	dc->Wy = 1.;
	dc->x = 0;
	dc->y = 0;
	dc->curx = 0;
	dc->cury = 0;
	dc->curfg = -1; 
	dc->curbg = -1; 
	dc->curfnt = -1;
	dc->curpal = -1;
	dc->curreg = -1;
	dc->txtalign = 0;
	dc->bkmode = 0;
	dc->pfm = 1;
	dc->bkclr = (Color) {255, 255, 255};
	dc->txtclr = (Color) {0, 0, 0};
}

void
init_page (Page* pg)
{
	DC mydc;

	init_dc (&mydc);
	pg->maxobj = 0;
	pg->curobj = 0;
	pg->curdc = 0;
	pg->w = 320;
	pg->h = 200;
	pg->zoom = 1.;
	pg->type = 0;
	pg->mfobjs = g_hash_table_new (g_direct_hash, g_direct_equal);
	pg->dcs = g_array_new (FALSE, FALSE, sizeof (DC));
	g_array_append_vals (pg->dcs, &mydc, 1);
}

void
mr_convcoord (double* x, double* y, Page* pg)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	*x = (*x - dc->x) * dc->VPx / dc->Wx + dc->VPOx;
	*y = (*y - dc->y) * dc->VPy / dc->Wy + dc->VPOy;
}

void
set_anchor (Page* pg, GtkAnchorType* anchor)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);

	switch(dc->txtalign & 6) {
	case 0: /* right */
		switch(dc->txtalign & 24) {
			case 0: /* top */
				*anchor = GTK_ANCHOR_SOUTH_WEST;
				break;
			case 8: /* bottom */
				*anchor = GTK_ANCHOR_NORTH_WEST;
				break;
			case 24: /* baseline */
				*anchor = GTK_ANCHOR_WEST;
				break;
		}
		break;
	case 2: /* left */
		switch(dc->txtalign & 24) {
			case 0: /* top */
				*anchor = GTK_ANCHOR_SOUTH_EAST;
				break;
			case 8: /* bottom */
				*anchor = GTK_ANCHOR_NORTH_EAST;
				break;
			case 24: /* baseline */
				*anchor = GTK_ANCHOR_EAST;
				break;
		}
		break;
	case 6: /* center */
		switch(dc->txtalign & 24) {
			case 0: /* top */
				*anchor = GTK_ANCHOR_SOUTH;
				break;
			case 8: /* bottom */
				*anchor = GTK_ANCHOR_NORTH;
				break;
			case 24: /* baseline */
				*anchor = GTK_ANCHOR_CENTER;
				break;
		}
	}
}

void
set_align (GsfInput* input, Page* pg, double* x, double* y)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	if (dc->txtalign % 2) {
		/* cpupdate */
		/* FIXME: have to update pg->curx, pg->cury with layout size*/
		*x = dc->curx;
		*y = dc->cury;
		gsf_input_seek (input, 4, G_SEEK_CUR);
	} else {
		read_point (input, y, x);
		mr_convcoord (x, y, pg);
	}
}

void
set_text (Page* pg, GocCanvas* canvas, char* txt, int len, GtkAnchorType* anchor, double* x, double* y)
{
	GocItem *gocitem;
	char *utxt;
	MFobj *mfo;
	Font *font;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	txt[len] = 0;
	if (-1 != dc->curfnt) {
		mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curfnt));
		font = mfo->values;

		if (!g_ascii_strcasecmp ("MT Extra", font->name)) {
			utxt = mtextra_to_utf (txt);
		} else if (!g_ascii_strcasecmp ("SYMBOL", font->name)) {
			utxt = symbol_to_utf (txt);
		} else {
			utxt = g_convert (txt, len, "utf8", font->charset, NULL, NULL, NULL);
		}
		
	} else {
		utxt = g_convert (txt, len, "utf8", "ASCII", NULL, NULL, NULL);
	}
	gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_TEXT,
					"x", *x, "y", *y, "text", utxt, "anchor", *anchor, NULL);
	set_font (pg, gocitem);
}

void
read_color (GsfInput* input, Color* clr)
{
	const guint8  *data = {0};

	data = gsf_input_read (input, 1, NULL);
	clr->r = *data;
	data = gsf_input_read (input, 1, NULL);
	clr->g = *data;
	data = gsf_input_read (input, 1, NULL);
	clr->b = *data;
}

void
read_point (GsfInput* input, double* y, double* x)
{
	const guint8  *data = {0};

	data = gsf_input_read (input, 2, NULL);
	*y = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	*x = GSF_LE_GET_GINT16 (data);
}

int
find_obj (Page* pg)
{
	int i = 0;

	while (i <= pg->maxobj) {
		if(NULL == g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (i)))
			break;
		else
			i++;
	}
	if (i == pg->maxobj)
		pg->maxobj++;
	return i;
}

typedef void 
(*Handler) (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas);

Handler mfrec_dump[70] = 
{
	mr0, mr1, mr2, mr3, mr4, mr5, mr6, mr7, mr8, mr9,
	mr10, mr11, mr12, mr13, mr14, mr15, mr16, mr17, mr18, mr19,
	mr20, mr21, mr22, mr23, mr24, mr25, mr26, mr27, mr28, mr29,
	mr30, mr31, mr32, mr33, mr34, mr35, mr36, mr37, mr38, mr39,
	mr40, mr41, mr42, mr43, mr44, mr45, mr46, mr47, mr48, mr49,
	mr50, mr51, mr52, mr53, mr54, mr55, mr56, mr57, mr58, mr59,
	mr60, mr61, mr62, mr63, mr64, mr65, mr66, mr67, mr68, mr69
};

static void
parse (GsfInput* input, GocCanvas* canvas)
{
	GHashTable	*mrecords, *escrecords;
	GHashTable	*objs;
	Page *mypg;
	guint32 rsize = 3;
	int rid = 0, escfunc;
	gint mr = 0;
	const guint8  *data = {0};
	guint32 type = 0; /* wmf, apm or emf */
	guint32 offset = 0;
	guint64 fsize = 0;
	double x1, y1, x2, y2, w, h;
	DC *dc;

	mypg = malloc (sizeof (Page));  
	fsize = gsf_input_size (input);
	data = gsf_input_read (input, 4, NULL);
	switch (GSF_LE_GET_GUINT32 (data)) {
	case 0x9ac6cdd7:
		g_print ("Aldus Placeable Metafile\n");
		type = 1;
		offset = 40;
		break;
	case 0x090001:
		g_print ("Standard metafile\n");
		type = 2 ;
		offset = 18;
		break;
	case 1:
		gsf_input_seek (input, 40, G_SEEK_SET);  /* 40 -- offset to EMF signature */
		data = gsf_input_read (input, 4, NULL);
		if (0x464D4520 == GSF_LE_GET_GUINT32 (data)) {
			type = 3;
			g_print ("EMF file\n");
		} else {
			g_print ("Unknown type");
		}
		break;
	default:
		g_print ("Unknown type");
	}

	if (1 == type || 2 == type) {
		mrecords = init_recs ();
		escrecords = init_esc ();
		init_page (mypg);
		objs = g_hash_table_new (g_direct_hash, g_direct_equal);
		dc = &g_array_index (mypg->dcs, DC, mypg->curdc);
		if (1 == type) {
			gsf_input_seek (input, 6, G_SEEK_SET);
			read_point (input, &x1, &y1);
			read_point (input, &x2, &y2);
			w = abs (x2 - x1);
			h = abs (y2 - y1);
			dc->VPx = mypg->w;
			dc->VPy = mypg->w * h / w;
			if (mypg->w * h / w > mypg->h) {
				dc->VPy = mypg->h;
				dc->VPx = mypg->h * w / h;
			}
			mypg->zoom = dc->VPx / w;
			mypg->type = 1; 
			dc->Wx = w;
			dc->Wy = h;
		} else {
			mypg->type = 2;
		}

		gsf_input_seek (input, offset, G_SEEK_SET);
		while (offset < fsize - 6) {  // check if it's end of file already
			data = gsf_input_read (input, 4, NULL);
			rsize = GSF_LE_GET_GUINT32 (data);
			data = gsf_input_read (input, 2, NULL);
			rid = GSF_LE_GET_GINT16 (data);
			if (0 == rid || offset + rsize * 2 > fsize)
				break;
			if (0x626 != rid) {
				mr = GPOINTER_TO_INT (g_hash_table_lookup (mrecords, GINT_TO_POINTER (rid)));
				g_print ("Offset: %d, Rid: %x, MR: %d, RSize: %d\n", offset, rid, mr, rsize);
				mfrec_dump[mr] (input, rsize, mypg, objs, canvas);
			} else {
				data = gsf_input_read (input, 2, NULL);
				escfunc = GSF_LE_GET_GINT16 (data);
				g_print ("ESCAPE! Function is %04x -- %s. Size: %d bytes.\n", escfunc, (char*) g_hash_table_lookup (escrecords, GINT_TO_POINTER (escfunc)), rsize * 2 - 6);
				if (0xf == escfunc)
					mr58 (input, rsize, mypg, objs, canvas);
				gsf_input_seek (input, -2, G_SEEK_CUR);
			}
			gsf_input_seek (input, rsize * 2 - 6, G_SEEK_CUR);
			offset += rsize * 2;
		}
	}
}

void
set_font (Page *pg, GocItem *item)
{
	MFobj *mfo;
	Font *font;
	double rot;
	PangoAttrList *pattl = pango_attr_list_new ();
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	if (-1 == dc->curfnt)
		return;

	mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curfnt));
	font = mfo->values;
	// convert font values to PangoAttrList
	if (font->escape > 0) {
		rot = (double) (font->escape % 3600) * M_PI / 1800.;
		goc_item_set (item, "rotation", M_PI * 2 - rot, NULL);
	}
	if (font->size < 0)
		font->size = -font->size;
	if (font->size == 0)
		font->size = 12;
	pango_attr_list_insert (pattl, pango_attr_size_new (font->size * 1024. * pg->zoom / 1.5));
	pango_attr_list_insert (pattl, pango_attr_foreground_new (dc->txtclr.r * 256, dc->txtclr.g * 256, dc->txtclr.b * 256));

	if (2 == dc->bkmode)
		pango_attr_list_insert (pattl, pango_attr_background_new (dc->bkclr.r * 256, dc->bkclr.g * 256, dc->bkclr.b * 256));
	pango_attr_list_insert (pattl, pango_attr_weight_new (font->weight));
	pango_attr_list_insert (pattl, pango_attr_family_new (font->name));
	if (0 != font->italic)
		pango_attr_list_insert (pattl, pango_attr_style_new (PANGO_STYLE_ITALIC));
	if (0 != font->under)
		pango_attr_list_insert (pattl, pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));
	if (0 != font->strike)
		pango_attr_list_insert (pattl, pango_attr_strikethrough_new (TRUE));
	goc_item_set(item, "attributes", pattl, NULL);
}

/*
 * MF: 0 - solid, 1 - dash, 2 - dot,  3 - dashdot,  4 - dashdotdot
 * GO: 1 - solid, 8 - dash, 6 - dot, 10 - dashdot, 11 - dashdotdot
  */

/*
MF2cairo: capjoin = {0:1,1:2,2:0}
MF: cap = {0:'Flat',1:'Round',2:'Square'}
MF: join = {0:'Miter',1:'Round',2:'Bevel'}
*/

void
stroke (Page *pg, GocItem *item)
{
	Pen *p;
	MFobj *mfo;
	GOStyle *style;
	int dashes[5] = {1, 8, 6, 10, 11};
	int capjoin[3] = {1, 2, 0};
	int pstyle = 0;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	if (-1 == dc->curfg)
		return;

	style = go_styled_object_get_style (GO_STYLED_OBJECT (item));

	mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curfg));
	p = mfo->values;
	pstyle = p->style;
	if (p->style == 5) {
		style->line.dash_type = GO_LINE_NONE;
		style->line.auto_dash = FALSE;
	} else {
		style->line.color = GO_COLOR_FROM_RGB (p->clr.r, p->clr.g, p->clr.b);
		style->line.dash_type = GO_LINE_SOLID;
		if (p->width > 1) {
			style->line.width = (double) p->width * pg->zoom;
		} else {
			style->line.width = 0.;
		}
		if (3 > (pstyle & 0xF00) >> 8) {
			style->line.cap = capjoin[(pstyle & 0xF00) >> 8];
		}
		if (3 > (pstyle & 0xF000) >> 12) {
			style->line.join = capjoin[(pstyle & 0xF000) >> 12];
		}
		
		if (pstyle > 5)
			pstyle = 0;
		style->line.dash_type = dashes[pstyle];
		if (1 == dc->bkmode) {
			style->line.fore = GO_COLOR_FROM_RGBA (0, 0, 0, 0);
		} else {
			style->line.color = GO_COLOR_FROM_RGB (p->clr.r, p->clr.g, p->clr.b);
		}
	}
}

/*
 * MF: 0 HS_HORIZONTAL, 1 HS_VERTICAL, 2 HS_FDIAGONAL, 3 HS_BDIAGONAL, 4 HS_CROSS, 5 HS_DIAGCROSS
 * GO: 12				13				14				15				16			17
 */
void
fill (Page *pg, GocItem *item)
{
	Brush *b;
	MFobj *mfo;
	GOStyle *style;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	if (-1 == dc->curbg)
		return;

	style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	mfo = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (dc->curbg));
	b = mfo->values;
	switch (b->style) {
	case 1:
		style->fill.type = GO_STYLE_FILL_NONE;
		break;
	case 2:
		style->fill.pattern.pattern = b->hatch + 12;
		if (1 == dc->bkmode) {
			style->fill.pattern.back = GO_COLOR_FROM_RGBA (0, 0, 0, 0);
		} else {
			style->fill.pattern.back = GO_COLOR_FROM_RGB (dc->bkclr.r, dc->bkclr.g, dc->bkclr.b);
		}
		style->fill.type = GO_STYLE_FILL_PATTERN;
		style->fill.pattern.fore = GO_COLOR_FROM_RGB (b->clr.r, b->clr.g, b->clr.b);
		break;
	case 3:
		style->fill.image.image = go_image_new_from_pixbuf (b->bdata.data);
		style->fill.image.type = GO_IMAGE_WALLPAPER;
		style->fill.type = GO_STYLE_FILL_IMAGE;
		break;
	default:
		style->fill.type = GO_STYLE_FILL_PATTERN;
		style->fill.pattern.back = GO_COLOR_FROM_RGB (b->clr.r, b->clr.g, b->clr.b);
	}
}


void
mr0 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
//g_print("!EOF ");
}

// -------------- SaveDC ----------------
void
mr1 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
	DC mydc;

	mydc = g_array_index (pg->dcs, DC, pg->curdc);
	g_array_append_vals (pg->dcs, &mydc, 1);
	pg->curdc = pg->dcs->len - 1;
}

void
mr2 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("RealizePalette ");
}

void
mr3 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("SetPalEntries ");
}

// ------------- CreatePalette ------------
void
mr4 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
	/* FIXME: do actual parsing */
	MFobj *mf;

	mf = malloc (sizeof (MFobj));
	mf->type = 5;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (find_obj (pg)), mf);
}

// ------------- SetBKMode -----------------
void
mr5 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
	const guint8  *data = {0};
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	dc->bkmode = GSF_LE_GET_GUINT16 (data);
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

void
mr6 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("SetMapMode ");
}

void
mr7 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("SetROP2 ");
}

// ------------- SetReLabs -------------
void
mr8 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
	/* FIXME: Exclude, should never be used */
}

// ----------------- SetPolyfillMode -----------------
void
mr9 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
	const guint8  *data = {0};
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	if (GSF_LE_GET_GUINT16 (data) > 1)
		dc->pfm = 0;
	else
		dc->pfm = 1;
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

void
mr10 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("SetStrechBLTMode ");
}

void
mr11 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("SetTextCharExtra ");
}

// ---------------- RestoreDC -----------------------
void
mr12 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
	const guint8  *data = {0};
	gint16 idx;

	data = gsf_input_read (input, 2, NULL);
	idx = GSF_LE_GET_GINT16 (data);
	if (idx > 0)
		pg->curdc = idx;
	else
		pg->curdc -= idx;
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

void
mr13 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("InvertRegion ");
}

void
mr14 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("PaintRegion ");
}

void
mr15 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas *canvas)
{
  //g_print("SelectClipRegion ");
}

// -------------------- Select Object -----------------
void
mr16 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	int idx;
	MFobj *mf;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	idx = GSF_LE_GET_GUINT16 (data);
 
	mf = g_hash_table_lookup (pg->mfobjs, GINT_TO_POINTER (idx));
	switch (mf->type) {
	case 1:
		dc->curfg = idx; /* pen */
		break;
	case 2:
		dc->curbg = idx; /* brush */
		break;
	case 3:
		dc->curfnt = idx; /* font */
		break;
	case 4:
		dc->curreg = idx; /* region */
		break;
	case 5:
		/* palette, shouldn't happen "SelectPalette" have to be used to select palette */
		break;
	default:
		break;
	}
	gsf_input_seek (input, -2, G_SEEK_CUR);
}

// ---------------- SetTextAlign -----------------------
void
mr17 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	dc->txtalign = GSF_LE_GET_GUINT16 (data);
	gsf_input_seek(input, -2, G_SEEK_CUR);
}

void
mr18 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("ResizePalette ");
}

// --------------- DIBCreatePatternBrush --------------
void
mr19 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	Brush *brush;
	MFobj *mf;
	guint32 w, h, bmpsize, bmpshift = 54;
	guint16 biDepth;
	GdkPixbufLoader *gpbloader = gdk_pixbuf_loader_new ();
	GdkPixbuf *gpb;
	GError **error = NULL;
	guint8 *tmp;
	gint32 rollback = 6 - rsize * 2;

	brush = malloc (sizeof (Brush));
	brush->style = 3; // DibPattern brush
	gsf_input_seek(input, 8, G_SEEK_CUR);
	data = gsf_input_read (input, 4, NULL);
	w = GSF_LE_GET_GUINT32 (data);
	data = gsf_input_read (input, 4, NULL);
	h = GSF_LE_GET_GUINT32 (data);
	gsf_input_seek (input, 2, G_SEEK_CUR);
	data = gsf_input_read (input, 2, NULL);
	biDepth = GSF_LE_GET_GUINT32 (data);
	gsf_input_seek(input, -16, G_SEEK_CUR);
	data = gsf_input_read (input, rsize * 2 - 6 - 4, NULL);
	bmpsize = rsize * 2 - 6 + 10;

	gsf_input_seek (input, rollback, G_SEEK_CUR);
	switch (biDepth) {
	case 1:
		bmpshift = 62;
		break;
	case 2:
		bmpshift = 70;
		break;
	case 4:
		bmpshift = 118;
		break;
	case 8:
		bmpshift = 1078;
		break;
	}

	tmp = malloc (bmpsize);
	tmp[0] = 0x42;
	tmp[1] = 0x4d;
	memcpy(tmp + 2, &bmpsize, 4);
	tmp[6] = 0;
	tmp[7] = 0;
	tmp[8] = 0;
	tmp[9] = 0;
	memcpy(tmp + 10, &bmpshift, 4);
	memcpy(tmp + 14, data, bmpsize - 14);

	gdk_pixbuf_loader_write (gpbloader, tmp, bmpsize, error);
	gdk_pixbuf_loader_close (gpbloader, error);
	gpb = gdk_pixbuf_loader_get_pixbuf (gpbloader);

	brush->bdata.width = w;
	brush->bdata.height = h;
	brush->bdata.data = gpb;
	
	mf = malloc (sizeof (MFobj));
	mf->type = 2;
	mf->values = brush;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (find_obj (pg)), mf);

}

//  -------------- SetLayout --------------------
void
mr20 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{

}

//  -------------- DeleteObject --------------------
void
mr21 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	int idx;

	data = gsf_input_read (input, 2, NULL);
	idx = GSF_LE_GET_GUINT16 (data);
	g_hash_table_remove (pg->mfobjs, GINT_TO_POINTER (idx));
	gsf_input_seek(input, -2, G_SEEK_CUR);
}

//  -------------- CreatePatternBrush --------------------
void
mr22 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	/* FIXME: add pixbufloader */
	MFobj *mf;

	mf = malloc (sizeof (MFobj));
	mf->type = 2; /* brush */
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (find_obj (pg)), mf);
}

//  -------------- SetBKColor --------------------
void
mr23 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_color(input, &dc->bkclr);
	gsf_input_seek (input, -3, G_SEEK_CUR);
}

//  -------------- SetTextColor --------------------
void
mr24 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_color (input, &dc->txtclr);
	gsf_input_seek (input, -3, G_SEEK_CUR);
}

void
mr25 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("SetTextJustification ");
}

// ---------------- SetWindowOrg -----------------
void
mr26 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &dc->y, &dc->x);
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

//  ---------------- SetWindowExt -------------------
void
mr27 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &dc->Wy, &dc->Wx);
	gsf_input_seek (input, -4, G_SEEK_CUR);
	if (2 == pg->type) {
		if(pg->w*dc->Wy / dc->Wx > pg->h) {
			dc->VPy = pg->h;
			dc->VPx = pg->h * dc->Wx / dc->Wy;
		} else {
			dc->VPx = pg->w;
			dc->VPy = pg->w * dc->Wy / dc->Wx;
		}
		pg->zoom = dc->VPx / dc->Wx;
	}
}

//  ----------------- SetViewportOrg -------------------
void
mr28 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &dc->VPOy, &dc->VPOx); 
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

//  ----------------- SetViewportExt --------------------
void
mr29 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &dc->VPy, &dc->VPx); 
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

// ------------------- OffsetWindowOrg ------------------
void
mr30 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;
	double x,y;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &x, &y);
	dc->x += x;
	dc->y += y;
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

// ------------------- OffsetViewportOrg ----------------
void
mr31 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	DC *dc;
	double x,y;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &x, &y);
	dc->VPx += x;
	dc->VPy += y;
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

//  ------------------ LineTo --------------------
void
mr32 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	double x,y;
	GocItem *gocitem;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &y, &x);
	gsf_input_seek (input, -4, G_SEEK_CUR);
	mr_convcoord (&x, &y, pg);
	gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_LINE,
		"x0", dc->curx, "y0", dc->cury, "x1", x, "y1", y, NULL);
	dc->curx = x;
	dc->cury = y;
	stroke (pg, gocitem);
}

//  ------------------ MoveTo --------------------
void
mr33 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	double x,y;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	read_point (input, &y, &x);
	gsf_input_seek (input, -4, G_SEEK_CUR);
	mr_convcoord (&x, &y, pg);
	dc->curx = x;
	dc->cury = y;
}

void
mr34 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("OffsetClipRgn ");
}

void
mr35 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("FillRegion ");
}

void
mr36 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("SetMapperFlags ");
}

void
mr37 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("SelectPalette ");
}

//  ------------------ CreatePenIndirect -------------------
void
mr38 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	Pen *pen;
	MFobj *mf;

	pen = malloc (sizeof (Pen));
	data = gsf_input_read (input, 2, NULL);
	pen->style = GSF_LE_GET_GUINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	pen->width = GSF_LE_GET_GINT16 (data);
	gsf_input_seek (input, 2, G_SEEK_CUR); /* skip "height" */
	read_color (input, &pen->clr);
	gsf_input_seek (input, 1, G_SEEK_CUR);
	data = gsf_input_read (input, 2, NULL);
	pen->flag = GSF_LE_GET_GUINT16 (data);
	gsf_input_seek (input, -12, G_SEEK_CUR);
	mf = malloc (sizeof (MFobj));
	mf->type = 1;
	mf->values = pen;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (find_obj (pg)), mf);
}


// ----------------- CreateFontIndirect -------------
void
mr39 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	Font *font;
	MFobj *mf;
	int i = 0;

	font = malloc (sizeof (Font));
	data = gsf_input_read (input, 2, NULL);
	font->size = GSF_LE_GET_GINT16 (data);
	gsf_input_seek (input, 2, G_SEEK_CUR); /* skip 'width' */
	data = gsf_input_read (input, 2, NULL);
	font->escape = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	font->orient = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	font->weight = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 1, NULL);
	font->italic = GSF_LE_GET_GUINT8 (data);
	data = gsf_input_read (input, 1, NULL);
	font->under = GSF_LE_GET_GUINT8 (data);
	data = gsf_input_read (input, 1, NULL);
	font->strike = GSF_LE_GET_GUINT8 (data);
	data = gsf_input_read (input, 1, NULL);
/*
77:'MAC_CHARSET',
*/
	switch (GSF_LE_GET_GUINT8(data)) {
	case 0:
	case 1:
		font->charset = g_strdup ("iso8859-1");
		break;
	case 2:
		font->charset = g_strdup ("Symbol");
		break;
	case 128:
		font->charset = g_strdup ("Shift_JIS");
		break;
	case 129:
		font->charset = g_strdup ("cp949");
		break;
	case 134:
		font->charset = g_strdup ("gb2312");
		break;
	case 136:
		font->charset = g_strdup ("Big5");
		break;
	case 161:
		font->charset = g_strdup ("cp1253");
		break;
	case 162:
		font->charset = g_strdup ("cp1254");
		break;
	case 163:
		font->charset = g_strdup ("cp1258");
		break;
	case 177:
		font->charset = g_strdup ("cp1255");
		break;
	case 178:
		font->charset = g_strdup ("cp1256");
		break;
	case 186:
		font->charset = g_strdup ("cp1257");
		break;
	case 204:
		font->charset = g_strdup ("cp1251");
		break;
	case 222:
		font->charset = g_strdup ("cp874");
		break;
	case 238:
		font->charset = g_strdup ("cp1250");
		break;
	case 255:
		font->charset = g_strdup ("cp437");
		break;
	default:
		font->charset = g_strdup ("cp437");
	}

	gsf_input_seek (input, 4, G_SEEK_CUR);
	data = gsf_input_read (input, 1, NULL);
	while (0 != data[0] && (guint) i < rsize * 2 - 6) {
		i++;
		data = gsf_input_read (input, 1, NULL);
	}
	gsf_input_seek (input, -i - 1, G_SEEK_CUR);
	font->name = malloc (i + 1);
	gsf_input_read (input, i + 1, font->name);
	gsf_input_seek (input, -19 - i, G_SEEK_CUR);
	mf = malloc (sizeof (MFobj));
	mf->type = 3;
	mf->values = font;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (find_obj (pg)), mf);
}

// ---------------- CreateBrushIndirect ---------------
void
mr40 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8 *data = {0};
	Brush *brush;
	MFobj *mf;

	brush = malloc (sizeof (Brush));
	data = gsf_input_read (input, 2, NULL);
	brush->style = GSF_LE_GET_GUINT16 (data);
	read_color (input, &brush->clr);
	gsf_input_seek (input, 1, G_SEEK_CUR); /* skip "clr.a" */
	data = gsf_input_read (input, 2, NULL);
	brush->hatch = GSF_LE_GET_GUINT16 (data); 
	gsf_input_seek (input, -8, G_SEEK_CUR);
	mf = malloc (sizeof (MFobj));
	mf->type = 2;
	mf->values = brush;
	g_hash_table_insert ((*pg). mfobjs, GINT_TO_POINTER (find_obj (pg)), mf);
}

void
mr_poly (GsfInput* input, Page* pg, GocCanvas* canvas, int type)
{
	const guint8  *data = {0};
	double len;
	guint i;
	GocItem *gocitem;
	GocPoints *points;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	len = GSF_LE_GET_GINT16 (data);
	points = goc_points_new (len);
	for (i = 0; i < len; i++) {
		read_point (input, &points->points[i].x, &points->points[i].y);
		mr_convcoord (&points->points[i].x, &points->points[i].y, pg);
	}

	if (0 == type)
		gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_POLYGON, "points", points, "fill-rule", dc->pfm, NULL);
	else
		gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_POLYLINE, "points", points, NULL);
	fill (pg,gocitem);
	stroke (pg,gocitem);
	gsf_input_seek (input, -len * 4 - 2, G_SEEK_CUR);
}

//  ---------- Polygon ----------------
void
mr41 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_poly (input, pg, canvas, 0);
}

//  ---------- Polyline ----------------
void
mr42 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_poly (input, pg, canvas, 1);
}

void
mr43 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("ScaleWindowExtEx ");
}

void
mr44 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("ScaleViewportExt ");
}

void
mr45 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("ExcludeClipRect ");
}

void
mr46 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("IntersectClipRect ");
}

void
mr_rect (GsfInput* input, Page* pg, GocCanvas* canvas, int type)
{
	double x1, x2, y1, y2, tx, ty, rx = 0, ry = 0;
	GocItem *gocitem;

	if (15 == type) {
		read_point (input, &ry, &rx);
		ry = abs (ry / 2);
		rx = abs (rx / 2);
		mr_convcoord (&rx, &ry, pg);
	}
	read_point (input, &y2, &x2);
	read_point (input, &y1, &x1);
	mr_convcoord (&x1, &y1, pg);
	mr_convcoord (&x2, &y2, pg);

	tx = x2 - x1;
	if (x1 > x2) {
		tx = x1;	x1 = x2;	tx -= x1;
	}
	ty = y2 - y1;
	if (y1 > y2 ) {
		ty = y1; y1 = y2; ty -= y1;
	}

	if (1 == type)
		gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_ELLIPSE,
			"height", (double) ty, "x", (double) x1, "y", (double) y1, "width", (double) tx, NULL);
	else
		gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_RECTANGLE,
			"height", (double) ty, "x", (double) x1, "y", (double) y1, "width",
			(double) tx, "rx", (double) rx, "ry", (double) ry, "type", type, NULL);
	fill (pg, gocitem);
	stroke (pg, gocitem);
	gsf_input_seek (input, -8, G_SEEK_CUR);
}

// ----------------- Ellipse ---------------
void
mr47 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_rect (input, pg, canvas, 1);
}

void
mr48 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("FloodFill ");
}

//  ---------------- Rectangle --------------
void
mr49 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_rect (input, pg, canvas, 0);
}

void
mr50 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("SetPixel ");
}

void
mr51 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("FrameRegion ");
}

void
mr52 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("AnimatePalette ");
}

//---------------- TextOut --------------------
void
mr53 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	char *txt;
	double x, y;
	int len, shift = 0;
	GtkAnchorType anchor = GTK_ANCHOR_NORTH_WEST;

	set_anchor (pg, &anchor);
	data = gsf_input_read (input, 2, NULL);
	len = GSF_LE_GET_GINT16 (data);
	txt = malloc (sizeof (char) * len + 1);
	gsf_input_read (input, len, txt);
	if (len % 2) {
		gsf_input_seek (input, 1, G_SEEK_CUR);
		shift = 1;
	}
	set_align (input, pg, &x, &y);
	gsf_input_seek (input, -6 - len - shift, G_SEEK_CUR);
	set_text (pg, canvas, txt, len, &anchor, &x, &y);
}

//  ------------ PolyPolygon ------------
void
mr54 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	double x, y;
	int npoly, len, curpos, sumlen = 0;
	int i, j;
	GocItem *gocitem;
	GocIntArray *array;
	GocPoints *points;
	DC *dc;

	dc = &g_array_index (pg->dcs, DC, pg->curdc);
	data = gsf_input_read (input, 2, NULL);
	npoly = GSF_LE_GET_GINT16 (data);
	curpos = gsf_input_tell (input);
	array = goc_int_array_new (npoly);
	
	for (j = 0; j < npoly; j++) {
		gsf_input_seek (input, curpos + j * 2, G_SEEK_SET);
		data = gsf_input_read (input, 2, NULL);
		len = GSF_LE_GET_GINT16 (data);
		array->vals[j] = len;
		sumlen += len;
	}
	points = goc_points_new (sumlen);

	for ( i = 0; i < sumlen; i++) {
			read_point (input, &x, &y);
			mr_convcoord (&x, &y, pg);
			points->points[i].x = x;
			points->points[i].y = y;
	}

	gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_POLYGON,
		"points", points, "sizes", array, "fill-rule", dc->pfm, NULL);
	fill (pg, gocitem);
	stroke (pg, gocitem);
	gsf_input_seek (input, curpos - 2, G_SEEK_SET);
}

void
mr55 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("ExtFloodFill ");
}

//  ---------------- RoundRect ----------------------
void
mr56 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_rect (input, pg, canvas, 15); 
	gsf_input_seek (input, -4, G_SEEK_CUR);
}

void
mr57 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("PatBLT ");
}

// ------------------ Escape ------------------------
void
mr58 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{

}

//------------------ CreateRegion ------------------
void
mr59 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	/* FIXME: do actual parsing */
	MFobj *mf;

	mf = malloc (sizeof (MFobj));
	mf->type = 4;
	g_hash_table_insert ((*pg).mfobjs, GINT_TO_POINTER (find_obj (pg)), mf);
}

void
mr_arc (GsfInput* input, Page* pg, GocCanvas* canvas, int type)
{
	Arc *arc;
	GocItem *gocitem;
	double a1, a2, xc, yc, rx, ry;
	
	arc = malloc (sizeof (Arc));
	read_point (input, &arc->ye, &arc->xe);
	read_point (input, &arc->ys, &arc->xs);
	read_point (input, &arc->b, &arc->r);
	read_point (input, &arc->t, &arc->l);
	gsf_input_seek (input, -16, G_SEEK_CUR);
	mr_convcoord (&arc->xe, &arc->ye, pg);
	mr_convcoord (&arc->xs, &arc->ys, pg);
	mr_convcoord (&arc->r, &arc->b, pg);
	mr_convcoord (&arc->l, &arc->t, pg);

	xc = (arc->l + arc->r) * 0.5;
	yc = (arc->b + arc->t) * 0.5;
	rx = abs ((arc->l - arc->r) * 0.5);
	ry = abs ((arc->b - arc->t) * 0.5);
	a2 = atan2 (arc->xs - xc, arc->ys - yc);
	a1 = atan2 (xc - arc->xe, yc - arc->ye);
	arc->xs = xc;
	arc->ys = yc;
	arc->r = rx;
	arc->t = ry;
	arc->l = a1;
	arc->b = a2;
	gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_ARC,
	 "xc", arc->xs, "yc", arc->ys, "xr",arc->r, "yr", arc->t,
	 "ang1",arc->l, "ang2", arc->b,"type", type, NULL);
	stroke (pg, gocitem);
	if (type > 0)
		fill (pg, gocitem);
}

//  ---------------- Arc ----------------
void
mr60 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_arc (input, pg, canvas, 0);
}

//  ----------------- Pie -----------------
void
mr61 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_arc (input, pg, canvas, 2);
}

//  ---------------- Chord ------------------
void
mr62 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	mr_arc (input, pg, canvas, 1);
}

void
mr63 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("BitBLT ");
}

void
mr64 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("DIBBitBLT ");
}

// ----------------- ExtTextOut ----------------
void
mr65 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	char *txt;
	double x, y;
	int len, flag;
	GtkAnchorType anchor = GTK_ANCHOR_SOUTH_WEST;

	set_anchor (pg, &anchor);
	set_align (input, pg, &x, &y);
	data = gsf_input_read (input, 2, NULL);
	len = GSF_LE_GET_GINT16 (data);
	data = gsf_input_read (input, 2, NULL);
	flag = GSF_LE_GET_GINT16 (data);
	txt = malloc (sizeof (char) * len + 1);
	gsf_input_read (input, len, txt);
	gsf_input_seek (input, -8 - len, G_SEEK_CUR);
	set_text (pg, canvas, txt, len, &anchor, &x, &y);
}

void
mr66 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("StretchBlt ");
}

// ----------------- DIBStretchBlt -----------------------
void
mr67 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{

}

void
mr68 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
  //g_print("SetDIBtoDEV ");
}

// ---------------- StretchDIB --------------------
void
mr69 (GsfInput* input, guint rsize, Page* pg, GHashTable* objs, GocCanvas* canvas)
{
	const guint8  *data = {0};
	GocItem *gocitem;
	double x, y, xe, ye, w, h;
	guint32 bmpsize, bmpshift = 54, dwROP;
	guint16 biDepth;
	GdkPixbufLoader *gpbloader = gdk_pixbuf_loader_new ();
	GdkPixbuf *gpb;
	GError **error = NULL;
	guint8 *tmp;
	gint32 rollback = 6 - rsize * 2;

	data = gsf_input_read (input, 4, NULL);
	dwROP = GSF_LE_GET_GUINT32 (data);
	gsf_input_seek(input, 10, G_SEEK_CUR); /* src x,y,w,h */
	read_point (input, &h, &w);
	read_point (input, &y, &x);
	gsf_input_seek(input, 14, G_SEEK_CUR);
	data = gsf_input_read (input, 2, NULL);
	biDepth = GSF_LE_GET_GUINT32 (data);
	gsf_input_seek(input, -16, G_SEEK_CUR);
	data = gsf_input_read (input, rsize * 2 - 6 - 22, NULL);
	bmpsize = rsize * 2 - 6 - 12;

	gsf_input_seek (input, rollback, G_SEEK_CUR);

	switch (biDepth) {
	case 1:
		bmpshift = 62;
		break;
	case 2:
		bmpshift = 70;
		break;
	case 4:
		bmpshift = 118;
		break;
	case 8:
		bmpshift = 1078;
		break;
	}

	tmp = malloc (bmpsize);
	tmp[0] = 0x42;
	tmp[1] = 0x4d;
	memcpy (tmp + 2, &bmpsize, 4);
	tmp[6] = 0;
	tmp[7] = 0;
	tmp[8] = 0;
	tmp[9] = 0;
	memcpy (tmp + 10, &bmpshift, 4);
	memcpy (tmp + 14, data, bmpsize - 14);

	gdk_pixbuf_loader_write (gpbloader, tmp, bmpsize, error);
	gdk_pixbuf_loader_close (gpbloader, error);
	gpb = gdk_pixbuf_loader_get_pixbuf (gpbloader);

	xe = w - x;
	ye = h -y;
	mr_convcoord (&x, &y, pg);
	mr_convcoord (&xe, &ye, pg);
	w = xe - x;
	h = ye - y;

	gocitem = goc_item_new (goc_canvas_get_root (canvas), GOC_TYPE_PIXBUF,
	 "height", (double) h, "x", (double) x, "y", (double) y, "width",(double) w, "pixbuf", gpb, NULL);
}

GHashTable*
init_recs (void)
{
	GHashTable	*mrecords;
	gint i =0;
	const gint ridarray[70] =
	{		 0, 0x001e, 0x0035, 0x0037, 0x00F7, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106,
		0x0107, 0x0108, 0x0127, 0x012a, 0x012b, 0x012c, 0x012d, 0x012e, 0x0139, 0x0142,
		0x0149, 0x01f0, 0x01f9, 0x0201, 0x0209, 0x020a, 0x020b, 0x020c, 0x020d, 0x020e,
		0x020f, 0x0211, 0x0213, 0x0214, 0x0220, 0x0228, 0x0231, 0x0234, 0x02fa, 0x02fb,
		0x02fc, 0x0324, 0x0325, 0x0410, 0x0412, 0x0415, 0x0416, 0x0418, 0x0419, 0x041b,
		0x041f, 0x0429, 0x0436, 0x0521, 0x0538, 0x0548, 0x061c, 0x061d, 0x0626, 0x06ff,
		0x0817, 0x081a, 0x0830, 0x0922, 0x0940, 0x0a32, 0x0b23, 0x0b41, 0x0d33, 0x0f43};

	mrecords = g_hash_table_new (g_direct_hash, g_direct_equal);
	for (i = 0; i < 70; i++)
		g_hash_table_insert (mrecords, GINT_TO_POINTER (ridarray[i]), GINT_TO_POINTER (i));
	return mrecords;
}

char*
mtextra_to_utf (char* txt)
{
	const guint16 mte[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x300,
	0x302, 0x303, 0x2d9, 0x27, 0x2322, 0x2323, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0xe13d, 0xe141, 0xe13e, 0x23af, 0x35, 0xe13b, 0xe140, 0xe13c, 0x39, 0x3a, 0x3b,
	0x25c3, 0x3d, 0x25b9, 0x3f, 0x40, 0x41, 0x42, 0x2210, 0x19b, 0x45, 0x46,
	0x47, 0x48, 0x2229, 0x4a, 0x2026, 0x22ef, 0x22ee, 0x22f0, 0x22f1, 0x50,
	0x2235, 0x52, 0x53, 0x54, 0x222a, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c,
	0x5d, 0x5e, 0x5f, 0x2035, 0x21a6, 0x2195, 0x21d5, 0x64, 0x65, 0x227b, 0x67,
	0x210f, 0x69, 0x6a, 0x6b, 0x2113, 0x2213, 0x6e, 0x2218, 0x227a, 0x71, 0x20d7,
	0x20d6, 0x20e1, 0x20d6, 0x20d1, 0x20d0, 0x78, 0x79, 0x7a, 0x23de, 0x7c,
	0x23df, 0x7e,0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
	0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94,
	0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
	0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac,
	0xad, 0xae, 0xaf, 0xb0, 0xa1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
	0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4,
	0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
	0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc,
	0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
	0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4,
	0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};
	guint16  *data = {0};
	char* utxt;
	guint i;

	data = malloc (strlen (txt) * 2);
	for (i = 0; i < strlen (txt); i++)
		data[i] = mte[(guint8) txt[i]];
	utxt =  g_utf16_to_utf8 (data, strlen (txt), NULL, NULL, NULL);
	return utxt;
}

char*
symbol_to_utf (char* txt)
{
	const guint16 utf[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x2200, 0x23,
	0x2203, 0x25, 0x26, 0x220d, 0x28, 0x29, 0x2217, 0x2b, 0x2c, 0x2212, 0x2e,
	0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
	0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x2245, 0x391, 0x392, 0x3a7, 0x394, 0x395,
	0x3a6, 0x393, 0x397, 0x399, 0x3d1, 0x39a, 0x39b, 0x39c, 0x39d, 0x39f,
	0x3a0, 0x398, 0x3a1, 0x3a3, 0x3a4, 0x3a5, 0x3c2, 0x3a9, 0x39e, 0x3a8,
	0x396, 0x5b, 0x2234, 0x5d, 0x22a5, 0x5f, 0xf8e5, 0x3b1, 0x3b2, 0x3c7,
	0x3b4, 0x3b5, 0x3c6, 0x3b3, 0x3b7, 0x3b9, 0x3d5, 0x3ba, 0x3bb, 0x3bc,
	0x3bd, 0x3bf, 0x3c0, 0x3b8, 0x3c1, 0x3c3, 0x3c4, 0x3c5, 0x3d6, 0x3c9,
	0x3be, 0x3c8, 0x3b6, 0x7b, 0x7c, 0x7d, 0x223c, 0x7f, 0x80, 0x81, 0x82,
	0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
	0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
	0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0x3d2, 0x2032, 0x2264, 0x2044, 0x201e,
	0x192, 0x2663, 0x2666, 0x2665, 0x2660, 0x2194, 0x2190, 0x2191, 0x2192,
	0x2193, 0xb0, 0xb1, 0x2033, 0x2265, 0xd7, 0x221d, 0x2202, 0x2022, 0xf7,
	0x2260, 0x2261, 0x2248, 0x2026, 0xf8e6, 0xf8e7, 0x21b5, 0x2135, 0x2111,
	0x211c, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229, 0x222a, 0x2283, 0x2287,
	0x2284, 0x2282, 0x2286, 0x2208, 0x2209, 0x2220, 0x2207, 0xae, 0xa9, 0x2122,
	0x220f, 0x221a, 0x22c5, 0xac, 0x2227, 0x2228, 0x21d4, 0x21d0, 0x21d1,
	0x21d2, 0x21d3, 0x22c4, 0x2329, 0xf8e8, 0xf8e9, 0xf8ea, 0x2211, 0xf8eb,
	0xf8ec, 0xf8ed, 0xf8ee, 0xf8ef, 0xf8f0, 0xf8f1, 0xf8f2, 0xf8f3, 0xf8f4,
	0xf8ff, 0x232a, 0x222b, 0x2320, 0xf8f5, 0x2321, 0xf8f6, 0xf8f7, 0xf8f8,
	0xf8f9, 0xf8fa, 0xf8fb, 0xf8fc, 0xf8fd, 0xf8fe, 0x2c7};
	guint16  *data = {0};
	char* utxt;
	guint i;

	data = malloc (strlen (txt) * 2);
	for (i = 0; i < strlen (txt); i++)
		data[i] = utf[(guint8) txt[i]];
	utxt =  g_utf16_to_utf8 (data, strlen (txt), NULL, NULL, NULL);
	return utxt;
}

GHashTable*
init_esc (void)
{
	GHashTable	*escrecords;
	gint i =0;
	const char* escarray[60] = {
		"NEWFRAME", "ABORTDOC", "NEXTBAND", "SETCOLORTABLE", "GETCOLORTABLE",
		"FLUSHOUT", "DRAFTMODE", "QUERYESCSUPPORT", "SETABORTPROC", "STARTDOC",
		"ENDDOC", "GETPHYSPAGESIZE", "GETPRINTINGOFFSET", "GETSCALINGFACTOR",
		"META_ESCAPE_ENHANCED_METAFILE", "SETPENWIDTH", "SETCOPYCOUNT",
		"SETPAPERSOURCE", "PASSTHROUGH", "GETTECHNOLOGY", "SETLINECAP", "SETLINEJOIN",
		"SETMITERLIMIT", "BANDINFO", "DRAWPATTERNRECT","GETVECTORPENSIZE",
		"GETVECTORBRUSHSIZE", "ENABLEDUPLEX", "GETSETPAPERBINS", "GETSETPRINTORIENT",
		"ENUMPAPERBINS", "SETDIBSCALING", "EPSPRINTING", "ENUMPAPERMETRICS",
		"GETSETPAPERMETRICS", "POSTSCRIPT_DATA", "POSTSCRIPT_IGNORE", "GETDEVICEUNITS",
		"GETEXTENDEDTEXTMETRICS", "GETPAIRKERNTABLE", "EXTTEXTOUT", "GETFACENAME",
		"DOWNLOADFACE", "METAFILE_DRIVER", "QUERYDIBSUPPORT", "BEGIN_PATH",
		"CLIP_TO_PATH", "END_PATH", "OPEN_CHANNEL", "DOWNLOADHEADER", "CLOSE_CHANNEL",
		"POSTSCRIPT_PASSTHROUGH", "ENCAPSULATED_POSTSCRIPT", "POSTSCRIPT_IDENTIFY",
		"POSTSCRIPT_INJECTION", "CHECKJPEGFORMAT", "CHECKPNGFORMAT",
		"GET_PS_FEATURESETTING", "MXDC_ESCAPE", "SPCLPASSTHROUGH2"};
	const int escid[60] = {
		0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A,
		0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014,
		0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E,
		0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0025, 0x0026, 0x002A, 0x0100, 0x0102,
		0x0200, 0x0201, 0x0202, 0x0801, 0x0C01, 0x1000, 0x1001, 0x1002, 0x100E, 0x100F,
		0x1010, 0x1013, 0x1014, 0x1015, 0x1016, 0x1017, 0x1018, 0x1019, 0x101A, 0x11D8};
		
	escrecords = g_hash_table_new (g_direct_hash, g_direct_equal);
	for (i = 0; i < 60; i++)
		g_hash_table_insert (escrecords, GINT_TO_POINTER (escid[i]), GINT_TO_POINTER (escarray[i]));
	return escrecords;
}

static void
on_quit (GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_destroy (user_data);
	gtk_main_quit ();
}

static void
my_test (GocCanvas *canvas, GdkEventButton *event, G_GNUC_UNUSED gpointer data)
{
	double ppu=1.;
	
	ppu = goc_canvas_get_pixels_per_unit (canvas);
	if (1 == event->button) {
		ppu = ppu / 1.5;
		goc_canvas_set_pixels_per_unit (canvas, ppu);
	} else {
		ppu = ppu * 1.5;
		goc_canvas_set_pixels_per_unit (canvas, ppu);
	}
}

static void
open_file (char const *filename, GtkWidget *nbook)
{
	GsfInput *input;
	GError *error = NULL;
	char *display_name;
	GocCanvas *canvas;
	GODoc *doc;
	GtkWidget *window, *label;

	g_print ("%s\n", filename);

	input = gsf_input_stdio_new (filename, &error);
	if (error) {
		display_name = g_filename_display_name (filename);
		g_printerr ("%s: Failed to open %s: %s\n",
				g_get_prgname (),
				display_name,
				error->message);
		g_free (display_name);
		return;
	}

	if (NULL == input) {
		g_print ("Error: can't open file.\n");
		return;
	}

	canvas =g_object_new (GOC_TYPE_CANVAS, NULL);
	doc = g_object_new (GO_TYPE_DOC, NULL);
	goc_canvas_set_document (canvas, doc); 
	g_signal_connect_swapped (canvas, "button-press-event", G_CALLBACK (my_test), canvas);

	window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (window), GTK_WIDGET (canvas));
	if (g_strrstr (filename, "/") != NULL)
		label = gtk_label_new (g_strrstr (filename, "/") + 1);
	else
		label = gtk_label_new (filename);
	gtk_notebook_append_page (GTK_NOTEBOOK (nbook), window, label);
	gtk_widget_show_all (GTK_WIDGET (window));

	parse (input, canvas);
	g_object_unref (input);
}

static void
on_close (GtkMenuItem *menuitem, GtkWidget *nbook)
{
	gtk_widget_destroy (gtk_notebook_get_nth_page (GTK_NOTEBOOK (nbook), gtk_notebook_get_current_page (GTK_NOTEBOOK (nbook))));
}

static void
on_open (GtkMenuItem *menuitem, GtkWidget *nbook)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new ("Open File",
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		open_file (filename, nbook);
		g_free (filename);
	}

	gtk_widget_destroy (dialog);
}

int
main (int argc, char *argv[])
{
	GtkWidget *window, *file_menu, *menu_bar, *file_item, *open_item, *close_item, *quit_item, *box, *nbook;

	gtk_init (&argc, &argv);
	gsf_init ();
	libgoffice_init ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_resize (GTK_WINDOW (window), 300, 340);
	gtk_window_set_title (GTK_WINDOW (window), "MF demo");
	g_signal_connect (window, "destroy", gtk_main_quit, NULL);

	box = gtk_vbox_new (FALSE, 0);
	menu_bar = gtk_menu_bar_new ();
	nbook = gtk_notebook_new ();

	file_menu = gtk_menu_new();
	file_item = gtk_menu_item_new_with_label ("File");
	open_item = gtk_menu_item_new_with_label ("Open");
	close_item = gtk_menu_item_new_with_label ("Close");
	quit_item = gtk_menu_item_new_with_label ("Quit");
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), open_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), close_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), quit_item);

	g_signal_connect (quit_item, "activate", G_CALLBACK (on_quit), window);
	g_signal_connect (close_item, "activate", G_CALLBACK (on_close), nbook);
	g_signal_connect (open_item, "activate", G_CALLBACK (on_open), nbook);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item), file_menu);

	gtk_menu_bar_append(GTK_MENU_BAR (menu_bar), file_item );

	gtk_box_pack_start (GTK_BOX (box), menu_bar, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (box), nbook, TRUE, TRUE, 2);
	gtk_container_add (GTK_CONTAINER (window), box);
	gtk_widget_show_all (GTK_WIDGET (window));

	if (argc > 1)
		open_file (*(argv+1), nbook);

	gtk_main ();

	libgoffice_shutdown ();
	gsf_shutdown ();
	return 0;
}
