/*
 * file.c: File loading and saving routines
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Zbigniew Chyla (cyba@gnome.pl)
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
#include <goffice/utils/go-file.h>
#include <goffice/utils/go-glib-extras.h>
#include <goffice/utils/go-marshalers.h>
#include <goffice/app/file.h>
#include <goffice/app/go-doc.h>
#include <goffice/app/file-priv.h>
#include <goffice/app/error-info.h>
#include <goffice/app/io-context.h>
#include <goffice/app/go-cmd-context.h>

#include <gsf/gsf-input.h>
#include <gsf/gsf-output.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-utils.h>
#include <string.h>
#include <glib/gi18n-lib.h>

enum {
	FO_PROP_0,
	FO_PROP_ID,
	FO_PROP_DESCRIPTION,
	FO_PROP_INTERACTIVE_ONLY
};

static void
go_file_opener_init (GOFileOpener *fo)
{
	fo->id = NULL;
	fo->description = NULL;
	fo->probe_func = NULL;
	fo->open_func = NULL;
	fo->interactive_only = FALSE;
}

static void
go_file_opener_finalize (GObject *obj)
{
	GOFileOpener *fo;

	g_return_if_fail (GO_IS_FILE_OPENER (obj));

	fo = GO_FILE_OPENER (obj);
	g_free (fo->id);
	g_free (fo->description);
	g_slist_foreach (fo->suffixes, (GFunc)g_free, NULL);
	g_slist_free (fo->suffixes);
	g_slist_foreach (fo->mimes, (GFunc)g_free, NULL);
	g_slist_free (fo->mimes);

	G_OBJECT_CLASS (g_type_class_peek (G_TYPE_OBJECT))->finalize (obj);
}

static void
go_file_opener_set_property (GObject *object, guint property_id,
			     GValue const *value, GParamSpec *pspec)
{
	GOFileOpener *fo = (GOFileOpener *)object;

	switch (property_id) {
	case FO_PROP_ID: {
		char *s = g_value_dup_string (value);
		g_free (fo->id);
		fo->id = s;
		break;
	}
	case FO_PROP_DESCRIPTION: {
		char *s = g_value_dup_string (value);
		g_free (fo->description);
		fo->description = s;
		break;
	}
	case FO_PROP_INTERACTIVE_ONLY:
		fo->interactive_only = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
go_file_opener_get_property (GObject     *object,
			     guint        property_id,
			     GValue      *value,
			     GParamSpec  *pspec)
{
	GOFileOpener *fo = GO_FILE_OPENER (object);

	switch (property_id) {
	case FO_PROP_ID:
		g_value_set_string (value, fo->id);
		break;
	case FO_PROP_DESCRIPTION:
		g_value_set_string (value, fo->description);
		break;
	case FO_PROP_INTERACTIVE_ONLY:
		g_value_set_boolean (value, fo->interactive_only);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static gboolean
go_file_opener_can_probe_real (GOFileOpener const *fo, GOFileProbeLevel pl)
{
	return fo->probe_func != NULL;
}

static gboolean
go_file_opener_probe_real (GOFileOpener const *fo, GsfInput *input,
			   GOFileProbeLevel pl)
{
	gboolean ret = FALSE;

	if (fo->probe_func != NULL) {
		ret = fo->probe_func (fo, input, pl);
		gsf_input_seek (input, 0, G_SEEK_SET);
	}
	return ret;
}

static void
go_file_opener_open_real (GOFileOpener const *fo, gchar const *opt_enc,
                          GOIOContext *io_context, GoView *view,
                          GsfInput *input)
{
	if (fo->open_func != NULL) {
		if (fo->encoding_dependent)
			((GOFileOpenerOpenFuncWithEnc)fo->open_func)
				(fo, opt_enc, io_context, view, input);
		else
			fo->open_func (fo, io_context, view, input);
	} else
		go_io_error_unknown (io_context);
}

static void
go_file_opener_class_init (GOFileOpenerClass *klass)
{
	GObjectClass *goc = G_OBJECT_CLASS (klass);

	goc->finalize = go_file_opener_finalize;
	goc->set_property = go_file_opener_set_property;
	goc->get_property = go_file_opener_get_property;

	g_object_class_install_property
		(goc,
		 FO_PROP_ID,
		 g_param_spec_string ("id",
				      _("ID"),
				      _("The identifier of the opener"),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_READABLE));

        g_object_class_install_property
		(goc,
		 FO_PROP_DESCRIPTION,
		 g_param_spec_string ("description",
				      _("Description"),
				      _("The description of the opener"),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FO_PROP_INTERACTIVE_ONLY,
		 g_param_spec_boolean ("interactive-only",
				      _("Interactive only"),
				      _("TRUE if this opener requires interaction"),
				       FALSE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));

	klass->can_probe = go_file_opener_can_probe_real;
	klass->probe = go_file_opener_probe_real;
	klass->open = go_file_opener_open_real;
}

GSF_CLASS (GOFileOpener, go_file_opener,
	   go_file_opener_class_init, go_file_opener_init,
	   G_TYPE_OBJECT)

/**
 * go_file_opener_setup:
 * @fo: Newly created GOFileOpener object
 * @id: (allow-none): ID of the opener
 * @description: Description of supported file format
 * @suffixes: (element-type utf8) (transfer full): List of suffixes to
 * associate with the opener
 * @mimes: (element-type utf8) (transfer full): List of mime types to
 * associate with the opener
 * @encoding_dependent: whether the opener depends on an encoding sel.
 * @probe_func: (scope async) (nullable): "probe" function
 * @open_func: (scope async): "open" function
 *
 * Sets up GOFileOpener object, newly created with g_object_new function.
 * This is intended to be used only by GOFileOpener derivates.
 * Use go_file_opener_new, if you want to create GOFileOpener object.
 **/
void
go_file_opener_setup (GOFileOpener *fo, gchar const *id,
		      gchar const *description,
		      GSList *suffixes,
		      GSList *mimes,
		      gboolean encoding_dependent,
		      GOFileOpenerProbeFunc probe_func,
		      GOFileOpenerOpenFunc open_func)
{
	g_return_if_fail (GO_IS_FILE_OPENER (fo));
	g_return_if_fail (description != NULL);

	fo->id = g_strdup (id);
	fo->description = g_strdup (description);
	fo->suffixes = suffixes;
	fo->mimes = mimes;

	fo->encoding_dependent = encoding_dependent;
	fo->probe_func = probe_func;
	fo->open_func = open_func;
}

/**
 * go_file_opener_new:
 * @id: (nullable): Optional ID of the opener
 * @description: Description of supported file format
 * @suffixes: (element-type utf8) (transfer full): List of suffixes to
 * associate with the opener
 * @mimes: (element-type utf8) (transfer full): List of mime types to
 * associate with the opener
 * @probe_func: (scope async) (nullable): "probe" function
 * @open_func: (scope async): "open" function
 *
 * Creates new GOFileOpener object. Optional @id will be used
 * after registering it with go_file_opener_register function.
 *
 * Returns: (transfer full): newly created GOFileOpener object.
 **/
GOFileOpener *
go_file_opener_new (gchar const *id,
		    gchar const *description,
		    GSList *suffixes,
		    GSList *mimes,
		    GOFileOpenerProbeFunc probe_func,
		    GOFileOpenerOpenFunc open_func)
{
	GOFileOpener *fo;

	fo = GO_FILE_OPENER (g_object_new (GO_TYPE_FILE_OPENER, NULL));
	go_file_opener_setup (fo, id, description, suffixes, mimes, FALSE,
			      probe_func, open_func);

	return fo;
}

/**
 * go_file_opener_new_with_enc:
 * @id: (nullable): Optional ID of the opener
 * @description: Description of supported file format
 * @suffixes: (element-type utf8) (transfer full): List of suffixes to
 * associate with the opener
 * @mimes: (element-type utf8) (transfer full): List of mime types to
 * associate with the opener
 * @probe_func: (scope async) (nullable): "probe" function
 * @open_func: (scope async): "open" function
 *
 * Creates new #GOFileOpener object. Optional @id will be used
 * after registering it with go_file_opener_register function.
 *
 * Returns: (transfer full): newly created #GOFileOpener object.
 **/
GOFileOpener *
go_file_opener_new_with_enc (gchar const *id,
			     gchar const *description,
			     GSList *suffixes,
			     GSList *mimes,
			     GOFileOpenerProbeFunc probe_func,
			     GOFileOpenerOpenFuncWithEnc open_func)
{
        GOFileOpener *fo;

        fo = GO_FILE_OPENER (g_object_new (GO_TYPE_FILE_OPENER, NULL));
        go_file_opener_setup (fo, id, description, suffixes, mimes, TRUE,
			      probe_func, (GOFileOpenerOpenFunc)open_func);
        return fo;
}



/**
 * go_file_opener_get_id:
 * @fo: #GOFileOpener to query
 *
 * Returns: (transfer none) (nullable): the id of @fo
 */
gchar const *
go_file_opener_get_id (GOFileOpener const *fo)
{
	g_return_val_if_fail (GO_IS_FILE_OPENER (fo), NULL);

	return fo->id;
}

/**
 * go_file_opener_get_description:
 * @fo: #GOFileOpener to query
 *
 * Returns: (transfer none): the description of @fo
 */
gchar const *
go_file_opener_get_description (GOFileOpener const *fo)
{
	g_return_val_if_fail (GO_IS_FILE_OPENER (fo), NULL);

	return fo->description;
}

/**
 * go_file_opener_is_encoding_dependent:
 * @fo: #GOFileOpener to query
 *
 * Returns: %TRUE if @fo is encoding dependent
 */
gboolean
go_file_opener_is_encoding_dependent (GOFileOpener const *fo)
{
        g_return_val_if_fail (GO_IS_FILE_OPENER (fo), FALSE);

	return fo->encoding_dependent;
}

/**
 * go_file_opener_can_probe:
 * @fo: #GOFileOpener to query
 * @pl: probe level
 *
 * Returns: %TRUE if @fo has a probe function
 */
gboolean
go_file_opener_can_probe (GOFileOpener const *fo, GOFileProbeLevel pl)
{
	g_return_val_if_fail (GO_IS_FILE_OPENER (fo), FALSE);

	return GO_FILE_OPENER_METHOD (fo, can_probe) (fo, pl);
}

/**
 * go_file_opener_get_suffixes:
 * @fo: #GOFileOpener
 *
 * Returns: (element-type utf8) (transfer none): the suffixes for the
 * supported file types.
 **/
GSList const *
go_file_opener_get_suffixes (GOFileOpener const *fo)
{
	g_return_val_if_fail (GO_IS_FILE_OPENER (fo), NULL);
	return fo->suffixes;
}

/**
 * go_file_opener_get_mimes:
 * @fo: #GOFileOpener to query
 *
 * Returns: (element-type utf8) (transfer none): the supported mime types.
 **/
GSList const *
go_file_opener_get_mimes (GOFileOpener const *fo)
{
	g_return_val_if_fail (GO_IS_FILE_OPENER (fo), NULL);
	return fo->mimes;
}


/**
 * go_file_opener_probe:
 * @fo: #GOFileOpener
 * @input: #GsfInput
 * @pl: #GOFileProbeLevel
 *
 * Checks if a given file is supported by the opener.
 *
 * Returns: %TRUE, if the opener can read given file and %FALSE otherwise.
 **/
gboolean
go_file_opener_probe (GOFileOpener const *fo, GsfInput *input, GOFileProbeLevel pl)
{
	g_return_val_if_fail (GO_IS_FILE_OPENER (fo), FALSE);
	g_return_val_if_fail (GSF_IS_INPUT (input), FALSE);

#if 0
	g_print ("Trying format %s at level %d...\n",
		 go_file_opener_get_id (fo),
		 (int)pl);
#endif
	return GO_FILE_OPENER_METHOD (fo, probe) (fo, input, pl);
}

/**
 * go_file_opener_open:
 * @fo: GOFileOpener object
 * @opt_enc: (nullable): Optional encoding
 * @io_context: Context for i/o operation
 * @view: #GoView
 * @input: Gsf input stream
 *
 * Reads content of @file_name file into workbook @wbv is attached to.
 * Results are reported using @io_context object, use
 * go_io_error_occurred to find out if operation was successful.
 * The state of @wbv and its workbook is undefined if operation fails, you
 * should destroy them in that case.
 */
void
go_file_opener_open (GOFileOpener const *fo, gchar const *opt_enc,
		     GOIOContext *io_context,
		     GoView *view, GsfInput *input)
{
	g_return_if_fail (GO_IS_FILE_OPENER (fo));
	g_return_if_fail (GSF_IS_INPUT (input));

	GO_FILE_OPENER_METHOD (fo, open) (fo, opt_enc, io_context, view, input);
}

/*
 * GOFileSaver
 */

/**
 * GOFileSaverClass:
 * @parent_class: parent class.
 * @save: saves the file.
 * @set_export_options: set the options.
 *
 * File saver base class.
 **/

static void
go_file_saver_init (GOFileSaver *fs)
{
	fs->id = NULL;
	fs->extension = NULL;
	fs->mime_type = NULL;
	fs->description = NULL;
	fs->overwrite_files = TRUE;
	fs->interactive_only = FALSE;
	fs->format_level = GO_FILE_FL_NEW;
	fs->save_scope = GO_FILE_SAVE_WORKBOOK;
	fs->save_func = NULL;
}

static void
go_file_saver_finalize (GObject *obj)
{
	GOFileSaver *fs;

	g_return_if_fail (GO_IS_FILE_SAVER (obj));

	fs = GO_FILE_SAVER (obj);
	g_free (fs->id);
	g_free (fs->extension);
	g_free (fs->description);
	g_free (fs->mime_type);

	G_OBJECT_CLASS (g_type_class_peek (G_TYPE_OBJECT))->finalize (obj);
}

enum {
	FS_PROP_0,
	FS_PROP_ID,
	FS_PROP_MIME_TYPE,
	FS_PROP_EXTENSION,
	FS_PROP_DESCRIPTION,
	FS_PROP_OVERWRITE,
	FS_PROP_INTERACTIVE_ONLY,
	FS_PROP_FORMAT_LEVEL,
	FS_PROP_SCOPE,
	FS_PROP_SHEET_SELECTION
};

enum {
	FS_SET_EXPORT_OPTIONS,
	FS_LAST_SIGNAL
};

static guint fs_signals[FS_LAST_SIGNAL];

static void
go_file_saver_set_property (GObject *object, guint property_id,
			    GValue const *value, GParamSpec *pspec)
{
	GOFileSaver *fs = (GOFileSaver *)object;

	switch (property_id) {
	case FS_PROP_ID: {
		char *s = g_value_dup_string (value);
		g_free (fs->id);
		fs->id = s;
		break;
	}
	case FS_PROP_MIME_TYPE: {
		char *s = g_value_dup_string (value);
		g_free (fs->mime_type);
		fs->mime_type = s;
		break;
	}
	case FS_PROP_EXTENSION: {
		char *s = g_value_dup_string (value);
		g_free (fs->extension);
		fs->extension = s;
		break;
	}
	case FS_PROP_DESCRIPTION: {
		char *s = g_value_dup_string (value);
		g_free (fs->description);
		fs->description = s;
		break;
	}
	case FS_PROP_OVERWRITE:
		fs->overwrite_files = g_value_get_boolean (value);
		break;
	case FS_PROP_INTERACTIVE_ONLY:
		fs->interactive_only = g_value_get_boolean (value);
		break;
	case FS_PROP_FORMAT_LEVEL:
		fs->format_level = g_value_get_enum (value);
		break;
	case FS_PROP_SCOPE:
		fs->save_scope = g_value_get_enum (value);
		break;
	case FS_PROP_SHEET_SELECTION:
		fs->sheet_selection = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
go_file_saver_get_property (GObject *object, guint property_id,
			    GValue *value, GParamSpec *pspec)
{
	GOFileSaver *fs = (GOFileSaver *)object;

	switch (property_id) {
	case FS_PROP_ID:
		g_value_set_string (value, fs->id);
		break;
	case FS_PROP_MIME_TYPE:
		g_value_set_string (value, fs->mime_type);
		break;
	case FS_PROP_EXTENSION:
		g_value_set_string (value, fs->extension);
		break;
	case FS_PROP_DESCRIPTION:
		g_value_set_string (value, fs->description);
		break;
	case FS_PROP_OVERWRITE:
		g_value_set_boolean (value, fs->overwrite_files);
		break;
	case FS_PROP_INTERACTIVE_ONLY:
		g_value_set_boolean (value, fs->interactive_only);
		break;
	case FS_PROP_FORMAT_LEVEL:
		g_value_set_enum (value, fs->format_level);
		break;
	case FS_PROP_SCOPE:
		g_value_set_enum (value, fs->save_scope);
		break;
	case FS_PROP_SHEET_SELECTION:
		g_value_set_boolean (value, fs->sheet_selection);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
go_file_saver_save_real (GOFileSaver const *fs, GOIOContext *io_context,
			 GoView const *view, GsfOutput *output)
{
	if (fs->save_func == NULL) {
		go_io_error_unknown (io_context);
		return;
	}

	fs->save_func (fs, io_context, view, output);
}

static void
go_file_saver_class_init (GOFileSaverClass *klass)
{
	GObjectClass *goc = G_OBJECT_CLASS (klass);

	goc->finalize = go_file_saver_finalize;
	goc->set_property = go_file_saver_set_property;
	goc->get_property = go_file_saver_get_property;

        g_object_class_install_property
		(goc,
		 FS_PROP_ID,
		 g_param_spec_string ("id",
				      _("ID"),
				      _("The identifier of the saver."),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FS_PROP_MIME_TYPE,
		 g_param_spec_string ("mime-type",
				      _("MIME type"),
				      _("The MIME type of the saver."),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FS_PROP_EXTENSION,
		 g_param_spec_string ("extension",
				      _("Extension"),
				      _("The standard file name extension of the saver."),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FS_PROP_DESCRIPTION,
		 g_param_spec_string ("description",
				      _("Description"),
				      _("The description of the saver."),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FS_PROP_OVERWRITE,
		 g_param_spec_boolean ("overwrite",
				       _("Overwrite"),
				       _("Whether the saver will overwrite files."),
				       TRUE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FS_PROP_INTERACTIVE_ONLY,
		 g_param_spec_boolean ("interactive-only",
				      _("Interactive only"),
				      _("TRUE if this saver requires interaction"),
				       FALSE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));

	/* What a load of crap! */
        g_object_class_install_property
		(goc,
		 FS_PROP_FORMAT_LEVEL,
		 g_param_spec_enum ("format-level",
				    _("Format Level"),
				    "?",
				    GO_TYPE_FILE_FORMAT_LEVEL,
				    GO_FILE_FL_NEW,
				    GSF_PARAM_STATIC |
				    G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FS_PROP_SCOPE,
		 g_param_spec_enum ("scope",
				    _("Scope"),
				    _("How much of a document is saved"),
				    GO_TYPE_FILE_SAVE_SCOPE,
				    GO_FILE_SAVE_WORKBOOK,
				    GSF_PARAM_STATIC |
				    G_PARAM_READWRITE));

        g_object_class_install_property
		(goc,
		 FS_PROP_SHEET_SELECTION,
		 g_param_spec_boolean ("sheet-selection",
				      _("Sheet Selection"),
				      _("TRUE if this saver supports saving a subset of all sheet"),
				       FALSE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));

	klass->save = go_file_saver_save_real;

	/* class signals */
	fs_signals[FS_SET_EXPORT_OPTIONS] =
		g_signal_new ("set-export-options",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOFileSaverClass, set_export_options),
			      NULL, NULL,
			      go__BOOLEAN__OBJECT_STRING_POINTER,
			      G_TYPE_BOOLEAN,
			      3, GO_TYPE_DOC, G_TYPE_STRING, G_TYPE_POINTER);
}

GSF_CLASS (GOFileSaver, go_file_saver,
	   go_file_saver_class_init, go_file_saver_init,
	   G_TYPE_OBJECT)

/**
 * go_file_saver_new:
 * @id: (nullable): ID of the saver
 * @extension: (nullable): Default extension of saved files
 * @description: Description of supported file format
 * @level: File format level
 * @save_func: (scope async): Pointer to "save" function
 *
 * Creates new #GOFileSaver object. Optional @id will be used
 * after registering it with go_file_saver_register or
 * go_file_saver_register_as_default function.
 *
 * Returns: newly created #GOFileSaver object.
 */
GOFileSaver *
go_file_saver_new (gchar const *id,
		   gchar const *extension,
		   gchar const *description,
		   GOFileFormatLevel level,
		   GOFileSaverSaveFunc save_func)
{
	GOFileSaver *fs;

	g_return_val_if_fail (description != NULL, NULL);

	fs = GO_FILE_SAVER (g_object_new (GO_TYPE_FILE_SAVER,
					  "id", id,
					  "extension", extension,
					  "description", description,
					  "format-level", level,
					  NULL));
	fs->save_func = save_func;

	return fs;
}

void
go_file_saver_set_save_scope (GOFileSaver *fs, GOFileSaveScope scope)
{
	g_return_if_fail (GO_IS_FILE_SAVER (fs));
	g_return_if_fail (scope < GO_FILE_SAVE_LAST);

	fs->save_scope = scope;
}

/**
 * go_file_saver_get_save_scope:
 * @fs: #GOFileSaver to query
 *
 * Returns: The save scope of @fs.
 */
GOFileSaveScope
go_file_saver_get_save_scope (GOFileSaver const *fs)
{
	g_return_val_if_fail (GO_IS_FILE_SAVER (fs), GO_FILE_SAVE_WORKBOOK);

	return fs->save_scope;
}

/**
 * go_file_saver_get_id:
 * @fs: #GOFileSaver to query
 *
 * Returns: (nullable) (transfer none): The id of @fs.
 */
gchar const *
go_file_saver_get_id (GOFileSaver const *fs)
{
	g_return_val_if_fail (GO_IS_FILE_SAVER (fs), NULL);

	return fs->id;
}

/**
 * go_file_saver_get_mime_type:
 * @fs: #GOFileSaver to query
 *
 * Returns: (nullable) (transfer none): The mime type of the @fs.
 */
gchar const *
go_file_saver_get_mime_type (GOFileSaver const *fs)
{
	g_return_val_if_fail (GO_IS_FILE_SAVER (fs), NULL);

	return fs->mime_type;
}

/**
 * go_file_saver_get_extension:
 * @fs: #GOFileSaver to query
 *
 * Returns: (nullable) (transfer none): The default extensions for files saved
 * by @fs.
 */
gchar const *
go_file_saver_get_extension (GOFileSaver const *fs)
{
	g_return_val_if_fail (GO_IS_FILE_SAVER (fs), NULL);

	return fs->extension;
}

/**
 * go_file_saver_get_description:
 * @fs: #GOFileSaver to query
 *
 * Returns: (transfer none): The description of @fs.
 */
gchar const *
go_file_saver_get_description (GOFileSaver const *fs)
{
	g_return_val_if_fail (GO_IS_FILE_SAVER (fs), NULL);

	return fs->description;
}

/**
 * go_file_saver_get_format_level:
 * @fs: #GOFileSaver to query
 *
 * Returns: The format level of @fs.
 */
GOFileFormatLevel
go_file_saver_get_format_level (GOFileSaver const *fs)
{
	g_return_val_if_fail (GO_IS_FILE_SAVER (fs), GO_FILE_FL_NEW);

	return fs->format_level;
}

gboolean
go_file_saver_set_export_options (GOFileSaver *fs,
				  GODoc *doc,
				  const char *options,
				  GError **err)
{
	gboolean res = TRUE;

	g_signal_emit (G_OBJECT (fs), fs_signals[FS_SET_EXPORT_OPTIONS], 0,
		       doc, options, err, &res);
	return res;
}

/**
 * go_file_saver_save:
 * @fs: GOFileSaver object
 * @io_context: Context for i/o operation
 * @view: #GoView
 * @output: Output stream
 *
 * Saves @wbv and the workbook it is attached to into @output stream.
 * Results are reported using @io_context object, use
 * go_io_error_occurred to find out if operation was successful.
 * It's possible that @file_name is created and contain some data if
 * operation fails, you should remove the file in that case.
 */
void
go_file_saver_save (GOFileSaver const *fs, GOIOContext *io_context,
		    GoView *view, GsfOutput *output)
{
	g_return_if_fail (GO_IS_FILE_SAVER (fs));
	g_return_if_fail (GSF_IS_OUTPUT (output));

	if (GSF_IS_OUTPUT_STDIO (output)) {
		const char *name = gsf_output_name (output);
		/*
		 * This is mostly bogus.  We need a proper source for the file
		 * name, not something double-converted.
		 */
		char *file_name = name
			? g_filename_from_utf8 (name, -1, NULL, NULL, NULL)
			: NULL;

		/* If we don't have a filename, silently skip the test.  */
		if (!fs->overwrite_files &&
		    file_name &&
		    g_file_test (file_name, G_FILE_TEST_EXISTS)) {
			GOErrorInfo *save_error;
			const char *msg = _("Saving over old files of this type is disabled for safety.");

			if (!gsf_output_error (output))
				gsf_output_set_error (output, 0, "%s", msg);

			g_free (file_name);

			save_error = go_error_info_new_str_with_details (
				msg,
				go_error_info_new_str (
					_("You can turn this safety feature off by editing appropriate plugin.xml file.")));
			go_io_error_info_set (io_context, save_error);
			return;
		}

		g_free (file_name);
	}

	GO_FILE_SAVER_METHOD (fs, save) (fs, io_context, view, output);
}

/**
 * go_file_saver_set_overwrite_files:
 * @fs: #GOFileSaver object
 * @overwrite: A boolean value saying whether the saver should overwrite
 *                existing files.
 *
 * Changes behaviour of the saver when saving a file. If @overwrite is set
 * to TRUE, existing file will be overwritten. Otherwise, the saver will
 * report an error without saving anything.
 */
void
go_file_saver_set_overwrite_files (GOFileSaver *fs, gboolean overwrite)
{
	g_return_if_fail (GO_IS_FILE_SAVER (fs));

	fs->overwrite_files = overwrite;
}

GType
go_file_format_level_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static GEnumValue values[] = {
			{ GO_FILE_FL_NONE, (char*)"GO_FILE_FL_NONE", (char*)"none" },
			{ GO_FILE_FL_WRITE_ONLY, (char*)"GO_FILE_FL_WRITE_ONLY", (char*)"write_only" },
			{ GO_FILE_FL_NEW, (char*)"GO_FILE_FL_NEW", (char*)"new" },
			{ GO_FILE_FL_MANUAL, (char*)"GO_FILE_FL_MANUAL", (char*)"manual" },
			{ GO_FILE_FL_MANUAL_REMEMBER, (char*)"GO_FL_MANUAL_REMEMBER", (char*)"manual_remember" },
			{ GO_FILE_FL_AUTO, (char*)"GO_FILE_FL_AUTO", (char*)"auto" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("GOFileFormatLevel", values);
	}
	return etype;
}

GType
go_file_save_scope_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static GEnumValue values[] = {
			{ GO_FILE_SAVE_WORKBOOK, (char*)"GO_FILE_SAVE_WORKBOOK", (char*)"workbook" },
			{ GO_FILE_SAVE_SHEET, (char*)"GO_FILE_SAVE_SHEET", (char*)"sheet" },
			{ GO_FILE_SAVE_RANGE, (char*)"GO_FILE_SAVE_RANGE", (char*)"range" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("GOFileSaveScope", values);
	}
	return etype;
}

/*-------------------------------------------------------------------------- */


/*
 * ------
 */

typedef struct {
	gint priority;
	GOFileSaver *saver;
} DefaultFileSaver;

static GHashTable *file_opener_id_hash = NULL,
	*file_saver_id_hash = NULL;
static GList *file_opener_list = NULL, *file_opener_priority_list = NULL;
static GList *file_saver_list = NULL, *default_file_saver_list = NULL;

static gint
cmp_int_less_than (gconstpointer list_i, gconstpointer i)
{
	return !(GPOINTER_TO_INT (list_i) < GPOINTER_TO_INT (i));
}

/**
 * go_file_opener_register:
 * @fo: GOFileOpener object
 * @priority: Opener's priority
 *
 * Adds @fo opener to the list of available file openers, making it
 * available for Gnumeric i/o routines. The opener is registered with given
 * @priority. The priority is used to determine the order in which openers
 * will be tried when reading a file. The higher the priority, the sooner it
 * will be tried. Default XML-based Gnumeric file opener is registered at
 * priority 50. Recommended range for @priority is [0, 100].
 * Reference count for the opener is incremented inside the function, but
 * you don't have to (and shouldn't) call g_object_unref on it if it's
 * floating object (for example, when you pass object newly created with
 * go_file_opener_new and not referenced anywhere).
 */
void
go_file_opener_register (GOFileOpener *fo, gint priority)
{
	gint pos;
	gchar const *id;

	g_return_if_fail (GO_IS_FILE_OPENER (fo));
	g_return_if_fail (priority >=0 && priority <= 100);

	pos = go_list_index_custom (file_opener_priority_list,
				    GINT_TO_POINTER (priority),
				    cmp_int_less_than);
	file_opener_priority_list = g_list_insert (
		file_opener_priority_list,
		GINT_TO_POINTER (priority), pos);
	file_opener_list = g_list_insert (file_opener_list, fo, pos);
	g_object_ref (fo);

	id = go_file_opener_get_id (fo);
	if (id != NULL) {
		if (file_opener_id_hash	== NULL)
			file_opener_id_hash = g_hash_table_new (&g_str_hash, &g_str_equal);
		g_hash_table_insert (file_opener_id_hash, (gpointer) id, fo);
	}
}

/**
 * go_file_opener_unregister:
 * @fo: #GOFileOpener object previously registered using
 *    go_file_opener_register
 *
 * Removes @fo opener from list of available file openers. Reference count
 * for the opener is decremented inside the function.
 */
void
go_file_opener_unregister (GOFileOpener *fo)
{
	gint pos;
	GList *l;
	gchar const *id;

	g_return_if_fail (GO_IS_FILE_OPENER (fo));

	pos = g_list_index (file_opener_list, fo);
	g_return_if_fail (pos != -1);
	l = g_list_nth (file_opener_list, pos);
	file_opener_list = g_list_remove_link (file_opener_list, l);
	g_list_free_1 (l);
	l = g_list_nth (file_opener_priority_list, pos);
	file_opener_priority_list = g_list_remove_link (file_opener_priority_list, l);
	g_list_free_1 (l);

	id = go_file_opener_get_id (fo);
	if (id != NULL) {
		g_hash_table_remove (file_opener_id_hash, (gpointer) id);
		if (g_hash_table_size (file_opener_id_hash) == 0) {
			g_hash_table_destroy (file_opener_id_hash);
			file_opener_id_hash = NULL;
		}
	}

	g_object_unref (fo);
}

static gint
default_file_saver_cmp_priority (gconstpointer a, gconstpointer b)
{
	DefaultFileSaver const *dfs_a = a, *dfs_b = b;

	return dfs_b->priority - dfs_a->priority;
}

/**
 * go_file_saver_register:
 * @fs: #GOFileSaver object
 *
 * Adds @fs saver to the list of available file savers, making it
 * available for the user when selecting file format for save.
 */
void
go_file_saver_register (GOFileSaver *fs)
{
	gchar const *id;

	g_return_if_fail (GO_IS_FILE_SAVER (fs));

	file_saver_list = g_list_prepend (file_saver_list, fs);
	g_object_ref (fs);

	id = go_file_saver_get_id (fs);
	if (id != NULL) {
		if (file_saver_id_hash	== NULL)
			file_saver_id_hash = g_hash_table_new (&g_str_hash,
							       &g_str_equal);
		g_hash_table_insert (file_saver_id_hash, (gpointer) id, fs);
	}
}

/**
 * go_file_saver_register_as_default:
 * @fs: GOFileSaver object
 * @priority: Saver's priority
 *
 * Adds @fs saver to the list of available file savers, making it
 * available for the user when selecting file format for save.
 * The saver is also marked as default saver with given priority.
 * When Gnumeric needs default file saver, it chooses the one with the
 * highest priority. Recommended range for @priority is [0, 100].
 */
void
go_file_saver_register_as_default (GOFileSaver *fs, gint priority)
{
	DefaultFileSaver *dfs;

	g_return_if_fail (GO_IS_FILE_SAVER (fs));
	g_return_if_fail (priority >=0 && priority <= 100);

	go_file_saver_register (fs);

	dfs = g_new (DefaultFileSaver, 1);
	dfs->priority = priority;
	dfs->saver = fs;
	default_file_saver_list = g_list_insert_sorted (
		default_file_saver_list, dfs,
		default_file_saver_cmp_priority);
}

/**
 * go_file_saver_unregister:
 * @fs: #GOFileSaver object previously registered using
 *                go_file_saver_register or go_file_saver_register_as_default
 *
 * Removes @fs saver from list of available file savers. Reference count
 * for the saver is decremented inside the function.
 **/
void
go_file_saver_unregister (GOFileSaver *fs)
{
	GList *l;
	gchar const *id;

	g_return_if_fail (GO_IS_FILE_SAVER (fs));

	l = g_list_find (file_saver_list, fs);
	g_return_if_fail (l != NULL);
	file_saver_list = g_list_remove_link (file_saver_list, l);
	g_list_free_1 (l);

	id = go_file_saver_get_id (fs);
	if (id != NULL) {
		g_hash_table_remove (file_saver_id_hash, (gpointer) id);
		if (g_hash_table_size (file_saver_id_hash) == 0) {
			g_hash_table_destroy (file_saver_id_hash);
			file_saver_id_hash = NULL;
		}
	}

	for (l = default_file_saver_list; l != NULL; l = l->next) {
		if (((DefaultFileSaver *) l->data)->saver == fs) {
			default_file_saver_list = g_list_remove_link (default_file_saver_list, l);
			g_free (l->data);
			g_list_free_1 (l);
			break;
		}
	}

	g_object_unref (fs);
}

/**
 * go_file_saver_get_default:
 *
 * Finds file saver registered as default saver with the highest priority.
 * Reference count for the saver is NOT incremented.
 *
 * Returns: (transfer none) (nullable): #GOFileSaver for the highest priority
 * default saver.
 **/
GOFileSaver *
go_file_saver_get_default (void)
{
	if (default_file_saver_list == NULL)
		return NULL;

	return ((DefaultFileSaver *) default_file_saver_list->data)->saver;
}

/**
 * go_file_saver_for_mime_type:
 * @mime_type: A mime type
 *
 * Returns: (transfer none) (nullable): A #GOFileSaver object associated with
 * @mime_type.
 **/
GOFileSaver *
go_file_saver_for_mime_type (gchar const *mime_type)
{
	GList *l;

	g_return_val_if_fail (mime_type != NULL, NULL);

	for (l = default_file_saver_list ; l != NULL; l = l->next) {
		DefaultFileSaver *dfs = l->data;
		GOFileSaver *fs = dfs->saver;
		const char *this_type = go_file_saver_get_mime_type (fs);
		if (this_type && !strcmp (this_type, mime_type))
			return fs;
	}

	for (l = file_saver_list; l != NULL; l = l->next) {
		GOFileSaver *fs = l->data;
		const char *this_type = go_file_saver_get_mime_type (fs);
		if (this_type && !strcmp (this_type, mime_type))
			return fs;
	}

	return NULL;
}

/**
 * go_file_saver_for_file_name:
 * @file_name: name
 *
 * Searches for file saver matching the given @file_name, registered using
 * go_file_saver_register.
 *
 * Returns: (transfer none) (nullable): #GOFileSaver for @file_name
 **/
GOFileSaver *
go_file_saver_for_file_name (char const *file_name)
{
	GList *l;
	GOFileSaver *best_saver = NULL;
	char const *extension = gsf_extension_pointer (file_name);

	for (l = default_file_saver_list; l != NULL; l = l->next) {
		DefaultFileSaver *def_saver = l->data;
		GOFileSaver *saver = def_saver->saver;
		if (g_strcmp0 (go_file_saver_get_extension (saver), extension))
			continue;
		return saver;
	}

	for (l = file_saver_list; l != NULL; l = l->next) {
		GOFileSaver *saver = l->data;
		if (g_strcmp0 (go_file_saver_get_extension (saver), extension))
			continue;

		if (!best_saver ||
		    (saver->save_scope < best_saver->save_scope))
			best_saver = saver;
	}

	return best_saver;
}

/**
 * go_file_opener_for_id:
 * @id: File opener's ID
 *
 * Searches for file opener with given @id, registered using
 * go_file_opener_register
 *
 * Returns: (transfer none) (nullable): #GOFileOpener with given id.
 **/
GOFileOpener *
go_file_opener_for_id (gchar const *id)
{
	g_return_val_if_fail (id != NULL, NULL);

	if (file_opener_id_hash == NULL)
		return NULL;
	return GO_FILE_OPENER (g_hash_table_lookup (file_opener_id_hash, id));
}

/**
 * go_file_saver_for_id:
 * @id: File saver's ID
 *
 * Searches for file saver with given @id, registered using
 * go_file_saver_register or register_file_opener_as_default.
 *
 * Returns: (transfer none) (nullable): #GOFileSaver with given id.
 **/
GOFileSaver *
go_file_saver_for_id (gchar const *id)
{
	g_return_val_if_fail (id != NULL, NULL);

	if (file_saver_id_hash == NULL)
		return NULL;
	return GO_FILE_SAVER (g_hash_table_lookup (file_saver_id_hash, id));
}

/**
 * go_get_file_savers:
 *
 * Returns: (element-type GOFileSaver) (transfer none): list of known
 * #GOFileSaver types (do not modify list)
 */
GList *
go_get_file_savers (void)
{
	return file_saver_list;
}

/**
 * go_get_file_openers:
 *
 * Returns: (element-type GOFileOpener) (transfer none): list of known
 * #GOFileOpener types (do not modify list)
 **/
GList *
go_get_file_openers (void)
{
	return file_opener_list;
}
