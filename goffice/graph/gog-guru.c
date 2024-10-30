/*
 * go-graph-guru.c:  The Graph guru
 *
 * Copyright (C) 2000-2004 Jody Goldberg (jody@gnome.org)
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
#include <goffice/goffice-priv.h>

#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#include <libxml/parser.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>

typedef struct _GraphGuruState		GraphGuruState;
typedef struct _GraphGuruTypeSelector	GraphGuruTypeSelector;

struct _GraphGuruState {
	GogGraph     *graph;
	GogChart     *chart;
	GogPlot	     *plot;
	GogGraphView *graph_view;

	GOCmdContext	 *cc;
	GogDataAllocator *dalloc;
	GClosure         *register_closure;

	/* GUI accessors */
	GtkBuilder    *gui;
	GtkWidget   *dialog;
	GtkWidget   *button_cancel;
	GtkWidget   *button_navigate;
	GtkWidget   *button_ok;
	GtkWidget   *child_button;
	GtkNotebook *steps;
	GtkWidget   *delete_button;

	GocItem	  *sample_graph_item;
	GocItem	  *sample_graph_frame;
	GocItem	  *sample_graph_shadow;

	GtkContainer  	  *prop_container;
	GtkTreeSelection  *prop_selection;
	GtkTreeView	  *prop_view;
	GtkTreeStore	  *prop_model;
	GtkTreeIter        prop_iter;
	GogObject	  *prop_object;

	GraphGuruTypeSelector *type_selector;

	struct {
		GtkWidget *inc_button, *dec_button, *first_button, *last_button;
	} prec;

	/* internal state */
	int current_page, initial_page;
	gboolean valid;
	gboolean updating;
	gboolean fmt_page_initialized;
	gboolean editing;

	/* hackish reuse of State as a closure */
	GogObject *search_target, *new_child;

	gulong selection_changed_handler;
};

struct _GraphGuruTypeSelector {
	GtkBuilder    	*gui;
	GtkWidget	*canvas;
	GtkWidget	*sample_canvas;
	GtkTreeView	*list_view;
	GtkListStore	*model;
	GocItem *selector;

	GocItem *sample_graph_item;

	GraphGuruState *state;

	GocGroup *graph_group;

	xmlNode const *plots;
	GogPlotType	*current_type;
	GocGroup const *current_family_item;
	GocItem const  *current_minor_item;

	int max_priority_so_far;
};

enum {
	PLOT_FAMILY_TYPE_IMAGE,
	PLOT_FAMILY_TYPE_NAME,
	PLOT_FAMILY_TYPE_CANVAS_GROUP,
	PLOT_FAMILY_NUM_COLUMNS
};
enum {
	PLOT_ATTR_NAME,
	PLOT_ATTR_OBJECT,
	PLOT_ATTR_NUM_COLUMNS
};

#define MINOR_PIXMAP_WIDTH	64
#define MINOR_PIXMAP_HEIGHT	60
#define BORDER	5

#define PLOT_TYPE_KEY		"plot_type"
#define TREND_LINE_TYPE_KEY	"trend_line_type"
#define FIRST_MINOR_TYPE	"first_minor_type"
#define ROLE_KEY		"role"
#define STATE_KEY		"plot_type"
#define ROWS_KEY		"rows-key"
#define PIXBUFS_LOADED_KEY      "pixbufs-loaded"

static void
get_pos (int col, int row, double *x, double *y)
{
	*x = (col-1) * (MINOR_PIXMAP_WIDTH + BORDER) + BORDER;
	*y = (row-1) * (MINOR_PIXMAP_HEIGHT + BORDER) + BORDER;
}

static void
cb_typesel_sample_plot_resize (GocCanvas *canvas,
			       GtkAllocation *alloc, GraphGuruTypeSelector *typesel)
{
	if (typesel->sample_graph_item != NULL)
		goc_item_set (typesel->sample_graph_item,
			"width", (double)alloc->width,
			"height", (double)alloc->height,
			NULL);
}


/*
 * graph_typeselect_minor:
 *
 * @typesel:
 * @item: A CanvasItem in the selector.
 *
 * Moves the typesel::selector overlay above the @item.
 * Assumes that the item is visible
 */
static void
graph_typeselect_minor (GraphGuruTypeSelector *typesel, GocItem *item)
{
	GraphGuruState *s = typesel->state;
	GogPlotType *type;
	double x1, y1, x2, y2;
	gboolean enable_next_button;
	GogPlot *plot;

	if (typesel->current_minor_item == item || typesel->sample_graph_item == item)
		return;

	type = g_object_get_data (G_OBJECT (item), PLOT_TYPE_KEY);

	g_return_if_fail (type != NULL);

	enable_next_button = (s->plot == NULL);

	plot = gog_plot_new_by_type (type);

	g_return_if_fail (plot != NULL);

	/* That that modules have been loaded we can get the pixbufs.  */
	if (!g_object_get_data (G_OBJECT (item->parent), PIXBUFS_LOADED_KEY)) {
		GPtrArray *children = goc_group_get_children (item->parent);
		unsigned ui;
		for (ui = 0; ui < children->len; ui++) {
			GocItem *i = g_ptr_array_index (children, ui);
			GogPlotType *t = i
				? g_object_get_data (G_OBJECT (i), PLOT_TYPE_KEY)
				: NULL;
			GdkPixbuf *image = t
				? go_gdk_pixbuf_get_from_cache (t->sample_image_file)
				: NULL;
			if (image) {
				double h = gdk_pixbuf_get_height (image);
				double w = gdk_pixbuf_get_width (image);
				if (w > MINOR_PIXMAP_WIDTH)
					w = MINOR_PIXMAP_WIDTH;
				if (h > MINOR_PIXMAP_HEIGHT)
					h = MINOR_PIXMAP_HEIGHT;
				goc_item_set (i,
					      "pixbuf", image,
					      "width", w,
					      "height", h,
					      NULL);
			}
		}
		g_ptr_array_unref (children);
		g_object_set_data (G_OBJECT (item->parent),
				   PIXBUFS_LOADED_KEY,
				   GINT_TO_POINTER (1));
	}

	typesel->current_type = type;
	typesel->current_minor_item = item;
	goc_item_get_bounds (item, &x1, &y1, &x2, &y2);
	goc_item_set (GOC_ITEM (typesel->selector),
		"x", x1-1., "y", y1-1.,
		"width", x2-x1+2., "height", y2-y1+2.,
		NULL);

	if (s->chart != NULL) {
		GogObject *obj = GOG_OBJECT (s->chart);
		gog_object_clear_parent (obj);
		g_object_unref (obj);
		s->chart = GOG_CHART (gog_object_add_by_name (
				GOG_OBJECT (s->graph), "Chart", NULL));
	}

	gog_object_add_by_name (GOG_OBJECT (s->chart),
		"Plot", GOG_OBJECT (s->plot = plot));
	gog_plot_guru_helper (plot);

	if (s->dalloc != NULL)
		gog_data_allocator_allocate (s->dalloc, s->plot);

	if (s->current_page == 0 && enable_next_button)
		gtk_widget_set_sensitive (s->button_navigate, TRUE);

	g_object_set_data (G_OBJECT (typesel->selector), PLOT_TYPE_KEY, (gpointer)type);
	if (typesel->sample_graph_item == NULL) {
		GtkAllocation size;
		typesel->sample_graph_item = goc_item_new (typesel->graph_group,
			GOC_TYPE_GRAPH,
			"graph", typesel->state->graph,
			NULL);
		gtk_widget_get_allocation (GTK_WIDGET (typesel->sample_canvas), &size);
		cb_typesel_sample_plot_resize (GOC_CANVAS (typesel->sample_canvas),
					       &size, typesel);
	}
}

static gboolean
graph_typeselect_minor_x_y (GraphGuruTypeSelector *typesel,
			    double x, double y)
{
	GocItem *item = goc_canvas_get_item_at (
		GOC_CANVAS (typesel->canvas), x, y);

	if (item != NULL) {
		if(item != typesel->selector)
			graph_typeselect_minor (typesel, item);
		return TRUE;
	}

	return FALSE;
}

static gboolean
cb_key_press_event (G_GNUC_UNUSED GtkWidget *wrapper,
		    GdkEventKey *event,
		    GraphGuruTypeSelector *typesel)
{
	int col, row;
	double x, y;
	GogPlotType const *type = g_object_get_data (
		G_OBJECT (typesel->current_minor_item), PLOT_TYPE_KEY);

	g_return_val_if_fail (type != NULL, FALSE);

	col = type->col;
	row = type->row;

	switch (event->keyval) {
	case GDK_KEY_KP_Left:
	case GDK_KEY_Left:
		--col;
		break;

	case GDK_KEY_KP_Up:
	case GDK_KEY_Up:
		--row;
		break;

	case GDK_KEY_KP_Right:
	case GDK_KEY_Right:
		++col;
		break;

	case GDK_KEY_KP_Down:
	case GDK_KEY_Down:
		++row;
		break;

	default:
		return FALSE;
	}

	get_pos (col, row, &x, &y);
	graph_typeselect_minor_x_y (typesel, x, y);

	return TRUE;
}

static gint
cb_button_press_event (GtkWidget *widget, GdkEventButton *event,
		       GraphGuruTypeSelector *typesel)
{
	if (event->button == 1)
		graph_typeselect_minor_x_y (typesel, event->x, event->y);

	return FALSE;
}

static void
cb_selection_changed (GraphGuruTypeSelector *typesel)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (typesel->list_view);
	GtkTreeIter  iter;
	GocItem *item;
	GocGroup *group;
	int rows;

	if (typesel->current_family_item != NULL)
		goc_item_hide (GOC_ITEM (typesel->current_family_item));
	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;
	gtk_tree_model_get (GTK_TREE_MODEL (typesel->model), &iter,
		PLOT_FAMILY_TYPE_CANVAS_GROUP, &group,
		-1);

	goc_item_show (GOC_ITEM (group));
	typesel->current_family_item = group;

	goc_item_hide (GOC_ITEM (typesel->selector));
	item = g_object_get_data (G_OBJECT (group), FIRST_MINOR_TYPE);
	rows = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (group), ROWS_KEY));
	if (item != NULL)
		graph_typeselect_minor (typesel, item);
	goc_item_show (GOC_ITEM (typesel->selector));
	gtk_widget_set_size_request (typesel->canvas,
		MINOR_PIXMAP_WIDTH*3 + BORDER*5,
		(MINOR_PIXMAP_HEIGHT + BORDER) * rows + BORDER);
}

typedef struct {
	GraphGuruTypeSelector	*typesel;
	GocGroup		*group;
	GocItem		*current_item;
	GogPlotType 		*current_type;
	int col, row;
	int max_row;
} type_list_closure;

typedef GocPixbuf GogGuruPixbuf;
typedef GocPixbufClass GogGuruPixbufClass;

static GType gog_guru_pixbuf_get_type (void);

static gboolean
gog_guru_item_enter_notify (GocItem *item, double x, double y)
{
	GogPlotType *type = (GogPlotType *) g_object_get_data (G_OBJECT (item), PLOT_TYPE_KEY);
	if (type && type->description)
		gtk_widget_set_tooltip_text (GTK_WIDGET (item->canvas), _(type->description));
	return TRUE;
}

static gboolean
gog_guru_item_leave_notify (GocItem *item, double x, double y)
{
	gtk_widget_set_tooltip_text (GTK_WIDGET (item->canvas), NULL);
	return TRUE;
}

static void
gog_guru_item_class_init (GocItemClass *klass)
{
	klass->enter_notify = gog_guru_item_enter_notify;
	klass->leave_notify = gog_guru_item_leave_notify;
}

GSF_CLASS (GogGuruPixbuf, gog_guru_pixbuf,
		  gog_guru_item_class_init, NULL,
		  GOC_TYPE_PIXBUF)

typedef GocRectangle GogGuruSelector;
typedef GocRectangleClass GogGuruSelectorClass;

static GType gog_guru_selector_get_type (void);

GSF_CLASS (GogGuruSelector, gog_guru_selector,
		  gog_guru_item_class_init, NULL,
		  GOC_TYPE_RECTANGLE)

static void
cb_plot_types_init (char const *id, GogPlotType *type,
		    type_list_closure *closure)
{
	double x1, y1;
	GocItem *item;
	int col, row;
	double h = MINOR_PIXMAP_HEIGHT;
	double w = MINOR_PIXMAP_WIDTH;

	col = type->col;
	row = type->row;
	get_pos (col, row, &x1, &y1);

	item = goc_item_new (closure->group,
		gog_guru_pixbuf_get_type (),
		"x",	 x1,	"y",	  y1,
		"width", w,	"height", h,
		/* no image */
		NULL);
	g_object_set_data (G_OBJECT (item), PLOT_TYPE_KEY, (gpointer)type);

	if (closure->current_type == NULL ||
	    closure->row > row ||
	    (closure->row == row && closure->col > col)) {
		closure->current_type = type;
		closure->current_item = item;
		closure->col = col;
		closure->row = row;
	}
	if (row > closure->max_row)
		closure->max_row = row;
}

static void
cb_plot_families_init (char const *id, GogPlotFamily *family,
		       GraphGuruTypeSelector *typesel)
{
	GocGroup		*group;
	GtkTreeIter		 iter;
	type_list_closure	 closure;

	if (g_hash_table_size (family->types) <= 0)
		return;

	/* Define a canvas group for all the minor types */
	group = GOC_GROUP (goc_item_new (
		goc_canvas_get_root (GOC_CANVAS (typesel->canvas)),
		goc_group_get_type (),
		"x", 0.0,
		"y", 0.0,
		NULL));
	goc_item_hide (GOC_ITEM (group));

	gtk_list_store_append (typesel->model, &iter);
	gtk_list_store_set (typesel->model, &iter,
		PLOT_FAMILY_TYPE_IMAGE,		go_gdk_pixbuf_get_from_cache (family->sample_image_file),
		PLOT_FAMILY_TYPE_NAME,		_(family->name),
		PLOT_FAMILY_TYPE_CANVAS_GROUP,	group,
		-1);

	if (typesel->max_priority_so_far < family->priority) {
		typesel->max_priority_so_far = family->priority;
		gtk_tree_selection_select_iter (
			gtk_tree_view_get_selection (typesel->list_view), &iter);
	}

	closure.typesel = typesel;
	closure.group	= group;
	closure.current_type = NULL;
	closure.current_item = NULL;
	closure.max_row = 2;

	/* Init the list and the canvas group for each family */
	g_hash_table_foreach (family->types,
		(GHFunc) cb_plot_types_init, &closure);
	g_object_set_data (G_OBJECT (group), FIRST_MINOR_TYPE,
		closure.current_item);
	g_object_set_data (G_OBJECT (group), ROWS_KEY,
		GUINT_TO_POINTER (closure.max_row));
}

static void
graph_guru_state_destroy (GraphGuruState *state)
{
	g_return_if_fail (state != NULL);

	if (state->graph_view) {
		g_object_unref (state->graph_view);
		state->graph_view = NULL;
	}

	if (state->graph != NULL) {
		g_object_unref (state->graph);
		state->graph = NULL;
	}

	if (state->gui != NULL) {
		g_object_unref (state->gui);
		state->gui = NULL;
	}

	if (state->register_closure != NULL) {
		g_closure_unref (state->register_closure);
		state->register_closure = NULL;
	}

	state->dialog = NULL;
	g_free (state);
}

static void populate_graph_item_list (GogObject *obj, GogObject *select,
				      GraphGuruState *s, GtkTreeIter *parent,
				      gboolean insert);

static void
cb_graph_guru_delete_item (GraphGuruState *s)
{
	if (s->prop_object != NULL) {
		GtkTreeIter iter;
		GogObject *obj = NULL;
		GtkTreeModel *model = GTK_TREE_MODEL (s->prop_model);

		/* we select the next or the previous object if they are of the same
		 * type than the deleted object, otherwise, we select the parent */
		/* search for next object */
		iter = s->prop_iter;
		if (gtk_tree_model_iter_next (model, &iter))
			gtk_tree_model_get (model, &iter, PLOT_ATTR_OBJECT, &obj, -1);
		if (obj == NULL || G_OBJECT_TYPE (obj) != G_OBJECT_TYPE (s->prop_object)) {
			/* search for previous object */
			obj = NULL;
			iter = s->prop_iter;
			if (gtk_tree_model_iter_previous (model, &iter))
				gtk_tree_model_get (model, &iter, PLOT_ATTR_OBJECT, &obj, -1);
			else
				obj = NULL;
			if (obj == NULL || G_OBJECT_TYPE (obj) != G_OBJECT_TYPE (s->prop_object)) {
				/* store parent iter */
				gtk_tree_model_iter_parent (model ,
					&iter, &s->prop_iter);
			}
		}
		obj = s->prop_object;
		gog_object_clear_parent (obj);
		g_object_unref (obj);
		/* then select the parent after we delete */
		gtk_tree_selection_select_iter (s->prop_selection, &iter);
	}
}

static void
update_prec_menu (GraphGuruState *s, gboolean inc_ok, gboolean dec_ok)
{
	gtk_widget_set_sensitive (s->prec.first_button,    inc_ok);
	gtk_widget_set_sensitive (s->prec.inc_button,	    inc_ok);
	gtk_widget_set_sensitive (s->prec.dec_button,	    dec_ok);
	gtk_widget_set_sensitive (s->prec.last_button,	    dec_ok);
}

static gboolean
cb_reordered_find (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		   GraphGuruState *s)
{
	GogObject *obj;
	gtk_tree_model_get (model, iter, PLOT_ATTR_OBJECT, &obj, -1);
	if (obj == s->search_target) {
		gtk_tree_store_move_after (s->prop_model, &s->prop_iter, iter);
		return TRUE;
	}

	return FALSE;
}
static void
reorder (GraphGuruState *s, gboolean inc, gboolean goto_max)
{
	gboolean inc_ok, dec_ok;
	GogObject *after;

	g_return_if_fail (s->search_target == NULL);

	after = gog_object_reorder (s->prop_object, inc, goto_max);
	if (after != NULL) {
		s->search_target = after;
		gtk_tree_model_foreach (GTK_TREE_MODEL (s->prop_model),
			(GtkTreeModelForeachFunc) cb_reordered_find, s);
		s->search_target = NULL;
	} else
		gtk_tree_store_move_after (s->prop_model, &s->prop_iter, NULL);

	gog_object_can_reorder (s->prop_object, &inc_ok, &dec_ok);
	update_prec_menu (s, inc_ok, dec_ok);
}
static void cb_prec_first (GraphGuruState *s) { reorder (s, TRUE,  TRUE); }
static void cb_prec_inc	  (GraphGuruState *s) { reorder (s, TRUE,  FALSE); }
static void cb_prec_dec   (GraphGuruState *s) { reorder (s, FALSE, FALSE); }
static void cb_prec_last  (GraphGuruState *s) { reorder (s, FALSE, TRUE); }

static void
cb_attr_tree_selection_change (GraphGuruState *s)
{
	gboolean delete_ok = FALSE;
	gboolean inc_ok = FALSE;
	gboolean dec_ok = FALSE;
	GtkTreeModel *model;
	GogObject  *obj = NULL;
	GtkWidget *w, *notebook;
	GtkTreePath *path;

	if (gtk_tree_selection_get_selected (s->prop_selection, &model, &s->prop_iter))
		gtk_tree_model_get (model, &s->prop_iter,
				    PLOT_ATTR_OBJECT, &obj,
				    -1);

	if (s->prop_object == obj)
		return;

	if (obj) {
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (s->prop_model), &s->prop_iter);
		gtk_tree_view_scroll_to_cell (s->prop_view, path, NULL, FALSE, 0, 0);
		gtk_tree_path_free (path);
	}

	/* remove the old prop page */
	s->prop_object = obj;
	w = gtk_bin_get_child (GTK_BIN (s->prop_container));
	if (w != NULL)
		gtk_container_remove (s->prop_container, w);

	if (s->prop_object != NULL) {
		gog_child_button_set_object (GOG_CHILD_BUTTON (s->child_button), s->prop_object);

		/* if we ever go back to the typeselector be sure to
		 * add the plot to the last selected chart */
		s->chart = (GogChart *)
			gog_object_get_parent_typed (obj, GOG_TYPE_CHART);
		s->plot = (GogPlot *)
			gog_object_get_parent_typed (obj, GOG_TYPE_PLOT);

		if (s->plot == NULL) {
			if (s->chart == NULL && s->graph != NULL) {
				GSList *charts = gog_object_get_children (GOG_OBJECT (s->graph),
					gog_object_find_role_by_name (GOG_OBJECT (s->graph), "Chart"));
				if (charts != NULL && charts->next == NULL)
					s->chart = charts->data;
				g_slist_free (charts);
			}
			if (s->chart != NULL) {
				GSList *plots = gog_object_get_children (GOG_OBJECT (s->chart),
					gog_object_find_role_by_name (GOG_OBJECT (s->chart), "Plot"));
				if (plots != NULL && plots->next == NULL)
					s->plot = plots->data;
				g_slist_free (plots);
			}
		}
		gtk_widget_set_sensitive (s->button_navigate, s->chart != NULL);

		delete_ok = gog_object_is_deletable (s->prop_object);
		gog_object_can_reorder (obj, &inc_ok, &dec_ok);

		/* create a prefs page for the graph obj */
		notebook = gog_object_get_editor (obj, s->dalloc, s->cc);
		gtk_widget_show (notebook);
		gtk_container_add (s->prop_container, notebook);

		/* set selection on graph view */
		gog_graph_view_set_selection (GOG_GRAPH_VIEW (s->graph_view), obj);
	}

	gtk_widget_set_sensitive (s->delete_button, delete_ok);
	update_prec_menu (s, inc_ok, dec_ok);
}

static gboolean
cb_find_renamed_item (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		      GraphGuruState *s)
{
	GogObject *obj;

	gtk_tree_model_get (model, iter, PLOT_ATTR_OBJECT, &obj, -1);
	if (obj == s->search_target) {
		s->search_target = NULL;
		gtk_tree_store_set (s->prop_model, iter,
			PLOT_ATTR_NAME, gog_object_get_name (obj),
			-1);
		return TRUE;
	}

	return FALSE;
}

static void
cb_obj_name_changed (GogObject *obj, GraphGuruState *s)
{
	s->search_target = obj;
	gtk_tree_model_foreach (GTK_TREE_MODEL (s->prop_model),
		(GtkTreeModelForeachFunc) cb_find_renamed_item, s);
}

static gboolean
cb_find_child_added (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		     GraphGuruState *s)
{
	GogObject *obj;

	gtk_tree_model_get (model, iter, PLOT_ATTR_OBJECT, &obj, -1);
	if (obj == s->search_target) {
		s->search_target = NULL;
		populate_graph_item_list (s->new_child, s->new_child, s, iter, TRUE);
		return TRUE;
	}

	return FALSE;
}

static void
cb_obj_child_added (GogObject *parent, GogObject *child, GraphGuruState *s)
{
	s->search_target = parent;
	s->new_child = child;
	gtk_tree_model_foreach (GTK_TREE_MODEL (s->prop_model),
		(GtkTreeModelForeachFunc) cb_find_child_added, s);
	s->new_child = NULL;
}

static gboolean
cb_find_child_removed (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
		       GraphGuruState *s)
{
	GogObject *obj;
	GtkWidget *w;

	gtk_tree_model_get (model, iter, PLOT_ATTR_OBJECT, &obj, -1);
	if (obj == s->search_target) {
		s->search_target = NULL;
		/* remove the tree element and the prop page */
		gtk_tree_store_remove (s->prop_model, iter);
		w = gtk_bin_get_child (GTK_BIN (s->prop_container));
		if (w != NULL)
			gtk_container_remove (s->prop_container, w);
		return TRUE;
	}

	return FALSE;
}
static void
cb_obj_child_removed (GogObject *parent, GogObject *child, GraphGuruState *s)
{
	s->search_target = child;
	gtk_tree_model_foreach (GTK_TREE_MODEL (s->prop_model),
		(GtkTreeModelForeachFunc) cb_find_child_removed, s);

	if (s->chart == (gpointer)child) {
		s->chart = NULL;
		s->plot = NULL;
		gtk_widget_set_sensitive (s->button_navigate, FALSE);
	} else if (s->plot == (gpointer)child)
		s->plot = NULL;
}

static void
populate_graph_item_list (GogObject *obj, GogObject *select, GraphGuruState *s,
			  GtkTreeIter *parent, gboolean insert)
{
	GSList *children, *ptr;
	GtkTreeIter iter;
	GtkTreePath *path;
	GClosure *closure;

	if (insert) {
		GogObject *gparent = gog_object_get_parent (obj);
		gint i = g_slist_index (gparent->children, obj);
		GtkTreeIter sibling;
		if (i > 0 && gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (s->prop_model),
													&sibling, parent, i-1))
				gtk_tree_store_insert_after (s->prop_model, &iter,
					parent, &sibling);
		else
			gtk_tree_store_prepend (s->prop_model, &iter, parent);
	} else
		gtk_tree_store_append (s->prop_model, &iter, parent);

	gtk_tree_store_set (s->prop_model, &iter,
		PLOT_ATTR_OBJECT,	obj,
		PLOT_ATTR_NAME, gog_object_get_name (obj),
		-1);

	/* remove the signal handlers when the guru goes away */
	closure = g_cclosure_new (G_CALLBACK (cb_obj_name_changed), s, NULL);
	g_object_watch_closure (G_OBJECT (s->prop_view), closure);
	g_signal_connect_closure (G_OBJECT (obj),
		"name-changed",
		closure, FALSE);
	closure = g_cclosure_new (G_CALLBACK (cb_obj_child_added), s, NULL);
	g_object_watch_closure (G_OBJECT (s->prop_view), closure);
	g_signal_connect_closure (G_OBJECT (obj),
		"child-added",
		closure, FALSE);
	closure = g_cclosure_new (G_CALLBACK (cb_obj_child_removed), s, NULL);
	g_object_watch_closure (G_OBJECT (s->prop_view), closure);
	g_signal_connect_closure (G_OBJECT (obj),
		"child-removed",
		closure, FALSE);

	children = gog_object_get_children (obj, NULL);
	for (ptr = children ; ptr != NULL ; ptr = ptr->next)
		populate_graph_item_list (ptr->data, select, s, &iter, FALSE);
	g_slist_free (children);

	/* ensure that new items are visible */
	path = gtk_tree_model_get_path (
		GTK_TREE_MODEL (s->prop_model), &iter);
	gtk_tree_view_expand_to_path (s->prop_view, path);
	gtk_tree_path_free (path);

	if (obj == select)
		/* select after expanding so that we do not lose selection due
		 * to visibility */
		gtk_tree_selection_select_iter (s->prop_selection, &iter);
}

static gint
cb_canvas_select_item (GocCanvas *canvas, GdkEvent *event,
		       GraphGuruState *s)
{
	double item_x, item_y;

	g_return_val_if_fail (GOC_IS_CANVAS (canvas), FALSE);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_MOTION_NOTIFY:
	case GDK_BUTTON_RELEASE:
		g_object_get (G_OBJECT(s->sample_graph_item), "x", &item_x, "y", &item_y, NULL);
		gog_graph_view_handle_event (s->graph_view, (GdkEvent *) event,
					     item_x * canvas->pixels_per_unit,
					     item_y * canvas->pixels_per_unit);
		return TRUE;
	default:
		return FALSE;
		break;
	}
}

static void
cb_sample_plot_resize (GocCanvas *canvas,
		       GtkAllocation *alloc, GraphGuruState *state)
{
	double aspect_ratio;
	double width, height, x, y;

	gog_graph_get_size (state->graph, &width, &height);
    aspect_ratio = width / height;

	if (alloc->width > alloc->height * aspect_ratio) {
		height = alloc->height;
		width = floor (height * aspect_ratio + .5);
		x = floor ((alloc->width - width) / 2.0 + .5);
		y = 0.0;
	} else {
		width = alloc->width;
		height = floor (width / aspect_ratio + .5);
		x = 0.0;
		y = floor ((alloc->height - height) / 2.0 + .5);
	}

	if (width > 2)
		width -= 2;
	else
		width = 0;
	if (height > 2)
		height -= 2;
	else
		height = 0;
	goc_item_set (state->sample_graph_item,
		"width", width,
		"height", height,
		"x", x,
		"y", y,
		NULL);
	if (width > .5)
		width -= 1.;
	if (height > .5)
		height -= 1.;
	goc_item_set (state->sample_graph_frame,
		"width", width,
		"height", height,
		"x", x,
		"y", y,
		NULL);
	if (width > .5)
		width -= 1.;
	if (height > .5)
		height -= 1.;
	goc_item_set (state->sample_graph_shadow,
		"width", width,
		"height", height,
		"x", x + 3,
		"y", y + 3,
		NULL);
}

static gboolean
cb_find_item (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
	      GraphGuruState *s)
{
	GogObject *obj;

	gtk_tree_model_get (model, iter, PLOT_ATTR_OBJECT, &obj, -1);
	if (obj == s->search_target) {
		s->search_target = NULL;
		gtk_tree_selection_select_iter (s->prop_selection, iter);
		return TRUE;
	}

	return FALSE;
}

static void
cb_graph_selection_changed (GogGraphView *view, GogObject *gobj,
			    GraphGuruState *state)
{
	state->search_target = gobj;
	gtk_tree_model_foreach (GTK_TREE_MODEL (state->prop_model),
				(GtkTreeModelForeachFunc) cb_find_item, state);
}

static void
graph_guru_init_format_page (GraphGuruState *s)
{
	GtkWidget *w, *canvas;
	GtkTreeViewColumn *column;
	GogRenderer *rend;
	GOStyle *style;
	GtkAllocation size;

	if (s->fmt_page_initialized)
		return;
	s->fmt_page_initialized = TRUE;

	w = go_gtk_builder_get_widget (s->gui, "menu_hbox");

	s->child_button = gog_child_button_new ();
	gtk_box_pack_start (GTK_BOX (w), s->child_button, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (w), s->child_button, 0);
	gtk_widget_show (s->child_button);

	s->delete_button     = go_gtk_builder_get_widget (s->gui, "delete");
	s->prec.inc_button   = go_gtk_builder_get_widget (s->gui, "inc_precedence");
	s->prec.dec_button   = go_gtk_builder_get_widget (s->gui, "dec_precedence");
	s->prec.first_button = go_gtk_builder_get_widget (s->gui, "first_precedence");
	s->prec.last_button  = go_gtk_builder_get_widget (s->gui, "last_precedence");

	g_signal_connect_swapped (G_OBJECT (s->delete_button),
		"clicked",
		G_CALLBACK (cb_graph_guru_delete_item), s);
	g_signal_connect_swapped (G_OBJECT (s->prec.first_button),
		"clicked",
		G_CALLBACK (cb_prec_first), s);
	g_signal_connect_swapped (G_OBJECT (s->prec.inc_button),
		"clicked",
		G_CALLBACK (cb_prec_inc), s);
	g_signal_connect_swapped (G_OBJECT (s->prec.dec_button),
		"clicked",
		G_CALLBACK (cb_prec_dec), s);
	g_signal_connect_swapped (G_OBJECT (s->prec.last_button),
		"clicked",
		G_CALLBACK (cb_prec_last), s);

	/* Load up the sample view and make it fill the entire canvas */
	w = go_gtk_builder_get_widget (s->gui, "sample-alignment");
	canvas = GTK_WIDGET (g_object_new (GOC_TYPE_CANVAS, NULL));
	gtk_container_add (GTK_CONTAINER (w), canvas);
	s->sample_graph_shadow = goc_item_new (goc_canvas_get_root (GOC_CANVAS (canvas)),
						      GOC_TYPE_RECTANGLE,
						      NULL);
	style= go_styled_object_get_style (GO_STYLED_OBJECT (s->sample_graph_shadow));
	style->line.width = 2;
	style->line.color = 0xa0a0a0ff;	/* grey */
	s->sample_graph_frame = goc_item_new (goc_canvas_get_root (GOC_CANVAS (canvas)),
						     GOC_TYPE_RECTANGLE,
						     NULL);
	style= go_styled_object_get_style (GO_STYLED_OBJECT (s->sample_graph_frame));
	style->line.width = 1;
	style->line.color = 0x707070ff;	/* grey */
	style->fill.pattern.back = 0xffffffff;	/* white */
	s->sample_graph_item = goc_item_new (goc_canvas_get_root (GOC_CANVAS (canvas)),
						    GOC_TYPE_GRAPH,
						    "graph", s->graph,
						    NULL);
	gtk_widget_add_events (canvas, GDK_POINTER_MOTION_HINT_MASK);
	gtk_widget_get_allocation (canvas, &size);
	cb_sample_plot_resize (GOC_CANVAS (canvas), &size, s);
	g_signal_connect (G_OBJECT (canvas),
			  "size_allocate",
			  G_CALLBACK (cb_sample_plot_resize), s);
	g_signal_connect_after (G_OBJECT (canvas),
				"event",
				G_CALLBACK (cb_canvas_select_item), s);
	gtk_widget_show (canvas);

	/* Connect to selection-changed signal of graph view */
	g_object_get (G_OBJECT (s->sample_graph_item), "renderer", &rend, NULL);
	g_object_get (G_OBJECT (rend), "view", &(s->graph_view), NULL);
	s->selection_changed_handler = g_signal_connect (G_OBJECT (s->graph_view), "selection-changed",
							 G_CALLBACK (cb_graph_selection_changed), s);
	g_object_unref (rend);

	w = go_gtk_builder_get_widget (s->gui, "prop_alignment");
	s->prop_container = GTK_CONTAINER (w);
	s->prop_model = gtk_tree_store_new (PLOT_ATTR_NUM_COLUMNS,
				    G_TYPE_STRING, G_TYPE_POINTER);
	s->prop_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model
				      (GTK_TREE_MODEL (s->prop_model)));
	g_object_unref (s->prop_model);
	s->prop_selection = gtk_tree_view_get_selection (s->prop_view);
	gtk_tree_selection_set_mode (s->prop_selection, GTK_SELECTION_BROWSE);
	g_signal_connect_swapped (s->prop_selection,
		"changed",
		G_CALLBACK (cb_attr_tree_selection_change), s);

	column = gtk_tree_view_column_new_with_attributes (_("Name"),
		gtk_cell_renderer_text_new (),
		"text", PLOT_ATTR_NAME, NULL);
	gtk_tree_view_append_column (s->prop_view, column);

	gtk_tree_view_set_headers_visible (s->prop_view, FALSE);

	gtk_tree_store_clear (s->prop_model);
	populate_graph_item_list (GOG_OBJECT (s->graph), GOG_OBJECT (s->graph),
				  s, NULL, FALSE);
	gtk_tree_view_expand_all (s->prop_view);

	w = go_gtk_builder_get_widget (s->gui, "attr_window");
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (s->prop_view));
	gtk_widget_show_all (w);

}

static void
graph_guru_set_page (GraphGuruState *s, int page)
{
	char *name;

	if (s->current_page == page)
		return;

	switch (page) {
	case 0: name = _("Step 1 of 2: Select Chart Type");
		gtk_widget_set_sensitive (s->button_navigate, s->plot != NULL);
		gtk_button_set_label (GTK_BUTTON (s->button_navigate),
				      GTK_STOCK_GO_FORWARD);
		break;

	case 1:
		if (s->initial_page == 0) {
			name = _("Step 2 of 2: Customize Chart");
			gtk_widget_set_sensitive (s->button_navigate, s->chart != NULL);
			gtk_button_set_label (GTK_BUTTON (s->button_navigate),
					      GTK_STOCK_GO_BACK);
		} else {
			name = _("Customize Chart");
			gtk_widget_hide	(s->button_navigate);
		}
		graph_guru_init_format_page (s);
		break;

	default:
		g_warning ("Invalid Chart Guru page");
		return;
	}

	s->current_page = page;
	gtk_notebook_set_current_page (s->steps, page - s->initial_page);
	gtk_window_set_title (GTK_WINDOW (s->dialog), name);
}

static void
cb_graph_guru_clicked (GtkWidget *button, GraphGuruState *s)
{
	if (s->dialog == NULL)
		return;

	if (button == s->button_navigate) {
		graph_guru_set_page (s, (s->current_page + 1) % 2);
		return;
	} else if (button == s->button_ok &&
		   s->register_closure != NULL && s->graph != NULL) {
		/* Invoking closure */
		GValue instance_and_params[2];
		gpointer data = s->register_closure->is_invalid ?
			NULL : s->register_closure->data;

		instance_and_params[0].g_type = 0;
		g_value_init (&instance_and_params[0], GOG_TYPE_GRAPH);
		g_value_set_instance (&instance_and_params[0], s->graph);

		instance_and_params[1].g_type = 0;
		g_value_init (&instance_and_params[1], G_TYPE_POINTER);
		g_value_set_pointer (&instance_and_params[1], data);

		g_closure_set_marshal (s->register_closure,
				       g_cclosure_marshal_VOID__POINTER);

		g_closure_invoke (s->register_closure,
				  NULL,
				  2,
				  instance_and_params,
				  NULL);
		g_value_unset (instance_and_params + 0);
	}
	gtk_widget_destroy (GTK_WIDGET (s->dialog));
}

static GtkWidget *
graph_guru_init_button (GraphGuruState *s, char const *widget_name)
{
	GtkWidget *button = go_gtk_builder_get_widget (s->gui, widget_name);
	g_signal_connect (G_OBJECT (button),
		"clicked",
		G_CALLBACK (cb_graph_guru_clicked), s);
	return button;
}

static GtkWidget *
graph_guru_init_ok_button (GraphGuruState *s)
{
	GtkButton *button = GTK_BUTTON (gtk_builder_get_object
				       (s->gui, "button_ok"));

	if (s->editing) {
		gtk_button_set_label (button, GTK_STOCK_APPLY);
		gtk_button_set_use_stock (button, TRUE);
	} else {
		gtk_button_set_use_stock (button, FALSE);
		gtk_button_set_use_underline (button, TRUE);
		gtk_button_set_label (button, _("_Insert"));
	}
	g_signal_connect (G_OBJECT (button),
		"clicked",
		G_CALLBACK (cb_graph_guru_clicked), s);
	return GTK_WIDGET (button);
}

static void
typesel_set_selection_color (GraphGuruTypeSelector *typesel)
{
	GtkWidget *w = gtk_entry_new ();
	GtkStyleContext *style_context = gtk_widget_get_style_context (w);
	GdkRGBA  rgba;
	GOColor    select_color;
	GOStyle   *style;

	gtk_style_context_get_background_color (style_context,
	                                        gtk_widget_has_focus (typesel->canvas)
						? GTK_STATE_FLAG_SELECTED
						: GTK_STATE_FLAG_ACTIVE,
						&rgba);
	if (rgba.alpha > 0.40)
				rgba.alpha = 0.40;
	select_color = GO_COLOR_FROM_GDK_RGBA (rgba);

	style = go_styled_object_get_style (GO_STYLED_OBJECT (typesel->selector));
	style->fill.pattern.back = select_color;
	goc_item_invalidate (typesel->selector);
	gtk_widget_destroy (w);
}

static GtkWidget *
graph_guru_type_selector_new (GraphGuruState *s)
{
	GtkTreeSelection *selection;
	GraphGuruTypeSelector *typesel;
	GtkWidget *selector;
	GtkBuilder *gui;
	GOStyle *style;

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-guru-type-selector.ui", GETTEXT_PACKAGE, s->cc);

	typesel = g_new0 (GraphGuruTypeSelector, 1);
	typesel->state = s;
	typesel->current_family_item = NULL;
	typesel->current_minor_item = NULL;
	typesel->current_type = NULL;
	typesel->sample_graph_item = NULL;
	typesel->max_priority_so_far = -1;
	s->type_selector = typesel;

	selector = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "type-selector")));

	/* List of family types */
	typesel->model = gtk_list_store_new (PLOT_FAMILY_NUM_COLUMNS,
					     GDK_TYPE_PIXBUF,
					     G_TYPE_STRING,
					     G_TYPE_POINTER);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (typesel->model),
		PLOT_FAMILY_TYPE_NAME, GTK_SORT_ASCENDING);

	typesel->list_view = GTK_TREE_VIEW (gtk_builder_get_object (gui, "type-treeview"));
	gtk_tree_view_set_model (typesel->list_view, GTK_TREE_MODEL (typesel->model));
	g_object_unref (typesel->model);
	gtk_tree_view_append_column (typesel->list_view,
		gtk_tree_view_column_new_with_attributes ("",
			gtk_cell_renderer_pixbuf_new (),
			"pixbuf", PLOT_FAMILY_TYPE_IMAGE,
			NULL));
	gtk_tree_view_append_column (typesel->list_view,
		gtk_tree_view_column_new_with_attributes (_("_Plot Type"),
			gtk_cell_renderer_text_new (),
			"text", PLOT_FAMILY_TYPE_NAME,
			NULL));

	gtk_label_set_mnemonic_widget (GTK_LABEL (gtk_builder_get_object (gui, "type_label")),
				       GTK_WIDGET (typesel->list_view));


	/* Setup an canvas to display the sample image & the sample plot. */
	typesel->canvas = GTK_WIDGET (g_object_new (GOC_TYPE_CANVAS, NULL));
	g_object_connect (typesel->canvas,
	    "signal_after::key_press_event", G_CALLBACK (cb_key_press_event), typesel,
		"signal::button_press_event", G_CALLBACK (cb_button_press_event), typesel,
		"swapped_signal::focus_in_event", G_CALLBACK (typesel_set_selection_color), typesel,
		"swapped_signal::focus_out_event", G_CALLBACK (typesel_set_selection_color), typesel,
		NULL);
	gtk_widget_set_can_focus (typesel->canvas, TRUE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (gtk_builder_get_object (gui, "subtype_label")),
			   typesel->canvas);
	gtk_widget_set_size_request (typesel->canvas,
		MINOR_PIXMAP_WIDTH*3 + BORDER*5,
		MINOR_PIXMAP_HEIGHT*3 + BORDER*4);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (gtk_builder_get_object (gui, "canvas-container")),
			   typesel->canvas);
	typesel->sample_canvas = GTK_WIDGET (g_object_new (GOC_TYPE_CANVAS, NULL));
	g_object_connect (typesel->sample_canvas,
		"signal::size_allocate", G_CALLBACK (cb_typesel_sample_plot_resize), typesel,
		NULL);
	gtk_widget_set_size_request (typesel->sample_canvas, -1, 120);
	typesel->graph_group = goc_canvas_get_root (GOC_CANVAS (typesel->sample_canvas));
	gtk_container_add (GTK_CONTAINER (gtk_builder_get_object (gui, "sample-container")), typesel->sample_canvas);

	/* Init the list and the canvas group for each family */
	g_hash_table_foreach ((GHashTable *)gog_plot_families (),
		(GHFunc) cb_plot_families_init, typesel);

	selection = gtk_tree_view_get_selection (typesel->list_view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect_swapped (selection,
		"changed",
		G_CALLBACK (cb_selection_changed), typesel);

	/* The alpha blended selection box */
	typesel->selector = goc_item_new (
		goc_canvas_get_root (GOC_CANVAS (typesel->canvas)),
		gog_guru_selector_get_type (),
		NULL);
	style= go_styled_object_get_style (GO_STYLED_OBJECT (typesel->selector));
	style->line.width = 1;
	style->line.color = 0x000000ff;	/* black */
	typesel_set_selection_color (typesel);

	g_object_set_data_full (G_OBJECT (selector),
		"state", typesel, (GDestroyNotify) g_free);

	g_object_unref (gui);

	return selector;
}

static gboolean
graph_guru_init (GraphGuruState *s)
{
	s->gui = go_gtk_builder_load_internal ("res:go:graph/gog-guru.ui", GETTEXT_PACKAGE, s->cc);
        if (s->gui == NULL)
                return TRUE;

	s->dialog = go_gtk_builder_get_widget (s->gui, "GraphGuru");
	s->steps  = GTK_NOTEBOOK (gtk_builder_get_object (s->gui, "notebook"));

	/* Buttons */
	s->button_cancel   = graph_guru_init_button (s, "button_cancel");
	s->button_navigate = graph_guru_init_button (s, "button_navigate");
	s->button_ok	   = graph_guru_init_ok_button (s);

	return FALSE;
}

/**
 * gog_guru_get_help_button:
 * @guru: #GtkWidget  (the result of gog_guru).
 *
 * Quick utility to allow application specific help.  Required until we clean
 * up the relationship between goffice and gnumeric to decide which parts of
 * the help live where.
 *
 * Returns: (transfer none): #GtkWidget associated with the gurus help button.
 **/
GtkWidget *
gog_guru_get_help_button (GtkWidget *guru)
{
	GraphGuruState *state = g_object_get_data (G_OBJECT (guru), "state");
	return state ? go_gtk_builder_get_widget (state->gui, "help_button") : NULL;
}

/**
 * gog_guru:
 * @graph: the graph to edit
 * @dalloc: The data allocator to use for editing
 * @cc: Where to report errors
 * @closure: #GClosure
 *
 * CHANGED 0.5.3
 * 	: drop the @toplevel window argument and have the callers handle
 * 	  widget_show and set_transient
 *
 * Returns: (transfer full): the dialog, and shows new graph guru.
 **/
GtkWidget *
gog_guru (GogGraph *graph, GogDataAllocator *dalloc,
	  GOCmdContext *cc, GClosure *closure)
{
	int page = (graph != NULL) ? 1 : 0;
	GraphGuruState *state;

	state = g_new0 (GraphGuruState, 1);
	state->valid	= FALSE;
	state->updating = FALSE;
	state->fmt_page_initialized = FALSE;
	state->editing  = (graph != NULL);
	state->gui	= NULL;
	state->cc       = cc;
	state->dalloc   = dalloc;
	state->current_page	= -1;
	state->register_closure	= closure;
	state->graph_view = NULL;
	g_closure_ref (closure);

	if (graph != NULL) {
		g_return_val_if_fail (GOG_IS_GRAPH (graph), NULL);

		state->graph = gog_graph_dup (graph);
		state->chart = NULL;
		state->plot  = NULL;
	} else {
		state->plot = NULL;
		state->graph = g_object_new (GOG_TYPE_GRAPH, NULL);
		state->chart = GOG_CHART (gog_object_add_by_name (
				GOG_OBJECT (state->graph), "Chart", NULL));
		if (GO_IS_DOC_CONTROL (dalloc))
			g_object_set (state->graph,
						  "document", go_doc_control_get_doc (GO_DOC_CONTROL (dalloc)),
						  NULL);
		else if (GO_IS_DOC_CONTROL (cc))
			g_object_set (state->graph,
						  "document", go_doc_control_get_doc (GO_DOC_CONTROL (cc)),
						  NULL);
	}

	if (state->graph == NULL || graph_guru_init (state)) {
		graph_guru_state_destroy (state);
		return NULL;
	}

	/* Ok everything is hooked up. Let-er rip */
	state->valid = TRUE;

	state->initial_page = page;
	if (page == 0) {
		GtkWidget *w = graph_guru_type_selector_new (state);
		gtk_notebook_prepend_page (state->steps, w, NULL);
		gtk_widget_show_all (w);
	}

	graph_guru_set_page (state, page);

	/* a candidate for merging into attach guru */
	g_signal_connect_swapped (G_OBJECT (state->dialog), "destroy", G_CALLBACK (graph_guru_state_destroy), state);

	g_object_set_data (G_OBJECT (state->dialog),
		"state", state);

	return state->dialog;
}

void
gog_guru_add_custom_widget (GtkWidget *guru, GtkWidget *custom)
{
	GraphGuruState *state = g_object_get_data (G_OBJECT (guru), "state");
	GtkWidget *w = gtk_widget_get_parent (gtk_widget_get_parent (state->type_selector->canvas));
	GtkGrid *box = GTK_GRID (gtk_widget_get_parent (w));
	if (custom) {
		gtk_grid_attach_next_to (GTK_GRID (box), custom, w, GTK_POS_BOTTOM, 1, 1);
		g_object_set_data (G_OBJECT (custom), "graph", state->graph);
		gtk_widget_show_all (custom);
	}
}
