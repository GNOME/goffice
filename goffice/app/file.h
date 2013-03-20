#ifndef _GOFFICE_FILE_H_
#define _GOFFICE_FILE_H_

#include <glib-object.h>
#include <goffice/app/goffice-app.h>

G_BEGIN_DECLS

/**
 * GOFileFormatLevel:
 * @GO_FILE_FL_NONE: no name assigned, won't happen.
 * @GO_FILE_FL_WRITE_ONLY: PostScript etc, won't be remembered.
 * @GO_FILE_FL_NEW: Wb just created.
 * @GO_FILE_FL_MANUAL: Save gets punted to save as.
 * @GO_FILE_FL_MANUAL_REMEMBER: Ditto, but remember in history.
 * @GO_FILE_FL_AUTO: Save will save to this filename.
 * @GO_FILE_FL_LAST: last value, won't happen.
 *
 * File format levels. They are ordered. When we save a file, we
 * remember the name, but not if we already have a name at a higher level.
 * When created, workbooks are assigned a name at level GO_FILE_FL_NEW.
 */
typedef enum {
	GO_FILE_FL_NONE,            /* No name assigned, won't happen */
	GO_FILE_FL_WRITE_ONLY,      /* PostScript etc, won't be remembered */
	GO_FILE_FL_NEW,             /* Wb just created */
	GO_FILE_FL_MANUAL,          /* Save gets punted to save as */
	GO_FILE_FL_MANUAL_REMEMBER, /* Ditto, but remember in history */
	GO_FILE_FL_AUTO,            /* Save will save to this filename */
	GO_FILE_FL_LAST
} GOFileFormatLevel;
GType go_file_format_level_get_type (void);
#define GO_TYPE_FILE_FORMAT_LEVEL (go_file_format_level_get_type ())

/**
 * GOFileSaveScope:
 * @GO_FILE_SAVE_WORKBOOK: save the whole file.
 * @GO_FILE_SAVE_SHEET: save only current page.
 * @GO_FILE_SAVE_RANGE: save only a selected range.
 * @GO_FILE_SAVE_LAST: last value, won't happen.
 *
 * GOFileSaveScope specifies what information file saver can save in a file.
 * Many savers can save the whole workbook (with all sheets), but others
 * save only current sheet, usually because of file format limitations.
 */
typedef enum {
	GO_FILE_SAVE_WORKBOOK,
	GO_FILE_SAVE_SHEET,
	GO_FILE_SAVE_RANGE,
	GO_FILE_SAVE_LAST
} GOFileSaveScope;
GType go_file_save_scope_get_type (void);
#define GO_TYPE_FILE_SAVE_SCOPE (go_file_save_scope_get_type ())

/*
 * GOFileOpener
 */

typedef struct _GOFileOpenerClass GOFileOpenerClass;

#define GO_TYPE_FILE_OPENER             (go_file_opener_get_type ())
#define GO_FILE_OPENER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GO_TYPE_FILE_OPENER, GOFileOpener))
#define GO_IS_FILE_OPENER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GO_TYPE_FILE_OPENER))

typedef gboolean (*GOFileOpenerProbeFunc) (GOFileOpener const *fo,
					    GsfInput *input,
					    GOFileProbeLevel pl);
typedef void     (*GOFileOpenerOpenFunc) (GOFileOpener const *fo,
					  GOIOContext *io_context,
					  GoView *view,
					  GsfInput *input);
typedef void     (*GOFileOpenerOpenFuncWithEnc) (GOFileOpener const *fo,
						 gchar const *enc,
						 GOIOContext *io_context,
						 GoView *view,
						 GsfInput *input);

GType go_file_opener_get_type (void);

GOFileOpener *go_file_opener_new (char const *id,
				  char const *description,
				  GSList *suffixes,
				  GSList *mimes,
				  GOFileOpenerProbeFunc probe_func,
				  GOFileOpenerOpenFunc open_func);

GOFileOpener *go_file_opener_new_with_enc (char const *id,
					   char const *description,
					   GSList *suffixes,
					   GSList *mimes,
					   GOFileOpenerProbeFunc probe_func,
					   GOFileOpenerOpenFuncWithEnc open_func);


gboolean     go_file_opener_probe (GOFileOpener const *fo, GsfInput *input,
				    GOFileProbeLevel pl);
void         go_file_opener_open (GOFileOpener const *fo, gchar const *opt_enc,
				  GOIOContext *io_context,
				  GoView *view,
				  GsfInput *input);

char const *go_file_opener_get_id		  (GOFileOpener const *fo);
char const *go_file_opener_get_description	  (GOFileOpener const *fo);
gboolean    go_file_opener_is_encoding_dependent  (GOFileOpener const *fo);
gboolean    go_file_opener_can_probe		  (GOFileOpener const *fo,
						   GOFileProbeLevel pl);
GSList const *go_file_opener_get_suffixes	  (GOFileOpener const *fo);
GSList const *go_file_opener_get_mimes	  	  (GOFileOpener const *fo);

/*
 * GOFileSaver
 */

typedef struct _GOFileSaverClass GOFileSaverClass;

#define GO_TYPE_FILE_SAVER             (go_file_saver_get_type ())
#define GO_FILE_SAVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GO_TYPE_FILE_SAVER, GOFileSaver))
#define GO_IS_FILE_SAVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GO_TYPE_FILE_SAVER))

typedef void (*GOFileSaverSaveFunc) (GOFileSaver const *fs,
				     GOIOContext *io_context,
				     GoView const *view,
				     GsfOutput *output);

GType go_file_saver_get_type (void);

GOFileSaver *go_file_saver_new (char const *id,
				char const *extension,
				char const *description,
				GOFileFormatLevel level,
				GOFileSaverSaveFunc save_func);


void          go_file_saver_set_save_scope (GOFileSaver *fs, GOFileSaveScope scope);
GOFileSaveScope go_file_saver_get_save_scope (GOFileSaver const *fs);

gboolean     go_file_saver_set_export_options (GOFileSaver *fs,
					       GODoc *doc,
					       const char *options,
					       GError **err);

void         go_file_saver_save (GOFileSaver const *fs, GOIOContext *io_context,
				 GoView *view,
				 GsfOutput *output);
void         go_file_saver_set_overwrite_files	(GOFileSaver *fs,
						 gboolean overwrite);
char const *go_file_saver_get_id	  	(GOFileSaver const *fs);
char const *go_file_saver_get_extension	(GOFileSaver const *fs);
char const *go_file_saver_get_mime_type	(GOFileSaver const *fs);
char const *go_file_saver_get_description	(GOFileSaver const *fs);
GOFileFormatLevel go_file_saver_get_format_level	(GOFileSaver const *fs);

/*
 *
 */

GList *go_get_file_openers (void);
void		 go_file_opener_unregister (GOFileOpener *fo);
void		 go_file_opener_register   (GOFileOpener *fo, gint priority);
GOFileOpener	*go_file_opener_for_id	    (char const *id);

GList *go_get_file_savers (void);
void go_file_saver_unregister	(GOFileSaver *fs);
void go_file_saver_register	(GOFileSaver *fs);
void go_file_saver_register_as_default (GOFileSaver *fs, gint priority);

GOFileSaver	*go_file_saver_get_default (void);
GOFileSaver	*go_file_saver_for_mime_type	(char const *mime_type);
GOFileSaver	*go_file_saver_for_file_name	(char const *file_name);
GOFileSaver	*go_file_saver_for_id		(char const *id);

G_END_DECLS

#endif /* _GOFFICE_FILE_H_ */
