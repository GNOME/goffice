#ifndef __GO_ACCUMULATOR_H
#define __GO_ACCUMULATOR_H

#include <glib.h>

G_BEGIN_DECLS

gboolean go_accumulator_functional (void);
void *go_accumulator_start (void);
void go_accumulator_end (void *state);

GOAccumulator *go_accumulator_new (void);
void go_accumulator_free (GOAccumulator *acc);
void go_accumulator_clear (GOAccumulator *acc);
void go_accumulator_add (GOAccumulator *acc, double x);
void go_accumulator_add_quad (GOAccumulator *acc, const GOQuad *x);
double go_accumulator_value (GOAccumulator *acc);


#ifdef GOFFICE_WITH_LONG_DOUBLE

gboolean go_accumulator_functionall (void);
void *go_accumulator_startl (void);
void go_accumulator_endl (void *state);

GOAccumulatorl *go_accumulator_newl (void);
void go_accumulator_freel (GOAccumulatorl *acc);
void go_accumulator_clearl (GOAccumulatorl *acc);
void go_accumulator_addl (GOAccumulatorl *acc, long double x);
void go_accumulator_add_quadl (GOAccumulatorl *acc, const GOQuadl *x);
long double go_accumulator_valuel (GOAccumulatorl *acc);
#endif

#ifdef GOFFICE_WITH_DECIMAL64

gboolean go_accumulator_functionalD (void);
void *go_accumulator_startD (void);
void go_accumulator_endD (void *state);

GOAccumulatorD *go_accumulator_newD (void);
void go_accumulator_freeD (GOAccumulatorD *acc);
void go_accumulator_clearD (GOAccumulatorD *acc);
void go_accumulator_addD (GOAccumulatorD *acc, _Decimal64 x);
void go_accumulator_add_quadD (GOAccumulatorD *acc, const GOQuadD *x);
_Decimal64 go_accumulator_valueD (GOAccumulatorD *acc);
#endif

G_END_DECLS

#endif
