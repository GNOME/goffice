#ifndef GO_FORMAT_IMPL_H
#define GO_FORMAT_IMPL_H

#include <goffice/utils/format.h>

G_BEGIN_DECLS

typedef struct {
        char const *format;
        char        restriction_type;
        double	    restriction_value;
	GOColor     go_color;

	gboolean    want_am_pm;
	gboolean    has_fraction;
	gboolean    suppress_minus;
	gboolean    elapsed_time;
} StyleFormatEntry;

void go_fmt_general_int    (GString *result, int val, int col_width);
void go_fmt_general_float  (GString *result, double val, double col_width);
void go_format_number      (GString *result,
			    double number, int col_width, StyleFormatEntry const *entry,
			    GODateConventions const *date_conv);
#if GOFFICE_WITH_LONG_DOUBLE
void go_fmt_general_floatl (GString *result, long double val, double col_width);
void go_format_numberl     (GString *result,
			    long double number, int col_width, StyleFormatEntry const *entry,
			    GODateConventions const *date_conv);
#endif

G_END_DECLS

#endif /* GO_FORMAT_IMPL_H */
