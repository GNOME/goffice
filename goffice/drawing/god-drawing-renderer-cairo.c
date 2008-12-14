/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * god-drawing-renderer-cairo.c: MS Office Graphic Object support
 *
 * Copyright (C) 2000-2002
 *	Jody Goldberg (jody@gnome.org)
 *	Michael Meeks (mmeeks@gnu.org)
 *      Christopher James Lahey <clahey@ximian.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include <goffice/drawing/god-drawing-renderer-cairo.h>
#include <string.h>
#include <math.h>

#include <cairo.h>
#include <pango/pangocairo.h>
#include <gdk/gdk.h>

static GdkPixbuf *
get_pixbuf (GodDrawing *drawing,
	    int which_pic)
{
	GodDrawingGroup *drawing_group;
	GdkPixbuf *ret_val = NULL;

	if (which_pic < 0)
		return NULL;

	drawing_group = god_drawing_get_drawing_group (drawing);
	if (drawing_group) {
		GodImageStore *image_store = god_drawing_group_get_image_store (drawing_group);
		if (which_pic < god_image_store_get_image_count (image_store)) {
			GodImage *image = god_image_store_get_image (image_store, which_pic);
			ret_val = god_image_get_pixbuf (image);
			g_object_unref (image);
		}
		g_object_unref (image_store);
		g_object_unref (drawing_group);
	}
	return ret_val;
}

typedef struct {
	GodDrawing *drawing;
	cairo_t *context;
	GORect *rect;
	long long y_ofs;
	const GodDefaultAttributes *default_attributes;
} DrawTextContext;

static void
draw_text (GodTextModel *text,
	   GodTextModelParagraph *paragraph,
	   gpointer user_data)
{
	int height;
	PangoLayout *layout;
	DrawTextContext *draw_context = user_data;
	double space_before = 0;
	double space_after = 0;
	double indent = 0;
	const GList *iterator;
	PangoAttrList *attributes;
	GodParagraphAlignment alignment = GOD_PARAGRAPH_ALIGNMENT_LEFT;
	const GodParagraphAttributes *default_para_attributes;
	gunichar bullet_character = 0;
	double bullet_indent = 0;
	double bullet_size = 0;
	char *bullet_family = NULL;
	gboolean bullet_on = FALSE;
	PangoFontDescription *bullet_desc;
	PangoAttrIterator *attr_iterator;
	cairo_t *context = draw_context->context;

	if (draw_context->default_attributes) {
		default_para_attributes = god_default_attributes_get_paragraph_attributes_for_indent ((GodDefaultAttributes *)draw_context->default_attributes, paragraph->indent);
		if (default_para_attributes) {
			g_object_get ((GodParagraphAttributes *) default_para_attributes,
				      "space_before", &space_before,
				      "space_after", &space_after,
				      "indent", &indent,
				      "alignment", &alignment,
				      "bullet_character", &bullet_character,
				      "bullet_indent", &bullet_indent,
				      "bullet_size", &bullet_size,
				      "bullet_family", &bullet_family,
				      "bullet_on", &bullet_on,
				      NULL);
		}
	}
	if (paragraph->para_attributes) {
		GodParagraphAttributesFlags flags;
		g_object_get (paragraph->para_attributes,
			      "flags", &flags,
			      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_SPACE_BEFORE)
			g_object_get (paragraph->para_attributes,
				      "space_before", &space_before,
				      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_SPACE_AFTER)
			g_object_get (paragraph->para_attributes,
				      "space_after", &space_after,
				      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_INDENT)
			g_object_get (paragraph->para_attributes,
				      "indent", &indent,
				      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_ALIGNMENT)
			g_object_get (paragraph->para_attributes,
				      "alignment", &alignment,
				      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_BULLET_CHARACTER)
			g_object_get (paragraph->para_attributes,
				      "bullet_character", &bullet_character,
				      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_BULLET_INDENT)
			g_object_get (paragraph->para_attributes,
				      "bullet_indent", &bullet_indent,
				      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_BULLET_SIZE)
			g_object_get (paragraph->para_attributes,
				      "bullet_size", &bullet_size,
				      NULL);
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_BULLET_FAMILY) {
			g_free (bullet_family);
			bullet_family = NULL;
			g_object_get (paragraph->para_attributes,
				      "bullet_family", &bullet_family,
				      NULL);
		}
		if (flags & GOD_PARAGRAPH_ATTRIBUTES_FLAGS_BULLET_ON) {
			g_object_get (paragraph->para_attributes,
				      "bullet_on", &bullet_on,
				      NULL);
		}
	}
	draw_context->y_ofs += space_before;

	layout = pango_cairo_create_layout (context);
	pango_layout_set_alignment (layout, alignment == GOD_PARAGRAPH_ALIGNMENT_JUSTIFY ? PANGO_ALIGN_LEFT : alignment);
	if (draw_context->rect->right - draw_context->rect->left != 0) {
		pango_layout_set_width (layout, GO_UN_TO_PT ((draw_context->rect->right - draw_context->rect->left) * PANGO_SCALE));
	}
	if (strchr (paragraph->text, 0xb)) {
		int i;
		char *paragraph_text = g_strdup (paragraph->text);
		for (i = 0; paragraph_text[i]; i++) {
			if (paragraph_text[i] == 0xb)
				paragraph_text[i] = '\r';
		}
		pango_layout_set_text (layout, paragraph_text, -1);
		g_free (paragraph_text);
	} else {
		pango_layout_set_text (layout, paragraph->text, -1);
	}
	pango_layout_set_auto_dir (layout, FALSE);
	if (paragraph->char_attributes)
		attributes = pango_attr_list_copy (paragraph->char_attributes);
	else
		attributes = pango_attr_list_new ();
	if (draw_context->default_attributes) {
		iterator = god_default_attributes_get_pango_attributes_for_indent ((GodDefaultAttributes *)draw_context->default_attributes, paragraph->indent);
		for (; iterator; iterator = iterator->next) {
			PangoAttribute *attr = pango_attribute_copy (iterator->data);
			attr->start_index = 0;
			attr->end_index = -1;
			pango_attr_list_insert_before (attributes, attr);
		}
	}
	pango_layout_set_attributes (layout, attributes);
	attr_iterator = pango_attr_list_get_iterator (attributes);
	bullet_desc = pango_font_description_new ();
	pango_attr_iterator_get_font (attr_iterator, bullet_desc, NULL, NULL);
	pango_attr_iterator_destroy (attr_iterator);
	pango_attr_list_unref (attributes);

	cairo_save (context);
	cairo_translate (context,
			 GO_UN_TO_PT ((double)(indent)),
			 GO_UN_TO_PT ((double)(draw_context->y_ofs)));
	pango_cairo_show_layout (context, layout);
	cairo_restore (context);

	pango_layout_get_size (layout, NULL, &height);

	g_object_unref (layout);
	layout = NULL;

	if (bullet_character != 0 &&
	    bullet_character != 0xe011 &&
	    bullet_size != 0 &&
	    bullet_family != NULL &&
	    bullet_on) {
		char utf8[7];
		int length;
		layout = pango_cairo_create_layout (context);
		pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
		length = g_unichar_to_utf8 (bullet_character, utf8);
		pango_layout_set_text (layout, utf8, length);
		pango_layout_set_auto_dir (layout, FALSE);
		g_print ("bullet_size = %f\n", bullet_size);
#if 0
		pango_font_description_set_size (bullet_desc, 
						 bullet_size * 100.0 * PANGO_SCALE * 72.0 / 96.0);
#endif
		pango_font_description_set_size (bullet_desc, pango_font_description_get_size (bullet_desc) * sqrt (bullet_size));
		pango_font_description_set_family (bullet_desc, bullet_family);
		pango_layout_set_font_description (layout, bullet_desc);

		cairo_save (context);
		cairo_translate (context,
				 GO_UN_TO_PT ((double)bullet_indent),
				 GO_UN_TO_PT ((double)draw_context->y_ofs));
		pango_cairo_show_layout (context, layout);
		cairo_restore (context);
		
		pango_font_description_free (bullet_desc);
		g_object_unref (layout);
		layout = NULL;
	}

	draw_context->y_ofs += GO_PT_TO_UN ((GODistance)height) / PANGO_SCALE;
	draw_context->y_ofs += space_after;
}

static void
god_drawing_renderer_cairo_render_shape (GodDrawing *drawing,
					 cairo_t    *context,
					 GodShape   *shape)
{
	GodAnchor *anchor;
	GORect anchor_rect;

	anchor = god_shape_get_anchor (shape);
	if (anchor) {
		god_anchor_get_rect (anchor, &anchor_rect);
	} else {
		anchor_rect.left = 0;
		anchor_rect.right = 0;
		anchor_rect.top = 0;
		anchor_rect.bottom = 0;
	}

	{
		GodPropertyTable *prop_table;
		gboolean filled;
		GodFillType fill_type;
		GodTextModel *text_model;
		DrawTextContext *draw_context;

		prop_table = god_shape_get_prop_table (shape);
		filled = god_property_table_get_flag (prop_table,
						      GOD_PROPERTY_FILLED,
						      TRUE);
		fill_type = god_property_table_get_int (prop_table,
							GOD_PROPERTY_FILL_TYPE,
							GOD_FILL_TYPE_SOLID);
		if (filled && fill_type == GOD_FILL_TYPE_PICTURE && anchor) {
			int which_pic = god_property_table_get_int (prop_table,
								    GOD_PROPERTY_BLIP_ID,
								    -1);
			GdkPixbuf *pixbuf = get_pixbuf (drawing, which_pic);
			cairo_pattern_t *pattern;

			if (pixbuf) {
				cairo_save (context);

				cairo_translate (context,
						 GO_UN_TO_PT ((double) anchor_rect.left),
						 GO_UN_TO_PT ((double) anchor_rect.top));
				cairo_scale (context,
					     GO_UN_TO_PT ((double)(anchor_rect.right - anchor_rect.left)),
					     GO_UN_TO_PT ((double)(anchor_rect.bottom - anchor_rect.top)));

				cairo_rectangle (context, 0, 0, 1, 1);
				cairo_clip (context);

				cairo_scale (context,
					     1.0 / gdk_pixbuf_get_width (pixbuf),
					     1.0 / gdk_pixbuf_get_height (pixbuf));

				gdk_cairo_set_source_pixbuf (context, pixbuf, 0, 0);

				pattern = cairo_get_source (context);
				cairo_pattern_set_filter (pattern, CAIRO_FILTER_BILINEAR);

				cairo_paint (context);

				cairo_restore (context);
			}
		}
		if (filled && fill_type == GOD_FILL_TYPE_SOLID) {
			guint32 colornum = god_property_table_get_uint (prop_table,
									GOD_PROPERTY_FILL_COLOR,
									0xffffff);

			cairo_save (context);

			cairo_set_source_rgb (context, ((colornum & 0xff0000) >> 16) / 256.0, ((colornum & 0xff00) >> 8) / 256.0, ((colornum & 0xff00)) / 256.0);

			if (anchor) {
				cairo_rectangle (context,
						 GO_UN_TO_PT ((double)(anchor_rect.left)),
						 GO_UN_TO_PT ((double)(anchor_rect.top)),
						 GO_UN_TO_PT ((double)(anchor_rect.right - anchor_rect.left)),
						 GO_UN_TO_PT ((double)(anchor_rect.bottom - anchor_rect.top)));
				cairo_clip (context);
			}

			cairo_paint (context);

			cairo_restore (context);
		}
		text_model = god_shape_get_text_model (shape);
		draw_context = g_new (DrawTextContext, 1);
		draw_context->drawing = drawing;
		draw_context->context = context;
		draw_context->rect = &anchor_rect;
		draw_context->y_ofs = 0;
		draw_context->default_attributes = god_text_model_get_default_attributes (text_model);
		cairo_save (context);
		cairo_translate (context,
				 GO_UN_TO_PT ((double)(anchor_rect.left)),
				 GO_UN_TO_PT ((double)(anchor_rect.top)));
		god_text_model_paragraph_foreach (text_model, draw_text, draw_context);
		cairo_restore (context);
		g_object_unref (prop_table);
	}

	{
		int i, child_count;
		GodShape *child;
		child_count = god_shape_get_child_count (shape);
		for (i = 0; i < child_count; i++) {
			child = god_shape_get_child (shape, i);
			god_drawing_renderer_cairo_render_shape (drawing, context, child);
		}
	}
}

void
god_drawing_renderer_cairo_render        (GodDrawing *drawing,
					  cairo_t    *context)
{
	GodShape *shape;

	shape = god_drawing_get_background (drawing);
	if (shape) {
		god_drawing_renderer_cairo_render_shape (drawing,
							 context,
							 shape);
		g_object_unref (shape);
	}

	shape = god_drawing_get_root_shape (drawing);
	if (shape) {
		god_drawing_renderer_cairo_render_shape (drawing,
							 context,
							 shape);
		g_object_unref (shape);
	}
}
