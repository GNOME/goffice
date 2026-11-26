/*
 * test-util.c: unit tests for the code in goffice/utils/.
 *
 * The tests use the GLib test framework.  Each file in goffice/utils/
 * gets its own section below with a function that registers the tests
 * for that file; test paths are of the form "/utils/<file>/<function>".
 *
 * Currently covered:
 *   - go-geometry.c
 *   - go-color.c
 *   - go-unit.c
 *   - datetime.c
 *   - go-string.c
 *   - go-line.c
 *   - go-undo.c
 *   - go-path.c
 *   - go-bezier.c
 *   - go-marker.c
 *   - go-pattern.c
 *   - go-glib-extras.c
 *   - go-cairo.c
 *   - regutf8.c
 *   - go-libxml-extras.c
 *   - go-image.c
 *   - go-font.c
 *   - go-locale.c
 *   - go-style.c
 *   - go-gradient.c
 *   - go-rsm.c
 *   - go-persist.c
 *   - go-pango-extras.c
 *   - go-file.c
 */

#include <goffice/goffice.h>
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include <gsf/gsf-output-memory.h>
#include <gsf/gsf-input-memory.h>
#ifdef G_OS_UNIX
#include <sys/mman.h>
#endif

#define CXML2C(s) ((char const *)(s))
#define CC2XML(s) ((const xmlChar *)(s))


#define EPS 1e-9

#define assert_close(a_, b_) \
	g_assert_cmpfloat_with_epsilon ((a_), (b_), EPS)

/* cairo keeps path coordinates in 24.8 fixed point */
#define assert_cairo_close(a_, b_) \
	g_assert_cmpfloat (fabs ((a_) - (b_)), <=, 1.0 / 256)

/* Like assert_close, but also usable when the expected value is 0 */
#define assert_near(a_, b_) \
	g_assert_cmpfloat (fabs ((a_) - (b_)), <, EPS)

/* ------------------------------------------------------------------ */
/* go-geometry.c                                                       */
/* ------------------------------------------------------------------ */

static void
test_geometry_cartesian_to_polar (void)
{
	double rho, theta;

	go_geometry_cartesian_to_polar (3, 4, &rho, &theta);
	assert_close (rho, 5);
	assert_close (theta, atan2 (4, 3));

	go_geometry_cartesian_to_polar (0, 0, &rho, &theta);
	assert_near (rho, 0);
	assert_near (theta, 0);

	go_geometry_cartesian_to_polar (1, 0, &rho, &theta);
	assert_close (rho, 1);
	assert_near (theta, 0);

	go_geometry_cartesian_to_polar (0, 2, &rho, &theta);
	assert_close (rho, 2);
	assert_close (theta, M_PI / 2);

	go_geometry_cartesian_to_polar (-1, 0, &rho, &theta);
	assert_close (rho, 1);
	assert_close (theta, M_PI);

	go_geometry_cartesian_to_polar (0, -2, &rho, &theta);
	assert_close (rho, 2);
	assert_close (theta, -M_PI / 2);

	go_geometry_cartesian_to_polar (-1, -1, &rho, &theta);
	assert_close (rho, M_SQRT2);
	assert_close (theta, -3 * M_PI / 4);

	/* Large values must not overflow (hypot rather than sqrt(x*x+y*y)) */
	go_geometry_cartesian_to_polar (3e200, 4e200, &rho, &theta);
	g_assert_cmpfloat (fabs (rho - 5e200) / 5e200, <, 1e-12);
}

static void
test_geometry_point_to_segment (void)
{
	/* Horizontal segment from (0,0) to (10,0) */
	assert_close (go_geometry_point_to_segment (5, 3, 0, 0, 10, 0), 3);
	assert_close (go_geometry_point_to_segment (5, -3, 0, 0, 10, 0), 3);
	assert_near (go_geometry_point_to_segment (5, 0, 0, 0, 10, 0), 0);

	/* Closest point is the start of the segment */
	assert_close (go_geometry_point_to_segment (-3, 4, 0, 0, 10, 0), 5);
	assert_near (go_geometry_point_to_segment (0, 0, 0, 0, 10, 0), 0);

	/* Closest point is the end of the segment */
	assert_close (go_geometry_point_to_segment (13, 4, 0, 0, 10, 0), 5);
	assert_near (go_geometry_point_to_segment (10, 0, 0, 0, 10, 0), 0);

	/* Vertical segment */
	assert_close (go_geometry_point_to_segment (2, 5, 0, 0, 0, 10), 2);
	assert_close (go_geometry_point_to_segment (3, -4, 0, 0, 0, 10), 5);

	/* Diagonal segment from (0,0) to (1,1) */
	assert_close (go_geometry_point_to_segment (0, 1, 0, 0, 1, 1),
		      M_SQRT2 / 2);
	assert_near (go_geometry_point_to_segment (0.5, 0.5, 0, 0, 1, 1), 0);

	/* Negative extents: segment from (5,5) to (3,5) */
	assert_close (go_geometry_point_to_segment (4, 7, 5, 5, -2, 0), 2);
	assert_close (go_geometry_point_to_segment (6, 5, 5, 5, -2, 0), 1);
	assert_close (go_geometry_point_to_segment (0, 5, 5, 5, -2, 0), 3);

	/* Offset segment */
	assert_close (go_geometry_point_to_segment (11, 12, 10, 10, 4, 0), 2);

	/* Degenerate segment: reduces to point-to-point distance */
	assert_close (go_geometry_point_to_segment (3, 4, 0, 0, 0, 0), 5);
	assert_near (go_geometry_point_to_segment (1, 1, 1, 1, 0, 0), 0);
}

static void
test_geometry_AABR_add (void)
{
	GOGeometryAABR a, b;

	/* Disjoint rectangles */
	a.x = 0; a.y = 0; a.w = 2; a.h = 2;
	b.x = 5; b.y = 6; b.w = 1; b.h = 3;
	go_geometry_AABR_add (&a, &b);
	assert_close (a.x, 0);
	assert_close (a.y, 0);
	assert_close (a.w, 6);
	assert_close (a.h, 9);
	/* The second argument is untouched */
	assert_close (b.x, 5);
	assert_close (b.y, 6);
	assert_close (b.w, 1);
	assert_close (b.h, 3);

	/* Rectangle to the lower left of the first one */
	a.x = 0; a.y = 0; a.w = 2; a.h = 2;
	b.x = -3; b.y = -4; b.w = 1; b.h = 1;
	go_geometry_AABR_add (&a, &b);
	assert_close (a.x, -3);
	assert_close (a.y, -4);
	assert_close (a.w, 5);
	assert_close (a.h, 6);

	/* Second rectangle contained in the first */
	a.x = 0; a.y = 0; a.w = 10; a.h = 10;
	b.x = 2; b.y = 3; b.w = 1; b.h = 1;
	go_geometry_AABR_add (&a, &b);
	assert_close (a.x, 0);
	assert_close (a.y, 0);
	assert_close (a.w, 10);
	assert_close (a.h, 10);

	/* First rectangle contained in the second */
	a.x = 2; a.y = 3; a.w = 1; a.h = 1;
	b.x = 0; b.y = 0; b.w = 10; b.h = 10;
	go_geometry_AABR_add (&a, &b);
	assert_close (a.x, 0);
	assert_close (a.y, 0);
	assert_close (a.w, 10);
	assert_close (a.h, 10);

	/* Partial overlap */
	a.x = 0; a.y = 0; a.w = 4; a.h = 4;
	b.x = 2; b.y = 2; b.w = 4; b.h = 4;
	go_geometry_AABR_add (&a, &b);
	assert_close (a.x, 0);
	assert_close (a.y, 0);
	assert_close (a.w, 6);
	assert_close (a.h, 6);

	/* Adding a rectangle to itself is the identity */
	a.x = 1; a.y = 2; a.w = 3; a.h = 4;
	b = a;
	go_geometry_AABR_add (&a, &b);
	assert_close (a.x, 1);
	assert_close (a.y, 2);
	assert_close (a.w, 3);
	assert_close (a.h, 4);

	/* Zero-sized rectangles act like points */
	a.x = 1; a.y = 1; a.w = 0; a.h = 0;
	b.x = 4; b.y = 5; b.w = 0; b.h = 0;
	go_geometry_AABR_add (&a, &b);
	assert_close (a.x, 1);
	assert_close (a.y, 1);
	assert_close (a.w, 3);
	assert_close (a.h, 4);
}

static void
test_geometry_OBR_to_AABR (void)
{
	GOGeometryOBR obr;
	GOGeometryAABR aabr;

	/* No rotation: same size, positioned around the center */
	obr.x = 10; obr.y = 20; obr.w = 4; obr.h = 2; obr.alpha = 0;
	go_geometry_OBR_to_AABR (&obr, &aabr);
	assert_close (aabr.w, 4);
	assert_close (aabr.h, 2);
	assert_close (aabr.x, 8);
	assert_close (aabr.y, 19);

	/* Quarter turn swaps width and height */
	obr.alpha = M_PI / 2;
	go_geometry_OBR_to_AABR (&obr, &aabr);
	assert_close (aabr.w, 2);
	assert_close (aabr.h, 4);
	assert_close (aabr.x, 9);
	assert_close (aabr.y, 18);

	/* Half turn is the identity as far as the AABR is concerned */
	obr.alpha = M_PI;
	go_geometry_OBR_to_AABR (&obr, &aabr);
	assert_close (aabr.w, 4);
	assert_close (aabr.h, 2);
	assert_close (aabr.x, 8);
	assert_close (aabr.y, 19);

	/* Negative angle behaves like the positive one */
	obr.alpha = -M_PI / 6;
	go_geometry_OBR_to_AABR (&obr, &aabr);
	{
		GOGeometryAABR aabr2;
		obr.alpha = M_PI / 6;
		go_geometry_OBR_to_AABR (&obr, &aabr2);
		assert_close (aabr.w, aabr2.w);
		assert_close (aabr.h, aabr2.h);
		assert_close (aabr.x, aabr2.x);
		assert_close (aabr.y, aabr2.y);
	}

	/* Square rotated by 45 degrees: sides become the diagonal */
	obr.x = 0; obr.y = 0; obr.w = 2; obr.h = 2; obr.alpha = M_PI / 4;
	go_geometry_OBR_to_AABR (&obr, &aabr);
	assert_close (aabr.w, 2 * M_SQRT2);
	assert_close (aabr.h, 2 * M_SQRT2);
	assert_close (aabr.x, -M_SQRT2);
	assert_close (aabr.y, -M_SQRT2);

	/* 30 degrees, general rectangle */
	obr.x = 1; obr.y = 1; obr.w = 4; obr.h = 2; obr.alpha = M_PI / 6;
	go_geometry_OBR_to_AABR (&obr, &aabr);
	assert_close (aabr.w, 4 * cos (M_PI / 6) + 2 * sin (M_PI / 6));
	assert_close (aabr.h, 4 * sin (M_PI / 6) + 2 * cos (M_PI / 6));
	assert_close (aabr.x + aabr.w / 2, 1);
	assert_close (aabr.y + aabr.h / 2, 1);

	/* Degenerate rectangle */
	obr.x = 3; obr.y = 4; obr.w = 0; obr.h = 0; obr.alpha = 1;
	go_geometry_OBR_to_AABR (&obr, &aabr);
	assert_near (aabr.w, 0);
	assert_near (aabr.h, 0);
	assert_close (aabr.x, 3);
	assert_close (aabr.y, 4);
}

static void
test_geometry_test_OBR_overlap (void)
{
	GOGeometryOBR a, b;

	/* Identical rectangles */
	a.x = 0; a.y = 0; a.w = 2; a.h = 2; a.alpha = 0;
	b = a;
	g_assert_true (go_geometry_test_OBR_overlap (&a, &b));

	/* Clearly separated along x and along y */
	b.x = 10;
	g_assert_false (go_geometry_test_OBR_overlap (&a, &b));
	b.x = 0; b.y = 10;
	g_assert_false (go_geometry_test_OBR_overlap (&a, &b));

	/* Overlapping axis-aligned rectangles */
	b.x = 1.5; b.y = 1.5;
	g_assert_true (go_geometry_test_OBR_overlap (&a, &b));

	/* Touching edge to edge counts as overlap; a hair further does not */
	b.x = 2; b.y = 0;
	g_assert_true (go_geometry_test_OBR_overlap (&a, &b));
	b.x = 2.001;
	g_assert_false (go_geometry_test_OBR_overlap (&a, &b));

	/* Rectangle completely inside the other */
	b.x = 0; b.y = 0; b.w = 0.5; b.h = 0.5;
	g_assert_true (go_geometry_test_OBR_overlap (&a, &b));
	g_assert_true (go_geometry_test_OBR_overlap (&b, &a));

	/* Rotated square, separated along the x axis */
	b.x = 2.2; b.y = 0; b.w = 2; b.h = 2; b.alpha = M_PI / 4;
	g_assert_true (go_geometry_test_OBR_overlap (&a, &b));
	b.x = 2.5;
	g_assert_false (go_geometry_test_OBR_overlap (&a, &b));

	/* Long thin rectangles forming a cross */
	a.x = 0; a.y = 0; a.w = 10; a.h = 0.2; a.alpha = 0;
	b.x = 0; b.y = 0; b.w = 10; b.h = 0.2; b.alpha = M_PI / 2;
	g_assert_true (go_geometry_test_OBR_overlap (&a, &b));

	/* Parallel thin rectangles, separated perpendicular to their length */
	b.alpha = 0; b.y = 1;
	g_assert_false (go_geometry_test_OBR_overlap (&a, &b));

	/* Negative sizes are treated as their absolute values */
	a.x = 0; a.y = 0; a.w = -2; a.h = -2; a.alpha = 0;
	b.x = 1.5; b.y = 0; b.w = 2; b.h = 2; b.alpha = 0;
	g_assert_true (go_geometry_test_OBR_overlap (&a, &b));
	b.x = 3;
	g_assert_false (go_geometry_test_OBR_overlap (&a, &b));
}

static void
test_geometry_test_OBR_overlap_symmetry (void)
{
	GRand *rand = g_rand_new_with_seed (42);
	int i;

	for (i = 0; i < 10000; i++) {
		GOGeometryOBR a, b;

		a.x = g_rand_double_range (rand, -5, 5);
		a.y = g_rand_double_range (rand, -5, 5);
		a.w = g_rand_double_range (rand, 0.1, 5);
		a.h = g_rand_double_range (rand, 0.1, 5);
		a.alpha = g_rand_double_range (rand, -M_PI, M_PI);
		b.x = g_rand_double_range (rand, -5, 5);
		b.y = g_rand_double_range (rand, -5, 5);
		b.w = g_rand_double_range (rand, 0.1, 5);
		b.h = g_rand_double_range (rand, 0.1, 5);
		b.alpha = g_rand_double_range (rand, -M_PI, M_PI);

		if (go_geometry_test_OBR_overlap (&a, &b) !=
		    go_geometry_test_OBR_overlap (&b, &a)) {
			g_error ("Overlap not symmetric for "
				 "(%g,%g,%g,%g,%g) and (%g,%g,%g,%g,%g)",
				 a.x, a.y, a.w, a.h, a.alpha,
				 b.x, b.y, b.w, b.h, b.alpha);
		}
	}

	g_rand_free (rand);
}

static void
test_geometry_test_OBR_overlap_vs_aabr (void)
{
	/* If the axis-aligned bounding rectangles of two rectangles are
	   disjoint, the rectangles can't overlap. */
	GRand *rand = g_rand_new_with_seed (4711);
	int i;

	for (i = 0; i < 10000; i++) {
		GOGeometryOBR a, b;
		GOGeometryAABR aa, ab;
		gboolean aabr_disjoint;

		a.x = g_rand_double_range (rand, -5, 5);
		a.y = g_rand_double_range (rand, -5, 5);
		a.w = g_rand_double_range (rand, 0.1, 5);
		a.h = g_rand_double_range (rand, 0.1, 5);
		a.alpha = g_rand_double_range (rand, -M_PI, M_PI);
		b.x = g_rand_double_range (rand, -5, 5);
		b.y = g_rand_double_range (rand, -5, 5);
		b.w = g_rand_double_range (rand, 0.1, 5);
		b.h = g_rand_double_range (rand, 0.1, 5);
		b.alpha = g_rand_double_range (rand, -M_PI, M_PI);

		go_geometry_OBR_to_AABR (&a, &aa);
		go_geometry_OBR_to_AABR (&b, &ab);
		aabr_disjoint =
			aa.x + aa.w < ab.x || ab.x + ab.w < aa.x ||
			aa.y + aa.h < ab.y || ab.y + ab.h < aa.y;

		if (aabr_disjoint)
			g_assert_false (go_geometry_test_OBR_overlap (&a, &b));
	}

	g_rand_free (rand);
}

static void
test_geometry_get_rotation_type (void)
{
	/* Exact multiples of a quarter turn */
	g_assert_cmpint (go_geometry_get_rotation_type (0), ==, GO_ROTATE_NONE);
	g_assert_cmpint (go_geometry_get_rotation_type (M_PI / 2), ==, GO_ROTATE_COUNTERCLOCKWISE);
	g_assert_cmpint (go_geometry_get_rotation_type (M_PI), ==, GO_ROTATE_UPSIDEDOWN);
	g_assert_cmpint (go_geometry_get_rotation_type (3 * M_PI / 2), ==, GO_ROTATE_CLOCKWISE);
	g_assert_cmpint (go_geometry_get_rotation_type (2 * M_PI), ==, GO_ROTATE_NONE);

	/* Angles outside [0,2pi] are normalized */
	g_assert_cmpint (go_geometry_get_rotation_type (-M_PI / 2), ==, GO_ROTATE_CLOCKWISE);
	g_assert_cmpint (go_geometry_get_rotation_type (-M_PI), ==, GO_ROTATE_UPSIDEDOWN);
	g_assert_cmpint (go_geometry_get_rotation_type (-3 * M_PI / 2), ==, GO_ROTATE_COUNTERCLOCKWISE);
	g_assert_cmpint (go_geometry_get_rotation_type (-2 * M_PI), ==, GO_ROTATE_NONE);
	g_assert_cmpint (go_geometry_get_rotation_type (5 * M_PI / 2), ==, GO_ROTATE_COUNTERCLOCKWISE);
	g_assert_cmpint (go_geometry_get_rotation_type (3 * M_PI), ==, GO_ROTATE_UPSIDEDOWN);
	g_assert_cmpint (go_geometry_get_rotation_type (4 * M_PI), ==, GO_ROTATE_NONE);
	g_assert_cmpint (go_geometry_get_rotation_type (-10 * M_PI), ==, GO_ROTATE_NONE);

	/* Free rotation */
	g_assert_cmpint (go_geometry_get_rotation_type (M_PI / 4), ==, GO_ROTATE_FREE);
	g_assert_cmpint (go_geometry_get_rotation_type (1), ==, GO_ROTATE_FREE);
	g_assert_cmpint (go_geometry_get_rotation_type (3 * M_PI / 4), ==, GO_ROTATE_FREE);
	g_assert_cmpint (go_geometry_get_rotation_type (-M_PI / 4), ==, GO_ROTATE_FREE);
	g_assert_cmpint (go_geometry_get_rotation_type (7 * M_PI / 4), ==, GO_ROTATE_FREE);

	/* Tolerance: anything within GO_GEOMETRY_ANGLE_TOLERANCE of a
	   quarter turn is snapped; anything a bit further is free. */
	{
		double tol = GO_GEOMETRY_ANGLE_TOLERANCE;
		double inside = tol / 2;
		double outside = tol * 10;
		int k;

		for (k = 0; k < 4; k++) {
			double base = k * M_PI / 2;
			GOGeometryRotationType expected = (GOGeometryRotationType)k;

			g_assert_cmpint (go_geometry_get_rotation_type (base + inside), ==, expected);
			g_assert_cmpint (go_geometry_get_rotation_type (base - inside), ==, expected);
			g_assert_cmpint (go_geometry_get_rotation_type (base + outside), ==, GO_ROTATE_FREE);
			g_assert_cmpint (go_geometry_get_rotation_type (base - outside), ==, GO_ROTATE_FREE);
		}
	}

	/* Just below zero wraps around to just below 2pi */
	g_assert_cmpint (go_geometry_get_rotation_type (-GO_GEOMETRY_ANGLE_TOLERANCE / 2), ==, GO_ROTATE_NONE);
}

static void
test_geometry_calc_label_anchor (void)
{
	GOGeometryOBR obr;

	/* Wide label, horizontal axis: attach on top or bottom */
	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	g_assert_cmpint (go_geometry_calc_label_anchor (&obr, 0), ==, GO_SIDE_TOP_BOTTOM);

	/* Wide label, vertical axis: attach on left or right */
	g_assert_cmpint (go_geometry_calc_label_anchor (&obr, M_PI / 2), ==, GO_SIDE_LEFT_RIGHT);
	g_assert_cmpint (go_geometry_calc_label_anchor (&obr, -M_PI / 2), ==, GO_SIDE_LEFT_RIGHT);

	/* Rotating the label by a quarter turn flips the answer */
	obr.alpha = M_PI / 2;
	g_assert_cmpint (go_geometry_calc_label_anchor (&obr, 0), ==, GO_SIDE_LEFT_RIGHT);
	g_assert_cmpint (go_geometry_calc_label_anchor (&obr, M_PI / 2), ==, GO_SIDE_TOP_BOTTOM);

	/* The function does not modify the rectangle */
	obr.x = 1; obr.y = 2; obr.w = 3; obr.h = 4; obr.alpha = 0.5;
	go_geometry_calc_label_anchor (&obr, 0.25);
	assert_close (obr.x, 1);
	assert_close (obr.y, 2);
	assert_close (obr.w, 3);
	assert_close (obr.h, 4);
	assert_close (obr.alpha, 0.5);
}

static void
test_geometry_calc_label_position (void)
{
	GOGeometryOBR obr;
	GOGeometrySide anchor;

	/* Horizontal axis, label above (GO_SIDE_LEFT here means "not right") */
	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	anchor = go_geometry_calc_label_position (&obr, 0, 3,
						  GO_SIDE_LEFT, GO_SIDE_AUTO);
	g_assert_cmpint (anchor, ==, GO_SIDE_TOP_BOTTOM);
	assert_near (obr.x, 0);
	assert_close (obr.y, -4);

	/* Same, but on the other side of the axis */
	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	anchor = go_geometry_calc_label_position (&obr, 0, 3,
						  GO_SIDE_RIGHT, GO_SIDE_AUTO);
	g_assert_cmpint (anchor, ==, GO_SIDE_TOP_BOTTOM);
	assert_near (obr.x, 0);
	assert_close (obr.y, 4);

	/* Vertical axis: label attaches with its left/right edge */
	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	anchor = go_geometry_calc_label_position (&obr, M_PI / 2, 3,
						  GO_SIDE_LEFT, GO_SIDE_AUTO);
	g_assert_cmpint (anchor, ==, GO_SIDE_LEFT_RIGHT);
	assert_close (obr.x, 8);
	assert_near (obr.y, 0);

	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	anchor = go_geometry_calc_label_position (&obr, M_PI / 2, 3,
						  GO_SIDE_RIGHT, GO_SIDE_AUTO);
	g_assert_cmpint (anchor, ==, GO_SIDE_LEFT_RIGHT);
	assert_close (obr.x, -8);
	assert_near (obr.y, 0);

	/* An explicit anchor is returned unchanged and overrides the
	   automatic choice: force left/right on a horizontal axis. */
	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	anchor = go_geometry_calc_label_position (&obr, 0, 3,
						  GO_SIDE_LEFT, GO_SIDE_LEFT_RIGHT);
	g_assert_cmpint (anchor, ==, GO_SIDE_LEFT_RIGHT);
	/* offset += ds (=1); label center moved by half its width, then
	   pushed up by the offset. */
	assert_close (obr.x, -5);
	assert_close (obr.y, -4);

	/* And force top/bottom on a vertical axis */
	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	anchor = go_geometry_calc_label_position (&obr, M_PI / 2, 3,
						  GO_SIDE_LEFT, GO_SIDE_TOP_BOTTOM);
	g_assert_cmpint (anchor, ==, GO_SIDE_TOP_BOTTOM);

	/* Zero offset puts the label directly against the axis */
	obr.x = 0; obr.y = 0; obr.w = 10; obr.h = 2; obr.alpha = 0;
	go_geometry_calc_label_position (&obr, 0, 0,
					 GO_SIDE_LEFT, GO_SIDE_AUTO);
	assert_near (obr.x, 0);
	assert_close (obr.y, -1);

	/* Only the position is modified, not the size or angle */
	assert_close (obr.w, 10);
	assert_close (obr.h, 2);
	assert_near (obr.alpha, 0);
}

static void
test_geometry_direction (void)
{
	g_assert_false (go_direction_is_horizontal (GO_DIRECTION_UP));
	g_assert_false (go_direction_is_horizontal (GO_DIRECTION_DOWN));
	g_assert_true (go_direction_is_horizontal (GO_DIRECTION_LEFT));
	g_assert_true (go_direction_is_horizontal (GO_DIRECTION_RIGHT));
	g_assert_false (go_direction_is_horizontal (GO_DIRECTION_NONE));

	g_assert_false (go_direction_is_forward (GO_DIRECTION_UP));
	g_assert_true (go_direction_is_forward (GO_DIRECTION_DOWN));
	g_assert_false (go_direction_is_forward (GO_DIRECTION_LEFT));
	g_assert_true (go_direction_is_forward (GO_DIRECTION_RIGHT));
	g_assert_false (go_direction_is_forward (GO_DIRECTION_NONE));
}

static void
test_geometry_direction_name (void)
{
	int d, e;

	/* Every direction has a name of its own */
	for (d = 0; d <= GO_DIRECTION_NONE; d++) {
		const char *name = go_direction_get_name ((GODirection)d);
		g_assert_nonnull (name);
		g_assert_cmpuint (strlen (name), >, 0);
		for (e = 0; e < d; e++)
			g_assert_cmpstr (name, !=, go_direction_get_name ((GODirection)e));
	}

	/* Out-of-range values are rejected */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
			       "*go_direction_get_name*");
	g_assert_null (go_direction_get_name ((GODirection)100));
	g_test_assert_expected_messages ();
}

static void
test_geometry_direction_type (void)
{
	GType t = GO_TYPE_DIRECTION;
	GEnumClass *klass;
	int d;

	g_assert_true (G_TYPE_IS_ENUM (t));
	/* Registering is idempotent */
	g_assert_true (go_direction_get_type () == t);

	/* All directions are registered, each with distinct names */
	klass = g_type_class_ref (t);
	g_assert_cmpint (klass->n_values, ==, GO_DIRECTION_NONE + 1);
	for (d = 0; d <= GO_DIRECTION_NONE; d++) {
		GEnumValue *v = g_enum_get_value (klass, d);
		g_assert_nonnull (v);
		g_assert_cmpint (v->value, ==, d);
		g_assert_true (g_enum_get_value_by_name (klass, v->value_name) == v);
		g_assert_true (g_enum_get_value_by_nick (klass, v->value_nick) == v);
	}
	g_assert_null (g_enum_get_value (klass, GO_DIRECTION_NONE + 1));
	g_type_class_unref (klass);
}


static void
register_geometry_tests (void)
{
	g_test_add_func ("/utils/go-geometry/cartesian_to_polar", test_geometry_cartesian_to_polar);
	g_test_add_func ("/utils/go-geometry/point_to_segment", test_geometry_point_to_segment);
	g_test_add_func ("/utils/go-geometry/AABR_add", test_geometry_AABR_add);
	g_test_add_func ("/utils/go-geometry/OBR_to_AABR", test_geometry_OBR_to_AABR);
	g_test_add_func ("/utils/go-geometry/test_OBR_overlap", test_geometry_test_OBR_overlap);
	g_test_add_func ("/utils/go-geometry/test_OBR_overlap/symmetry", test_geometry_test_OBR_overlap_symmetry);
	g_test_add_func ("/utils/go-geometry/test_OBR_overlap/vs-aabr", test_geometry_test_OBR_overlap_vs_aabr);
	g_test_add_func ("/utils/go-geometry/get_rotation_type", test_geometry_get_rotation_type);
	g_test_add_func ("/utils/go-geometry/calc_label_anchor", test_geometry_calc_label_anchor);
	g_test_add_func ("/utils/go-geometry/calc_label_position", test_geometry_calc_label_position);
	g_test_add_func ("/utils/go-geometry/direction", test_geometry_direction);
	g_test_add_func ("/utils/go-geometry/direction_name", test_geometry_direction_name);
	g_test_add_func ("/utils/go-geometry/direction_type", test_geometry_direction_type);
}

/* ------------------------------------------------------------------ */
/* go-color.c                                                          */
/* ------------------------------------------------------------------ */

static void
test_color_macros (void)
{
	GOColor c = GO_COLOR_FROM_RGBA (0x12, 0x34, 0x56, 0x78);
	guint r, g, b, a;

	g_assert_cmphex (c, ==, 0x12345678u);
	g_assert_cmphex (GO_COLOR_FROM_RGB (0x12, 0x34, 0x56), ==, 0x123456ffu);

	g_assert_cmphex (GO_COLOR_UINT_R (c), ==, 0x12);
	g_assert_cmphex (GO_COLOR_UINT_G (c), ==, 0x34);
	g_assert_cmphex (GO_COLOR_UINT_B (c), ==, 0x56);
	g_assert_cmphex (GO_COLOR_UINT_A (c), ==, 0x78);

	/* High bit set in the red channel must not sign-extend */
	c = GO_COLOR_FROM_RGBA (0xff, 0x80, 0x00, 0xff);
	g_assert_cmphex (c, ==, 0xff8000ffu);
	g_assert_cmphex (GO_COLOR_UINT_R (c), ==, 0xff);

	/* Arguments are masked to 8 bits */
	g_assert_cmphex (GO_COLOR_FROM_RGBA (0x1ff, 0x1ff, 0x1ff, 0x1ff), ==, 0xffffffffu);
	g_assert_cmphex (GO_COLOR_FROM_RGBA (0x100, 0x100, 0x100, 0x100), ==, 0u);

	GO_COLOR_TO_RGBA (0x12345678u, &r, &g, &b, &a);
	g_assert_cmphex (r, ==, 0x12);
	g_assert_cmphex (g, ==, 0x34);
	g_assert_cmphex (b, ==, 0x56);
	g_assert_cmphex (a, ==, 0x78);

	GO_COLOR_TO_RGB (0xaabbccddu, &r, &g, &b);
	g_assert_cmphex (r, ==, 0xaa);
	g_assert_cmphex (g, ==, 0xbb);
	g_assert_cmphex (b, ==, 0xcc);

	assert_close (GO_COLOR_DOUBLE_R (GO_COLOR_RED), 1.0);
	assert_close (GO_COLOR_DOUBLE_G (GO_COLOR_RED), 0.0);
	assert_close (GO_COLOR_DOUBLE_B (GO_COLOR_BLUE), 1.0);
	assert_close (GO_COLOR_DOUBLE_A (GO_COLOR_RED), 1.0);
	assert_close (GO_COLOR_DOUBLE_A (GO_COLOR_FROM_RGBA (0, 0, 0, 0)), 0.0);
	assert_close (GO_COLOR_DOUBLE_G (GO_COLOR_FROM_RGB (0, 51, 0)), 0.2);
}

static void
test_color_change_macros (void)
{
	GOColor c = 0x12345678u;

	g_assert_cmphex (GO_COLOR_CHANGE_R (c, 0xab), ==, 0xab345678u);
	g_assert_cmphex (GO_COLOR_CHANGE_G (c, 0xab), ==, 0x12ab5678u);
	g_assert_cmphex (GO_COLOR_CHANGE_B (c, 0xab), ==, 0x1234ab78u);
	g_assert_cmphex (GO_COLOR_CHANGE_A (c, 0xab), ==, 0x123456abu);

	g_assert_cmphex (GO_COLOR_CHANGE_A (c, 0), ==, 0x12345600u);
	g_assert_cmphex (GO_COLOR_CHANGE_A (c, 0xff), ==, 0x123456ffu);
	g_assert_cmphex (GO_COLOR_CHANGE_G (c, 0), ==, 0x12005678u);
	g_assert_cmphex (GO_COLOR_CHANGE_B (c, 0xff), ==, 0x1234ff78u);
}

static void
test_color_interpolate (void)
{
	GOColor c1 = GO_COLOR_FROM_RGBA (0, 100, 200, 0);
	GOColor c2 = GO_COLOR_FROM_RGBA (100, 200, 0, 100);

	g_assert_cmphex (GO_COLOR_INTERPOLATE (c1, c2, 0.0), ==, c1);
	g_assert_cmphex (GO_COLOR_INTERPOLATE (c1, c2, 1.0), ==, c2);
	g_assert_cmphex (GO_COLOR_INTERPOLATE (c1, c2, 0.5), ==,
			 GO_COLOR_FROM_RGBA (50, 150, 100, 50));
	g_assert_cmphex (GO_COLOR_INTERPOLATE (c1, c2, 0.25), ==,
			 GO_COLOR_FROM_RGBA (25, 125, 150, 25));

	g_assert_cmphex (GO_COLOR_INTERPOLATE (GO_COLOR_BLACK, GO_COLOR_WHITE, 0.0),
			 ==, GO_COLOR_BLACK);
	g_assert_cmphex (GO_COLOR_INTERPOLATE (GO_COLOR_BLACK, GO_COLOR_WHITE, 1.0),
			 ==, GO_COLOR_WHITE);

	/* Interpolating a color with itself is the identity */
	g_assert_cmphex (GO_COLOR_INTERPOLATE (c1, c1, 0.37), ==, c1);

	g_assert_cmpint (GO_COLOR_MONO_INTERPOLATE (10, 20, 0.5), ==, 15);
	g_assert_cmpint (GO_COLOR_MONO_INTERPOLATE (20, 10, 0.5), ==, 15);
	g_assert_cmpint (GO_COLOR_MONO_INTERPOLATE (0, 255, 0.5), ==, 128); /* round to even */
}

static void
test_color_as_str (void)
{
	char *s;

	s = go_color_as_str (0x12345678u);
	g_assert_cmpstr (s, ==, "12:34:56:78");
	g_free (s);

	s = go_color_as_str (GO_COLOR_BLACK);
	g_assert_cmpstr (s, ==, "0:0:0:FF");
	g_free (s);

	s = go_color_as_str (GO_COLOR_WHITE);
	g_assert_cmpstr (s, ==, "FF:FF:FF:FF");
	g_free (s);

	s = go_color_as_str (0);
	g_assert_cmpstr (s, ==, "0:0:0:0");
	g_free (s);

	s = go_color_as_str (0x0a0b0c0du);
	g_assert_cmpstr (s, ==, "A:B:C:D");
	g_free (s);
}

static void
test_color_from_str (void)
{
	GOColor c = 12345;

	g_assert_true (go_color_from_str ("12:34:56:78", &c));
	g_assert_cmphex (c, ==, 0x12345678u);

	/* Hex digits are case insensitive; leading zeros are optional */
	g_assert_true (go_color_from_str ("ab:CD:eF:1", &c));
	g_assert_cmphex (c, ==, 0xabcdef01u);

	g_assert_true (go_color_from_str ("0:0:0:0", &c));
	g_assert_cmphex (c, ==, 0u);

	g_assert_true (go_color_from_str ("FF:FF:FF:FF", &c));
	g_assert_cmphex (c, ==, 0xffffffffu);

	/* Round trip through go_color_as_str */
	{
		const GOColor samples[] = {
			0, 0xffffffffu, 0x01020304u, 0x80000000u, 0x000000ffu,
			0xdeadbeefu, GO_COLOR_RED, GO_COLOR_VIOLET
		};
		unsigned i;
		for (i = 0; i < G_N_ELEMENTS (samples); i++) {
			char *s = go_color_as_str (samples[i]);
			c = 1;
			g_assert_true (go_color_from_str (s, &c));
			g_assert_cmphex (c, ==, samples[i]);
			g_free (s);
		}
	}

	/* Anything GdkRGBA understands is accepted too */
	g_assert_true (go_color_from_str ("red", &c));
	g_assert_cmphex (c, ==, GO_COLOR_RED);
	g_assert_true (go_color_from_str ("#00ff00", &c));
	g_assert_cmphex (c, ==, GO_COLOR_GREEN);
	g_assert_true (go_color_from_str ("rgb(0,0,255)", &c));
	g_assert_cmphex (c, ==, GO_COLOR_BLUE);
	g_assert_true (go_color_from_str ("rgba(255,255,0,0.5)", &c));
	g_assert_cmphex (GO_COLOR_UINT_R (c), ==, 0xff);
	g_assert_cmphex (GO_COLOR_UINT_G (c), ==, 0xff);
	g_assert_cmphex (GO_COLOR_UINT_B (c), ==, 0x00);
	g_assert_cmphex (GO_COLOR_UINT_A (c), ==, 0x80);

	/* Failures set the result to 0 */
	c = 12345;
	g_assert_false (go_color_from_str ("not a color", &c));
	g_assert_cmphex (c, ==, 0u);
	c = 12345;
	g_assert_false (go_color_from_str ("", &c));
	g_assert_cmphex (c, ==, 0u);
	c = 12345;
	g_assert_false (go_color_from_str ("1:2:3", &c));
	g_assert_cmphex (c, ==, 0u);
	c = 12345;
	g_assert_false (go_color_from_str ("zz:00:00:00", &c));
	g_assert_cmphex (c, ==, 0u);
}

static void
test_color_to_pango (void)
{
	PangoAttrColor *attr;

	attr = (PangoAttrColor *) go_color_to_pango (0x80ff0010u, TRUE);
	g_assert_nonnull (attr);
	g_assert_cmpint (attr->attr.klass->type, ==, PANGO_ATTR_FOREGROUND);
	g_assert_cmphex (attr->color.red, ==, 0x8080);
	g_assert_cmphex (attr->color.green, ==, 0xffff);
	g_assert_cmphex (attr->color.blue, ==, 0x0000);
	pango_attribute_destroy ((PangoAttribute *)attr);

	attr = (PangoAttrColor *) go_color_to_pango (0x01020304u, FALSE);
	g_assert_nonnull (attr);
	g_assert_cmpint (attr->attr.klass->type, ==, PANGO_ATTR_BACKGROUND);
	g_assert_cmphex (attr->color.red, ==, 0x0101);
	g_assert_cmphex (attr->color.green, ==, 0x0202);
	g_assert_cmphex (attr->color.blue, ==, 0x0303);
	pango_attribute_destroy ((PangoAttribute *)attr);

	/* Alpha does not enter into it */
	attr = (PangoAttrColor *) go_color_to_pango (GO_COLOR_WHITE, TRUE);
	g_assert_cmphex (attr->color.red, ==, 0xffff);
	g_assert_cmphex (attr->color.green, ==, 0xffff);
	g_assert_cmphex (attr->color.blue, ==, 0xffff);
	pango_attribute_destroy ((PangoAttribute *)attr);

	attr = (PangoAttrColor *) go_color_to_pango (GO_COLOR_BLACK, TRUE);
	g_assert_cmphex (attr->color.red, ==, 0);
	g_assert_cmphex (attr->color.green, ==, 0);
	g_assert_cmphex (attr->color.blue, ==, 0);
	pango_attribute_destroy ((PangoAttribute *)attr);
}

#ifdef GOFFICE_WITH_GTK
static void
test_color_gdk_rgba (void)
{
	GdkRGBA rgba;
	GOColor c, res;
	int v;

	g_assert_true (go_color_to_gdk_rgba (GO_COLOR_RED, &rgba) == &rgba);
	assert_close (rgba.red, 1.0);
	assert_close (rgba.green, 0.0);
	assert_close (rgba.blue, 0.0);
	assert_close (rgba.alpha, 1.0);

	go_color_to_gdk_rgba (GO_COLOR_FROM_RGBA (51, 102, 153, 0), &rgba);
	assert_close (rgba.red, 0.2);
	assert_close (rgba.green, 0.4);
	assert_close (rgba.blue, 0.6);
	assert_close (rgba.alpha, 0.0);

	rgba.red = 1; rgba.green = 0; rgba.blue = 0; rgba.alpha = 1;
	c = go_color_from_gdk_rgba (&rgba, &res);
	g_assert_cmphex (c, ==, GO_COLOR_RED);
	g_assert_cmphex (res, ==, GO_COLOR_RED);

	/* The result pointer is optional */
	g_assert_cmphex (go_color_from_gdk_rgba (&rgba, NULL), ==, GO_COLOR_RED);

	/* Out of range components are clamped */
	rgba.red = 2; rgba.green = -1; rgba.blue = 0.5; rgba.alpha = 100;
	c = go_color_from_gdk_rgba (&rgba, NULL);
	g_assert_cmphex (GO_COLOR_UINT_R (c), ==, 255);
	g_assert_cmphex (GO_COLOR_UINT_G (c), ==, 0);
	g_assert_cmphex (GO_COLOR_UINT_B (c), ==, 128);
	g_assert_cmphex (GO_COLOR_UINT_A (c), ==, 255);

	/* Every 8-bit value must survive a round trip */
	for (v = 0; v < 256; v++) {
		GOColor orig = GO_COLOR_FROM_RGBA (v, 255 - v, v, 255 - v);
		go_color_to_gdk_rgba (orig, &rgba);
		g_assert_cmphex (go_color_from_gdk_rgba (&rgba, NULL), ==, orig);
	}

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
			       "*go_color_from_gdk_rgba*");
	g_assert_cmphex (go_color_from_gdk_rgba (NULL, NULL), ==, 0);
	g_test_assert_expected_messages ();
}
#endif

static void
register_color_tests (void)
{
	g_test_add_func ("/utils/go-color/macros", test_color_macros);
	g_test_add_func ("/utils/go-color/change-macros", test_color_change_macros);
	g_test_add_func ("/utils/go-color/interpolate", test_color_interpolate);
	g_test_add_func ("/utils/go-color/as_str", test_color_as_str);
	g_test_add_func ("/utils/go-color/from_str", test_color_from_str);
	g_test_add_func ("/utils/go-color/to_pango", test_color_to_pango);
#ifdef GOFFICE_WITH_GTK
	g_test_add_func ("/utils/go-color/gdk_rgba", test_color_gdk_rgba);
#endif
}

/* ------------------------------------------------------------------ */
/* go-unit.c                                                           */
/* ------------------------------------------------------------------ */

static void
test_unit_builtin (void)
{
	int id, j;

	for (id = 0; id < GO_UNIT_MAX; id++) {
		const GoUnit *u = go_unit_get ((GoUnitId)id);
		const char *sym;
		char *upper;

		g_assert_nonnull (u);
		sym = go_unit_get_symbol (u);
		g_assert_nonnull (sym);
		g_assert_cmpuint (strlen (sym), >, 0);
		g_assert_cmpint (go_unit_get_id (u), ==, id);

		/* Lookup by symbol finds the very same object */
		g_assert_true (go_unit_get_from_symbol (sym) == u);

		/* Symbols are distinct and case sensitive */
		for (j = 0; j < id; j++)
			g_assert_cmpstr (sym, !=, go_unit_get_symbol (go_unit_get ((GoUnitId)j)));
		upper = g_ascii_strup (sym, -1);
		if (strcmp (upper, sym) != 0)
			g_assert_null (go_unit_get_from_symbol (upper));
		g_free (upper);
	}

	g_assert_null (go_unit_get (GO_UNIT_UNKNOWN));
	g_assert_null (go_unit_get ((GoUnitId)-5));
	g_assert_null (go_unit_get ((GoUnitId)(GO_UNIT_MAX + 1000)));

	g_assert_null (go_unit_get_from_symbol ("no-such-unit"));
	g_assert_null (go_unit_get_from_symbol (""));
}

static void
test_unit_null_args (void)
{
	const GoUnit *m = go_unit_get (GO_UNIT_METER);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
			       "*go_unit_get_symbol*");
	g_assert_null (go_unit_get_symbol (NULL));
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
			       "*go_unit_get_id*");
	g_assert_cmpint (go_unit_get_id (NULL), ==, GO_UNIT_UNKNOWN);
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
			       "*go_unit_convert*");
	g_assert_true (isnan (go_unit_convert (NULL, m, 1)));
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
			       "*go_unit_convert*");
	g_assert_true (isnan (go_unit_convert (m, NULL, 1)));
	g_test_assert_expected_messages ();
}

static void
test_unit_convert (void)
{
	const GoUnit *m = go_unit_get (GO_UNIT_METER);
	const GoUnit *cm = go_unit_get (GO_UNIT_CENTIMETER);
	const GoUnit *in = go_unit_get (GO_UNIT_INCH);
	const GoUnit *pt = go_unit_get (GO_UNIT_POINT);

	assert_close (go_unit_convert (m, cm, 1), 100);
	assert_close (go_unit_convert (cm, m, 250), 2.5);
	assert_close (go_unit_convert (in, cm, 1), 2.54);
	assert_close (go_unit_convert (cm, in, 2.54), 1);
	assert_close (go_unit_convert (in, pt, 1), 72);
	assert_close (go_unit_convert (pt, in, 72), 1);
	assert_close (go_unit_convert (m, in, 0.0254), 1);
	assert_close (go_unit_convert (pt, cm, 72), 2.54);

	/* Identity, zero, negative */
	assert_close (go_unit_convert (m, m, 42.5), 42.5);
	assert_close (go_unit_convert (in, in, -3), -3);
	assert_near (go_unit_convert (m, in, 0), 0);
	assert_close (go_unit_convert (m, cm, -1.5), -150);

	/* Round trips */
	assert_close (go_unit_convert (cm, in, go_unit_convert (in, cm, 7.25)), 7.25);
	assert_close (go_unit_convert (pt, m, go_unit_convert (m, pt, 0.123)), 0.123);
}

static void
test_unit_define (void)
{
	const GoUnit *m = go_unit_get (GO_UNIT_METER);
	const GoUnit *furlong, *furlong2, *second, *minute;

	furlong = go_unit_define ("furlong", "L", 201.168);
	g_assert_nonnull (furlong);
	g_assert_cmpstr (go_unit_get_symbol (furlong), ==, "furlong");
	g_assert_cmpint (go_unit_get_id (furlong), >=, GO_UNIT_MAX);

	/* Can be found again by id and by symbol */
	g_assert_true (go_unit_get (go_unit_get_id (furlong)) == furlong);
	g_assert_true (go_unit_get_from_symbol ("furlong") == furlong);

	/* Redefining returns the existing unit unchanged */
	furlong2 = go_unit_define ("furlong", "L", 1);
	g_assert_true (furlong2 == furlong);
	assert_close (go_unit_convert (furlong, m, 1), 201.168);

	/* Defining a builtin returns the builtin */
	g_assert_true (go_unit_define ("m", "L", 5) == m);
	assert_close (go_unit_convert (m, go_unit_get (GO_UNIT_CENTIMETER), 1), 100);

	/* Conversion between builtin and user-defined units */
	assert_close (go_unit_convert (furlong, m, 2), 402.336);
	assert_close (go_unit_convert (m, furlong, 201.168), 1);

	/* Different dimensions don't convert */
	second = go_unit_define ("s", "T", 1);
	minute = go_unit_define ("min", "T", 60);
	g_assert_cmpint (go_unit_get_id (second), !=, go_unit_get_id (furlong));
	g_assert_cmpint (go_unit_get_id (minute), !=, go_unit_get_id (second));
	assert_close (go_unit_convert (minute, second, 2), 120);
	g_assert_true (isnan (go_unit_convert (m, second, 1)));
	g_assert_true (isnan (go_unit_convert (second, furlong, 1)));
	g_assert_true (isnan (go_unit_convert (second, m, 0)));

	/* Ids are handed out consecutively */
	{
		const GoUnit *a = go_unit_define ("test-a", "X", 1);
		const GoUnit *b = go_unit_define ("test-b", "X", 2);
		g_assert_cmpint (go_unit_get_id (b), ==, go_unit_get_id (a) + 1);
	}

	/* Id just past the last defined one doesn't exist */
	{
		const GoUnit *last = go_unit_define ("test-last", "X", 3);
		g_assert_null (go_unit_get (go_unit_get_id (last) + 1));
	}
}

static void
test_unit_type (void)
{
	const GoUnit *m = go_unit_get (GO_UNIT_METER);
	GType t = go_unit_get_type ();

	g_assert_true (G_TYPE_IS_BOXED (t));
	g_assert_cmpstr (g_type_name (t), ==, "GoUnit");
	g_assert_true (go_unit_get_type () == t);

	/* Units are static objects; "copying" returns the same pointer */
	g_assert_true (g_boxed_copy (t, m) == m);
	g_boxed_free (t, (gpointer)m);
	g_assert_cmpstr (go_unit_get_symbol (m), ==, "m");
}

static void
register_unit_tests (void)
{
	/* Order matters: later tests define units that would be visible to
	   earlier ones if the order were different. */
	g_test_add_func ("/utils/go-unit/builtin", test_unit_builtin);
	g_test_add_func ("/utils/go-unit/null_args", test_unit_null_args);
	g_test_add_func ("/utils/go-unit/convert", test_unit_convert);
	g_test_add_func ("/utils/go-unit/define", test_unit_define);
	g_test_add_func ("/utils/go-unit/type", test_unit_type);
}

/* ------------------------------------------------------------------ */
/* datetime.c                                                          */
/* ------------------------------------------------------------------ */

static const GODateConventions conv1900 = { FALSE };
static const GODateConventions conv1904 = { TRUE };

static void
set_date (GDate *d, int y, int m, int day)
{
	g_date_clear (d, 1);
	g_date_set_dmy (d, day, m, y);
	g_assert_true (g_date_valid (d));
}

static void
assert_date (const GDate *d, int y, int m, int day)
{
	g_assert_true (g_date_valid (d));
	g_assert_cmpint (g_date_get_year (d), ==, y);
	g_assert_cmpint (g_date_get_month (d), ==, m);
	g_assert_cmpint (g_date_get_day (d), ==, day);
}

static int
serial_of (int y, int m, int day, const GODateConventions *conv)
{
	GDate d;
	set_date (&d, y, m, day);
	return go_date_g_to_serial (&d, conv);
}

static void
test_datetime_g_to_serial_1900 (void)
{
	/* Reference values from Excel */
	g_assert_cmpint (serial_of (1900, 1, 1, &conv1900), ==, 1);
	g_assert_cmpint (serial_of (1900, 1, 31, &conv1900), ==, 31);
	g_assert_cmpint (serial_of (1900, 2, 1, &conv1900), ==, 32);
	g_assert_cmpint (serial_of (1900, 2, 28, &conv1900), ==, 59);
	/* Excel's phantom 1900-02-29 is serial 60 */
	g_assert_cmpint (serial_of (1900, 3, 1, &conv1900), ==, 61);
	g_assert_cmpint (serial_of (1900, 12, 31, &conv1900), ==, 366);
	g_assert_cmpint (serial_of (1901, 1, 1, &conv1900), ==, 367);
	g_assert_cmpint (serial_of (1970, 1, 1, &conv1900), ==, 25569);
	g_assert_cmpint (serial_of (1999, 12, 31, &conv1900), ==, 36525);
	g_assert_cmpint (serial_of (2000, 1, 1, &conv1900), ==, 36526);
	g_assert_cmpint (serial_of (2000, 2, 29, &conv1900), ==, 36585);
	g_assert_cmpint (serial_of (2008, 1, 1, &conv1900), ==, 39448);
	g_assert_cmpint (serial_of (9999, 12, 31, &conv1900), ==, 2958465);

	/* NULL means 1900 */
	{
		GDate d;
		set_date (&d, 2000, 1, 1);
		g_assert_cmpint (go_date_g_to_serial (&d, NULL), ==, 36526);
	}

	/* Day before the epoch */
	g_assert_cmpint (serial_of (1899, 12, 31, &conv1900), ==, 0);
}

static void
test_datetime_g_to_serial_1904 (void)
{
	g_assert_cmpint (serial_of (1904, 1, 1, &conv1904), ==, 0);
	g_assert_cmpint (serial_of (1904, 1, 2, &conv1904), ==, 1);
	g_assert_cmpint (serial_of (1904, 3, 1, &conv1904), ==, 60);
	g_assert_cmpint (serial_of (1970, 1, 1, &conv1904), ==, 24107);
	g_assert_cmpint (serial_of (2000, 1, 1, &conv1904), ==, 35064);
	g_assert_cmpint (serial_of (9999, 12, 31, &conv1904), ==, 2957003);
	g_assert_cmpint (serial_of (1903, 12, 31, &conv1904), ==, -1);

	/* The two conventions differ by 1462 days after Feb 1900 */
	g_assert_cmpint (serial_of (2020, 5, 17, &conv1900) -
			 serial_of (2020, 5, 17, &conv1904), ==, 1462);
}

static void
test_datetime_serial_to_g (void)
{
	GDate d;

	go_date_serial_to_g (&d, 1, &conv1900);
	assert_date (&d, 1900, 1, 1);
	go_date_serial_to_g (&d, 59, &conv1900);
	assert_date (&d, 1900, 2, 28);
	go_date_serial_to_g (&d, 61, &conv1900);
	assert_date (&d, 1900, 3, 1);
	go_date_serial_to_g (&d, 36526, &conv1900);
	assert_date (&d, 2000, 1, 1);
	go_date_serial_to_g (&d, 36526, NULL);
	assert_date (&d, 2000, 1, 1);
	go_date_serial_to_g (&d, 2958465, &conv1900);
	assert_date (&d, 9999, 12, 31);
	go_date_serial_to_g (&d, 0, &conv1900);
	assert_date (&d, 1899, 12, 31);

	/* The phantom leap day has no GDate */
	go_date_serial_to_g (&d, 60, &conv1900);
	g_assert_false (g_date_valid (&d));

	/* Past 9999-12-31 */
	go_date_serial_to_g (&d, 2958466, &conv1900);
	g_assert_false (g_date_valid (&d));

	go_date_serial_to_g (&d, 0, &conv1904);
	assert_date (&d, 1904, 1, 1);
	go_date_serial_to_g (&d, 60, &conv1904);
	assert_date (&d, 1904, 3, 1);
	go_date_serial_to_g (&d, 35064, &conv1904);
	assert_date (&d, 2000, 1, 1);
	go_date_serial_to_g (&d, 2957003, &conv1904);
	assert_date (&d, 9999, 12, 31);
	go_date_serial_to_g (&d, 2957004, &conv1904);
	g_assert_false (g_date_valid (&d));
}

static void
test_datetime_serial_roundtrip (void)
{
	int serial;

	for (serial = 1; serial <= 2958465; serial += 37) {
		GDate d;
		if (serial == 60)
			continue;
		go_date_serial_to_g (&d, serial, &conv1900);
		g_assert_true (g_date_valid (&d));
		g_assert_cmpint (go_date_g_to_serial (&d, &conv1900), ==, serial);
	}
	/* Make sure we hit the interesting ones too */
	for (serial = 55; serial <= 65; serial++) {
		GDate d;
		if (serial == 60)
			continue;
		go_date_serial_to_g (&d, serial, &conv1900);
		g_assert_cmpint (go_date_g_to_serial (&d, &conv1900), ==, serial);
	}
	for (serial = 0; serial <= 2957003; serial += 37) {
		GDate d;
		go_date_serial_to_g (&d, serial, &conv1904);
		g_assert_true (g_date_valid (&d));
		g_assert_cmpint (go_date_g_to_serial (&d, &conv1904), ==, serial);
	}
}

static void
test_datetime_raw_to_serial (void)
{
	g_assert_cmpint (go_date_serial_raw_to_serial (36526.0), ==, 36526);
	g_assert_cmpint (go_date_serial_raw_to_serial (36526.5), ==, 36526);
	g_assert_cmpint (go_date_serial_raw_to_serial (36526.75), ==, 36526);
	g_assert_cmpint (go_date_serial_raw_to_serial (36526.99), ==, 36526);
	/* Within half a second of the next midnight rounds up */
	g_assert_cmpint (go_date_serial_raw_to_serial (36527 - 0.1 / 86400), ==, 36527);
	g_assert_cmpint (go_date_serial_raw_to_serial (36527 - 2.0 / 86400), ==, 36526);
	/* Just after midnight stays */
	g_assert_cmpint (go_date_serial_raw_to_serial (36526 + 0.1 / 86400), ==, 36526);
	g_assert_cmpint (go_date_serial_raw_to_serial (0.0), ==, 0);
	g_assert_cmpint (go_date_serial_raw_to_serial (-0.5), ==, -1);
	g_assert_cmpint (go_date_serial_raw_to_serial (-1.0), ==, -1);
}

static void
test_datetime_raw_to_seconds (void)
{
	g_assert_cmpint (go_date_serial_raw_to_seconds (0.0), ==, 0);
	g_assert_cmpint (go_date_serial_raw_to_seconds (0.25), ==, 6 * 3600);
	g_assert_cmpint (go_date_serial_raw_to_seconds (0.5), ==, 12 * 3600);
	g_assert_cmpint (go_date_serial_raw_to_seconds (0.75), ==, 18 * 3600);
	g_assert_cmpint (go_date_serial_raw_to_seconds (1.0 / 86400), ==, 1);
	g_assert_cmpint (go_date_serial_raw_to_seconds (45296.0 / 86400), ==, 45296);
	g_assert_cmpint (go_date_serial_raw_to_seconds (86399.0 / 86400), ==, 86399);

	/* The date part is ignored */
	g_assert_cmpint (go_date_serial_raw_to_seconds (36526.5), ==, 12 * 3600);
	g_assert_cmpint (go_date_serial_raw_to_seconds (36526 + 45296.0 / 86400), ==, 45296);

	/* Rounds to the nearest second */
	g_assert_cmpint (go_date_serial_raw_to_seconds (0.5 - 0.1 / 86400), ==, 12 * 3600);
	g_assert_cmpint (go_date_serial_raw_to_seconds (0.5 + 0.1 / 86400), ==, 12 * 3600);
	/* ... including up to the next midnight */
	g_assert_cmpint (go_date_serial_raw_to_seconds (1 - 0.1 / 86400), ==, 0);
}

/* Build a time_t for the given local date/time.  Dates and times used in
   the tests are chosen well away from any DST transition. */
static time_t
local_timet (int y, int m, int d, int hh, int mm, int ss)
{
	struct tm tm;
	memset (&tm, 0, sizeof (tm));
	tm.tm_year = y - 1900;
	tm.tm_mon = m - 1;
	tm.tm_mday = d;
	tm.tm_hour = hh;
	tm.tm_min = mm;
	tm.tm_sec = ss;
	tm.tm_isdst = -1;
	return mktime (&tm);
}

static void
test_datetime_timet (void)
{
	time_t t = local_timet (2000, 1, 15, 12, 0, 0);
	time_t t2;
	struct tm *tm;

	assert_close (go_date_timet_to_serial_raw (t, &conv1900), 36540.5);
	assert_close (go_date_timet_to_serial_raw (t, NULL), 36540.5);
	assert_close (go_date_timet_to_serial_raw (t, &conv1904), 36540.5 - 1462);

	g_assert_cmpint (go_date_timet_to_serial (t, &conv1900), ==, 36540);
	g_assert_cmpint (go_date_timet_to_serial (t, &conv1904), ==, 36540 - 1462);

	t = local_timet (2000, 1, 15, 12, 34, 56);
	assert_close (go_date_timet_to_serial_raw (t, &conv1900),
		      36540 + 45296.0 / 86400);
	g_assert_cmpint (go_date_timet_to_seconds (t), ==, 45296);

	t = local_timet (2000, 1, 15, 0, 0, 0);
	g_assert_cmpint (go_date_timet_to_seconds (t), ==, 0);
	g_assert_cmpint (go_date_timet_to_serial (t, &conv1900), ==, 36540);

	t = local_timet (2000, 1, 15, 23, 59, 59);
	g_assert_cmpint (go_date_timet_to_seconds (t), ==, 86399);
	g_assert_cmpint (go_date_timet_to_serial (t, &conv1900), ==, 36540);

	/* serial -> time_t gives local midnight */
	t2 = go_date_serial_to_timet (36540, &conv1900);
	g_assert_cmpint (t2, ==, local_timet (2000, 1, 15, 0, 0, 0));
	tm = localtime (&t2);
	g_assert_cmpint (tm->tm_year, ==, 100);
	g_assert_cmpint (tm->tm_mon, ==, 0);
	g_assert_cmpint (tm->tm_mday, ==, 15);
	g_assert_cmpint (tm->tm_hour, ==, 0);

	t2 = go_date_serial_to_timet (36540 - 1462, &conv1904);
	g_assert_cmpint (t2, ==, local_timet (2000, 1, 15, 0, 0, 0));

	/* Round trip */
	t = local_timet (2013, 7, 20, 15, 0, 0);
	g_assert_cmpint (go_date_serial_to_timet (go_date_timet_to_serial (t, &conv1900), &conv1900),
			 ==, local_timet (2013, 7, 20, 0, 0, 0));

	/* Invalid serials */
	g_assert_cmpint (go_date_serial_to_timet (60, &conv1900), ==, (time_t)-1);
	g_assert_cmpint (go_date_serial_to_timet (2958466, &conv1900), ==, (time_t)-1);
	g_assert_cmpint (go_date_serial_to_timet (2957004, &conv1904), ==, (time_t)-1);
}

static void
test_datetime_months_between (void)
{
	GDate a, b;

	set_date (&a, 2000, 1, 15);
	set_date (&b, 2000, 1, 15);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 0);

	set_date (&b, 2000, 3, 15);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 2);
	g_assert_cmpint (go_date_g_months_between (&b, &a), ==, -2);

	set_date (&b, 2000, 3, 14);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 1);
	set_date (&b, 2000, 3, 16);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 2);

	/* Across years */
	set_date (&b, 2001, 1, 15);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 12);
	set_date (&b, 2002, 2, 20);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 25);
	set_date (&b, 1999, 12, 15);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, -1);

	/* End of month: Mar 31 -> Apr 30 is not a full month */
	set_date (&a, 2000, 3, 31);
	set_date (&b, 2000, 4, 30);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 0);
	set_date (&a, 2000, 1, 31);
	set_date (&b, 2000, 2, 29);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 0);
	set_date (&b, 2000, 3, 31);
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 2);

	/* Invalid dates */
	g_date_clear (&b, 1);
	set_date (&a, 2000, 1, 1);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*g_date_valid*");
	g_assert_cmpint (go_date_g_months_between (&a, &b), ==, 0);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*g_date_valid*");
	g_assert_cmpint (go_date_g_months_between (&b, &a), ==, 0);
	g_test_assert_expected_messages ();
}

static void
test_datetime_years_between (void)
{
	GDate a, b;

	set_date (&a, 2000, 2, 29);
	set_date (&b, 2000, 2, 29);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, 0);

	set_date (&b, 2001, 2, 28);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, 0);
	set_date (&b, 2001, 3, 1);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, 1);
	set_date (&b, 2004, 2, 29);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, 4);
	set_date (&b, 2004, 2, 28);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, 3);

	/* Truncates towards zero in both directions */
	set_date (&a, 2001, 3, 1);
	set_date (&b, 2000, 2, 29);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, -1);
	set_date (&a, 2004, 1, 1);
	set_date (&b, 2000, 1, 1);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, -4);
	set_date (&b, 2000, 6, 1);
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, -3);

	g_date_clear (&b, 1);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*g_date_valid*");
	g_assert_cmpint (go_date_g_years_between (&a, &b), ==, 0);
	g_test_assert_expected_messages ();
}

static void
test_datetime_weeknum (void)
{
	GDate d;

	/* 2006-01-01 is a Sunday */
	set_date (&d, 2006, 1, 1);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY), ==, 1);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_MONDAY), ==, 1);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 52);

	set_date (&d, 2006, 1, 2);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY), ==, 1);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_MONDAY), ==, 2);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 1);

	set_date (&d, 2006, 1, 7);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY), ==, 1);

	set_date (&d, 2006, 1, 8);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY), ==, 2);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_MONDAY), ==, 2);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 1);

	set_date (&d, 2006, 12, 31);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY), ==, 53);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_MONDAY), ==, 53);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 52);

	/* 2007-01-01 is a Monday */
	set_date (&d, 2007, 1, 1);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY), ==, 1);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_MONDAY), ==, 1);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 1);
	set_date (&d, 2007, 1, 7);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY), ==, 2);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_MONDAY), ==, 1);

	/* ISO year boundaries */
	set_date (&d, 2008, 12, 29);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 1);
	set_date (&d, 2010, 1, 3);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 53);
	set_date (&d, 2010, 1, 4);
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, 1);

	/* Invalid input */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_weeknum*");
	g_assert_cmpint (go_date_weeknum (&d, 3), ==, -1);
	g_test_assert_expected_messages ();
	g_date_clear (&d, 1);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_weeknum*");
	g_assert_cmpint (go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO), ==, -1);
	g_test_assert_expected_messages ();
}

static void
test_datetime_weeknum_consistency (void)
{
	/* Week numbers never go backwards within a year, increase by at most
	   one per day, and increase exactly on the week's first day. */
	int year;

	for (year = 1990; year <= 2030; year++) {
		GDate d;
		int prev_sun = 0, prev_mon = 0;
		int doy;

		set_date (&d, year, 1, 1);
		for (doy = 1; doy <= 365; doy++, g_date_add_days (&d, 1)) {
			int sun = go_date_weeknum (&d, GO_WEEKNUM_METHOD_SUNDAY);
			int mon = go_date_weeknum (&d, GO_WEEKNUM_METHOD_MONDAY);
			int iso = go_date_weeknum (&d, GO_WEEKNUM_METHOD_ISO);

			g_assert_cmpint (sun, >=, 1);
			g_assert_cmpint (mon, >=, 1);
			g_assert_cmpint (iso, >=, 1);
			g_assert_cmpint (iso, <=, 53);
			if (doy > 1) {
				g_assert_cmpint (sun - prev_sun, ==,
						 g_date_get_weekday (&d) == G_DATE_SUNDAY ? 1 : 0);
				g_assert_cmpint (mon - prev_mon, ==,
						 g_date_get_weekday (&d) == G_DATE_MONDAY ? 1 : 0);
			}
			prev_sun = sun;
			prev_mon = mon;
		}
	}
}

static void
test_datetime_days_between_basis (void)
{
	GDate a, b;
	int basis;

	/* Identical dates: zero for every basis */
	set_date (&a, 2020, 3, 15);
	for (basis = GO_BASIS_MSRB_30_360; basis <= GO_BASIS_MSRB_30_360_SYM; basis++)
		g_assert_cmpint (go_date_days_between_basis (&a, &a, basis), ==, 0);
	/* ... except that 30E+/360 moves an end date of the 31st to the 1st of
	   the next month, so it counts one day even from the 31st to itself. */
	set_date (&a, 2020, 3, 31);
	for (basis = GO_BASIS_MSRB_30_360; basis <= GO_BASIS_MSRB_30_360_SYM; basis++)
		g_assert_cmpint (go_date_days_between_basis (&a, &a, basis), ==,
				 basis == GO_BASIS_30Ep_360 ? 1 : 0);

	/* Actual-day bases */
	set_date (&a, 2020, 1, 31);
	set_date (&b, 2020, 3, 1);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_ACT_ACT), ==, 30);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_ACT_360), ==, 30);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_ACT_365), ==, 30);
	/* Reversed arguments negate */
	g_assert_cmpint (go_date_days_between_basis (&b, &a, GO_BASIS_ACT_ACT), ==, -30);
	set_date (&a, 2000, 1, 1);
	set_date (&b, 2001, 1, 1);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_ACT_ACT), ==, 366);

	/* US 30/360 */
	set_date (&a, 2020, 1, 31);
	set_date (&b, 2020, 3, 31);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_MSRB_30_360), ==, 60);
	set_date (&a, 2020, 1, 15);
	set_date (&b, 2020, 3, 20);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_MSRB_30_360), ==, 65);
	set_date (&a, 2020, 1, 1);
	set_date (&b, 2021, 1, 1);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_MSRB_30_360), ==, 360);
	/* Last day of Feb to last day of Feb counts as 30 -> 30 */
	set_date (&a, 2020, 2, 29);
	set_date (&b, 2021, 2, 28);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_MSRB_30_360), ==, 360);
	/* Unknown basis values fall back to US 30/360 */
	g_assert_cmpint (go_date_days_between_basis (&a, &b, (GOBasisType)99), ==, 360);
	/* Reversal */
	g_assert_cmpint (go_date_days_between_basis (&b, &a, GO_BASIS_MSRB_30_360), ==, -360);

	/* European 30E/360 */
	set_date (&a, 2020, 1, 31);
	set_date (&b, 2020, 3, 31);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_30E_360), ==, 60);
	set_date (&a, 2021, 2, 28);
	set_date (&b, 2021, 3, 31);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_30E_360), ==, 32);
	g_assert_cmpint (go_date_days_between_basis (&b, &a, GO_BASIS_30E_360), ==, -32);

	/* 30E+/360 rolls day 31 of the end date into the next month */
	set_date (&a, 2021, 1, 30);
	set_date (&b, 2021, 3, 31);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_30Ep_360), ==, 61);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_30E_360), ==, 60);
	set_date (&a, 2021, 1, 1);
	set_date (&b, 2021, 12, 31);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_30Ep_360), ==, 360);

	/* Symmetric US 30/360 */
	set_date (&a, 2021, 2, 28);
	set_date (&b, 2021, 3, 31);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_MSRB_30_360_SYM), ==, 30);
	set_date (&a, 2020, 2, 29);
	set_date (&b, 2021, 2, 28);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_MSRB_30_360_SYM), ==, 360);
	set_date (&a, 2020, 1, 31);
	set_date (&b, 2020, 3, 31);
	g_assert_cmpint (go_date_days_between_basis (&a, &b, GO_BASIS_MSRB_30_360_SYM), ==, 60);
}

static void
test_datetime_coup_cd (void)
{
	GDate settle, mat, res;

	/* Example from the Excel documentation: COUPPCD / COUPNCD */
	set_date (&settle, 2011, 1, 25);
	set_date (&mat, 2011, 11, 15);
	go_coup_cd (&res, &settle, &mat, 2, FALSE, FALSE);
	assert_date (&res, 2010, 11, 15);
	go_coup_cd (&res, &settle, &mat, 2, FALSE, TRUE);
	assert_date (&res, 2011, 5, 15);

	/* Quarterly, settlement a few days before a coupon */
	set_date (&settle, 2011, 5, 10);
	go_coup_cd (&res, &settle, &mat, 4, FALSE, FALSE);
	assert_date (&res, 2011, 2, 15);
	go_coup_cd (&res, &settle, &mat, 4, FALSE, TRUE);
	assert_date (&res, 2011, 5, 15);

	/* Annual */
	set_date (&settle, 2011, 3, 1);
	go_coup_cd (&res, &settle, &mat, 1, FALSE, FALSE);
	assert_date (&res, 2010, 11, 15);
	go_coup_cd (&res, &settle, &mat, 1, FALSE, TRUE);
	assert_date (&res, 2011, 11, 15);

	/* Multi-year span */
	set_date (&settle, 2005, 6, 1);
	set_date (&mat, 2011, 11, 15);
	go_coup_cd (&res, &settle, &mat, 2, FALSE, FALSE);
	assert_date (&res, 2005, 5, 15);
	go_coup_cd (&res, &settle, &mat, 2, FALSE, TRUE);
	assert_date (&res, 2005, 11, 15);

	/* End-of-month handling: coupons fall on the last day of the month */
	set_date (&settle, 2011, 1, 15);
	set_date (&mat, 2011, 11, 30);
	go_coup_cd (&res, &settle, &mat, 4, TRUE, FALSE);
	assert_date (&res, 2010, 11, 30);
	go_coup_cd (&res, &settle, &mat, 4, TRUE, TRUE);
	assert_date (&res, 2011, 2, 28);
	/* ... and without the flag the day of month is kept (and clipped) */
	go_coup_cd (&res, &settle, &mat, 4, FALSE, TRUE);
	assert_date (&res, 2011, 2, 28);
	go_coup_cd (&res, &settle, &mat, 4, FALSE, FALSE);
	assert_date (&res, 2010, 11, 30);
}

static void
test_datetime_coupon_days (void)
{
	GDate settle, mat;
	GoCouponConvention conv;

	/* Excel documentation example: COUPDAYBS=71, COUPDAYSNC=110,
	   COUPDAYS=181 for basis 1 (actual/actual) */
	set_date (&settle, 2011, 1, 25);
	set_date (&mat, 2011, 11, 15);
	conv.freq = 2;
	conv.basis = GO_BASIS_ACT_ACT;
	conv.eom = FALSE;
	conv.date_conv = &conv1900;
	assert_close (go_coupdaybs (&settle, &mat, &conv), 71);
	assert_close (go_coupdaysnc (&settle, &mat, &conv), 110);
	assert_close (go_coupdays (&settle, &mat, &conv), 181);

	/* 30/360 */
	conv.basis = GO_BASIS_MSRB_30_360;
	assert_close (go_coupdaybs (&settle, &mat, &conv), 70);
	assert_close (go_coupdaysnc (&settle, &mat, &conv), 110);
	assert_close (go_coupdays (&settle, &mat, &conv), 180);

	/* Fixed-length periods */
	conv.basis = GO_BASIS_ACT_360;
	assert_close (go_coupdays (&settle, &mat, &conv), 180);
	conv.basis = GO_BASIS_ACT_365;
	assert_close (go_coupdays (&settle, &mat, &conv), 182.5);
	conv.basis = GO_BASIS_30E_360;
	assert_close (go_coupdays (&settle, &mat, &conv), 180);
	conv.basis = GO_BASIS_30Ep_360;
	assert_close (go_coupdays (&settle, &mat, &conv), 180);

	conv.freq = 4;
	conv.basis = GO_BASIS_MSRB_30_360;
	assert_close (go_coupdays (&settle, &mat, &conv), 90);
	conv.basis = GO_BASIS_ACT_365;
	assert_close (go_coupdays (&settle, &mat, &conv), 365.0 / 4);
	conv.freq = 1;
	conv.basis = GO_BASIS_ACT_360;
	assert_close (go_coupdays (&settle, &mat, &conv), 360);

	/* The pieces add up: days before + days to next = days in period */
	conv.freq = 2;
	conv.basis = GO_BASIS_ACT_ACT;
	assert_close (go_coupdaybs (&settle, &mat, &conv) +
		      go_coupdaysnc (&settle, &mat, &conv),
		      go_coupdays (&settle, &mat, &conv));
}

static void
test_datetime_conventions (void)
{
	const GODateConventions *a, *b;

	g_assert_cmpint (go_date_convention_base (&conv1900), ==, 1900);
	g_assert_cmpint (go_date_convention_base (&conv1904), ==, 1904);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_convention_base*");
	g_assert_cmpint (go_date_convention_base (NULL), ==, 1900);
	g_test_assert_expected_messages ();

	a = go_date_conv_from_str ("Apple:1904");
	b = go_date_conv_from_str ("Lotus:1900");
	g_assert_nonnull (a);
	g_assert_nonnull (b);
	g_assert_true (a->use_1904);
	g_assert_false (b->use_1904);
	g_assert_null (go_date_conv_from_str ("Lotus:1904"));
	g_assert_null (go_date_conv_from_str ("apple:1904"));
	g_assert_null (go_date_conv_from_str (""));
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_conv_from_str*");
	g_assert_null (go_date_conv_from_str (NULL));
	g_test_assert_expected_messages ();

	g_assert_true (go_date_conv_equal (a, a));
	g_assert_true (go_date_conv_equal (b, &conv1900));
	g_assert_true (go_date_conv_equal (a, &conv1904));
	g_assert_false (go_date_conv_equal (a, b));
	g_assert_false (go_date_conv_equal (&conv1900, &conv1904));
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_conv_equal*");
	g_assert_false (go_date_conv_equal (NULL, b));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_conv_equal*");
	g_assert_false (go_date_conv_equal (a, NULL));
	g_test_assert_expected_messages ();
}

static void
test_datetime_conv_translate (void)
{
	/* Fixed points */
	assert_close (go_date_conv_translate (36526, &conv1900, &conv1904), 35064);
	assert_close (go_date_conv_translate (35064, &conv1904, &conv1900), 36526);
	assert_close (go_date_conv_translate (36526.25, &conv1900, &conv1904), 35064.25);

	/* Same convention: identity */
	assert_close (go_date_conv_translate (36526.5, &conv1900, &conv1900), 36526.5);
	assert_close (go_date_conv_translate (-7, &conv1904, &conv1904), -7);

	/* Non-finite values are passed through */
	g_assert_true (isnan (go_date_conv_translate (go_nan, &conv1900, &conv1904)));
	g_assert_true (go_date_conv_translate (go_pinf, &conv1900, &conv1904) == go_pinf);
	g_assert_true (go_date_conv_translate (go_ninf, &conv1904, &conv1900) == go_ninf);

	/* Translation must agree with converting through real dates, for
	   every day from 1900-01-01 to 2100-12-31 (except the phantom day). */
	{
		GDate d;
		int i;

		set_date (&d, 1900, 1, 1);
		for (i = 0; i < 73414; i++, g_date_add_days (&d, 1)) {
			int s1900 = go_date_g_to_serial (&d, &conv1900);
			int s1904 = go_date_g_to_serial (&d, &conv1904);

			g_assert_cmpfloat (go_date_conv_translate (s1900, &conv1900, &conv1904), ==, s1904);
			g_assert_cmpfloat (go_date_conv_translate (s1904, &conv1904, &conv1900), ==, s1900);
		}
	}

	/* NULL conventions are rejected, returning the input */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_conv_translate*");
	assert_close (go_date_conv_translate (5, NULL, &conv1904), 5);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_conv_translate*");
	assert_close (go_date_conv_translate (5, &conv1900, NULL), 5);
	g_test_assert_expected_messages ();
}

static void
test_datetime_names (void)
{
	/* The names come from the C library's idea of the current locale */
	int i;

	for (i = 0; i < 7; i++) {
		/* G_DATE_MONDAY is 1 ... G_DATE_SUNDAY is 7; tm_wday has Sunday as 0 */
		struct tm tm;
		char full[100], abbr[100], *s;

		memset (&tm, 0, sizeof (tm));
		tm.tm_wday = (i + 1) % 7;
		strftime (full, sizeof (full), "%A", &tm);
		strftime (abbr, sizeof (abbr), "%a", &tm);

		s = go_date_weekday_name ((GDateWeekday)(i + 1), FALSE);
		g_assert_cmpstr (s, ==, full);
		g_free (s);
		s = go_date_weekday_name ((GDateWeekday)(i + 1), TRUE);
		g_assert_cmpstr (s, ==, abbr);
		g_free (s);
	}

	for (i = 0; i < 12; i++) {
		struct tm tm;
		char full[100], abbr[100], *s;

		memset (&tm, 0, sizeof (tm));
		tm.tm_mon = i;
		tm.tm_mday = 15;
		strftime (full, sizeof (full), "%B", &tm);
		strftime (abbr, sizeof (abbr), "%b", &tm);

		s = go_date_month_name ((GDateMonth)(i + 1), FALSE);
		g_assert_cmpstr (s, ==, full);
		g_free (s);
		s = go_date_month_name ((GDateMonth)(i + 1), TRUE);
		g_assert_cmpstr (s, ==, abbr);
		g_free (s);
	}

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_weekday_name*");
	g_assert_null (go_date_weekday_name (G_DATE_BAD_WEEKDAY, FALSE));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_weekday_name*");
	g_assert_null (go_date_weekday_name ((GDateWeekday)8, TRUE));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_month_name*");
	g_assert_null (go_date_month_name (G_DATE_BAD_MONTH, FALSE));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_date_month_name*");
	g_assert_null (go_date_month_name ((GDateMonth)13, TRUE));
	g_test_assert_expected_messages ();
}

static void
test_datetime_conventions_type (void)
{
	GType t = go_date_conventions_get_type ();
	GODateConventions *copy;

	g_assert_true (G_TYPE_IS_BOXED (t));
	g_assert_cmpstr (g_type_name (t), ==, "GODateConventions");

	copy = g_boxed_copy (t, &conv1904);
	g_assert_nonnull (copy);
	g_assert_true (copy != &conv1904);
	g_assert_true (go_date_conv_equal (copy, &conv1904));
	g_assert_false (go_date_conv_equal (copy, &conv1900));
	g_boxed_free (t, copy);
}

static void
register_datetime_tests (void)
{
	g_test_add_func ("/utils/datetime/g_to_serial/1900", test_datetime_g_to_serial_1900);
	g_test_add_func ("/utils/datetime/g_to_serial/1904", test_datetime_g_to_serial_1904);
	g_test_add_func ("/utils/datetime/serial_to_g", test_datetime_serial_to_g);
	g_test_add_func ("/utils/datetime/serial_roundtrip", test_datetime_serial_roundtrip);
	g_test_add_func ("/utils/datetime/raw_to_serial", test_datetime_raw_to_serial);
	g_test_add_func ("/utils/datetime/raw_to_seconds", test_datetime_raw_to_seconds);
	g_test_add_func ("/utils/datetime/timet", test_datetime_timet);
	g_test_add_func ("/utils/datetime/months_between", test_datetime_months_between);
	g_test_add_func ("/utils/datetime/years_between", test_datetime_years_between);
	g_test_add_func ("/utils/datetime/weeknum", test_datetime_weeknum);
	g_test_add_func ("/utils/datetime/weeknum/consistency", test_datetime_weeknum_consistency);
	g_test_add_func ("/utils/datetime/days_between_basis", test_datetime_days_between_basis);
	g_test_add_func ("/utils/datetime/coup_cd", test_datetime_coup_cd);
	g_test_add_func ("/utils/datetime/coupon_days", test_datetime_coupon_days);
	g_test_add_func ("/utils/datetime/conventions", test_datetime_conventions);
	g_test_add_func ("/utils/datetime/conv_translate", test_datetime_conv_translate);
	g_test_add_func ("/utils/datetime/names", test_datetime_names);
	g_test_add_func ("/utils/datetime/conventions_type", test_datetime_conventions_type);
}

/* ------------------------------------------------------------------ */
/* go-string.c                                                         */
/* ------------------------------------------------------------------ */

static void
cb_count_matching (gpointer key, G_GNUC_UNUSED gpointer value, gpointer user)
{
	gpointer *data = user;
	const GOString *gs = key;
	if (strcmp (gs->str, data[0]) == 0)
		(*(int *)data[1])++;
}

/* Number of entries in the shared string table with the given contents */
static int
string_pool_count (const char *str)
{
	int n = 0;
	gpointer data[2] = { (gpointer)str, &n };
	go_string_foreach_base (cb_count_matching, data);
	return n;
}

static void
test_string_new (void)
{
	GOString *a, *b, *c;

	a = go_string_new ("go-string-test-new");
	g_assert_nonnull (a);
	g_assert_cmpstr (a->str, ==, "go-string-test-new");
	g_assert_cmpuint (go_string_get_len (a), ==, strlen ("go-string-test-new"));
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 1);

	/* Strings are shared: same contents gives the same object */
	b = go_string_new ("go-string-test-new");
	g_assert_true (a == b);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 2);

	/* The string is copied, not referenced */
	{
		char buf[] = "go-string-test-copy";
		c = go_string_new (buf);
		g_assert_true (c->str != buf);
		buf[0] = 'X';
		g_assert_cmpstr (c->str, ==, "go-string-test-copy");
		go_string_unref (c);
	}

	go_string_unref (a);
	go_string_unref (b);

	g_assert_null (go_string_new (NULL));

	/* Empty string is fine */
	a = go_string_new ("");
	g_assert_nonnull (a);
	g_assert_cmpstr (a->str, ==, "");
	g_assert_cmpuint (go_string_get_len (a), ==, 0);
	go_string_unref (a);

	/* UTF-8 is stored as is; the length is in bytes */
	a = go_string_new ("h\xc3\xa9llo");
	g_assert_cmpuint (go_string_get_len (a), ==, 6);
	g_assert_cmpstr (a->str, ==, "h\xc3\xa9llo");
	go_string_unref (a);
}

static void
test_string_new_len (void)
{
	GOString *a, *b, *c;

	a = go_string_new_len ("hello world", 5);
	g_assert_cmpstr (a->str, ==, "hello");
	g_assert_cmpuint (go_string_get_len (a), ==, 5);

	/* Same as the equivalent full string */
	b = go_string_new ("hello");
	g_assert_true (a == b);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 2);
	go_string_unref (b);
	go_string_unref (a);

	/* Zero length */
	a = go_string_new_len ("ignored", 0);
	g_assert_cmpstr (a->str, ==, "");
	g_assert_cmpuint (go_string_get_len (a), ==, 0);
	b = go_string_new ("");
	g_assert_true (a == b);
	go_string_unref (a);
	go_string_unref (b);

	g_assert_null (go_string_new_len (NULL, 3));

	/* Embedded NULs are part of the string: strings that only differ after
	   the NUL are different strings */
	a = go_string_new_len ("a\0b", 3);
	b = go_string_new_len ("a\0c", 3);
	c = go_string_new_len ("a\0b", 3);
	g_assert_cmpuint (go_string_get_len (a), ==, 3);
	g_assert_cmpuint (go_string_get_len (b), ==, 3);
	g_assert_true (a != b);
	g_assert_true (a == c);
	g_assert_cmpint (memcmp (a->str, "a\0b", 4), ==, 0);
	g_assert_cmpint (memcmp (b->str, "a\0c", 4), ==, 0);
	g_assert_false (go_string_equal (a, b));
	go_string_unref (a);
	go_string_unref (b);
	go_string_unref (c);
}

static void
test_string_new_nocopy (void)
{
	char *buf, *buf2;
	GOString *a, *b;

	/* Ownership of the buffer is taken */
	buf = g_strdup ("go-string-test-nocopy");
	a = go_string_new_nocopy (buf);
	g_assert_true (a->str == buf);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 1);
	g_assert_cmpuint (go_string_get_len (a), ==, strlen (buf));

	/* A second, equal buffer is freed and the existing string returned */
	buf2 = g_strdup ("go-string-test-nocopy");
	b = go_string_new_nocopy (buf2);
	g_assert_true (a == b);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 2);
	go_string_unref (b);

	/* ... also when the existing string was created with a copy */
	b = go_string_new ("go-string-test-nocopy");
	g_assert_true (a == b);
	go_string_unref (b);
	go_string_unref (a);

	g_assert_null (go_string_new_nocopy (NULL));
	g_assert_null (go_string_new_nocopy_len (NULL, 5));

	/* _len variant: the buffer is truncated by writing a terminator */
	buf = g_strdup ("abcdef");
	a = go_string_new_nocopy_len (buf, 3);
	g_assert_cmpstr (a->str, ==, "abc");
	g_assert_cmpuint (go_string_get_len (a), ==, 3);
	b = go_string_new ("abc");
	g_assert_true (a == b);
	go_string_unref (a);
	go_string_unref (b);
}

static void
test_string_refcount (void)
{
	GOString *a = go_string_new ("go-string-test-refcount");
	GOString *b;

	g_assert_cmpint (string_pool_count ("go-string-test-refcount"), ==, 1);

	b = go_string_ref (a);
	g_assert_true (a == b);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 2);
	go_string_unref (b);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 1);
	g_assert_cmpint (string_pool_count ("go-string-test-refcount"), ==, 1);

	/* Last unref removes it from the table */
	go_string_unref (a);
	g_assert_cmpint (string_pool_count ("go-string-test-refcount"), ==, 0);

	/* NULL handling */
	g_assert_null (go_string_ref (NULL));
	go_string_unref (NULL);
	g_assert_cmpuint (go_string_get_ref_count (NULL), ==, 0);

	/* A string with the same contents can be created afresh */
	a = go_string_new ("go-string-test-refcount");
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 1);
	go_string_unref (a);
}

static void
test_string_hash (void)
{
	GOString *a = go_string_new ("go-string-test-hash");
	GOString *a2 = go_string_new ("go-string-test-hash");
	GOString *b = go_string_new ("go-string-test-hash2");
	GOString *rich = go_string_new_rich ("go-string-test-hash", -1, NULL, NULL);
	GHashTable *h;

	g_assert_cmpuint (go_string_hash (NULL), ==, 0);

	/* Equal strings have equal hashes, whichever way they were made */
	g_assert_cmpuint (go_string_hash (a), ==, go_string_hash (a2));
	g_assert_cmpuint (go_string_hash (a), ==, go_string_hash (rich));

	/* Together with go_string_equal it works as a hash table key */
	h = g_hash_table_new (go_string_hash, go_string_equal);
	g_hash_table_insert (h, a, GINT_TO_POINTER (1));
	g_hash_table_insert (h, b, GINT_TO_POINTER (2));
	g_assert_cmpint (g_hash_table_size (h), ==, 2);
	g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (h, a2)), ==, 1);
	g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (h, rich)), ==, 1);
	g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (h, b)), ==, 2);
	g_hash_table_destroy (h);

	go_string_unref (a);
	go_string_unref (a2);
	go_string_unref (b);
	go_string_unref (rich);
}

static void
test_string_equal (void)
{
	GOString *a = go_string_new ("go-string-test-equal");
	GOString *a2 = go_string_new ("go-string-test-equal");
	GOString *b = go_string_new ("go-string-test-EQUAL");

	g_assert_true (go_string_equal (a, a));
	g_assert_true (go_string_equal (a, a2));
	g_assert_false (go_string_equal (a, b));
	g_assert_false (go_string_equal (b, a));

	g_assert_true (go_string_equal (NULL, NULL));
	g_assert_false (go_string_equal (a, NULL));
	g_assert_false (go_string_equal (NULL, a));

	g_assert_false (go_string_equal_ignorecase (a, NULL));
	g_assert_false (go_string_equal_ignorecase (NULL, a));
	g_assert_true (go_string_equal_ignorecase (a, a));
	g_assert_true (go_string_equal_ignorecase (a, b));
	g_assert_true (go_string_equal_ignorecase (b, a));
	g_assert_true (go_string_equal_ignorecase (NULL, NULL));

	g_assert_true (go_string_equal_rich (a, a2));
	g_assert_false (go_string_equal_rich (a, b));

	go_string_unref (a);
	go_string_unref (a2);
	go_string_unref (b);
}

static void
test_string_compare (void)
{
	GOString *a = go_string_new ("apple");
	GOString *b = go_string_new ("banana");
	GOString *A = go_string_new ("APPLE");
	GOString *e = go_string_new ("");

	g_assert_cmpint (go_string_cmp (a, a), ==, 0);
	g_assert_cmpint (go_string_cmp (a, b), <, 0);
	g_assert_cmpint (go_string_cmp (b, a), >, 0);
	/* Case sensitive: different, but antisymmetric */
	g_assert_cmpint (go_string_cmp (a, A), !=, 0);
	g_assert_true ((go_string_cmp (a, A) < 0) == (go_string_cmp (A, a) > 0));

	/* NULL sorts as the empty string */
	g_assert_cmpint (go_string_cmp (NULL, NULL), ==, 0);
	g_assert_cmpint (go_string_cmp (NULL, a), <, 0);
	g_assert_cmpint (go_string_cmp (a, NULL), >, 0);
	g_assert_cmpint (go_string_cmp (NULL, e), ==, 0);
	g_assert_cmpint (go_string_cmp (e, a), <, 0);

	g_assert_cmpint (go_string_cmp_ignorecase (a, A), ==, 0);
	g_assert_cmpint (go_string_cmp_ignorecase (A, a), ==, 0);
	g_assert_cmpint (go_string_cmp_ignorecase (A, b), <, 0);
	g_assert_cmpint (go_string_cmp_ignorecase (b, A), >, 0);
	g_assert_cmpint (go_string_cmp_ignorecase (NULL, a), <, 0);
	g_assert_cmpint (go_string_cmp_ignorecase (a, NULL), >, 0);
	g_assert_cmpint (go_string_cmp_ignorecase (NULL, NULL), ==, 0);

	go_string_unref (a);
	go_string_unref (b);
	go_string_unref (A);
	go_string_unref (e);
}

static void
test_string_collation_and_casefold (void)
{
	GOString *a = go_string_new ("HeLLo");
	GOString *s = go_string_new ("Stra\xc3\x9f" "e");
	char *key;

	g_assert_cmpstr (go_string_get_casefold (a), ==, "hello");
	g_assert_cmpstr (go_string_get_casefold (s), ==, "strasse");

	key = g_utf8_collate_key (a->str, -1);
	g_assert_cmpstr (go_string_get_collation (a), ==, key);
	g_free (key);

	key = g_utf8_collate_key ("hello", -1);
	g_assert_cmpstr (go_string_get_casefolded_collate (a), ==, key);
	g_free (key);

	/* NULL gives the empty string for everything */
	g_assert_cmpstr (go_string_get_casefold (NULL), ==, "");
	g_assert_cmpstr (go_string_get_collation (NULL), ==, "");
	g_assert_cmpstr (go_string_get_casefolded_collate (NULL), ==, "");

	go_string_unref (a);
	go_string_unref (s);
}

static PangoAttrList *
bold_attrs (guint start, guint end)
{
	PangoAttrList *l = pango_attr_list_new ();
	PangoAttribute *attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
	attr->start_index = start;
	attr->end_index = end;
	pango_attr_list_insert (l, attr);
	return l;
}

static void
test_string_rich (void)
{
	PangoAttrList *markup = bold_attrs (1, 3);
	GOString *base, *rich, *rich2;
	char *buf;

	base = go_string_new ("go-string-test-rich");
	g_assert_null (go_string_get_markup (base));
	g_assert_null (go_string_get_phonetic (base));

	/* Rich strings share the characters of the plain string, but are
	   distinct objects that own the markup */
	rich = go_string_new_rich ("go-string-test-rich", -1, markup, NULL);
	g_assert_nonnull (rich);
	g_assert_true (rich != base);
	g_assert_cmpstr (rich->str, ==, base->str);
	g_assert_cmpstr (rich->str, ==, "go-string-test-rich");
	g_assert_cmpuint (go_string_get_len (rich), ==, strlen (rich->str));
	g_assert_true (go_string_get_markup (rich) == markup);
	g_assert_null (go_string_get_phonetic (rich));
	g_assert_cmpuint (go_string_get_ref_count (rich), ==, 1);
	g_assert_cmpuint (go_string_hash (rich), ==, go_string_hash (base));

	/* Plain equality ignores markup */
	g_assert_true (go_string_equal (rich, base));
	g_assert_true (go_string_equal_rich (rich, base));

	/* Two rich strings with the same text are separate objects */
	rich2 = go_string_new_rich ("go-string-test-rich", -1,
				    bold_attrs (0, 2), NULL);
	g_assert_true (rich2 != rich);
	g_assert_true (go_string_equal (rich, rich2));
	g_assert_true (go_string_get_markup (rich2) != go_string_get_markup (rich));

	/* Collation data is that of the plain string */
	g_assert_cmpstr (go_string_get_collation (rich), ==, go_string_get_collation (base));
	g_assert_cmpstr (go_string_get_casefold (rich), ==, go_string_get_casefold (base));
	g_assert_cmpstr (go_string_get_casefolded_collate (rich), ==,
			 go_string_get_casefolded_collate (base));
	g_assert_cmpint (go_string_cmp (rich, base), ==, 0);
	g_assert_cmpint (go_string_cmp_ignorecase (rich, base), ==, 0);

	go_string_unref (rich);
	go_string_unref (rich2);
	/* The plain string is still there and unaffected */
	g_assert_cmpuint (go_string_get_ref_count (base), ==, 1);
	g_assert_cmpstr (base->str, ==, "go-string-test-rich");
	go_string_unref (base);
	g_assert_cmpint (string_pool_count ("go-string-test-rich"), ==, 0);

	/* Explicit byte length */
	rich = go_string_new_rich ("go-string-rich-len-xxx", 14, bold_attrs (0, 1), NULL);
	g_assert_cmpstr (rich->str, ==, "go-string-rich");
	g_assert_cmpuint (go_string_get_len (rich), ==, 14);
	go_string_unref (rich);

	/* nocopy variant takes the buffer */
	buf = g_strdup ("go-string-test-rich-nocopy");
	rich = go_string_new_rich_nocopy (buf, -1, bold_attrs (0, 1), NULL);
	g_assert_true (rich->str == buf);
	g_assert_nonnull (go_string_get_markup (rich));
	go_string_unref (rich);

	buf = g_strdup ("go-string-test-rich-nocopy2-xx");
	rich = go_string_new_rich_nocopy (buf, 4, bold_attrs (0, 1), NULL);
	g_assert_cmpstr (rich->str, ==, "go-s");
	g_assert_cmpuint (go_string_get_len (rich), ==, 4);
	go_string_unref (rich);
}

static void
test_string_rich_no_markup (void)
{
	GOString *rich = go_string_new_rich ("go-string-test-rich-nomarkup", -1, NULL, NULL);

	g_assert_nonnull (rich);
	g_assert_null (go_string_get_markup (rich));
	g_assert_cmpstr (rich->str, ==, "go-string-test-rich-nomarkup");
	go_string_unref (rich);
}

static PangoAttrList *
attrs_weight_range (const char **text_out, GOString *gs, int *start, int *end)
{
	PangoAttrList *l = go_string_get_markup (gs);
	PangoAttrIterator *it;
	PangoAttribute *w;

	*text_out = gs->str;
	it = pango_attr_list_get_iterator (l);
	*start = *end = -1;
	do {
		int s, e;
		pango_attr_iterator_range (it, &s, &e);
		w = pango_attr_iterator_get (it, PANGO_ATTR_WEIGHT);
		if (w) {
			*start = s;
			*end = MIN (e, (int)strlen (gs->str));
			break;
		}
	} while (pango_attr_iterator_next (it));
	pango_attr_iterator_destroy (it);
	return l;
}

static void
test_string_trim (void)
{
	GOString *gs, *r;
	const char *text;
	int s, e;

	/* Plain strings are returned untouched */
	gs = go_string_new ("  go-string-trim-plain  ");
	r = go_string_trim (gs, TRUE);
	g_assert_true (r == gs);
	g_assert_cmpstr (r->str, ==, "  go-string-trim-plain  ");
	go_string_unref (r);

	/* Leading and trailing spaces are removed, markup follows the text */
	gs = go_string_new_rich ("  hello  world  ", -1, bold_attrs (2, 7), NULL);
	r = go_string_trim (gs, FALSE);
	g_assert_cmpstr (r->str, ==, "hello  world");
	g_assert_cmpuint (go_string_get_len (r), ==, 12);
	attrs_weight_range (&text, r, &s, &e);
	g_assert_cmpint (s, ==, 0);
	g_assert_cmpint (e, ==, 5);
	go_string_unref (r);

	/* With internal=TRUE runs of spaces collapse to a single space */
	gs = go_string_new_rich ("  hello   big   world  ", -1, bold_attrs (2, 7), NULL);
	r = go_string_trim (gs, TRUE);
	g_assert_cmpstr (r->str, ==, "hello big world");
	g_assert_cmpuint (go_string_get_len (r), ==, 15);
	attrs_weight_range (&text, r, &s, &e);
	g_assert_cmpint (s, ==, 0);
	g_assert_cmpint (e, ==, 5);
	go_string_unref (r);

	/* Attribute on text after collapsed space shifts accordingly */
	gs = go_string_new_rich ("a   bcd", -1, bold_attrs (4, 7), NULL);
	r = go_string_trim (gs, TRUE);
	g_assert_cmpstr (r->str, ==, "a bcd");
	attrs_weight_range (&text, r, &s, &e);
	g_assert_cmpint (s, ==, 2);
	g_assert_cmpint (e, ==, 5);
	go_string_unref (r);

	/* Nothing to trim */
	gs = go_string_new_rich ("go-string-trim-clean", -1, bold_attrs (0, 2), NULL);
	r = go_string_trim (gs, TRUE);
	g_assert_cmpstr (r->str, ==, "go-string-trim-clean");
	go_string_unref (r);

	/* Only spaces are trimmed, not other white space */
	gs = go_string_new_rich (" \tx\t ", -1, bold_attrs (0, 1), NULL);
	r = go_string_trim (gs, TRUE);
	g_assert_cmpstr (r->str, ==, "\tx\t");
	go_string_unref (r);

	/* All spaces: becomes empty */
	gs = go_string_new_rich ("     ", -1, bold_attrs (0, 5), NULL);
	r = go_string_trim (gs, FALSE);
	g_assert_cmpstr (r->str, ==, "");
	g_assert_cmpuint (go_string_get_len (r), ==, 0);
	go_string_unref (r);
}

static void
test_string_trim_no_markup (void)
{
	/* A rich string whose markup is NULL must be handled too */
	GOString *gs = go_string_new_rich ("  a  b  ", -1, NULL, NULL);
	GOString *r = go_string_trim (gs, TRUE);

	g_assert_cmpstr (r->str, ==, "a b");
	go_string_unref (r);
}

static void
test_string_error (void)
{
	GOString *e = go_string_ERROR ();

	g_assert_nonnull (e);
	g_assert_cmpstr (e->str, ==, "<ERROR>");
	g_assert_true (go_string_ERROR () == e);
}

static void
test_string_boxed (void)
{
	GType t = go_string_get_type ();
	GOString *a = go_string_new ("go-string-test-boxed");
	GOString *b;
	GValue v = G_VALUE_INIT, sv = G_VALUE_INIT;

	g_assert_true (G_TYPE_IS_BOXED (t));
	g_assert_cmpstr (g_type_name (t), ==, "GOString");

	/* Copy is a ref, free is an unref */
	b = g_boxed_copy (t, a);
	g_assert_true (a == b);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 2);
	g_boxed_free (t, b);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 1);

	/* Transformation to a plain string */
	g_value_init (&v, t);
	g_value_set_boxed (&v, a);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 2);
	g_assert_true (g_value_type_transformable (t, G_TYPE_STRING));
	g_value_init (&sv, G_TYPE_STRING);
	g_assert_true (g_value_transform (&v, &sv));
	g_assert_cmpstr (g_value_get_string (&sv), ==, "go-string-test-boxed");
	g_assert_true (g_value_get_string (&sv) != a->str);
	g_value_unset (&sv);
	g_value_unset (&v);
	g_assert_cmpuint (go_string_get_ref_count (a), ==, 1);

	/* NULL boxed value */
	g_value_init (&v, t);
	g_value_init (&sv, G_TYPE_STRING);
	g_assert_true (g_value_transform (&v, &sv));
	g_assert_null (g_value_get_string (&sv));
	g_value_unset (&sv);
	g_value_unset (&v);

	go_string_unref (a);
}

static void
test_string_foreach (void)
{
	GOString *a = go_string_new ("go-string-test-foreach-1");
	GOString *b = go_string_new ("go-string-test-foreach-2");

	g_assert_cmpint (string_pool_count ("go-string-test-foreach-1"), ==, 1);
	g_assert_cmpint (string_pool_count ("go-string-test-foreach-2"), ==, 1);
	g_assert_cmpint (string_pool_count ("go-string-test-foreach-3"), ==, 0);

	go_string_unref (a);
	go_string_unref (b);
}

static void
register_string_tests (void)
{
	g_test_add_func ("/utils/go-string/new", test_string_new);
	g_test_add_func ("/utils/go-string/new_len", test_string_new_len);
	g_test_add_func ("/utils/go-string/new_nocopy", test_string_new_nocopy);
	g_test_add_func ("/utils/go-string/refcount", test_string_refcount);
	g_test_add_func ("/utils/go-string/hash", test_string_hash);
	g_test_add_func ("/utils/go-string/equal", test_string_equal);
	g_test_add_func ("/utils/go-string/compare", test_string_compare);
	g_test_add_func ("/utils/go-string/collation_and_casefold", test_string_collation_and_casefold);
	g_test_add_func ("/utils/go-string/rich", test_string_rich);
	g_test_add_func ("/utils/go-string/rich/no_markup", test_string_rich_no_markup);
	g_test_add_func ("/utils/go-string/trim", test_string_trim);
	g_test_add_func ("/utils/go-string/trim/no_markup", test_string_trim_no_markup);
	g_test_add_func ("/utils/go-string/ERROR", test_string_error);
	g_test_add_func ("/utils/go-string/boxed", test_string_boxed);
	g_test_add_func ("/utils/go-string/foreach", test_string_foreach);
}

/* ------------------------------------------------------------------ */
/* go-line.c                                                           */
/* ------------------------------------------------------------------ */

static void
test_line_dash_names (void)
{
	int i, j;

	/* Names round-trip and are distinct */
	for (i = 0; i < GO_LINE_MAX; i++) {
		const char *name = go_line_dash_as_str ((GOLineDashType)i);
		g_assert_nonnull (name);
		g_assert_cmpuint (strlen (name), >, 0);
		g_assert_cmpint (go_line_dash_from_str (name), ==, i);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (name, !=, go_line_dash_as_str ((GOLineDashType)j));
	}

	/* Invalid / unknown input falls back to "none" */
	g_assert_cmpstr (go_line_dash_as_str (GO_LINE_MAX), ==, go_line_dash_as_str (GO_LINE_NONE));
	g_assert_cmpstr (go_line_dash_as_str ((GOLineDashType)-1), ==, go_line_dash_as_str (GO_LINE_NONE));
	g_assert_cmpstr (go_line_dash_as_str ((GOLineDashType)1000), ==, go_line_dash_as_str (GO_LINE_NONE));
	g_assert_cmpint (go_line_dash_from_str ("bogus"), ==, GO_LINE_NONE);
	g_assert_cmpint (go_line_dash_from_str (""), ==, GO_LINE_NONE);
	/* Case sensitive */
	{
		char *up = g_ascii_strup (go_line_dash_as_str (GO_LINE_SOLID), -1);
		g_assert_cmpint (go_line_dash_from_str (up), ==, GO_LINE_NONE);
		g_free (up);
	}
}

static void
test_line_dash_labels (void)
{
	int i, j;

	/* Every type has a distinct, non-empty label */
	for (i = 0; i < GO_LINE_MAX; i++) {
		const char *li = go_line_dash_as_label ((GOLineDashType)i);
		g_assert_nonnull (li);
		g_assert_cmpuint (strlen (li), >, 0);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (li, !=, go_line_dash_as_label ((GOLineDashType)j));
	}

	/* Invalid values give the label of GO_LINE_NONE */
	g_assert_cmpstr (go_line_dash_as_label (GO_LINE_MAX), ==, go_line_dash_as_label (GO_LINE_NONE));
	g_assert_cmpstr (go_line_dash_as_label ((GOLineDashType)-3), ==, go_line_dash_as_label (GO_LINE_NONE));
}

static void
test_line_dash_sequence (void)
{
	int i;
	unsigned j;

	for (i = 0; i < GO_LINE_MAX; i++) {
		GOLineDashSequence *s = go_line_dash_get_sequence ((GOLineDashType)i, 1.0);
		GOLineDashSequence *s2;
		double len = go_line_dash_get_length ((GOLineDashType)i);

		if (i == GO_LINE_NONE || i == GO_LINE_SOLID) {
			/* No pattern for invisible and continuous lines */
			g_assert_null (s);
			assert_close (len, 1);
			continue;
		}

		g_assert_nonnull (s);
		g_assert_cmpuint (s->n_dash, >, 0);
		g_assert_cmpuint (s->ref_count, ==, 1);
		/* cairo wants an even number of entries, all non-negative,
		   and a pattern that isn't entirely empty */
		g_assert_cmpuint (s->n_dash % 2, ==, 0);
		assert_near (s->offset, 0);
		{
			double total = 0;
			for (j = 0; j < s->n_dash; j++) {
				g_assert_cmpfloat (s->dash[j], >=, 0);
				total += s->dash[j];
			}
			g_assert_cmpfloat (total, >, 0);
		}
		g_assert_cmpfloat (len, >, 0);

		/* Scaling multiplies all entries, but not the offset */
		s2 = go_line_dash_get_sequence ((GOLineDashType)i, 2.5);
		g_assert_nonnull (s2);
		g_assert_cmpuint (s2->n_dash, ==, s->n_dash);
		assert_near (s2->offset, 0);
		for (j = 0; j < s->n_dash; j++)
			assert_close (s2->dash[j], 2.5 * s->dash[j]);
		go_line_dash_sequence_free (s2);

		s2 = go_line_dash_get_sequence ((GOLineDashType)i, 0);
		for (j = 0; j < s2->n_dash; j++)
			assert_near (s2->dash[j], 0);
		go_line_dash_sequence_free (s2);

		go_line_dash_sequence_free (s);
	}

	/* No sequence for invalid types, and the default length is 1 */
	g_assert_null (go_line_dash_get_sequence (GO_LINE_MAX, 1));
	g_assert_null (go_line_dash_get_sequence ((GOLineDashType)-1, 1));
	assert_close (go_line_dash_get_length (GO_LINE_MAX), 1);
	assert_close (go_line_dash_get_length ((GOLineDashType)-1), 1);

	/* NULL is allowed when freeing */
	go_line_dash_sequence_free (NULL);
}

static void
test_line_dash_sequence_boxed (void)
{
	GType t = go_line_dash_sequence_get_type ();
	GOLineDashSequence *s = go_line_dash_get_sequence (GO_LINE_DASH, 1);
	GOLineDashSequence *s2;

	g_assert_true (G_TYPE_IS_BOXED (t));
	g_assert_cmpstr (g_type_name (t), ==, "GOLineDashSequence");
	g_assert_cmpuint (s->ref_count, ==, 1);

	/* Copy is a reference */
	s2 = g_boxed_copy (t, s);
	g_assert_true (s2 == s);
	g_assert_cmpuint (s->ref_count, ==, 2);
	g_boxed_free (t, s2);
	g_assert_cmpuint (s->ref_count, ==, 1);
	g_assert_cmpuint (s->n_dash, ==, 2);
	assert_close (s->dash[0], 12);

	go_line_dash_sequence_free (s);
}

static void
test_line_interpolation (void)
{
	int i, j;

	/* Names round-trip and are distinct */
	for (i = 0; i < GO_LINE_INTERPOLATION_MAX; i++) {
		const char *name = go_line_interpolation_as_str ((GOLineInterpolation)i);
		g_assert_nonnull (name);
		g_assert_cmpuint (strlen (name), >, 0);
		g_assert_cmpint (go_line_interpolation_from_str (name), ==, i);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (name, !=, go_line_interpolation_as_str ((GOLineInterpolation)j));
	}

	/* A few properties */
	g_assert_true (go_line_interpolation_supports_radial (GO_LINE_INTERPOLATION_LINEAR));
	g_assert_false (go_line_interpolation_supports_radial (GO_LINE_INTERPOLATION_CUBIC_SPLINE));
	g_assert_false (go_line_interpolation_auto_skip (GO_LINE_INTERPOLATION_LINEAR));
	g_assert_true (go_line_interpolation_auto_skip (GO_LINE_INTERPOLATION_CLOSED_SPLINE));

	/* Unknown names and invalid values fall back to linear */
	g_assert_cmpint (go_line_interpolation_from_str ("bogus"), ==, GO_LINE_INTERPOLATION_LINEAR);
	g_assert_cmpint (go_line_interpolation_from_str (""), ==, GO_LINE_INTERPOLATION_LINEAR);
	g_assert_cmpstr (go_line_interpolation_as_str (GO_LINE_INTERPOLATION_MAX), ==,
			 go_line_interpolation_as_str (GO_LINE_INTERPOLATION_LINEAR));
	g_assert_cmpstr (go_line_interpolation_as_str ((GOLineInterpolation)-1), ==,
			 go_line_interpolation_as_str (GO_LINE_INTERPOLATION_LINEAR));
	g_assert_false (go_line_interpolation_supports_radial (GO_LINE_INTERPOLATION_MAX));
	g_assert_false (go_line_interpolation_auto_skip (GO_LINE_INTERPOLATION_MAX));
}

static void
test_line_interpolation_labels (void)
{
	int i, j;

	for (i = 0; i < GO_LINE_INTERPOLATION_MAX; i++) {
		const char *li = go_line_interpolation_as_label ((GOLineInterpolation)i);
		g_assert_nonnull (li);
		g_assert_cmpuint (strlen (li), >, 0);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (li, !=, go_line_interpolation_as_label ((GOLineInterpolation)j));
	}
	g_assert_cmpstr (go_line_interpolation_as_label (GO_LINE_INTERPOLATION_MAX), ==,
			 go_line_interpolation_as_label (GO_LINE_INTERPOLATION_LINEAR));
}

static void
test_line_arrow_basic (void)
{
	GOArrow a, b;
	GOArrow *d;

	{
		const GOArrowType types[] = { GO_ARROW_NONE, GO_ARROW_KITE, GO_ARROW_OVAL };
		unsigned i, j;

		/* Names round-trip and are distinct */
		for (i = 0; i < G_N_ELEMENTS (types); i++) {
			const char *name = go_arrow_type_as_str (types[i]);
			g_assert_nonnull (name);
			g_assert_cmpint (go_arrow_type_from_str (name), ==, types[i]);
			for (j = 0; j < i; j++)
				g_assert_cmpstr (name, !=, go_arrow_type_as_str (types[j]));
		}
		g_assert_null (go_arrow_type_as_str ((GOArrowType)17));
		g_assert_cmpint (go_arrow_type_from_str ("bogus"), ==, GO_ARROW_NONE);
		g_assert_cmpint (go_arrow_type_from_str (""), ==, GO_ARROW_NONE);
	}

	go_arrow_init (&a, GO_ARROW_KITE, 1, 2, 3);
	g_assert_cmpint (a.typ, ==, GO_ARROW_KITE);
	assert_close (a.a, 1);
	assert_close (a.b, 2);
	assert_close (a.c, 3);

	go_arrow_init_kite (&a, 4, 5, 6);
	g_assert_cmpint (a.typ, ==, GO_ARROW_KITE);
	assert_close (a.a, 4);
	assert_close (a.b, 5);
	assert_close (a.c, 6);

	go_arrow_init_oval (&a, 7, 8);
	g_assert_cmpint (a.typ, ==, GO_ARROW_OVAL);
	assert_close (a.a, 7);
	assert_close (a.b, 8);
	assert_near (a.c, 0);

	go_arrow_clear (&a);
	g_assert_cmpint (a.typ, ==, GO_ARROW_NONE);
	assert_near (a.a, 0);
	assert_near (a.b, 0);
	assert_near (a.c, 0);

	go_arrow_init_kite (&a, 1, 2, 3);
	d = go_arrow_dup (&a);
	g_assert_true (d != &a);
	g_assert_true (go_arrow_equal (d, &a));
	d->c = 4;
	g_assert_false (go_arrow_equal (d, &a));
	g_free (d);

	/* Boxed type */
	g_assert_true (G_TYPE_IS_BOXED (GO_ARROW_TYPE));
	g_assert_cmpstr (g_type_name (GO_ARROW_TYPE), ==, "GOArrow");
	d = g_boxed_copy (GO_ARROW_TYPE, &a);
	g_assert_true (d != &a);
	g_assert_true (go_arrow_equal (d, &a));
	g_boxed_free (GO_ARROW_TYPE, d);

	(void)b;
}

static void
test_line_arrow_equal (void)
{
	GOArrow a, b;

	go_arrow_init_kite (&a, 1, 2, 3);
	go_arrow_init_kite (&b, 1, 2, 3);
	g_assert_true (go_arrow_equal (&a, &a));
	g_assert_true (go_arrow_equal (&a, &b));

	go_arrow_init_kite (&b, 9, 2, 3);
	g_assert_false (go_arrow_equal (&a, &b));
	go_arrow_init_kite (&b, 1, 9, 3);
	g_assert_false (go_arrow_equal (&a, &b));
	go_arrow_init_kite (&b, 1, 2, 9);
	g_assert_false (go_arrow_equal (&a, &b));

	/* Different types are never equal */
	go_arrow_init_oval (&b, 1, 2);
	g_assert_false (go_arrow_equal (&a, &b));
	go_arrow_clear (&b);
	g_assert_false (go_arrow_equal (&a, &b));

	/* Ovals ignore c */
	go_arrow_init_oval (&a, 1, 2);
	go_arrow_init_oval (&b, 1, 2);
	b.c = 77;
	g_assert_true (go_arrow_equal (&a, &b));
	go_arrow_init_oval (&b, 1, 3);
	g_assert_false (go_arrow_equal (&a, &b));
	go_arrow_init_oval (&b, 2, 2);
	g_assert_false (go_arrow_equal (&a, &b));

	/* None ignores all parameters */
	go_arrow_init (&a, GO_ARROW_NONE, 1, 2, 3);
	go_arrow_init (&b, GO_ARROW_NONE, 4, 5, 6);
	g_assert_true (go_arrow_equal (&a, &b));

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_arrow_equal*");
	g_assert_false (go_arrow_equal (NULL, &a));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_arrow_equal*");
	g_assert_false (go_arrow_equal (&a, NULL));
	g_test_assert_expected_messages ();
}

/* Alpha value at pixel (x,y) of an ARGB32 image surface */
static unsigned
alpha_at (cairo_surface_t *surf, int x, int y)
{
	const unsigned char *data;
	int stride;

	cairo_surface_flush (surf);
	data = cairo_image_surface_get_data (surf);
	stride = cairo_image_surface_get_stride (surf);
	return ((const guint32 *)(data + y * stride))[x] >> 24;
}

static void
test_line_arrow_draw (void)
{
	cairo_surface_t *surf;
	cairo_t *cr;
	GOArrow arrow;
	double dx, dy;

	/* Kite pointing "up" (towards negative y) at (50,50) */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
	cr = cairo_create (surf);
	cairo_translate (cr, 50, 50);
	go_arrow_init_kite (&arrow, 20, 10, 5);
	dx = dy = 99;
	go_arrow_draw (&arrow, cr, &dx, &dy, 0);
	assert_near (dx, 0);
	assert_close (dy, -20);
	g_assert_cmpuint (alpha_at (surf, 50, 45), ==, 255);   /* inside */
	g_assert_cmpuint (alpha_at (surf, 50, 38), ==, 255);   /* near the back */
	g_assert_cmpuint (alpha_at (surf, 50, 25), ==, 0);     /* beyond the back */
	g_assert_cmpuint (alpha_at (surf, 80, 45), ==, 0);     /* outside */
	g_assert_cmpuint (alpha_at (surf, 40, 45), ==, 0);     /* outside */
	g_assert_cmpuint (alpha_at (surf, 50, 60), ==, 0);     /* in front of tip */
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Rotated by a quarter turn: points to the right */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
	cr = cairo_create (surf);
	cairo_translate (cr, 50, 50);
	go_arrow_draw (&arrow, cr, &dx, &dy, M_PI / 2);
	assert_close (dx, 20);
	assert_near (dy, 0);
	g_assert_cmpuint (alpha_at (surf, 54, 50), ==, 255);
	g_assert_cmpuint (alpha_at (surf, 50, 45), ==, 0);
	g_assert_cmpuint (alpha_at (surf, 75, 50), ==, 0);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Optional output arguments */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
	cr = cairo_create (surf);
	cairo_translate (cr, 50, 50);
	go_arrow_draw (&arrow, cr, NULL, NULL, 0);
	g_assert_cmpuint (alpha_at (surf, 50, 45), ==, 255);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Oval: filled ellipse, never moves the line end */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
	cr = cairo_create (surf);
	cairo_translate (cr, 50, 50);
	go_arrow_init_oval (&arrow, 10, 5);
	dx = dy = 99;
	go_arrow_draw (&arrow, cr, &dx, &dy, 0);
	assert_near (dx, 0);
	assert_near (dy, 0);
	g_assert_cmpuint (alpha_at (surf, 50, 50), ==, 255);
	g_assert_cmpuint (alpha_at (surf, 57, 50), ==, 255);
	g_assert_cmpuint (alpha_at (surf, 50, 53), ==, 255);
	g_assert_cmpuint (alpha_at (surf, 50, 58), ==, 0);
	g_assert_cmpuint (alpha_at (surf, 62, 50), ==, 0);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Oval with a non-positive radius draws nothing */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
	cr = cairo_create (surf);
	cairo_translate (cr, 50, 50);
	go_arrow_init_oval (&arrow, 0, 5);
	go_arrow_draw (&arrow, cr, NULL, NULL, 0);
	go_arrow_init_oval (&arrow, 5, -1);
	go_arrow_draw (&arrow, cr, NULL, NULL, 0);
	g_assert_cmpuint (alpha_at (surf, 50, 50), ==, 0);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* None: nothing drawn, outputs zeroed */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
	cr = cairo_create (surf);
	cairo_translate (cr, 50, 50);
	go_arrow_clear (&arrow);
	dx = dy = 99;
	go_arrow_draw (&arrow, cr, &dx, &dy, 1);
	assert_near (dx, 0);
	assert_near (dy, 0);
	g_assert_cmpuint (alpha_at (surf, 50, 50), ==, 0);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);
}

static void
register_line_tests (void)
{
	g_test_add_func ("/utils/go-line/dash_names", test_line_dash_names);
	g_test_add_func ("/utils/go-line/dash_labels", test_line_dash_labels);
	g_test_add_func ("/utils/go-line/dash_sequence", test_line_dash_sequence);
	g_test_add_func ("/utils/go-line/dash_sequence_boxed", test_line_dash_sequence_boxed);
	g_test_add_func ("/utils/go-line/interpolation", test_line_interpolation);
	g_test_add_func ("/utils/go-line/interpolation_labels", test_line_interpolation_labels);
	g_test_add_func ("/utils/go-line/arrow_basic", test_line_arrow_basic);
	g_test_add_func ("/utils/go-line/arrow_equal", test_line_arrow_equal);
	g_test_add_func ("/utils/go-line/arrow_draw", test_line_arrow_draw);
}

/* ------------------------------------------------------------------ */
/* go-undo.c                                                           */
/* ------------------------------------------------------------------ */

static GString *undo_log;

static void
log_unary (gpointer a, gpointer data)
{
	g_string_append_printf (undo_log, "U(%s,%s)", (const char *)a,
				data ? (const char *)data : "-");
}

static void
log_binary (gpointer a, gpointer b, gpointer data)
{
	g_string_append_printf (undo_log, "B(%s,%s,%s)", (const char *)a,
				(const char *)b,
				data ? (const char *)data : "-");
}

static void
log_dispose (gpointer a)
{
	g_string_append_printf (undo_log, "D(%s)", (const char *)a);
}

static void
undo_setup (void)
{
	undo_log = g_string_new (NULL);
}

static void
undo_check_log (const char *expected)
{
	g_assert_cmpstr (undo_log->str, ==, expected);
	g_string_truncate (undo_log, 0);
}

static void
undo_teardown (void)
{
	g_string_free (undo_log, TRUE);
	undo_log = NULL;
}

static void
test_undo_unary (void)
{
	GOUndo *u;

	undo_setup ();

	u = go_undo_unary_new ((gpointer)"a", log_unary, NULL);
	g_assert_true (GO_IS_UNDO (u));
	g_assert_true (GO_IS_UNDO_UNARY (u));
	g_assert_false (GO_IS_UNDO_GROUP (u));
	g_assert_false (GO_IS_UNDO_BINARY (u));
	undo_check_log ("");

	go_undo_undo (u);
	undo_check_log ("U(a,-)");

	/* Undo does not consume the operation */
	go_undo_undo (u);
	undo_check_log ("U(a,-)");

	/* Extra data is passed through */
	go_undo_undo_with_data (u, (gpointer)"ctx");
	undo_check_log ("U(a,ctx)");

	g_object_unref (u);
	undo_check_log ("");

	/* Dispose function is called exactly once, on finalization */
	u = go_undo_unary_new ((gpointer)"a", log_unary, log_dispose);
	go_undo_undo (u);
	undo_check_log ("U(a,-)");
	g_object_unref (u);
	undo_check_log ("D(a)");

	undo_teardown ();
}

static void
test_undo_binary (void)
{
	GOUndo *u;

	undo_setup ();

	u = go_undo_binary_new ((gpointer)"a", (gpointer)"b", log_binary, NULL, NULL);
	g_assert_true (GO_IS_UNDO (u));
	g_assert_true (GO_IS_UNDO_BINARY (u));
	g_assert_false (GO_IS_UNDO_UNARY (u));

	go_undo_undo (u);
	undo_check_log ("B(a,b,-)");
	go_undo_undo_with_data (u, (gpointer)"ctx");
	undo_check_log ("B(a,b,ctx)");
	g_object_unref (u);
	undo_check_log ("");

	/* Both dispose functions run, each on its own argument */
	u = go_undo_binary_new ((gpointer)"a", (gpointer)"b", log_binary,
				log_dispose, log_dispose);
	g_object_unref (u);
	undo_check_log ("D(a)D(b)");

	/* Either dispose function may be missing */
	u = go_undo_binary_new ((gpointer)"a", (gpointer)"b", log_binary,
				log_dispose, NULL);
	g_object_unref (u);
	undo_check_log ("D(a)");
	u = go_undo_binary_new ((gpointer)"a", (gpointer)"b", log_binary,
				NULL, log_dispose);
	g_object_unref (u);
	undo_check_log ("D(b)");

	undo_teardown ();
}

static void
test_undo_group (void)
{
	GOUndoGroup *g;
	GOUndo *u1, *u2, *u3;

	undo_setup ();

	/* Empty group is fine */
	g = go_undo_group_new ();
	g_assert_true (GO_IS_UNDO (g));
	g_assert_true (GO_IS_UNDO_GROUP (g));
	g_assert_cmpuint (g->undos->len, ==, 0);
	go_undo_undo (GO_UNDO (g));
	undo_check_log ("");
	g_object_unref (g);

	/* Members are undone in reverse order of addition */
	g = go_undo_group_new ();
	u1 = go_undo_unary_new ((gpointer)"1", log_unary, log_dispose);
	u2 = go_undo_binary_new ((gpointer)"2", (gpointer)"x", log_binary, log_dispose, NULL);
	u3 = go_undo_unary_new ((gpointer)"3", log_unary, log_dispose);
	go_undo_group_add (g, u1);
	go_undo_group_add (g, u2);
	go_undo_group_add (g, u3);
	g_assert_cmpuint (g->undos->len, ==, 3);

	go_undo_undo (GO_UNDO (g));
	undo_check_log ("U(3,-)B(2,x,-)U(1,-)");

	/* Context is passed on to every member */
	go_undo_undo_with_data (GO_UNDO (g), (gpointer)"ctx");
	undo_check_log ("U(3,ctx)B(2,x,ctx)U(1,ctx)");

	/* Group owns its members and releases them in reverse order */
	g_object_unref (g);
	undo_check_log ("D(3)D(2)D(1)");

	undo_teardown ();
}

static void
test_undo_group_nested (void)
{
	GOUndoGroup *outer = go_undo_group_new ();
	GOUndoGroup *inner = go_undo_group_new ();

	undo_setup ();

	go_undo_group_add (inner, go_undo_unary_new ((gpointer)"i1", log_unary, NULL));
	go_undo_group_add (inner, go_undo_unary_new ((gpointer)"i2", log_unary, NULL));
	go_undo_group_add (outer, go_undo_unary_new ((gpointer)"o1", log_unary, NULL));
	go_undo_group_add (outer, GO_UNDO (inner));
	go_undo_group_add (outer, go_undo_unary_new ((gpointer)"o2", log_unary, NULL));

	go_undo_undo (GO_UNDO (outer));
	undo_check_log ("U(o2,-)U(i2,-)U(i1,-)U(o1,-)");

	g_object_unref (outer);
	undo_teardown ();
}

static void
test_undo_combine (void)
{
	GOUndo *a, *b, *c, *r;

	undo_setup ();

	/* NULL operands */
	g_assert_null (go_undo_combine (NULL, NULL));

	a = go_undo_unary_new ((gpointer)"a", log_unary, log_dispose);
	g_assert_true (go_undo_combine (a, NULL) == a);
	g_assert_true (go_undo_combine (NULL, a) == a);
	g_assert_false (GO_IS_UNDO_GROUP (a));

	/* Two plain operations become a group; the second is undone first */
	b = go_undo_unary_new ((gpointer)"b", log_unary, log_dispose);
	r = go_undo_combine (a, b);
	g_assert_true (GO_IS_UNDO_GROUP (r));
	g_assert_true (r != a);
	g_assert_true (r != b);
	g_assert_cmpuint (GO_UNDO_GROUP (r)->undos->len, ==, 2);
	go_undo_undo (r);
	undo_check_log ("U(b,-)U(a,-)");

	/* Combining into an existing group appends to it */
	c = go_undo_unary_new ((gpointer)"c", log_unary, log_dispose);
	g_assert_true (go_undo_combine (r, c) == r);
	g_assert_cmpuint (GO_UNDO_GROUP (r)->undos->len, ==, 3);
	go_undo_undo (r);
	undo_check_log ("U(c,-)U(b,-)U(a,-)");

	g_object_unref (r);
	undo_check_log ("D(c)D(b)D(a)");

	/* Plain + group: the group is nested as the last member */
	{
		GOUndoGroup *g = go_undo_group_new ();
		go_undo_group_add (g, go_undo_unary_new ((gpointer)"g1", log_unary, NULL));
		go_undo_group_add (g, go_undo_unary_new ((gpointer)"g2", log_unary, NULL));
		a = go_undo_unary_new ((gpointer)"a", log_unary, NULL);
		r = go_undo_combine (a, GO_UNDO (g));
		g_assert_true (GO_IS_UNDO_GROUP (r));
		g_assert_true (r != GO_UNDO (g));
		go_undo_undo (r);
		undo_check_log ("U(g2,-)U(g1,-)U(a,-)");
		g_object_unref (r);
	}

	/* Chaining combines gives reverse-chronological undo */
	r = NULL;
	r = go_undo_combine (r, go_undo_unary_new ((gpointer)"1", log_unary, NULL));
	r = go_undo_combine (r, go_undo_unary_new ((gpointer)"2", log_unary, NULL));
	r = go_undo_combine (r, go_undo_unary_new ((gpointer)"3", log_unary, NULL));
	go_undo_undo (r);
	undo_check_log ("U(3,-)U(2,-)U(1,-)");
	g_object_unref (r);

	undo_teardown ();
}

static void
test_undo_invalid_args (void)
{
	GOUndoGroup *g = go_undo_group_new ();
	GObject *not_undo = g_object_new (G_TYPE_OBJECT, NULL);

	undo_setup ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_undo_undo_with_data*");
	go_undo_undo (NULL);
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_undo_undo_with_data*");
	go_undo_undo ((GOUndo *)not_undo);
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_undo_group_add*");
	go_undo_group_add (NULL, (GOUndo *)g);
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_undo_group_add*");
	go_undo_group_add (g, NULL);
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_undo_group_add*");
	go_undo_group_add (g, (GOUndo *)not_undo);
	g_test_assert_expected_messages ();
	g_assert_cmpuint (g->undos->len, ==, 0);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_undo_combine*");
	g_assert_null (go_undo_combine ((GOUndo *)not_undo, (GOUndo *)g));
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_undo_combine*");
	g_assert_null (go_undo_combine ((GOUndo *)g, (GOUndo *)not_undo));
	g_test_assert_expected_messages ();

	g_object_unref (not_undo);
	g_object_unref (g);
	undo_teardown ();
}

static void
test_undo_types (void)
{
	g_assert_true (g_type_is_a (GO_TYPE_UNDO_GROUP, GO_TYPE_UNDO));
	g_assert_true (g_type_is_a (GO_TYPE_UNDO_UNARY, GO_TYPE_UNDO));
	g_assert_true (g_type_is_a (GO_TYPE_UNDO_BINARY, GO_TYPE_UNDO));
	g_assert_true (g_type_is_a (GO_TYPE_UNDO, G_TYPE_OBJECT));
	g_assert_cmpstr (g_type_name (GO_TYPE_UNDO_GROUP), ==, "GOUndoGroup");
	g_assert_cmpstr (g_type_name (GO_TYPE_UNDO_UNARY), ==, "GOUndoUnary");
	g_assert_cmpstr (g_type_name (GO_TYPE_UNDO_BINARY), ==, "GOUndoBinary");
}

static void
register_undo_tests (void)
{
	g_test_add_func ("/utils/go-undo/types", test_undo_types);
	g_test_add_func ("/utils/go-undo/unary", test_undo_unary);
	g_test_add_func ("/utils/go-undo/binary", test_undo_binary);
	g_test_add_func ("/utils/go-undo/group", test_undo_group);
	g_test_add_func ("/utils/go-undo/group/nested", test_undo_group_nested);
	g_test_add_func ("/utils/go-undo/combine", test_undo_combine);
	g_test_add_func ("/utils/go-undo/invalid_args", test_undo_invalid_args);
}

/* ------------------------------------------------------------------ */
/* go-path.c                                                           */
/* ------------------------------------------------------------------ */

/* Event recording through go_path_interpret.  Events are collected as
   "M x y;L x y;C x0 y0 x1 y1 x2 y2;Z" strings. */

static void
rec_move (void *cl, const GOPathPoint *p)
{
	g_string_append_printf (cl, "%sM %.17g %.17g", ((GString *)cl)->len ? ";" : "", p->x, p->y);
}

static void
rec_line (void *cl, const GOPathPoint *p)
{
	g_string_append_printf (cl, "%sL %.17g %.17g", ((GString *)cl)->len ? ";" : "", p->x, p->y);
}

static void
rec_curve (void *cl, const GOPathPoint *a, const GOPathPoint *b, const GOPathPoint *c)
{
	g_string_append_printf (cl, "%sC %.17g %.17g %.17g %.17g %.17g %.17g",
				((GString *)cl)->len ? ";" : "",
				a->x, a->y, b->x, b->y, c->x, c->y);
}

static void
rec_close (void *cl)
{
	g_string_append_printf (cl, "%sZ", ((GString *)cl)->len ? ";" : "");
}

static char *
path_events (GOPath *path, GOPathDirection dir)
{
	GString *s = g_string_new (NULL);
	go_path_interpret (path, dir, rec_move, rec_line, rec_curve, rec_close, s);
	return g_string_free (s, FALSE);
}

static char *
path_events_full (GOPath *path, gssize start, gssize end, GOPathDirection dir)
{
	GString *s = g_string_new (NULL);
	go_path_interpret_full (path, start, end, dir,
				rec_move, rec_line, rec_curve, rec_close, s);
	return g_string_free (s, FALSE);
}

/* Compare two event strings; numbers are compared with a tolerance. */
static void
assert_events_equal (const char *got, const char *expected)
{
	char **g = g_strsplit (got, ";", -1);
	char **e = g_strsplit (expected, ";", -1);
	gboolean ok = g_strv_length (g) == g_strv_length (e);
	guint i;

	for (i = 0; ok && g[i]; i++) {
		char **gt = g_strsplit (g[i], " ", -1);
		char **et = g_strsplit (e[i], " ", -1);
		guint j;

		ok = g_strv_length (gt) == g_strv_length (et) &&
			g_strcmp0 (gt[0], et[0]) == 0;
		for (j = 1; ok && gt[j]; j++) {
			double a = g_ascii_strtod (gt[j], NULL);
			double b = g_ascii_strtod (et[j], NULL);
			ok = fabs (a - b) <= 1e-9;
		}
		g_strfreev (gt);
		g_strfreev (et);
	}
	if (!ok)
		g_error ("Path events differ:\n  got:      %s\n  expected: %s", got, expected);
	g_strfreev (g);
	g_strfreev (e);
}

static void
assert_path (GOPath *path, GOPathDirection dir, const char *expected)
{
	char *got = path_events (path, dir);
	assert_events_equal (got, expected);
	g_free (got);
}

#define T_ 0.55228474983079345	/* 4/3 * tan (pi/8): quarter circle handle */

static void
test_path_lifecycle (void)
{
	GOPath *p = go_path_new ();
	GOPath *q;
	GType t = go_path_get_type ();

	g_assert_nonnull (p);
	g_assert_true (G_TYPE_IS_BOXED (t));
	g_assert_cmpstr (g_type_name (t), ==, "GOPath");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "");
	assert_path (p, GO_PATH_DIRECTION_BACKWARD, "");

	/* Reference counting; the path survives until the last free */
	go_path_move_to (p, 1, 2);
	q = go_path_ref (p);
	g_assert_true (q == p);
	go_path_free (q);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 1 2");

	q = g_boxed_copy (t, p);
	g_assert_true (q == p);
	g_boxed_free (t, q);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 1 2");
	go_path_free (p);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_path_ref*");
	g_assert_null (go_path_ref (NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_path_free*");
	go_path_free (NULL);
	g_test_assert_expected_messages ();
}

static void
test_path_options (void)
{
	GOPath *p = go_path_new ();

	g_assert_cmpint (go_path_get_options (p), ==, 0);
	go_path_set_options (p, GO_PATH_OPTIONS_SNAP_COORDINATES);
	g_assert_cmpint (go_path_get_options (p), ==, GO_PATH_OPTIONS_SNAP_COORDINATES);
	go_path_set_options (p, GO_PATH_OPTIONS_SNAP_WIDTH);
	g_assert_cmpint (go_path_get_options (p), ==, GO_PATH_OPTIONS_SNAP_WIDTH);
	go_path_set_options (p, GO_PATH_OPTIONS_SHARP);
	g_assert_cmpint (go_path_get_options (p), ==,
			 GO_PATH_OPTIONS_SNAP_COORDINATES | GO_PATH_OPTIONS_SNAP_WIDTH);

	/* Options are preserved by the copying operations */
	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 1, 1);
	{
		GOPath *c = go_path_copy (p);
		g_assert_cmpint (go_path_get_options (c), ==, GO_PATH_OPTIONS_SHARP);
		go_path_free (c);
		c = go_path_copy_restricted (p, 0, 1);
		g_assert_cmpint (go_path_get_options (c), ==, GO_PATH_OPTIONS_SHARP);
		go_path_free (c);
	}
	go_path_free (p);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_path_set_options*");
	go_path_set_options (NULL, 0);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_path_get_options*");
	g_assert_cmpint (go_path_get_options (NULL), ==, 0);
	g_test_assert_expected_messages ();
}

static void
test_path_building (void)
{
	GOPath *p = go_path_new ();

	go_path_move_to (p, 1, 2);
	go_path_line_to (p, 3, 4);
	go_path_curve_to (p, 5, 6, 7, 8, 9, 10);
	go_path_close (p);
	go_path_move_to (p, -1, -2);
	go_path_line_to (p, 0.5, 0.25);
	assert_path (p, GO_PATH_DIRECTION_FORWARD,
		     "M 1 2;L 3 4;C 5 6 7 8 9 10;Z;M -1 -2;L 0.5 0.25");
	go_path_free (p);

	/* Rectangle */
	p = go_path_new ();
	go_path_rectangle (p, 1, 2, 3, 4);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 1 2;L 4 2;L 4 6;L 1 6;Z");
	go_path_clear (p);
	go_path_rectangle (p, 5, 5, -2, -3);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 5 5;L 3 5;L 3 2;L 5 2;Z");
	go_path_free (p);

	/* Coordinates are clamped to what cairo can represent */
	p = go_path_new ();
	go_path_move_to (p, 1e30, -1e30);
	go_path_line_to (p, 8000000, -8000000);
	go_path_curve_to (p, 1e9, 0, -1e9, 0, 7999999, 1);
	assert_path (p, GO_PATH_DIRECTION_FORWARD,
		     "M 8000000 -8000000;L 8000000 -8000000;"
		     "C 8000000 0 -8000000 0 7999999 1");
	go_path_free (p);

	/* Null paths are rejected */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_path_add_points*");
	go_path_move_to (NULL, 0, 0);
	g_test_assert_expected_messages ();
}

static void
test_path_clear (void)
{
	GOPath *p = go_path_new ();
	int i;

	go_path_clear (p);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "");

	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 1, 1);
	go_path_clear (p);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "");
	go_path_move_to (p, 2, 2);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 2 2");

	/* Clearing a path that spans several internal buffers */
	go_path_clear (p);
	for (i = 0; i < 500; i++)
		go_path_line_to (p, i, i);
	go_path_clear (p);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "");
	go_path_rectangle (p, 0, 0, 1, 1);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 0;L 1 1;L 0 1;Z");

	go_path_free (p);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_path_clear*");
	go_path_clear (NULL);
	g_test_assert_expected_messages ();
}

static void
test_path_large (void)
{
	/* More actions and points than fit in one internal buffer, in
	   several mixes, must keep their order in both directions. */
	int sizes[] = { 63, 64, 65, 128, 200 };
	unsigned k;

	for (k = 0; k < G_N_ELEMENTS (sizes); k++) {
		int n = sizes[k], i;
		GOPath *p = go_path_new ();
		GString *fwd = g_string_new ("M 0 0");
		GString *bwd = g_string_new (NULL);
		char *got;

		go_path_move_to (p, 0, 0);
		for (i = 1; i <= n; i++) {
			go_path_line_to (p, i, 2 * i);
			g_string_append_printf (fwd, ";L %d %d", i, 2 * i);
		}
		got = path_events (p, GO_PATH_DIRECTION_FORWARD);
		assert_events_equal (got, fwd->str);
		g_free (got);

		g_string_append_printf (bwd, "M %d %d", n, 2 * n);
		for (i = n - 1; i >= 0; i--)
			g_string_append_printf (bwd, ";L %d %d", i, 2 * i);
		got = path_events (p, GO_PATH_DIRECTION_BACKWARD);
		assert_events_equal (got, bwd->str);
		g_free (got);

		go_path_free (p);
		g_string_free (fwd, TRUE);
		g_string_free (bwd, TRUE);
	}

	/* Curves use three points each, so the buffers fill up differently */
	{
		GOPath *p = go_path_new ();
		GString *fwd = g_string_new ("M 0 0");
		int i;
		char *got;

		go_path_move_to (p, 0, 0);
		for (i = 1; i <= 100; i++) {
			go_path_curve_to (p, i, 0, i, 1, i, 2);
			g_string_append_printf (fwd, ";C %d 0 %d 1 %d 2", i, i, i);
		}
		got = path_events (p, GO_PATH_DIRECTION_FORWARD);
		assert_events_equal (got, fwd->str);
		g_free (got);
		go_path_free (p);
		g_string_free (fwd, TRUE);
	}
}

static void
test_path_interpret (void)
{
	GOPath *p = go_path_new ();

	/* Nothing to do for a NULL path */
	go_path_interpret (NULL, GO_PATH_DIRECTION_FORWARD, NULL, NULL, NULL, NULL, NULL);

	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 1, 0);
	go_path_line_to (p, 1, 1);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 0;L 1 1");
	/* Backward: same shape, traversed from the other end */
	assert_path (p, GO_PATH_DIRECTION_BACKWARD, "M 1 1;L 1 0;L 0 0");
	go_path_free (p);

	/* Curves have their control points swapped when reversed */
	p = go_path_new ();
	go_path_move_to (p, 0, 0);
	go_path_curve_to (p, 1, 2, 3, 4, 5, 6);
	go_path_line_to (p, 7, 7);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;C 1 2 3 4 5 6;L 7 7");
	assert_path (p, GO_PATH_DIRECTION_BACKWARD, "M 7 7;L 5 6;C 3 4 1 2 0 0");
	go_path_free (p);

	/* Several subpaths */
	p = go_path_new ();
	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 1, 1);
	go_path_move_to (p, 5, 5);
	go_path_line_to (p, 6, 6);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 1;M 5 5;L 6 6");
	assert_path (p, GO_PATH_DIRECTION_BACKWARD, "M 6 6;L 5 5;M 1 1;L 0 0");
	go_path_free (p);

	/* Closed paths are walked forward as recorded */
	p = go_path_new ();
	go_path_rectangle (p, 0, 0, 2, 1);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 2 0;L 2 1;L 0 1;Z");
	go_path_free (p);
}

static void
test_path_interpret_full (void)
{
	GOPath *p = go_path_new ();

	/* Action indices:  0:M 1:L 2:M 3:L 4:Z */
	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 1, 1);
	go_path_move_to (p, 5, 5);
	go_path_line_to (p, 6, 6);
	go_path_close (p);

	/* Negative start: from the beginning */
	assert_events_equal (path_events_full (p, -1, 1, GO_PATH_DIRECTION_FORWARD), "M 0 0;L 1 1");
	assert_events_equal (path_events_full (p, -1, 3, GO_PATH_DIRECTION_FORWARD), "M 0 0;L 1 1;M 5 5;L 6 6");
	assert_events_equal (path_events_full (p, -1, 4, GO_PATH_DIRECTION_FORWARD), "M 0 0;L 1 1;M 5 5;L 6 6;Z");
	assert_events_equal (path_events_full (p, 0, 4, GO_PATH_DIRECTION_FORWARD), "M 0 0;L 1 1;M 5 5;L 6 6;Z");
	/* ... and the same as an unrestricted interpretation when the end is past the last action */
	assert_events_equal (path_events_full (p, 0, 50, GO_PATH_DIRECTION_FORWARD), "M 0 0;L 1 1;M 5 5;L 6 6;Z");

	/* The end action is included */
	assert_events_equal (path_events_full (p, 0, 1, GO_PATH_DIRECTION_FORWARD), "M 0 0;L 1 1");
	assert_events_equal (path_events_full (p, 0, 2, GO_PATH_DIRECTION_FORWARD), "M 0 0;L 1 1;M 5 5");

	/* The start action becomes the starting point of the piece */
	assert_events_equal (path_events_full (p, 1, 3, GO_PATH_DIRECTION_FORWARD), "M 1 1;M 5 5;L 6 6");
	assert_events_equal (path_events_full (p, 2, 3, GO_PATH_DIRECTION_FORWARD), "M 5 5;L 6 6");
	assert_events_equal (path_events_full (p, 3, 4, GO_PATH_DIRECTION_FORWARD), "M 6 6;Z");

	/* Empty ranges */
	assert_events_equal (path_events_full (p, 3, 3, GO_PATH_DIRECTION_FORWARD), "");
	assert_events_equal (path_events_full (p, 4, 2, GO_PATH_DIRECTION_FORWARD), "");
	assert_events_equal (path_events_full (p, 5, 9, GO_PATH_DIRECTION_FORWARD), "");

	/* NULL path */
	go_path_interpret_full (NULL, 0, 1, GO_PATH_DIRECTION_FORWARD, NULL, NULL, NULL, NULL, NULL);
	go_path_free (p);

	/* A curve action used as start begins at the curve's end point */
	p = go_path_new ();
	go_path_move_to (p, 0, 0);
	go_path_curve_to (p, 1, 2, 3, 4, 5, 6);
	go_path_line_to (p, 7, 7);
	assert_events_equal (path_events_full (p, 1, 2, GO_PATH_DIRECTION_FORWARD), "M 5 6;L 7 7");
	assert_events_equal (path_events_full (p, 0, 1, GO_PATH_DIRECTION_FORWARD), "M 0 0;C 1 2 3 4 5 6");
	go_path_free (p);
}

static void
test_path_copy (void)
{
	GOPath *p = go_path_new (), *c;
	static const char *const expected =
		"M 0 0;L 1 0;C 1 1 2 2 3 3;Z;M 9 9;L 8 8";

	g_assert_null (go_path_copy (NULL));

	/* An empty path copies to an empty path */
	c = go_path_copy (p);
	g_assert_nonnull (c);
	g_assert_true (c != p);
	assert_path (c, GO_PATH_DIRECTION_FORWARD, "");
	go_path_free (c);

	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 1, 0);
	go_path_curve_to (p, 1, 1, 2, 2, 3, 3);
	go_path_close (p);
	go_path_move_to (p, 9, 9);
	go_path_line_to (p, 8, 8);

	c = go_path_copy (p);
	g_assert_true (c != p);
	assert_path (c, GO_PATH_DIRECTION_FORWARD, expected);

	/* The copy is independent */
	go_path_line_to (c, 100, 100);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, expected);
	go_path_clear (p);
	assert_path (c, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 0;C 1 1 2 2 3 3;Z;M 9 9;L 8 8;L 100 100");

	go_path_free (c);
	go_path_free (p);
}

static void
test_path_copy_restricted (void)
{
	GOPath *p = go_path_new (), *c;

	g_assert_null (go_path_copy_restricted (NULL, 0, 1));

	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 1, 1);
	go_path_move_to (p, 5, 5);
	go_path_line_to (p, 6, 6);
	go_path_close (p);

	c = go_path_copy_restricted (p, 2, 3);
	g_assert_true (c != p);
	assert_path (c, GO_PATH_DIRECTION_FORWARD, "M 5 5;L 6 6");
	go_path_free (c);

	c = go_path_copy_restricted (p, 0, 1);
	assert_path (c, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 1");
	go_path_free (c);

	c = go_path_copy_restricted (p, 1, 3);
	assert_path (c, GO_PATH_DIRECTION_FORWARD, "M 1 1;M 5 5;L 6 6");
	go_path_free (c);

	/* Empty range gives an empty path */
	c = go_path_copy_restricted (p, 3, 3);
	g_assert_nonnull (c);
	assert_path (c, GO_PATH_DIRECTION_FORWARD, "");
	go_path_free (c);

	/* Original untouched */
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 1;M 5 5;L 6 6;Z");
	go_path_free (p);
}

static void
test_path_append (void)
{
	GOPath *a = go_path_new (), *b = go_path_new (), *r;

	go_path_move_to (a, 0, 0);
	go_path_line_to (a, 1, 1);
	go_path_move_to (b, 5, 5);
	go_path_curve_to (b, 6, 6, 7, 7, 8, 8);
	go_path_close (b);

	/* path1 is extended in place and returned */
	r = go_path_append (a, b);
	g_assert_true (r == a);
	assert_path (a, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 1;M 5 5;C 6 6 7 7 8 8;Z");
	/* path2 is left alone */
	assert_path (b, GO_PATH_DIRECTION_FORWARD, "M 5 5;C 6 6 7 7 8 8;Z");

	/* NULL operands */
	g_assert_true (go_path_append (a, NULL) == a);
	g_assert_null (go_path_append (NULL, NULL));
	r = go_path_append (NULL, b);
	g_assert_nonnull (r);
	g_assert_true (r != b);
	assert_path (r, GO_PATH_DIRECTION_FORWARD, "M 5 5;C 6 6 7 7 8 8;Z");
	go_path_free (r);

	/* Appending an empty path changes nothing */
	{
		GOPath *e = go_path_new ();
		go_path_append (b, e);
		assert_path (b, GO_PATH_DIRECTION_FORWARD, "M 5 5;C 6 6 7 7 8 8;Z");
		go_path_free (e);
	}

	go_path_free (a);
	go_path_free (b);
}

static void
test_path_scale (void)
{
	GOPath *p = go_path_new (), *s;

	go_path_move_to (p, 1, 2);
	go_path_line_to (p, 3, 4);
	go_path_curve_to (p, 1, 1, 2, 2, 3, 3);
	go_path_close (p);

	s = go_path_scale (p, 2, 10);
	g_assert_true (s != p);
	assert_path (s, GO_PATH_DIRECTION_FORWARD, "M 2 20;L 6 40;C 2 10 4 20 6 30;Z");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 1 2;L 3 4;C 1 1 2 2 3 3;Z");
	go_path_free (s);

	s = go_path_scale (p, -1, 0.5);
	assert_path (s, GO_PATH_DIRECTION_FORWARD, "M -1 1;L -3 2;C -1 0.5 -2 1 -3 1.5;Z");
	go_path_free (s);

	s = go_path_scale (p, 1, 1);
	assert_path (s, GO_PATH_DIRECTION_FORWARD, "M 1 2;L 3 4;C 1 1 2 2 3 3;Z");
	go_path_free (s);

	/* Scaling an empty path gives an empty path */
	go_path_clear (p);
	s = go_path_scale (p, 3, 3);
	g_assert_nonnull (s);
	assert_path (s, GO_PATH_DIRECTION_FORWARD, "");
	go_path_free (s);
	go_path_free (p);
}

static void
test_path_to_svg (void)
{
	GOPath *p = go_path_new ();
	char *s;

	s = go_path_to_svg (p);
	g_assert_cmpstr (s, ==, "");
	g_free (s);

	/* The operator is only repeated when it changes */
	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 10, 0);
	go_path_line_to (p, 10, 10);
	go_path_close (p);
	s = go_path_to_svg (p);
	g_assert_cmpstr (s, ==, " M 0 0 L 10 0 10 10 Z");
	g_free (s);

	go_path_clear (p);
	go_path_move_to (p, 1.5, -2.25);
	go_path_curve_to (p, 1, 2, 3, 4, 5, 6);
	go_path_curve_to (p, 7, 8, 9, 10, 11, 12);
	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 0.5, 0.5);
	s = go_path_to_svg (p);
	g_assert_cmpstr (s, ==, " M 1.5 -2.25 C 1 2 3 4 5 6 7 8 9 10 11 12 M 0 0 L 0.5 0.5");
	g_free (s);

	go_path_free (p);
}

static void
test_path_svg_basic (void)
{
	GOPath *p;

	g_assert_null (go_path_new_from_svg (NULL));

	/* Nothing at all is an empty path */
	p = go_path_new_from_svg ("");
	g_assert_nonnull (p);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "");
	go_path_free (p);
	p = go_path_new_from_svg ("   ");
	g_assert_nonnull (p);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "");
	go_path_free (p);

	p = go_path_new_from_svg ("M 0 0 L 10 0 L 10 10 Z");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 10 0;L 10 10;Z");
	go_path_free (p);

	/* Lower case close, no spaces needed around commands, comma separators */
	p = go_path_new_from_svg ("M0,0L10,10z");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 10 10;Z");
	go_path_free (p);

	/* Compact negative numbers */
	p = go_path_new_from_svg ("M0 0L10-5");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 10 -5");
	go_path_free (p);

	/* Several coordinate pairs after one operator */
	p = go_path_new_from_svg ("M 0 0 L 1 1 2 2 3 3");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 1 1;L 2 2;L 3 3");
	go_path_free (p);

	/* Relative coordinates accumulate */
	p = go_path_new_from_svg ("m 1 1 l 2 2 3 3 l -1 -1");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 1 1;L 3 3;L 6 6;L 5 5");
	go_path_free (p);

	/* Cubic curves, absolute then relative */
	p = go_path_new_from_svg ("M 0 0 C 1 2 3 4 5 6 c 1 1 1 1 1 1");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;C 1 2 3 4 5 6;C 6 7 6 7 6 7");
	go_path_free (p);

	/* A command without arguments is harmless */
	p = go_path_new_from_svg ("M 0 0 L");
	g_assert_nonnull (p);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0");
	go_path_free (p);

	/* Marker-style paths (as used by go-marker.c) */
	p = go_path_new_from_svg ("M-1,-1 L-1,1 1,1 1,-1 z");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M -1 -1;L -1 1;L 1 1;L 1 -1;Z");
	go_path_free (p);
}

static void
test_path_svg_numbers (void)
{
	GOPath *p;

	p = go_path_new_from_svg ("M 1.5e1 -2.5E-1 L .5 5.");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 15 -0.25;L 0.5 5");
	go_path_free (p);

	p = go_path_new_from_svg ("M +3 -.5 L 1e2 2e+1");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 3 -0.5;L 100 20");
	go_path_free (p);

	p = go_path_new_from_svg ("M 0.125 123.456 L 007 0");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0.125 123.456;L 7 0");
	go_path_free (p);

	p = go_path_new_from_svg ("M 1e-3 5E0");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0.001 5");
	go_path_free (p);
}

static void
test_path_svg_errors (void)
{
	/* Unknown commands and stray numbers make the whole path invalid */
	g_assert_null (go_path_new_from_svg ("X"));
	g_assert_null (go_path_new_from_svg ("M 0 0 X 1 1"));
	g_assert_null (go_path_new_from_svg ("M 1 2 3"));
	g_assert_null (go_path_new_from_svg ("M 1e 2"));
	g_assert_null (go_path_new_from_svg ("5"));
	g_assert_null (go_path_new_from_svg ("M . ."));

	/* Commands that are recognised but not implemented (yet) fail when
	   they are given arguments.  Update when they get implemented. */
	g_assert_null (go_path_new_from_svg ("M 0 0 H 10"));
	g_assert_null (go_path_new_from_svg ("M 0 0 V 10"));
	g_assert_null (go_path_new_from_svg ("M 0 0 S 1 1 2 2"));
	g_assert_null (go_path_new_from_svg ("M 0 0 T 1 1"));
	g_assert_null (go_path_new_from_svg ("M 0 0 A 1 1 0 0 0 2 2"));
}

static void
test_path_svg_quadratic (void)
{
	GOPath *p;

	/* A quadratic Bezier is emitted as the equivalent cubic */
	p = go_path_new_from_svg ("M 0 0 Q 3 6 6 0");
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;C 2 4 4 4 6 0");
	go_path_free (p);

	/* The current point after a quadratic is its end point, for
	   absolute commands ... */
	p = go_path_new_from_svg ("M 10 0 Q 15 10 20 0 Q 25 -10 30 0");
	assert_path (p, GO_PATH_DIRECTION_FORWARD,
		     "M 10 0;C 13.333333333333334 6.666666666666667 16.666666666666668 6.666666666666667 20 0;"
		     "C 23.333333333333332 -6.666666666666667 26.666666666666668 -6.666666666666667 30 0");
	go_path_free (p);

	/* ... and for the commands that follow it */
	p = go_path_new_from_svg ("M 10 0 Q 15 10 20 0 L 25 0");
	assert_path (p, GO_PATH_DIRECTION_FORWARD,
		     "M 10 0;C 13.333333333333334 6.666666666666667 16.666666666666668 6.666666666666667 20 0;L 25 0");
	go_path_free (p);
	p = go_path_new_from_svg ("M 10 0 Q 15 10 20 0 l 5 0");
	assert_path (p, GO_PATH_DIRECTION_FORWARD,
		     "M 10 0;C 13.333333333333334 6.666666666666667 16.666666666666668 6.666666666666667 20 0;L 25 0");
	go_path_free (p);

	/* Relative quadratic */
	p = go_path_new_from_svg ("m 10 0 q 5 10 10 0 l 5 0");
	assert_path (p, GO_PATH_DIRECTION_FORWARD,
		     "M 10 0;C 13.333333333333334 6.666666666666667 16.666666666666668 6.666666666666667 20 0;L 25 0");
	go_path_free (p);
}

static void
test_path_svg_roundtrip (void)
{
	GOPath *p = go_path_new (), *q;
	char *svg, *a, *b;

	go_path_move_to (p, 0, 0);
	go_path_line_to (p, 10, 0);
	go_path_line_to (p, 10, 10);
	go_path_curve_to (p, 5, 12, 2, 12, 0, 10);
	go_path_close (p);
	go_path_move_to (p, 20, 20);
	go_path_line_to (p, 21.5, -3.25);
	go_path_move_to (p, 30, 30);
	go_path_move_to (p, 31, 31);

	svg = go_path_to_svg (p);
	q = go_path_new_from_svg (svg);
	g_assert_nonnull (q);
	a = path_events (p, GO_PATH_DIRECTION_FORWARD);
	b = path_events (q, GO_PATH_DIRECTION_FORWARD);
	assert_events_equal (b, a);
	g_free (a);
	g_free (b);
	g_free (svg);
	go_path_free (p);
	go_path_free (q);
}

static void
test_path_arc (void)
{
	GOPath *p = go_path_new ();
	char *e;

	/* Quarter circle */
	go_path_arc (p, 0, 0, 1, 1, 0, M_PI / 2);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g 1 0 1", T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* Centered elsewhere, elliptical */
	go_path_clear (p);
	go_path_arc (p, 10, 20, 2, 3, 0, M_PI / 2);
	e = g_strdup_printf ("M 12 20;C 12 %.17g %.17g 23 10 23", 20 + 3 * T_, 10 + 2 * T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* Negative sweep */
	go_path_clear (p);
	go_path_arc (p, 0, 0, 1, 1, 0, -M_PI / 2);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g -1 0 -1", -T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* Full circle: four segments, not closed */
	go_path_clear (p);
	go_path_arc (p, 0, 0, 1, 1, 0, 2 * M_PI);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g 1 0 1;C %.17g 1 -1 %.17g -1 0;"
			     "C -1 %.17g %.17g -1 0 -1;C %.17g -1 1 %.17g 1 0",
			     T_, T_, -T_, T_, -T_, -T_, T_, -T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* Sweeps of more than a full turn are clipped to a full circle */
	{
		GOPath *q = go_path_new ();
		go_path_arc (q, 0, 0, 1, 1, 0, 4 * M_PI);
		e = path_events (q, GO_PATH_DIRECTION_FORWARD);
		{
			char *e2 = path_events (p, GO_PATH_DIRECTION_FORWARD);
			assert_events_equal (e, e2);
			g_free (e2);
		}
		g_free (e);
		go_path_free (q);
	}

	/* A three-quarter arc needs three segments */
	go_path_clear (p);
	go_path_arc (p, 0, 0, 1, 1, 0, 3 * M_PI / 2);
	e = path_events (p, GO_PATH_DIRECTION_FORWARD);
	{
		char **parts = g_strsplit (e, ";", -1);
		g_assert_cmpuint (g_strv_length (parts), ==, 4);
		g_strfreev (parts);
	}
	g_free (e);

	go_path_free (p);
}

static void
test_path_arc_to (void)
{
	GOPath *p = go_path_new ();
	char *e;

	/* arc_to joins the arc to the current point with a line */
	go_path_move_to (p, 0, 0);
	go_path_arc_to (p, 5, 0, 1, 1, M_PI, 0);
	e = g_strdup_printf ("M 0 0;L 4 0;C 4 %.17g %.17g 1 5 1;C %.17g 1 6 %.17g 6 0",
			     T_, 5 - T_, 5 + T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* arc is a new subpath instead */
	go_path_clear (p);
	go_path_move_to (p, 0, 0);
	go_path_arc (p, 5, 0, 1, 1, M_PI, 0);
	e = path_events (p, GO_PATH_DIRECTION_FORWARD);
	g_assert_true (g_str_has_prefix (e, "M 0 0;M 4 "));
	g_free (e);

	go_path_free (p);
}

static void
test_path_wedges (void)
{
	GOPath *p = go_path_new ();
	char *e;

	/* Pie wedge: arc, line to the center, close */
	go_path_pie_wedge (p, 0, 0, 1, 1, 0, M_PI / 2);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g 1 0 1;L 0 0;Z", T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* Full pie is a closed ellipse, with no line to the center */
	go_path_clear (p);
	go_path_pie_wedge (p, 0, 0, 1, 1, 0, 2 * M_PI);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g 1 0 1;C %.17g 1 -1 %.17g -1 0;"
			     "C -1 %.17g %.17g -1 0 -1;C %.17g -1 1 %.17g 1 0;Z",
			     T_, T_, -T_, T_, -T_, -T_, T_, -T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* Ring wedge: outer arc, line to the inner arc, inner arc back, close */
	go_path_clear (p);
	go_path_ring_wedge (p, 0, 0, 2, 2, 1, 1, 0, M_PI / 2);
	e = g_strdup_printf ("M 2 0;C 2 %.17g %.17g 2 0 2;L 0 1;C %.17g 1 1 %.17g 1 0;Z",
			     2 * T_, 2 * T_, T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);

	/* Inner and outer radii are swapped if given the wrong way around */
	{
		GOPath *q = go_path_new ();
		char *e2;
		go_path_ring_wedge (q, 0, 0, 1, 1, 2, 2, 0, M_PI / 2);
		e2 = path_events (q, GO_PATH_DIRECTION_FORWARD);
		assert_path (p, GO_PATH_DIRECTION_FORWARD, e2);
		g_free (e2);
		go_path_free (q);
	}

	/* A zero inner radius degenerates to a pie wedge */
	{
		GOPath *q = go_path_new ();
		go_path_ring_wedge (q, 0, 0, 1, 1, 0, 0, 0, M_PI / 2);
		e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g 1 0 1;L 0 0;Z", T_, T_);
		assert_path (q, GO_PATH_DIRECTION_FORWARD, e);
		g_free (e);
		go_path_free (q);
	}

	go_path_free (p);
}

static void
test_path_odf_basic (void)
{
	GOPath *p;
	GHashTable *vars = g_hash_table_new (g_str_hash, g_str_equal);
	double f0 = 7, v1 = -3;

	g_hash_table_insert (vars, (gpointer)"?f0", &f0);
	g_hash_table_insert (vars, (gpointer)"$1", &v1);

	g_assert_null (go_path_new_from_odf_enhanced_path (NULL, NULL));
	g_assert_null (go_path_new_from_odf_enhanced_path ("", NULL));
	g_assert_null (go_path_new_from_odf_enhanced_path ("   ", NULL));
	/* No actions: nothing to build */
	g_assert_null (go_path_new_from_odf_enhanced_path ("N", NULL));
	g_assert_null (go_path_new_from_odf_enhanced_path ("F S N", NULL));

	p = go_path_new_from_odf_enhanced_path ("M 0 0 L 10 0 L 10 10 Z", NULL);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;L 10 0;L 10 10;Z");
	go_path_free (p);

	p = go_path_new_from_odf_enhanced_path ("M 0 0 C 1 2 3 4 5 6 L 7 7", NULL);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;C 1 2 3 4 5 6;L 7 7");
	go_path_free (p);

	/* Variables, and the commands that have no effect */
	p = go_path_new_from_odf_enhanced_path ("M ?f0 0 L $1 4 N F S Z", vars);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 7 0;L -3 4;Z");
	go_path_free (p);

	/* Unknown variable, or no variables at all */
	g_assert_null (go_path_new_from_odf_enhanced_path ("M ?f9 0", vars));
	g_assert_null (go_path_new_from_odf_enhanced_path ("M ?f0 0", NULL));

	/* Unknown commands */
	g_assert_null (go_path_new_from_odf_enhanced_path ("M 0 0 K 1 1", NULL));
	g_assert_null (go_path_new_from_odf_enhanced_path ("?", NULL));
	g_assert_null (go_path_new_from_odf_enhanced_path ("M 0 0 5", NULL));

	g_hash_table_destroy (vars);
}

static void
test_path_odf_quadratic (void)
{
	GOPath *p;

	p = go_path_new_from_odf_enhanced_path ("M 0 0 Q 3 6 6 0", NULL);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, "M 0 0;C 2 4 4 4 6 0");
	go_path_free (p);

	/* The current point after a quadratic is its end point */
	p = go_path_new_from_odf_enhanced_path ("M 10 0 Q 15 10 20 0 Q 25 -10 30 0", NULL);
	assert_path (p, GO_PATH_DIRECTION_FORWARD,
		     "M 10 0;C 13.333333333333334 6.666666666666667 16.666666666666668 6.666666666666667 20 0;"
		     "C 23.333333333333332 -6.666666666666667 26.666666666666668 -6.666666666666667 30 0");
	go_path_free (p);
}

static void
test_path_odf_arcs (void)
{
	GOPath *p;
	char *e;

	/* Clockwise arc from (1,0) to (0,1) around the unit circle, joined
	   to the current point (W) or starting a new subpath (V) */
	p = go_path_new_from_odf_enhanced_path ("W -1 -1 1 1 1 0 0 1", NULL);
	e = g_strdup_printf ("L 1 0;C 1 %.17g %.17g 1 0 1", T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);

	p = go_path_new_from_odf_enhanced_path ("V -1 -1 1 1 1 0 0 1", NULL);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g 1 0 1", T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);

	/* Counter-clockwise: the long way around, in three segments, ending
	   in the same place */
	p = go_path_new_from_odf_enhanced_path ("B -1 -1 1 1 1 0 0 1", NULL);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g -1 0 -1;C %.17g -1 -1 %.17g -1 0;"
			     "C -1 %.17g %.17g 1 0 1", -T_, T_, -T_, -T_, T_, -T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);

	p = go_path_new_from_odf_enhanced_path ("A -1 -1 1 1 1 0 0 1", NULL);
	e = path_events (p, GO_PATH_DIRECTION_FORWARD);
	g_assert_true (g_str_has_prefix (e, "L 1 "));
	g_free (e);
	go_path_free (p);

	/* Arcs specified by angles in degrees */
	p = go_path_new_from_odf_enhanced_path ("U 0 0 1 1 0 90", NULL);
	e = g_strdup_printf ("M 1 0;C 1 %.17g %.17g 1 0 1", T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);

	p = go_path_new_from_odf_enhanced_path ("M 5 5 T 0 0 1 1 0 90", NULL);
	e = g_strdup_printf ("M 5 5;L 1 0;C 1 %.17g %.17g 1 0 1", T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);
}

static void
test_path_odf_after_arc (void)
{
	GOPath *p;
	char *e;

	/* After an arc the current point is where the arc ended, here (0,1);
	   a quadratic curve starting there has its first control point
	   one third of the way from it to the quadratic's control point. */
	p = go_path_new_from_odf_enhanced_path ("W -1 -1 1 1 1 0 0 1 Q 1 1 2 2", NULL);
	e = g_strdup_printf ("L 1 0;C 1 %.17g %.17g 1 0 1;"
			     "C 0.66666666666666663 1 1.3333333333333333 1.3333333333333333 2 2",
			     T_, T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);
}

static void
test_path_odf_quadrants (void)
{
	GOPath *p;
	char *e;

	/* A quarter of an ellipse from the current point (0,0) to (1,1),
	   starting horizontally */
	p = go_path_new_from_odf_enhanced_path ("X 1 1", NULL);
	e = g_strdup_printf ("M 0 0;C %.17g 0 1 %.17g 1 1", T_, 1 - T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);

	/* Starting vertically instead */
	p = go_path_new_from_odf_enhanced_path ("Y 1 1", NULL);
	e = g_strdup_printf ("M 0 0;C 0 %.17g %.17g 1 1 1", T_, 1 - T_);
	assert_path (p, GO_PATH_DIRECTION_FORWARD, e);
	g_free (e);
	go_path_free (p);
}

static void
register_path_tests (void)
{
	g_test_add_func ("/utils/go-path/lifecycle", test_path_lifecycle);
	g_test_add_func ("/utils/go-path/options", test_path_options);
	g_test_add_func ("/utils/go-path/building", test_path_building);
	g_test_add_func ("/utils/go-path/clear", test_path_clear);
	g_test_add_func ("/utils/go-path/large", test_path_large);
	g_test_add_func ("/utils/go-path/interpret", test_path_interpret);
	g_test_add_func ("/utils/go-path/interpret_full", test_path_interpret_full);
	g_test_add_func ("/utils/go-path/copy", test_path_copy);
	g_test_add_func ("/utils/go-path/copy_restricted", test_path_copy_restricted);
	g_test_add_func ("/utils/go-path/append", test_path_append);
	g_test_add_func ("/utils/go-path/scale", test_path_scale);
	g_test_add_func ("/utils/go-path/to_svg", test_path_to_svg);
	g_test_add_func ("/utils/go-path/svg/basic", test_path_svg_basic);
	g_test_add_func ("/utils/go-path/svg/numbers", test_path_svg_numbers);
	g_test_add_func ("/utils/go-path/svg/errors", test_path_svg_errors);
	g_test_add_func ("/utils/go-path/svg/quadratic", test_path_svg_quadratic);
	g_test_add_func ("/utils/go-path/svg/roundtrip", test_path_svg_roundtrip);
	g_test_add_func ("/utils/go-path/arc", test_path_arc);
	g_test_add_func ("/utils/go-path/arc_to", test_path_arc_to);
	g_test_add_func ("/utils/go-path/wedges", test_path_wedges);
	g_test_add_func ("/utils/go-path/odf/basic", test_path_odf_basic);
	g_test_add_func ("/utils/go-path/odf/quadratic", test_path_odf_quadratic);
	g_test_add_func ("/utils/go-path/odf/arcs", test_path_odf_arcs);
	g_test_add_func ("/utils/go-path/odf/after_arc", test_path_odf_after_arc);
	g_test_add_func ("/utils/go-path/odf/quadrants", test_path_odf_quadrants);
}

/* ------------------------------------------------------------------ */
/* go-bezier.c                                                         */
/* ------------------------------------------------------------------ */

/* The shape of a spline is examined through the path it produces: a
   starting point followed by one cubic segment per interval. */

typedef struct {
	double x, y;
} Pt;

typedef struct {
	Pt start;
	GArray *c1, *c2, *end;		/* of Pt, one entry per segment */
	gboolean closed;
	int n_other;			/* number of unexpected actions */
} Segments;

static void
seg_move (void *cl, const GOPathPoint *p)
{
	Segments *s = cl;
	s->start.x = p->x;
	s->start.y = p->y;
}

static void
seg_line (void *cl, G_GNUC_UNUSED const GOPathPoint *p)
{
	((Segments *)cl)->n_other++;
}

static void
seg_curve (void *cl, const GOPathPoint *a, const GOPathPoint *b, const GOPathPoint *c)
{
	Segments *s = cl;
	Pt pa = { a->x, a->y }, pb = { b->x, b->y }, pc = { c->x, c->y };
	g_array_append_val (s->c1, pa);
	g_array_append_val (s->c2, pb);
	g_array_append_val (s->end, pc);
}

static void
seg_close (void *cl)
{
	((Segments *)cl)->closed = TRUE;
}

static void
segments_init (Segments *s, GOPath *path)
{
	memset (s, 0, sizeof (*s));
	s->c1 = g_array_new (FALSE, FALSE, sizeof (Pt));
	s->c2 = g_array_new (FALSE, FALSE, sizeof (Pt));
	s->end = g_array_new (FALSE, FALSE, sizeof (Pt));
	go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
			   seg_move, seg_line, seg_curve, seg_close, s);
}

static void
segments_clear (Segments *s)
{
	g_array_free (s->c1, TRUE);
	g_array_free (s->c2, TRUE);
	g_array_free (s->end, TRUE);
}

#define SEG(s, field, i) g_array_index ((s)->field, Pt, (i))

/* Check the defining properties of a natural/periodic cubic spline
   through the points (x[i],y[i]), i = 0..n-1. */
static void
check_spline (const GOBezierSpline *sp, const double *x, const double *y, int n, gboolean closed)
{
	GOPath *path = go_bezier_spline_to_path ((GOBezierSpline *)sp);
	Segments seg;
	int nseg = closed ? n : n - 1;
	int i, d;

	segments_init (&seg, path);
	g_assert_cmpint (seg.n_other, ==, 0);
	g_assert_true (seg.closed == closed);
	g_assert_cmpuint (seg.c1->len, ==, (guint)nseg);

	/* Passes through all the data points, in order */
	assert_close (seg.start.x, x[0]);
	assert_close (seg.start.y, y[0]);
	for (i = 0; i < nseg; i++) {
		assert_close (SEG (&seg, end, i).x, x[(i + 1) % n]);
		assert_close (SEG (&seg, end, i).y, y[(i + 1) % n]);
	}

	for (d = 0; d < 2; d++) {
		/* Coordinates of data point i, and handles around it */
#define P(i)   (d ? y[(i) % n] : x[(i) % n])
#define OUT(i) (d ? SEG (&seg, c1, (i)).y : SEG (&seg, c1, (i)).x)
#define IN_SEG(j) (d ? SEG (&seg, c2, (j)).y : SEG (&seg, c2, (j)).x)
#define IN(i)  IN_SEG ((i) - 1)
		/* Tangent at point i, three times the handle offset */
#define B(i)   ((i) % n < nseg ? 3 * (OUT ((i) % n) - P (i)) : 3 * (P (i) - IN ((i) % n)))

		/* C1: the handles on both sides of a point are symmetric */
		for (i = 1; i < nseg; i++)
			assert_close (IN (i) + OUT (i), 2 * P (i));
		if (closed)
			assert_close (IN_SEG (nseg - 1) + OUT (0), 2 * P (0));

		if (closed) {
			/* C2, periodic: b[i-1] + 4 b[i] + b[i+1] = 3 (P[i+1] - P[i-1]) */
			for (i = 0; i < n; i++)
				assert_close (B ((i + n - 1) % n) + 4 * B (i) + B ((i + 1) % n),
					      3 * (P (i + 1) - P ((i + n - 1) % n)));
		} else {
			for (i = 1; i + 1 < n; i++)
				assert_close (B (i - 1) + 4 * B (i) + B (i + 1),
					      3 * (P (i + 1) - P (i - 1)));
			/* End conditions */
			assert_close (2 * B (0) + B (1), 3 * (P (1) - P (0)));
			assert_close (B (n - 2) + 2 * B (n - 1),
				      3 * (P (n - 1) - P (n - 2)));
		}
#undef P
#undef OUT
#undef IN_SEG
#undef IN
#undef B
	}

	segments_clear (&seg);
	go_path_free (path);
}

static void
test_bezier_degenerate (void)
{
	double x[] = { 3 }, y[] = { 4 };
	GOBezierSpline *sp;
	GOPath *path;

	g_assert_null (go_bezier_spline_init (x, y, 0, FALSE));
	g_assert_null (go_bezier_spline_init (x, y, -3, TRUE));

	/* A single point is never closed, and is just a point */
	sp = go_bezier_spline_init (x, y, 1, TRUE);
	g_assert_nonnull (sp);
	g_assert_cmpint (sp->n, ==, 1);
	g_assert_false (sp->closed);
	path = go_bezier_spline_to_path (sp);
	assert_path (path, GO_PATH_DIRECTION_FORWARD, "M 3 4");
	go_path_free (path);
	go_bezier_spline_destroy (sp);

	sp = go_bezier_spline_init (x, y, 1, FALSE);
	g_assert_cmpint (sp->n, ==, 1);
	g_assert_false (sp->closed);
	go_bezier_spline_destroy (sp);
}

static void
test_bezier_fields (void)
{
	double x[] = { 0, 1, 2, 3 }, y[] = { 0, 1, 0, 1 };
	GOBezierSpline *sp;

	sp = go_bezier_spline_init (x, y, 4, FALSE);
	g_assert_cmpint (sp->n, ==, 4);
	g_assert_false (sp->closed);
	g_assert_cmpuint (sp->ref_count, ==, 1);
	go_bezier_spline_destroy (sp);

	sp = go_bezier_spline_init (x, y, 4, TRUE);
	g_assert_cmpint (sp->n, ==, 4);
	g_assert_true (sp->closed);
	go_bezier_spline_destroy (sp);
}

static void
test_bezier_two_points (void)
{
	double x[] = { 0, 3 }, y[] = { 0, 6 };
	GOBezierSpline *sp = go_bezier_spline_init (x, y, 2, FALSE);
	GOPath *path;

	/* Two points give a straight line with the handles at the thirds */
	path = go_bezier_spline_to_path (sp);
	assert_path (path, GO_PATH_DIRECTION_FORWARD, "M 0 0;C 1 2 2 4 3 6");
	go_path_free (path);
	go_bezier_spline_destroy (sp);

	/* The closed case goes there and back */
	sp = go_bezier_spline_init (x, y, 2, TRUE);
	g_assert_nonnull (sp);
	path = go_bezier_spline_to_path (sp);
	{
		Segments seg;
		segments_init (&seg, path);
		g_assert_true (seg.closed);
		g_assert_cmpuint (seg.c1->len, ==, 2);
		assert_close (SEG (&seg, end, 0).x, 3);
		assert_close (SEG (&seg, end, 0).y, 6);
		assert_close (SEG (&seg, end, 1).x, 0);
		assert_close (SEG (&seg, end, 1).y, 0);
		segments_clear (&seg);
	}
	go_path_free (path);
	go_bezier_spline_destroy (sp);
}

static void
test_bezier_collinear (void)
{
	/* Equally spaced points on a line: the spline is that line and its
	   handles sit at the thirds of every interval. */
	double x[6], y[6];
	GOBezierSpline *sp;
	GOPath *path;
	GString *expected = g_string_new (NULL);
	int i;

	for (i = 0; i < 6; i++) {
		x[i] = 1 + 2 * i;
		y[i] = 5 - 3 * i;
	}
	g_string_append_printf (expected, "M %g %g", x[0], y[0]);
	for (i = 0; i < 5; i++)
		g_string_append_printf (expected, ";C %.17g %.17g %.17g %.17g %g %g",
					x[i] + (x[i + 1] - x[i]) / 3, y[i] + (y[i + 1] - y[i]) / 3,
					x[i] + 2 * (x[i + 1] - x[i]) / 3, y[i] + 2 * (y[i + 1] - y[i]) / 3,
					x[i + 1], y[i + 1]);

	sp = go_bezier_spline_init (x, y, 6, FALSE);
	path = go_bezier_spline_to_path (sp);
	assert_path (path, GO_PATH_DIRECTION_FORWARD, expected->str);
	go_path_free (path);
	go_bezier_spline_destroy (sp);
	g_string_free (expected, TRUE);
}

static void
test_bezier_random (void)
{
	GRand *rand = g_rand_new_with_seed (1234);
	int n, closed;

	for (closed = 0; closed <= 1; closed++) {
		for (n = closed ? 3 : 2; n <= 12; n++) {
			double x[12], y[12];
			GOBezierSpline *sp;
			int i;

			for (i = 0; i < n; i++) {
				x[i] = g_rand_double_range (rand, -10, 10);
				y[i] = g_rand_double_range (rand, -10, 10);
			}
			sp = go_bezier_spline_init (x, y, n, closed);
			g_assert_nonnull (sp);
			g_assert_cmpint (sp->n, ==, n);
			g_assert_true (sp->closed == (closed != 0));
			check_spline (sp, x, y, n, closed);
			go_bezier_spline_destroy (sp);
		}
	}
	g_rand_free (rand);
}

static void
test_bezier_closed_symmetric (void)
{
	/* A square, point-symmetric about the origin: by symmetry the tangent
	   at each corner is 3/4 of the vector between its neighbours, which
	   puts the outgoing handle a quarter of that vector away. */
	double x[] = { 1, -1, -1, 1 };
	double y[] = { 1, 1, -1, -1 };
	GOBezierSpline *sp = go_bezier_spline_init (x, y, 4, TRUE);
	GOPath *path = go_bezier_spline_to_path (sp);
	Segments seg;
	int i;

	segments_init (&seg, path);
	g_assert_cmpuint (seg.c1->len, ==, 4);
	for (i = 0; i < 4; i++) {
		int prev = (i + 3) % 4, next = (i + 1) % 4;
		assert_close (SEG (&seg, c1, i).x, x[i] + 0.25 * (x[next] - x[prev]));
		assert_close (SEG (&seg, c1, i).y, y[i] + 0.25 * (y[next] - y[prev]));
	}
	segments_clear (&seg);
	go_path_free (path);
	go_bezier_spline_destroy (sp);
}

static void
test_bezier_refcount (void)
{
	double x[] = { 0, 1, 2 }, y[] = { 0, 1, 0 };
	GType t = go_bezier_spline_get_type ();
	GOBezierSpline *sp = go_bezier_spline_init (x, y, 3, FALSE);
	GOBezierSpline *sp2;
	GOPath *path;

	g_assert_true (G_TYPE_IS_BOXED (t));

	sp2 = g_boxed_copy (t, sp);
	g_assert_true (sp2 == sp);
	g_assert_cmpuint (sp->ref_count, ==, 2);
	g_boxed_free (t, sp2);
	g_assert_cmpuint (sp->ref_count, ==, 1);

	/* Still usable after the extra reference is gone */
	path = go_bezier_spline_to_path (sp);
	g_assert_nonnull (path);
	go_path_free (path);

	go_bezier_spline_destroy (sp);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
			       "*go_bezier_spline_destroy*");
	go_bezier_spline_destroy (NULL);
	g_test_assert_expected_messages ();
}

static void
test_bezier_to_cairo (void)
{
	double x[] = { 0, 1, 3, 4 }, y[] = { 0, 2, 1, 3 };
	int closed, flip;

	/* Whatever the path says, cairo gets the same thing, optionally
	   mirrored horizontally */
	for (closed = 0; closed <= 1; closed++) {
		for (flip = 0; flip <= 1; flip++) {
			GOBezierSpline *sp = go_bezier_spline_init (x, y, 4, closed);
			GOPath *path = go_bezier_spline_to_path (sp);
			cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
			cairo_t *cr = cairo_create (surf);
			cairo_path_t *cp;
			Segments seg;
			double sign = flip ? -1 : 1;
			unsigned i;
			int k = 0;

			segments_init (&seg, path);

			/* Anything pre-existing is discarded */
			cairo_move_to (cr, 77, 88);
			go_bezier_spline_to_cairo (sp, cr, flip);
			cp = cairo_copy_path (cr);
			g_assert_cmpint (cp->status, ==, CAIRO_STATUS_SUCCESS);

			g_assert_cmpint (cp->data[k].header.type, ==, CAIRO_PATH_MOVE_TO);
			assert_cairo_close (cp->data[k + 1].point.x, sign * seg.start.x);
			assert_cairo_close (cp->data[k + 1].point.y, seg.start.y);
			k += cp->data[k].header.length;

			for (i = 0; i < seg.c1->len; i++) {
				g_assert_cmpint (cp->data[k].header.type, ==, CAIRO_PATH_CURVE_TO);
				assert_cairo_close (cp->data[k + 1].point.x, sign * SEG (&seg, c1, i).x);
				assert_cairo_close (cp->data[k + 1].point.y, SEG (&seg, c1, i).y);
				assert_cairo_close (cp->data[k + 2].point.x, sign * SEG (&seg, c2, i).x);
				assert_cairo_close (cp->data[k + 2].point.y, SEG (&seg, c2, i).y);
				assert_cairo_close (cp->data[k + 3].point.x, sign * SEG (&seg, end, i).x);
				assert_cairo_close (cp->data[k + 3].point.y, SEG (&seg, end, i).y);
				k += cp->data[k].header.length;
			}
			if (closed)
				g_assert_cmpint (cp->data[k].header.type, ==, CAIRO_PATH_CLOSE_PATH);

			cairo_path_destroy (cp);
			cairo_destroy (cr);
			cairo_surface_destroy (surf);
			segments_clear (&seg);
			go_path_free (path);
			go_bezier_spline_destroy (sp);
		}
	}
}

static void
register_bezier_tests (void)
{
	g_test_add_func ("/utils/go-bezier/degenerate", test_bezier_degenerate);
	g_test_add_func ("/utils/go-bezier/fields", test_bezier_fields);
	g_test_add_func ("/utils/go-bezier/two_points", test_bezier_two_points);
	g_test_add_func ("/utils/go-bezier/collinear", test_bezier_collinear);
	g_test_add_func ("/utils/go-bezier/random", test_bezier_random);
	g_test_add_func ("/utils/go-bezier/closed_symmetric", test_bezier_closed_symmetric);
	g_test_add_func ("/utils/go-bezier/refcount", test_bezier_refcount);
	g_test_add_func ("/utils/go-bezier/to_cairo", test_bezier_to_cairo);
}

/* ------------------------------------------------------------------ */
/* go-marker.c                                                         */
/* ------------------------------------------------------------------ */

static void
test_marker_shape_names (void)
{
	int i, j;

	/* Names round-trip and are distinct */
	for (i = 0; i < GO_MARKER_MAX; i++) {
		const char *name = go_marker_shape_as_str ((GOMarkerShape)i);
		g_assert_nonnull (name);
		g_assert_cmpuint (strlen (name), >, 0);
		g_assert_cmpint (go_marker_shape_from_str (name), ==, i);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (name, !=, go_marker_shape_as_str ((GOMarkerShape)j));
	}

	/* Names are matched ignoring case */
	{
		char *up = g_ascii_strup (go_marker_shape_as_str (GO_MARKER_TRIANGLE_UP), -1);
		g_assert_cmpint (go_marker_shape_from_str (up), ==, GO_MARKER_TRIANGLE_UP);
		g_free (up);
	}

	/* Unknown names are no marker */
	g_assert_cmpint (go_marker_shape_from_str ("bogus"), ==, GO_MARKER_NONE);
	g_assert_cmpint (go_marker_shape_from_str (""), ==, GO_MARKER_NONE);

	/* Out of range shapes all get the same placeholder name, which is
	   not the name of any real shape */
	{
		const char *p = go_marker_shape_as_str (GO_MARKER_MAX);
		g_assert_nonnull (p);
		g_assert_cmpstr (go_marker_shape_as_str ((GOMarkerShape)-1), ==, p);
		g_assert_cmpstr (go_marker_shape_as_str ((GOMarkerShape)1000), ==, p);
		for (i = 0; i < GO_MARKER_MAX; i++)
			g_assert_cmpstr (go_marker_shape_as_str ((GOMarkerShape)i), !=, p);
	}
}

static void
test_marker_defaults (void)
{
	GOMarker *m = go_marker_new ();

	g_assert_true (GO_IS_MARKER (m));
	g_assert_true (G_IS_OBJECT (m));
	g_assert_cmpint (go_marker_get_shape (m), ==, GO_MARKER_NONE);
	g_assert_cmphex (go_marker_get_outline_color (m), ==, GO_COLOR_BLACK);
	g_assert_cmphex (go_marker_get_fill_color (m), ==, GO_COLOR_WHITE);
	g_assert_cmpint (go_marker_get_size (m), ==, 5);
	assert_close (go_marker_get_outline_width (m), 0.5);
	g_assert_true (go_marker_is_closed_shape (m));
	g_assert_cmpstr (g_type_name (go_marker_get_type ()), ==, "GOMarker");
	g_object_unref (m);
}

static void
test_marker_accessors (void)
{
	GOMarker *m = go_marker_new ();

	go_marker_set_shape (m, GO_MARKER_DIAMOND);
	g_assert_cmpint (go_marker_get_shape (m), ==, GO_MARKER_DIAMOND);
	go_marker_set_shape (m, GO_MARKER_DIAMOND);	/* no-op */
	g_assert_cmpint (go_marker_get_shape (m), ==, GO_MARKER_DIAMOND);

	go_marker_set_outline_color (m, 0x11223344u);
	g_assert_cmphex (go_marker_get_outline_color (m), ==, 0x11223344u);
	go_marker_set_fill_color (m, 0x55667788u);
	g_assert_cmphex (go_marker_get_fill_color (m), ==, 0x55667788u);
	/* ... and each is independent of the other */
	g_assert_cmphex (go_marker_get_outline_color (m), ==, 0x11223344u);

	go_marker_set_size (m, 12);
	g_assert_cmpint (go_marker_get_size (m), ==, 12);
	assert_close (go_marker_get_outline_width (m), 1.2);
	go_marker_set_size (m, 0);
	g_assert_cmpint (go_marker_get_size (m), ==, 0);
	assert_near (go_marker_get_outline_width (m), 0);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_set_size*");
	go_marker_set_size (m, -1);
	g_test_assert_expected_messages ();
	g_assert_cmpint (go_marker_get_size (m), ==, 0);

	/* Shapes made of lines only are not closed */
	go_marker_set_shape (m, GO_MARKER_SQUARE);
	g_assert_true (go_marker_is_closed_shape (m));
	go_marker_set_shape (m, GO_MARKER_CIRCLE);
	g_assert_true (go_marker_is_closed_shape (m));
	go_marker_set_shape (m, GO_MARKER_CROSS);
	g_assert_false (go_marker_is_closed_shape (m));
	go_marker_set_shape (m, GO_MARKER_X);
	g_assert_false (go_marker_is_closed_shape (m));
	go_marker_set_shape (m, GO_MARKER_ASTERISK);
	g_assert_false (go_marker_is_closed_shape (m));

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_is_closed_shape*");
	g_assert_false (go_marker_is_closed_shape (NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_set_shape*");
	go_marker_set_shape (NULL, GO_MARKER_SQUARE);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_set_outline_color*");
	go_marker_set_outline_color (NULL, 0);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_set_fill_color*");
	go_marker_set_fill_color (NULL, 0);
	g_test_assert_expected_messages ();

	g_object_unref (m);
}

static void
test_marker_assign_dup (void)
{
	GOMarker *a = go_marker_new (), *b = go_marker_new (), *c;

	go_marker_set_shape (a, GO_MARKER_CIRCLE);
	go_marker_set_size (a, 9);
	go_marker_set_outline_color (a, GO_COLOR_RED);
	go_marker_set_fill_color (a, GO_COLOR_GREEN);

	go_marker_assign (b, a);
	g_assert_cmpint (go_marker_get_shape (b), ==, GO_MARKER_CIRCLE);
	g_assert_cmpint (go_marker_get_size (b), ==, 9);
	g_assert_cmphex (go_marker_get_outline_color (b), ==, GO_COLOR_RED);
	g_assert_cmphex (go_marker_get_fill_color (b), ==, GO_COLOR_GREEN);

	/* Assigning to itself is harmless */
	go_marker_assign (a, a);
	g_assert_cmpint (go_marker_get_shape (a), ==, GO_MARKER_CIRCLE);

	c = go_marker_dup (a);
	g_assert_true (c != a);
	g_assert_true (GO_IS_MARKER (c));
	g_assert_cmpint (go_marker_get_shape (c), ==, GO_MARKER_CIRCLE);
	g_assert_cmpint (go_marker_get_size (c), ==, 9);
	g_assert_cmphex (go_marker_get_outline_color (c), ==, GO_COLOR_RED);
	g_assert_cmphex (go_marker_get_fill_color (c), ==, GO_COLOR_GREEN);

	/* Changes to the copy don't affect the original */
	go_marker_set_size (c, 3);
	go_marker_set_shape (c, GO_MARKER_X);
	g_assert_cmpint (go_marker_get_size (a), ==, 9);
	g_assert_cmpint (go_marker_get_shape (a), ==, GO_MARKER_CIRCLE);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_assign*");
	go_marker_assign (b, NULL);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_assign*");
	go_marker_assign (NULL, b);
	g_test_assert_expected_messages ();

	g_object_unref (a);
	g_object_unref (b);
	g_object_unref (c);
}

/* Pixel (premultiplied ARGB32) at (x,y) of an image surface */
static guint32
pixel_at (cairo_surface_t *surf, int x, int y)
{
	const unsigned char *data;
	int stride;

	cairo_surface_flush (surf);
	data = cairo_image_surface_get_data (surf);
	stride = cairo_image_surface_get_stride (surf);
	return ((const guint32 *)(data + y * stride))[x];
}

/* Opaque ARGB32 value for a GOColor */
static guint32
argb_of (GOColor c)
{
	return 0xff000000u | (GO_COLOR_UINT_R (c) << 16) |
		(GO_COLOR_UINT_G (c) << 8) | GO_COLOR_UINT_B (c);
}

static void
test_marker_render (void)
{
	GOMarker *m = go_marker_new ();
	cairo_surface_t *surf;
	cairo_t *cr;
	const GOColor fill = GO_COLOR_FROM_RGB (0, 0, 255);
	const GOColor line = GO_COLOR_FROM_RGB (255, 0, 0);

	go_marker_set_size (m, 20);
	go_marker_set_fill_color (m, fill);
	go_marker_set_outline_color (m, line);

	/* No marker: nothing is drawn */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surf);
	go_marker_render (m, cr, 30, 30, 1);
	g_assert_cmphex (pixel_at (surf, 30, 30), ==, 0);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Square of size 20: spans 20..40 in both directions, with a 2 pixel
	   wide outline centered on the border */
	go_marker_set_shape (m, GO_MARKER_SQUARE);
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surf);
	go_marker_render (m, cr, 30, 30, 1);
	g_assert_cmphex (pixel_at (surf, 30, 30), ==, argb_of (fill));
	g_assert_cmphex (pixel_at (surf, 24, 36), ==, argb_of (fill));
	g_assert_cmphex (pixel_at (surf, 20, 30), ==, argb_of (line));	/* left border */
	g_assert_cmphex (pixel_at (surf, 39, 30), ==, argb_of (line));	/* right border */
	g_assert_cmphex (pixel_at (surf, 30, 20), ==, argb_of (line));	/* top border */
	g_assert_cmpuint (pixel_at (surf, 15, 30), ==, 0);		/* outside */
	g_assert_cmpuint (pixel_at (surf, 30, 45), ==, 0);
	g_assert_cmpuint (pixel_at (surf, 5, 5), ==, 0);
	/* The drawing state of the caller is unchanged */
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Scale scales the marker; position moves it */
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surf);
	go_marker_render (m, cr, 20, 20, 0.5);	/* now 10 wide: 15..25 */
	g_assert_cmphex (pixel_at (surf, 20, 20), ==, argb_of (fill));
	g_assert_cmpuint (pixel_at (surf, 30, 20), ==, 0);
	g_assert_cmpuint (pixel_at (surf, 40, 40), ==, 0);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* A cross is drawn with the outline color over a square filled
	   with the fill color */
	go_marker_set_shape (m, GO_MARKER_CROSS);
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surf);
	go_marker_render (m, cr, 30, 30, 1);
	g_assert_cmphex (pixel_at (surf, 25, 25), ==, argb_of (fill));
	g_assert_cmphex (pixel_at (surf, 22, 29), ==, argb_of (line));	/* on the horizontal bar */
	g_assert_cmphex (pixel_at (surf, 29, 22), ==, argb_of (line));	/* on the vertical bar */
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Circle: center filled, corner of the bounding box not */
	go_marker_set_shape (m, GO_MARKER_CIRCLE);
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surf);
	go_marker_render (m, cr, 30, 30, 1);
	g_assert_cmphex (pixel_at (surf, 30, 30), ==, argb_of (fill));
	g_assert_cmpuint (pixel_at (surf, 21, 21), ==, 0);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Triangle up: the apex is at the top, the base at the bottom */
	go_marker_set_shape (m, GO_MARKER_TRIANGLE_UP);
	surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 60, 60);
	cr = cairo_create (surf);
	go_marker_render (m, cr, 30, 30, 1);
	g_assert_cmpuint (pixel_at (surf, 22, 22), ==, 0);	/* top left is empty */
	g_assert_cmphex (pixel_at (surf, 30, 36), ==, argb_of (fill));	/* the lower middle is filled */
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	g_object_unref (m);
}

static void
test_marker_cairo_surface (void)
{
	GOMarker *m = go_marker_new ();
	cairo_surface_t *target, *surf;
	cairo_t *cr;
	double w, h;

	go_marker_set_size (m, 10);
	go_marker_set_shape (m, GO_MARKER_SQUARE);

	/* On a pixel-based target everything is rounded up to whole pixels */
	target = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cr = cairo_create (target);

	surf = go_marker_create_cairo_surface (m, cr, 1, &w, &h);
	g_assert_nonnull (surf);
	g_assert_cmpint (cairo_surface_status (surf), ==, CAIRO_STATUS_SUCCESS);
	assert_close (w, 13);
	assert_close (h, 13);
	g_assert_cmpint (cairo_image_surface_get_width (surf), ==, 13);
	g_assert_cmpint (cairo_image_surface_get_height (surf), ==, 13);
	/* The marker is drawn into it, centered */
	g_assert_cmphex (pixel_at (surf, 6, 6), ==, argb_of (GO_COLOR_WHITE));
	g_assert_cmpuint (pixel_at (surf, 0, 0), ==, 0);
	cairo_surface_destroy (surf);

	surf = go_marker_create_cairo_surface (m, cr, 2, &w, NULL);
	assert_close (w, 23);
	g_assert_cmpint (cairo_image_surface_get_width (surf), ==, 23);
	cairo_surface_destroy (surf);

	/* Output arguments are optional */
	surf = go_marker_create_cairo_surface (m, cr, 1, NULL, NULL);
	g_assert_nonnull (surf);
	cairo_surface_destroy (surf);

	/* Vector targets don't round */
	if (cairo_surface_status (target) == CAIRO_STATUS_SUCCESS) {
		cairo_surface_t *pdf = cairo_pdf_surface_create (NULL, 100, 100);
		cairo_t *pcr = cairo_create (pdf);

		if (cairo_status (pcr) == CAIRO_STATUS_SUCCESS) {
			surf = go_marker_create_cairo_surface (m, pcr, 1, &w, &h);
			g_assert_nonnull (surf);
			assert_close (w, 12);
			assert_close (h, 12);
			cairo_surface_destroy (surf);
		}
		cairo_destroy (pcr);
		cairo_surface_destroy (pdf);
	}

	cairo_destroy (cr);
	cairo_surface_destroy (target);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_marker_create_cairo_surface*");
	g_assert_null (go_marker_create_cairo_surface (NULL, NULL, 1, NULL, NULL));
	g_test_assert_expected_messages ();

	g_object_unref (m);
}

static void
register_marker_tests (void)
{
	g_test_add_func ("/utils/go-marker/shape_names", test_marker_shape_names);
	g_test_add_func ("/utils/go-marker/defaults", test_marker_defaults);
	g_test_add_func ("/utils/go-marker/accessors", test_marker_accessors);
	g_test_add_func ("/utils/go-marker/assign_dup", test_marker_assign_dup);
	g_test_add_func ("/utils/go-marker/render", test_marker_render);
	g_test_add_func ("/utils/go-marker/cairo_surface", test_marker_cairo_surface);
}

/* ------------------------------------------------------------------ */
/* go-pattern.c                                                        */
/* ------------------------------------------------------------------ */

static void
test_pattern_names (void)
{
	int i, j;

	/* Names round-trip and are distinct */
	for (i = 0; i < GO_PATTERN_MAX; i++) {
		const char *name = go_pattern_as_str ((GOPatternType)i);
		g_assert_nonnull (name);
		g_assert_cmpuint (strlen (name), >, 0);
		g_assert_cmpint (go_pattern_from_str (name), ==, i);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (name, !=, go_pattern_as_str ((GOPatternType)j));
	}

	/* Unknown names are solid; invalid values all get one placeholder
	   name that isn't the name of any real pattern */
	g_assert_cmpint (go_pattern_from_str ("bogus"), ==, GO_PATTERN_SOLID);
	g_assert_cmpint (go_pattern_from_str (""), ==, GO_PATTERN_SOLID);
	{
		const char *p = go_pattern_as_str (GO_PATTERN_MAX);
		g_assert_nonnull (p);
		g_assert_cmpstr (go_pattern_as_str ((GOPatternType)-1), ==, p);
		for (i = 0; i < GO_PATTERN_MAX; i++)
			g_assert_cmpstr (go_pattern_as_str ((GOPatternType)i), !=, p);
	}
}

static void
test_pattern_is_solid (void)
{
	const GOColor fore = GO_COLOR_BLACK, back = GO_COLOR_WHITE;
	GOColor c;
	GOPattern pat;
	unsigned i;

	pat.fore = fore;
	pat.back = back;

	/* Plain solid uses the background, foreground-solid the foreground */
	pat.pattern = GO_PATTERN_SOLID;
	c = 0x12345678u;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, back);

	pat.pattern = GO_PATTERN_FOREGROUND_SOLID;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, fore);

	/* Greys mix the two colors */
	pat.pattern = GO_PATTERN_GREY75;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, 0x404040ffu);
	pat.pattern = GO_PATTERN_GREY50;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, 0x808080ffu);
	pat.pattern = GO_PATTERN_GREY25;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, 0xbfbfbfffu);
	pat.pattern = GO_PATTERN_GREY125;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, 0xdfdfdfffu);
	pat.pattern = GO_PATTERN_GREY625;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, 0xefefefffu);

	/* Same with colors */
	pat.fore = GO_COLOR_FROM_RGBA (200, 0, 100, 255);
	pat.back = GO_COLOR_FROM_RGBA (0, 100, 0, 255);
	pat.pattern = GO_PATTERN_GREY50;
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, GO_COLOR_FROM_RGBA (100, 50, 50, 255));

	/* Patterns with a visible structure are not solid, and leave @color alone */
	{
		const GOPatternType structured[] = {
			GO_PATTERN_HORIZ, GO_PATTERN_VERT, GO_PATTERN_DIAG_CROSS,
			GO_PATTERN_SMALL_CIRCLES, GO_PATTERN_THATCH, GO_PATTERN_BRICKS
		};
		for (i = 0; i < G_N_ELEMENTS (structured); i++) {
			pat.pattern = structured[i];
			c = 0xdeadbeefu;
			g_assert_false (go_pattern_is_solid (&pat, &c));
			g_assert_cmphex (c, ==, 0xdeadbeefu);
		}
	}

	/* Identical colors make anything solid */
	pat.fore = pat.back = GO_COLOR_FROM_RGB (1, 2, 3);
	for (i = 0; i < GO_PATTERN_MAX; i++) {
		pat.pattern = i;
		c = 0;
		g_assert_true (go_pattern_is_solid (&pat, &c));
		g_assert_cmphex (c, ==, GO_COLOR_FROM_RGB (1, 2, 3));
	}

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_is_solid*");
	g_assert_false (go_pattern_is_solid (NULL, &c));
	g_test_assert_expected_messages ();
}

static void
test_pattern_set_solid (void)
{
	GOPattern pat;
	GOColor c;

	pat.pattern = GO_PATTERN_BRICKS;
	pat.fore = GO_COLOR_RED;
	pat.back = GO_COLOR_GREEN;

	go_pattern_set_solid (&pat, GO_COLOR_BLUE);
	g_assert_cmpint (pat.pattern, ==, GO_PATTERN_SOLID);
	g_assert_true (go_pattern_is_solid (&pat, &c));
	g_assert_cmphex (c, ==, GO_COLOR_BLUE);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_set_solid*");
	go_pattern_set_solid (NULL, GO_COLOR_BLUE);
	g_test_assert_expected_messages ();
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
test_pattern_bitmaps (void)
{
	GOPattern pat;
	const guint8 *d;
	unsigned i;

	pat.fore = GO_COLOR_BLACK;
	pat.back = GO_COLOR_WHITE;

	/* Solid uses a full bitmap, foreground-solid an empty one */
	pat.pattern = GO_PATTERN_SOLID;
	d = go_pattern_get_pattern (&pat);
	g_assert_nonnull (d);
	for (i = 0; i < 8; i++)
		g_assert_cmphex (d[i], ==, 0xff);

	pat.pattern = GO_PATTERN_FOREGROUND_SOLID;
	d = go_pattern_get_pattern (&pat);
	for (i = 0; i < 8; i++)
		g_assert_cmphex (d[i], ==, 0x00);

	/* The share of set bits in the grey patterns matches the solid color
	   they stand for: black on white, with a share f of black, is a grey
	   with level 255 * (1 - f). */
	{
		const GOPatternType greys[] = {
			GO_PATTERN_GREY75, GO_PATTERN_GREY50, GO_PATTERN_GREY25,
			GO_PATTERN_GREY125, GO_PATTERN_GREY625
		};
		for (i = 0; i < G_N_ELEMENTS (greys); i++) {
			int bits = 0, k;
			GOColor c;

			pat.pattern = greys[i];
			d = go_pattern_get_pattern (&pat);
			for (k = 0; k < 8; k++)
				bits += __builtin_popcount (d[k]);
			g_assert_true (go_pattern_is_solid (&pat, &c));
			g_assert_cmpint (GO_COLOR_UINT_R (c), ==,
					 (int)rint (255 * (1 - bits / 64.0)));
		}
	}

	/* Every pattern has data */
	for (i = 0; i < GO_PATTERN_MAX; i++) {
		pat.pattern = i;
		g_assert_nonnull (go_pattern_get_pattern (&pat));
	}

	pat.pattern = GO_PATTERN_MAX;
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_get_pattern*");
	g_assert_null (go_pattern_get_pattern (&pat));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_get_pattern*");
	g_assert_null (go_pattern_get_pattern (NULL));
	g_test_assert_expected_messages ();
}

static void
test_pattern_svg_path (void)
{
	GOPattern pat;
	unsigned i;

	pat.fore = GO_COLOR_BLACK;
	pat.back = GO_COLOR_WHITE;

	/* Every pattern except the two plain solid ones has a description in
	   the built-in resource */
	for (i = 0; i < GO_PATTERN_MAX; i++) {
		double w = -1, h = -1;
		xmlChar *path;

		if (i == GO_PATTERN_SOLID || i == GO_PATTERN_FOREGROUND_SOLID)
			continue;
		pat.pattern = i;
		path = go_pattern_get_svg_path (&pat, &w, &h);
		g_assert_nonnull (path);
		g_assert_cmpuint (strlen (CXML2C (path)), >, 0);
		g_assert_cmpfloat (w, >, 0);
		g_assert_cmpfloat (h, >, 0);

		/* and the path can be understood */
		{
			GOPath *p = go_path_new_from_svg (CXML2C (path));
			g_assert_nonnull (p);
			go_path_free (p);
		}
		xmlFree (path);

		/* The size is optional */
		path = go_pattern_get_svg_path (&pat, NULL, NULL);
		g_assert_nonnull (path);
		xmlFree (path);
	}

	pat.pattern = GO_PATTERN_MAX;
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_get_svg_path*");
	g_assert_null (go_pattern_get_svg_path (&pat, NULL, NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_get_svg_path*");
	g_assert_null (go_pattern_get_svg_path (NULL, NULL, NULL));
	g_test_assert_expected_messages ();
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_pattern_cairo_solid (void)
{
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cairo_t *cr = cairo_create (surf);
	GOPattern pat;
	cairo_pattern_t *cp;
	double r, g, b, a;

	pat.fore = GO_COLOR_RED;
	pat.back = GO_COLOR_FROM_RGBA (0, 0, 255, 128);

	pat.pattern = GO_PATTERN_SOLID;
	cp = go_pattern_create_cairo_pattern (&pat, cr);
	g_assert_nonnull (cp);
	g_assert_cmpint (cairo_pattern_get_type (cp), ==, CAIRO_PATTERN_TYPE_SOLID);
	g_assert_cmpint (cairo_pattern_get_rgba (cp, &r, &g, &b, &a), ==, CAIRO_STATUS_SUCCESS);
	assert_close (r, 0);
	assert_close (g, 0);
	assert_close (b, 1);
	assert_close (a, 128 / 255.0);
	cairo_pattern_destroy (cp);

	pat.pattern = GO_PATTERN_FOREGROUND_SOLID;
	cp = go_pattern_create_cairo_pattern (&pat, cr);
	g_assert_cmpint (cairo_pattern_get_type (cp), ==, CAIRO_PATTERN_TYPE_SOLID);
	cairo_pattern_get_rgba (cp, &r, &g, &b, &a);
	assert_close (r, 1);
	assert_close (b, 0);
	assert_close (a, 1);
	cairo_pattern_destroy (cp);

	/* Greys are mixed */
	pat.fore = GO_COLOR_BLACK;
	pat.back = GO_COLOR_WHITE;
	pat.pattern = GO_PATTERN_GREY50;
	cp = go_pattern_create_cairo_pattern (&pat, cr);
	g_assert_cmpint (cairo_pattern_get_type (cp), ==, CAIRO_PATTERN_TYPE_SOLID);
	cairo_pattern_get_rgba (cp, &r, &g, &b, &a);
	assert_close (r, 128 / 255.0);
	assert_close (g, 128 / 255.0);
	cairo_pattern_destroy (cp);

	/* Equal colors: solid whatever the pattern */
	pat.fore = pat.back = GO_COLOR_GREEN;
	pat.pattern = GO_PATTERN_BRICKS;
	cp = go_pattern_create_cairo_pattern (&pat, cr);
	g_assert_cmpint (cairo_pattern_get_type (cp), ==, CAIRO_PATTERN_TYPE_SOLID);
	cairo_pattern_destroy (cp);

	pat.pattern = GO_PATTERN_MAX;
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_create_cairo_pattern*");
	g_assert_null (go_pattern_create_cairo_pattern (&pat, cr));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_pattern_create_cairo_pattern*");
	g_assert_null (go_pattern_create_cairo_pattern (NULL, cr));
	g_test_assert_expected_messages ();

	cairo_destroy (cr);
	cairo_surface_destroy (surf);
}

/* Create a tile for pattern type @t, with red foreground and blue background */
static cairo_pattern_t *
make_tile (cairo_t *cr, GOPatternType t, cairo_surface_t **tile)
{
	GOPattern pat;
	cairo_pattern_t *cp;

	pat.fore = GO_COLOR_RED;
	pat.back = GO_COLOR_BLUE;
	pat.pattern = t;
	cp = go_pattern_create_cairo_pattern (&pat, cr);
	g_assert_nonnull (cp);
	g_assert_cmpint (cairo_pattern_get_type (cp), ==, CAIRO_PATTERN_TYPE_SURFACE);
	g_assert_cmpint (cairo_pattern_get_surface (cp, tile), ==, CAIRO_STATUS_SUCCESS);
	return cp;
}

static void
test_pattern_cairo_tiles (void)
{
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cairo_t *cr = cairo_create (surf);
	const guint32 R = argb_of (GO_COLOR_RED), B = argb_of (GO_COLOR_BLUE);
	int i;

	/* Every pattern with a structure is a repeating square tile that uses
	   both colors (up to anti-aliasing); all others are plain colors */
	for (i = 0; i < GO_PATTERN_MAX; i++) {
		GOPattern pat;
		GOColor solid;
		cairo_surface_t *tile;
		cairo_pattern_t *cp;
		int size, x, y;
		gboolean has_fore = FALSE, has_back = FALSE;

		pat.fore = GO_COLOR_RED;
		pat.back = GO_COLOR_BLUE;
		pat.pattern = i;
		if (go_pattern_is_solid (&pat, &solid)) {
			cp = go_pattern_create_cairo_pattern (&pat, cr);
			g_assert_cmpint (cairo_pattern_get_type (cp), ==, CAIRO_PATTERN_TYPE_SOLID);
			cairo_pattern_destroy (cp);
			continue;
		}

		cp = make_tile (cr, i, &tile);
		g_assert_cmpint (cairo_pattern_get_extend (cp), ==, CAIRO_EXTEND_REPEAT);
		size = cairo_image_surface_get_width (tile);
		g_assert_cmpint (size, >, 0);
		g_assert_cmpint (cairo_image_surface_get_height (tile), ==, size);

		for (y = 0; y < size; y++)
			for (x = 0; x < size; x++) {
				guint32 p = pixel_at (tile, x, y);
				has_fore |= (p == R);
				has_back |= (p == B);
			}
		g_assert_true (has_fore);
		g_assert_true (has_back);
		cairo_pattern_destroy (cp);
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surf);
}

static void
test_pattern_cairo_pixels (void)
{
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cairo_t *cr = cairo_create (surf);
	const guint32 R = argb_of (GO_COLOR_RED), B = argb_of (GO_COLOR_BLUE);
	cairo_surface_t *t;
	cairo_pattern_t *cp;
	int x, y;

	/* Horizontal stripe, tile 8x8, 4 pixel wide line at y = 4: rows 2..5 */
	cp = make_tile (cr, GO_PATTERN_HORIZ, &t);
	for (y = 0; y < 8; y++)
		for (x = 0; x < 8; x++)
			g_assert_cmphex (pixel_at (t, x, y), ==, (y >= 2 && y <= 5) ? R : B);
	cairo_pattern_destroy (cp);

	/* Vertical stripe: columns 2..5 */
	cp = make_tile (cr, GO_PATTERN_VERT, &t);
	for (y = 0; y < 8; y++)
		for (x = 0; x < 8; x++)
			g_assert_cmphex (pixel_at (t, x, y), ==, (x >= 2 && x <= 5) ? R : B);
	cairo_pattern_destroy (cp);

	/* Thin horizontal and vertical: 2 pixel lines in a 6x6 tile */
	cp = make_tile (cr, GO_PATTERN_THIN_HORIZ, &t);
	for (y = 0; y < 6; y++)
		for (x = 0; x < 6; x++)
			g_assert_cmphex (pixel_at (t, x, y), ==, (y == 2 || y == 3) ? R : B);
	cairo_pattern_destroy (cp);

	cp = make_tile (cr, GO_PATTERN_THIN_VERT, &t);
	for (y = 0; y < 6; y++)
		for (x = 0; x < 6; x++)
			g_assert_cmphex (pixel_at (t, x, y), ==, (x == 2 || x == 3) ? R : B);
	cairo_pattern_destroy (cp);

	cp = make_tile (cr, GO_PATTERN_THIN_HORIZ_CROSS, &t);
	for (y = 0; y < 6; y++)
		for (x = 0; x < 6; x++)
			g_assert_cmphex (pixel_at (t, x, y), ==,
					 (x == 2 || x == 3 || y == 2 || y == 3) ? R : B);
	cairo_pattern_destroy (cp);

	/* Diagonal crosshatch: the center is on both diagonals, the middle of
	   an edge is on neither */
	cp = make_tile (cr, GO_PATTERN_DIAG_CROSS, &t);
	g_assert_cmphex (pixel_at (t, 8, 8), ==, R);
	g_assert_cmphex (pixel_at (t, 7, 7), ==, R);
	g_assert_cmphex (pixel_at (t, 8, 0), ==, B);
	g_assert_cmphex (pixel_at (t, 0, 8), ==, B);
	cairo_pattern_destroy (cp);

	/* Circles: ring versus inside */
	cp = make_tile (cr, GO_PATTERN_SMALL_CIRCLES, &t);
	g_assert_cmphex (pixel_at (t, 4, 4), ==, B);	/* inside */
	g_assert_cmphex (pixel_at (t, 7, 4), ==, R);	/* on the ring */
	g_assert_cmphex (pixel_at (t, 0, 0), ==, B);	/* corner */
	cairo_pattern_destroy (cp);

	/* Large circles are drawn with inverted colors: the ring is the
	   background color */
	cp = make_tile (cr, GO_PATTERN_LARGE_CIRCLES, &t);
	g_assert_cmphex (pixel_at (t, 8, 8), ==, R);
	g_assert_cmphex (pixel_at (t, 14, 8), ==, B);
	g_assert_cmphex (pixel_at (t, 0, 0), ==, R);
	cairo_pattern_destroy (cp);

	/* Diagonals: opposite orientations */
	cp = make_tile (cr, GO_PATTERN_REV_DIAG, &t);
	g_assert_cmphex (pixel_at (t, 3, 3), ==, R);	/* main diagonal */
	g_assert_cmphex (pixel_at (t, 6, 0), ==, B);
	cairo_pattern_destroy (cp);
	cp = make_tile (cr, GO_PATTERN_DIAG, &t);
	g_assert_cmphex (pixel_at (t, 3, 8), ==, R);	/* anti-diagonal x+y = 12 */
	g_assert_cmphex (pixel_at (t, 3, 3), ==, B);
	cairo_pattern_destroy (cp);

	cairo_destroy (cr);
	cairo_surface_destroy (surf);
}

static void
register_pattern_tests (void)
{
	g_test_add_func ("/utils/go-pattern/names", test_pattern_names);
	g_test_add_func ("/utils/go-pattern/is_solid", test_pattern_is_solid);
	g_test_add_func ("/utils/go-pattern/set_solid", test_pattern_set_solid);
	g_test_add_func ("/utils/go-pattern/bitmaps", test_pattern_bitmaps);
	g_test_add_func ("/utils/go-pattern/svg_path", test_pattern_svg_path);
	g_test_add_func ("/utils/go-pattern/cairo_solid", test_pattern_cairo_solid);
	g_test_add_func ("/utils/go-pattern/cairo_tiles", test_pattern_cairo_tiles);
	g_test_add_func ("/utils/go-pattern/cairo_pixels", test_pattern_cairo_pixels);
}

/* ------------------------------------------------------------------ */
/* go-glib-extras.c                                                    */
/* ------------------------------------------------------------------ */

static void
test_glib_extras_slist (void)
{
	GSList *l, *m;
	int sum = 0;
	char *a = (char *)"a", *b = (char *)"b", *c = (char *)"c";

	/* go_slist_create takes a NULL terminated list; a NULL first item
	   gives the empty list */
	l = go_slist_create (a, b, c, NULL);
	g_assert_cmpuint (g_slist_length (l), ==, 3);
	g_assert_true (l->data == a);
	g_assert_true (l->next->data == b);
	g_assert_true (l->next->next->data == c);
	g_slist_free (l);

	l = go_slist_create (a, NULL);
	g_assert_cmpuint (g_slist_length (l), ==, 1);
	g_slist_free (l);

	g_assert_null (go_slist_create (NULL));

	/* Copying string lists copies the strings */
	l = go_slist_create ("x", "y", NULL);
	m = go_string_slist_copy (l);
	g_assert_cmpuint (g_slist_length (m), ==, 2);
	g_assert_cmpstr (m->data, ==, "x");
	g_assert_cmpstr (m->next->data, ==, "y");
	g_assert_true (m->data != l->data);
	g_slist_free_full (m, g_free);
	g_slist_free (l);

	/* The list macros */
	l = NULL;
	GO_SLIST_APPEND (l, GINT_TO_POINTER (2));
	GO_SLIST_APPEND (l, GINT_TO_POINTER (3));
	GO_SLIST_PREPEND (l, GINT_TO_POINTER (1));
	g_assert_cmpuint (g_slist_length (l), ==, 3);
	GO_SLIST_FOREACH (l, void, item, sum = sum * 10 + GPOINTER_TO_INT (item));
	g_assert_cmpint (sum, ==, 123);
	GO_SLIST_REVERSE (l);
	sum = 0;
	GO_SLIST_FOREACH (l, void, item, sum = sum * 10 + GPOINTER_TO_INT (item));
	g_assert_cmpint (sum, ==, 321);
	g_slist_free (l);
}

static gint
cmp_pointer_int (gconstpointer a, gconstpointer b)
{
	return GPOINTER_TO_INT (a) - GPOINTER_TO_INT (b);
}

static void
test_glib_extras_list_macros (void)
{
	GSList *s = NULL, *t = NULL;
	GList *l = NULL, *m = NULL;
	int sum = 0;

	GO_SLIST_APPEND (s, GINT_TO_POINTER (3));
	GO_SLIST_APPEND (s, GINT_TO_POINTER (1));
	GO_SLIST_APPEND (t, GINT_TO_POINTER (2));
	GO_SLIST_CONCAT (s, t);
	g_assert_cmpuint (g_slist_length (s), ==, 3);
	GO_SLIST_SORT (s, cmp_pointer_int);
	g_assert_cmpint (GPOINTER_TO_INT (s->data), ==, 1);
	g_assert_cmpint (GPOINTER_TO_INT (s->next->data), ==, 2);
	g_assert_cmpint (GPOINTER_TO_INT (s->next->next->data), ==, 3);
	GO_SLIST_REMOVE (s, GINT_TO_POINTER (2));
	g_assert_cmpuint (g_slist_length (s), ==, 2);
	GO_SLIST_REVERSE (s);
	g_assert_cmpint (GPOINTER_TO_INT (s->data), ==, 3);
	g_slist_free (s);

	GO_LIST_APPEND (l, GINT_TO_POINTER (3));
	GO_LIST_APPEND (l, GINT_TO_POINTER (1));
	GO_LIST_PREPEND (l, GINT_TO_POINTER (5));
	GO_LIST_APPEND (m, GINT_TO_POINTER (2));
	GO_LIST_CONCAT (l, m);
	g_assert_cmpuint (g_list_length (l), ==, 4);
	GO_LIST_SORT (l, cmp_pointer_int);
	GO_LIST_FOREACH (l, void, item, sum = sum * 10 + GPOINTER_TO_INT (item));
	g_assert_cmpint (sum, ==, 1235);
	GO_LIST_REMOVE (l, GINT_TO_POINTER (3));
	GO_LIST_REVERSE (l);
	g_assert_cmpint (GPOINTER_TO_INT (l->data), ==, 5);
	g_assert_cmpuint (g_list_length (l), ==, 3);
	g_list_free (l);
}

static gpointer
map_double (gpointer v)
{
	return GINT_TO_POINTER (2 * GPOINTER_TO_INT (v));
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
test_glib_extras_deprecated (void)
{
	GSList *l = go_slist_create (GINT_TO_POINTER (1), GINT_TO_POINTER (2), GINT_TO_POINTER (3), NULL);
	GSList *m = go_slist_map (l, map_double);
	GPtrArray *arr = g_ptr_array_new ();
	GHashTable *h = g_hash_table_new (g_str_hash, g_str_equal);
	GSList *keys;

	g_assert_cmpuint (g_slist_length (m), ==, 3);
	g_assert_cmpint (GPOINTER_TO_INT (m->data), ==, 2);
	g_assert_cmpint (GPOINTER_TO_INT (m->next->data), ==, 4);
	g_assert_cmpint (GPOINTER_TO_INT (m->next->next->data), ==, 6);
	/* The original is untouched */
	g_assert_cmpint (GPOINTER_TO_INT (l->data), ==, 1);
	g_slist_free (l);
	g_slist_free (m);

	g_assert_null (go_slist_map (NULL, map_double));

	go_ptr_array_insert (arr, (gpointer)"b", 0);
	go_ptr_array_insert (arr, (gpointer)"d", 1);
	go_ptr_array_insert (arr, (gpointer)"a", 0);
	go_ptr_array_insert (arr, (gpointer)"c", 2);
	g_assert_cmpuint (arr->len, ==, 4);
	g_assert_cmpstr (arr->pdata[0], ==, "a");
	g_assert_cmpstr (arr->pdata[1], ==, "b");
	g_assert_cmpstr (arr->pdata[2], ==, "c");
	g_assert_cmpstr (arr->pdata[3], ==, "d");
	g_ptr_array_free (arr, TRUE);

	g_assert_null (go_hash_keys (h));
	g_hash_table_insert (h, (gpointer)"one", GINT_TO_POINTER (1));
	g_hash_table_insert (h, (gpointer)"two", GINT_TO_POINTER (2));
	g_hash_table_insert (h, (gpointer)"three", GINT_TO_POINTER (3));
	keys = go_hash_keys (h);
	g_assert_cmpuint (g_slist_length (keys), ==, 3);
	g_assert_nonnull (g_slist_find_custom (keys, "one", (GCompareFunc)strcmp));
	g_assert_nonnull (g_slist_find_custom (keys, "two", (GCompareFunc)strcmp));
	g_assert_nonnull (g_slist_find_custom (keys, "three", (GCompareFunc)strcmp));
	g_slist_free (keys);
	g_hash_table_destroy (h);
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_glib_extras_list_index (void)
{
	GList *l = NULL;

	g_assert_cmpint (go_list_index_custom (NULL, (gpointer)"x", (GCompareFunc)strcmp), ==, -1);

	l = g_list_append (l, (gpointer)"a");
	l = g_list_append (l, (gpointer)"b");
	l = g_list_append (l, (gpointer)"c");
	g_assert_cmpint (go_list_index_custom (l, (gpointer)"a", (GCompareFunc)strcmp), ==, 0);
	g_assert_cmpint (go_list_index_custom (l, (gpointer)"b", (GCompareFunc)strcmp), ==, 1);
	g_assert_cmpint (go_list_index_custom (l, (gpointer)"c", (GCompareFunc)strcmp), ==, 2);
	g_assert_cmpint (go_list_index_custom (l, (gpointer)"d", (GCompareFunc)strcmp), ==, -1);
	/* The comparison, not the pointer, decides */
	{
		char copy[] = "b";
		g_assert_cmpint (go_list_index_custom (l, copy, (GCompareFunc)strcmp), ==, 1);
	}
	g_list_free (l);
}

static void
assert_split (const char *str, char delim, ...)
{
	GSList *l = go_strsplit_to_slist (str, delim), *p;
	va_list args;
	const char *expected;

	va_start (args, delim);
	for (p = l; (expected = va_arg (args, const char *)); p = p->next) {
		g_assert_nonnull (p);
		g_assert_cmpstr (p->data, ==, expected);
	}
	va_end (args);
	g_assert_null (p);
	g_slist_free_full (l, g_free);
}

static void
test_glib_extras_strsplit (void)
{
	assert_split ("a,b,c", ',', "a", "b", "c", NULL);
	assert_split ("abc", ',', "abc", NULL);
	assert_split ("", ',', NULL);
	assert_split ("a,,b", ',', "a", "", "b", NULL);
	assert_split (",a", ',', "", "a", NULL);
	assert_split ("a,", ',', "a", "", NULL);
	assert_split (",", ',', "", "", NULL);
	assert_split ("a b  c", ' ', "a", "b", "", "c", NULL);
	assert_split ("h\xc3\xa9;llo", ';', "h\xc3\xa9", "llo", NULL);
}

static void
test_glib_extras_str_compare (void)
{
	g_assert_cmpint (go_str_compare ("a", "a"), ==, 0);
	g_assert_cmpint (go_str_compare ("a", "b"), <, 0);
	g_assert_cmpint (go_str_compare ("b", "a"), >, 0);
	g_assert_cmpint (go_str_compare ("", "a"), <, 0);
	g_assert_cmpint (go_str_compare ("ab", "a"), >, 0);

	/* NULL is allowed, and sorts after everything else */
	g_assert_cmpint (go_str_compare (NULL, NULL), ==, 0);
	g_assert_cmpint (go_str_compare (NULL, "a"), >, 0);
	g_assert_cmpint (go_str_compare ("a", NULL), <, 0);
	g_assert_cmpint (go_str_compare (NULL, ""), >, 0);

	/* Usable for sorting */
	{
		GSList *l;

		l = g_slist_append (NULL, (gpointer)"pear");
		l = g_slist_append (l, NULL);
		l = g_slist_append (l, (gpointer)"apple");
		l = g_slist_sort (l, (GCompareFunc)go_str_compare);
		g_assert_cmpstr (l->data, ==, "apple");
		g_assert_cmpstr (l->next->data, ==, "pear");
		g_assert_null (l->next->next->data);
		g_slist_free (l);
	}
}

static void
test_glib_extras_ascii_strcase (void)
{
	GHashTable *h;

	g_assert_true (go_ascii_strcase_equal ("abc", "abc"));
	g_assert_true (go_ascii_strcase_equal ("abc", "ABC"));
	g_assert_true (go_ascii_strcase_equal ("AbC", "aBc"));
	g_assert_true (go_ascii_strcase_equal ("", ""));
	g_assert_false (go_ascii_strcase_equal ("abc", "abd"));
	g_assert_false (go_ascii_strcase_equal ("abc", "ab"));
	g_assert_false (go_ascii_strcase_equal ("", "a"));
	/* Only ASCII is folded */
	g_assert_false (go_ascii_strcase_equal ("\xc3\xa4", "\xc3\x84"));
	g_assert_true (go_ascii_strcase_equal ("\xc3\xa4x", "\xc3\xa4X"));

	/* Equal strings have equal hashes */
	g_assert_cmpuint (go_ascii_strcase_hash ("abc"), ==, go_ascii_strcase_hash ("ABC"));
	g_assert_cmpuint (go_ascii_strcase_hash ("Hello, World"), ==, go_ascii_strcase_hash ("hELLO, wORLD"));
	g_assert_cmpuint (go_ascii_strcase_hash ("\xc3\xa4x"), ==, go_ascii_strcase_hash ("\xc3\xa4X"));
	g_assert_cmpuint (go_ascii_strcase_hash (""), ==, go_ascii_strcase_hash (""));

	/* A hash table using them is case insensitive */
	h = g_hash_table_new (go_ascii_strcase_hash, go_ascii_strcase_equal);
	g_hash_table_insert (h, (gpointer)"Alpha", GINT_TO_POINTER (1));
	g_hash_table_insert (h, (gpointer)"beta", GINT_TO_POINTER (2));
	g_hash_table_insert (h, (gpointer)"ALPHA", GINT_TO_POINTER (3));	/* replaces */
	g_assert_cmpuint (g_hash_table_size (h), ==, 2);
	g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (h, "alpha")), ==, 3);
	g_assert_cmpint (GPOINTER_TO_INT (g_hash_table_lookup (h, "BETA")), ==, 2);
	g_assert_null (g_hash_table_lookup (h, "gamma"));
	g_hash_table_destroy (h);
}

static void
test_glib_extras_collate_casefold (void)
{
	g_assert_cmpint (go_utf8_collate_casefold ("abc", "abc"), ==, 0);
	g_assert_cmpint (go_utf8_collate_casefold ("abc", "ABC"), ==, 0);
	g_assert_cmpint (go_utf8_collate_casefold ("a", "B"), <, 0);
	g_assert_cmpint (go_utf8_collate_casefold ("B", "a"), >, 0);
	g_assert_cmpint (go_utf8_collate_casefold ("", "a"), <, 0);

	/* Non-ASCII letters and special foldings */
	g_assert_cmpint (go_utf8_collate_casefold ("\xc3\x84\x42", "\xc3\xa4\x62"), ==, 0);
	g_assert_cmpint (go_utf8_collate_casefold ("Stra\xc3\x9f" "e", "STRASSE"), ==, 0);
	g_assert_cmpint (go_utf8_collate_casefold ("\xce\xa3", "\xcf\x83"), ==, 0);	/* capital and small sigma */
}

static void
test_glib_extras_strcapital (void)
{
	static const struct {
		const char *in;
		gssize len;
		const char *out;
	} cases[] = {
		{ "hello", -1, "Hello" },
		{ "HELLO", -1, "Hello" },
		{ "hello world", -1, "Hello World" },
		{ "HELLO WORLD", -1, "Hello World" },
		{ "Very Much Like: this, ONE", -1, "Very Much Like: This, One" },
		{ "ab-cd", -1, "Ab-Cd" },
		{ "", -1, "" },
		{ " a", -1, " A" },
		{ "hello world", 5, "Hello" },
		{ "hello world", 0, "" },
		{ "hello world", 7, "Hello W" },
		{ "\xc3\xa9lan vital", -1, "\xc3\x89lan Vital" },
		{ "\xc3\x89LAN", -1, "\xc3\x89lan" }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		char *s = go_utf8_strcapital (cases[i].in, cases[i].len);
		g_assert_cmpstr (s, ==, cases[i].out);
		g_free (s);
	}
}

static void
test_glib_extras_strescape (void)
{
	GString *s = g_string_new (NULL);
	const char *samples[] = {
		"", "plain", "with \"double\" quotes", "back\\slash", "\\\"", "it's",
		"h\xc3\xa9llo w\xc3\xb6rld", "tab\there\nand there"
	};
	unsigned i;

	go_strescape (s, "abc");
	g_assert_cmpstr (s->str, ==, "\"abc\"");

	g_string_truncate (s, 0);
	go_strescape (s, "a\"b\\c");
	g_assert_cmpstr (s->str, ==, "\"a\\\"b\\\\c\"");

	g_string_truncate (s, 0);
	go_strescape (s, "");
	g_assert_cmpstr (s->str, ==, "\"\"");

	/* It appends */
	g_string_assign (s, "pre:");
	go_strescape (s, "x");
	g_assert_cmpstr (s->str, ==, "pre:\"x\"");

	/* Round trip through go_strunescape */
	for (i = 0; i < G_N_ELEMENTS (samples); i++) {
		GString *esc = g_string_new (NULL);
		GString *back = g_string_new (NULL);
		const char *end;

		go_strescape (esc, samples[i]);
		end = go_strunescape (back, esc->str);
		g_assert_nonnull (end);
		g_assert_true (end == esc->str + esc->len);
		g_assert_cmpstr (back->str, ==, samples[i]);
		g_string_free (esc, TRUE);
		g_string_free (back, TRUE);
	}
	g_string_free (s, TRUE);
}

static void
test_glib_extras_strunescape (void)
{
	GString *s = g_string_new ("pre:");
	const char *src, *end;

	/* Result is appended and the return value points past the closing quote */
	src = "\"abc\"rest";
	end = go_strunescape (s, src);
	g_assert_true (end == src + 5);
	g_assert_cmpstr (end, ==, "rest");
	g_assert_cmpstr (s->str, ==, "pre:abc");

	/* Any ASCII character can be the quote */
	g_string_truncate (s, 0);
	src = "'it\\'s' ok";
	end = go_strunescape (s, src);
	g_assert_cmpstr (s->str, ==, "it's");
	g_assert_cmpstr (end, ==, " ok");

	/* The other kind of quote needs no escaping */
	g_string_truncate (s, 0);
	end = go_strunescape (s, "\"it's\"");
	g_assert_cmpstr (s->str, ==, "it's");
	g_assert_cmpstr (end, ==, "");

	/* Backslash escapes the next character, whatever it is */
	g_string_truncate (s, 0);
	end = go_strunescape (s, "\"a\\\\b\\\"c\"");
	g_assert_cmpstr (s->str, ==, "a\\b\"c");
	g_assert_cmpstr (end, ==, "");

	/* Empty string */
	g_string_truncate (s, 0);
	end = go_strunescape (s, "\"\"x");
	g_assert_cmpstr (s->str, ==, "");
	g_assert_cmpstr (end, ==, "x");

	/* Unterminated: failure, and the target is left as it was */
	g_string_assign (s, "keep");
	g_assert_null (go_strunescape (s, "\"abc"));
	g_assert_cmpstr (s->str, ==, "keep");
	g_assert_null (go_strunescape (s, "\"abc\\"));
	g_assert_cmpstr (s->str, ==, "keep");
	g_assert_null (go_strunescape (s, "\"abc\\\""));
	g_assert_cmpstr (s->str, ==, "keep");
	g_assert_null (go_strunescape (s, "'abc\""));
	g_assert_cmpstr (s->str, ==, "keep");

	g_string_free (s, TRUE);
}

static void
test_glib_extras_gstring_append (void)
{
	GString *a = g_string_new ("abc"), *b = g_string_new ("def");

	go_string_append_gstring (a, b);
	g_assert_cmpstr (a->str, ==, "abcdef");
	g_assert_cmpuint (a->len, ==, 6);
	g_assert_cmpstr (b->str, ==, "def");

	/* Embedded NULs are copied too */
	g_string_truncate (b, 0);
	g_string_append_len (b, "x\0y", 3);
	go_string_append_gstring (a, b);
	g_assert_cmpuint (a->len, ==, 9);
	g_assert_cmpint (memcmp (a->str + 6, "x\0y", 3), ==, 0);

	g_string_truncate (b, 0);
	go_string_append_gstring (a, b);
	g_assert_cmpuint (a->len, ==, 9);

	g_string_assign (a, "ab");
	go_string_append_c_n (a, 'x', 3);
	g_assert_cmpstr (a->str, ==, "abxxx");
	go_string_append_c_n (a, 'y', 0);
	g_assert_cmpstr (a->str, ==, "abxxx");
	g_assert_cmpuint (a->len, ==, 5);
	go_string_append_c_n (a, 'z', 1);
	g_assert_cmpstr (a->str, ==, "abxxxz");
	go_string_append_c_n (a, 'q', 1000);
	g_assert_cmpuint (a->len, ==, 1006);
	g_assert_cmpint (a->str[1005], ==, 'q');
	g_assert_cmpint (a->str[1006], ==, 0);

	g_string_free (a, TRUE);
	g_string_free (b, TRUE);
}

static void
test_glib_extras_string_replace (void)
{
	GString *s;

	/* Same length, shorter, longer, at start, middle and end */
	s = g_string_new ("0123456789");
	go_string_replace (s, 2, 3, "ABC", 3);
	g_assert_cmpstr (s->str, ==, "01ABC56789");
	go_string_replace (s, 2, 3, "x", 1);
	g_assert_cmpstr (s->str, ==, "01x56789");
	go_string_replace (s, 2, 1, "yyyy", 4);
	g_assert_cmpstr (s->str, ==, "01yyyy56789");
	go_string_replace (s, 0, 2, "", 0);
	g_assert_cmpstr (s->str, ==, "yyyy56789");
	go_string_replace (s, 0, 0, "<<", 2);
	g_assert_cmpstr (s->str, ==, "<<yyyy56789");
	go_string_replace (s, s->len, 0, ">>", 2);
	g_assert_cmpstr (s->str, ==, "<<yyyy56789>>");
	g_assert_cmpuint (s->len, ==, 13);

	/* Negative lengths mean "to the end" and "all of the text" */
	go_string_replace (s, 6, -1, "!", -1);
	g_assert_cmpstr (s->str, ==, "<<yyyy!");
	go_string_replace (s, 0, -1, "all new", -1);
	g_assert_cmpstr (s->str, ==, "all new");
	go_string_replace (s, 3, 0, "_", -1);
	g_assert_cmpstr (s->str, ==, "all_ new");

	/* Only the given part of the text is used */
	go_string_replace (s, 0, 3, "abcdef", 2);
	g_assert_cmpstr (s->str, ==, "ab_ new");

	/* Nothing to do */
	go_string_replace (s, 1, 0, "zzz", 0);
	g_assert_cmpstr (s->str, ==, "ab_ new");
	g_string_free (s, TRUE);

	/* Empty target */
	s = g_string_new (NULL);
	go_string_replace (s, 0, 0, "abc", 3);
	g_assert_cmpstr (s->str, ==, "abc");
	g_string_free (s, TRUE);

	/* Bad position and null target */
	s = g_string_new ("abc");
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_string_replace*");
	go_string_replace (s, 4, 0, "x", 1);
	g_test_assert_expected_messages ();
	g_assert_cmpstr (s->str, ==, "abc");
	g_string_free (s, TRUE);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_string_replace*");
	go_string_replace (NULL, 0, 0, "x", 1);
	g_test_assert_expected_messages ();
}

static void
test_glib_extras_string_replace_random (void)
{
	/* Compare with doing the same with plain C strings */
	GRand *rand = g_rand_new_with_seed (99);
	int i;

	for (i = 0; i < 5000; i++) {
		char text[16], repl[8];
		int tlen = g_rand_int_range (rand, 0, 12);
		int rlen = g_rand_int_range (rand, 0, 6);
		int pos, oldlen, k;
		GString *s;
		GString *expected = g_string_new (NULL);

		for (k = 0; k < tlen; k++)
			text[k] = 'a' + g_rand_int_range (rand, 0, 26);
		text[tlen] = 0;
		for (k = 0; k < rlen; k++)
			repl[k] = 'A' + g_rand_int_range (rand, 0, 26);
		repl[rlen] = 0;

		pos = g_rand_int_range (rand, 0, tlen + 1);
		oldlen = g_rand_int_range (rand, 0, tlen - pos + 1);

		g_string_append_len (expected, text, pos);
		g_string_append_len (expected, repl, rlen);
		g_string_append (expected, text + pos + oldlen);

		s = g_string_new (text);
		go_string_replace (s, pos, oldlen, repl, rlen);
		g_assert_cmpstr (s->str, ==, expected->str);
		g_assert_cmpuint (s->len, ==, strlen (expected->str));

		g_string_free (s, TRUE);
		g_string_free (expected, TRUE);
	}
	g_rand_free (rand);
}

static void
test_glib_extras_issign (void)
{
	g_assert_cmpint (go_unichar_issign ('+'), ==, +1);
	g_assert_cmpint (go_unichar_issign ('-'), ==, -1);
	g_assert_cmpint (go_unichar_issign (0x2212), ==, -1);	/* minus sign */
	g_assert_cmpint (go_unichar_issign (0xff0b), ==, +1);	/* full-width plus */
	g_assert_cmpint (go_unichar_issign (0xff0d), ==, -1);	/* full-width hyphen-minus */

	g_assert_cmpint (go_unichar_issign (0), ==, 0);
	g_assert_cmpint (go_unichar_issign ('a'), ==, 0);
	g_assert_cmpint (go_unichar_issign ('0'), ==, 0);
	g_assert_cmpint (go_unichar_issign ('='), ==, 0);
	g_assert_cmpint (go_unichar_issign ('*'), ==, 0);
	g_assert_cmpint (go_unichar_issign (0x2013), ==, 0);	/* en dash */

	/* Whatever the character, the answer is one of three */
	{
		gunichar c;
		int plus = 0, minus = 0;
		for (c = 0; c < 0x110000; c++) {
			int r = go_unichar_issign (c);
			g_assert_true (r == -1 || r == 0 || r == 1);
			plus += (r == 1);
			minus += (r == -1);
		}
		/* There are several of each kind */
		g_assert_cmpint (plus, >, 1);
		g_assert_cmpint (minus, >, 1);
	}
}

static void
test_glib_extras_memdup (void)
{
	char src[] = "hello\0world";
	char *copy;
	int ints[] = { 1, 2, 3, 4 };
	int *icopy;

	copy = go_memdup (src, sizeof (src));
	g_assert_nonnull (copy);
	g_assert_true (copy != src);
	g_assert_cmpint (memcmp (copy, src, sizeof (src)), ==, 0);
	src[0] = 'X';
	g_assert_cmpint (copy[0], ==, 'h');
	g_free (copy);

	g_assert_null (go_memdup (NULL, 10));
	g_assert_null (go_memdup (src, 0));

	icopy = go_memdup_n (ints, 4, sizeof (int));
	g_assert_nonnull (icopy);
	g_assert_cmpint (memcmp (icopy, ints, sizeof (ints)), ==, 0);
	g_free (icopy);

	g_assert_null (go_memdup_n (NULL, 4, sizeof (int)));
	g_assert_null (go_memdup_n (ints, 0, sizeof (int)));
	g_assert_null (go_memdup_n (ints, 4, 0));
}

static void
test_glib_extras_destroy_password (void)
{
	char pw[] = "s3cr3t!";
	char other[] = "other";

	go_destroy_password (pw);
	g_assert_cmpint (strlen (pw), ==, 0);
	{
		unsigned i;
		for (i = 0; i < sizeof (pw); i++)
			g_assert_cmpint (pw[i], ==, 0);
	}
	g_assert_cmpstr (other, ==, "other");

	pw[0] = 0;
	go_destroy_password (pw);	/* empty string is fine */
}

static void
test_glib_extras_guess_encoding (void)
{
	GString *out = NULL;
	guint truncated = 99;
	const char *enc;

	/* Plain ASCII */
	enc = go_guess_encoding ("hello", 5, NULL, &out, &truncated);
	g_assert_nonnull (enc);
	g_assert_cmpstr (out->str, ==, "hello");
	g_assert_cmpuint (truncated, ==, 0);
	g_string_free (out, TRUE);

	/* Valid UTF-8 */
	out = NULL;
	enc = go_guess_encoding ("h\xc3\xa9llo", 6, NULL, &out, NULL);
	g_assert_cmpstr (enc, ==, "UTF-8");
	g_assert_cmpstr (out->str, ==, "h\xc3\xa9llo");
	g_string_free (out, TRUE);

	/* Not UTF-8: falls back to Latin-1 */
	out = NULL;
	enc = go_guess_encoding ("h\xe9llo", 5, NULL, &out, NULL);
	g_assert_cmpstr (enc, ==, "ISO-8859-1");
	g_assert_cmpstr (out->str, ==, "h\xc3\xa9llo");
	g_string_free (out, TRUE);

	/* Anything at all can be read as Latin-1 */
	enc = go_guess_encoding ("\x80\x81\x82\xff", 4, NULL, NULL, NULL);
	g_assert_nonnull (enc);

	/* The caller's guess is tried first, if it works */
	out = NULL;
	enc = go_guess_encoding ("h\xe9llo", 5, "ISO-8859-1", &out, NULL);
	g_assert_cmpstr (enc, ==, "ISO-8859-1");
	g_assert_cmpstr (out->str, ==, "h\xc3\xa9llo");
	g_string_free (out, TRUE);

	out = NULL;
	enc = go_guess_encoding ("h\xc3\xa9llo", 6, "ISO-8859-1", &out, NULL);
	g_assert_cmpstr (enc, ==, "ISO-8859-1");
	g_assert_cmpstr (out->str, ==, "h\xc3\x83\xc2\xa9llo");
	g_string_free (out, TRUE);

	/* ... and ignored if it doesn't, or is not an encoding at all */
	out = NULL;
	enc = go_guess_encoding ("h\xc3\xa9llo", 6, "ASCII", &out, NULL);
	g_assert_cmpstr (enc, ==, "UTF-8");
	g_string_free (out, TRUE);
	out = NULL;
	enc = go_guess_encoding ("h\xc3\xa9llo", 6, "NO-SUCH-ENCODING", &out, NULL);
	g_assert_cmpstr (enc, ==, "UTF-8");
	g_assert_cmpstr (out->str, ==, "h\xc3\xa9llo");
	g_string_free (out, TRUE);

	/* UTF-16 is recognized from its byte order mark */
	out = NULL;
	enc = go_guess_encoding ("\xff\xfeh\0i\0", 6, NULL, &out, NULL);
	g_assert_cmpstr (enc, ==, "UTF-16LE");
	g_assert_true (g_str_has_suffix (out->str, "hi"));
	g_string_free (out, TRUE);
	out = NULL;
	enc = go_guess_encoding ("\xfe\xff\0h\0i", 6, NULL, &out, NULL);
	g_assert_cmpstr (enc, ==, "UTF-16BE");
	g_assert_true (g_str_has_suffix (out->str, "hi"));
	g_string_free (out, TRUE);

	/* Empty input */
	out = NULL;
	enc = go_guess_encoding ("", 0, NULL, &out, &truncated);
	g_assert_nonnull (enc);
	g_assert_cmpstr (out->str, ==, "");
	g_assert_cmpuint (truncated, ==, 0);
	g_string_free (out, TRUE);

	/* An incomplete UTF-8 sequence at the very end is cut off and the
	   number of bytes left out is reported */
	out = NULL;
	truncated = 99;
	enc = go_guess_encoding ("abc\xc3", 4, NULL, &out, &truncated);
	g_assert_cmpstr (enc, ==, "UTF-8");
	g_assert_cmpstr (out->str, ==, "abc");
	g_assert_cmpuint (truncated, ==, 1);
	g_string_free (out, TRUE);
	out = NULL;
	enc = go_guess_encoding ("ab\xe2\x82", 4, NULL, &out, &truncated);
	g_assert_cmpstr (enc, ==, "UTF-8");
	g_assert_cmpstr (out->str, ==, "ab");
	g_assert_cmpuint (truncated, ==, 2);
	g_string_free (out, TRUE);

	/* Only the first len bytes are looked at */
	out = NULL;
	enc = go_guess_encoding ("abc\xe9", 3, NULL, &out, NULL);
	g_assert_cmpstr (out->str, ==, "abc");
	g_string_free (out, TRUE);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_guess_encoding*");
	g_assert_null (go_guess_encoding (NULL, 0, NULL, NULL, NULL));
	g_test_assert_expected_messages ();
}

static void
test_glib_extras_real_name (void)
{
	char *saved = g_strdup (g_getenv ("NAME"));
	char *name;
	const char *cname;

	/* NAME in the environment wins; reset the cache to get at it */
	g_setenv ("NAME", "Test User", TRUE);
	_go_glib_extras_shutdown ();
	g_assert_cmpstr (go_get_real_name (), ==, "Test User");
	/* The result is cached, and stable */
	g_assert_true (go_get_real_name () == go_get_real_name ());

	/* Names that are not UTF-8 are converted */
	g_setenv ("NAME", "Jos\xe9 Smith", TRUE);
	_go_glib_extras_shutdown ();
	name = g_strdup (go_get_real_name ());
	g_assert_true (g_utf8_validate (name, -1, NULL));
	g_assert_cmpstr (name, ==, "Jos\xc3\xa9 Smith");
	g_free (name);

	/* Without it something else is found */
	g_unsetenv ("NAME");
	_go_glib_extras_shutdown ();
	cname = go_get_real_name ();
	g_assert_nonnull (cname);
	g_assert_cmpuint (strlen (cname), >, 0);
	g_assert_true (g_utf8_validate (cname, -1, NULL));

	if (saved)
		g_setenv ("NAME", saved, TRUE);
	else
		g_unsetenv ("NAME");
	g_free (saved);
	_go_glib_extras_shutdown ();
}

static void
test_glib_extras_debug_flag (void)
{
	char *saved = g_strdup (g_getenv ("GO_DEBUG"));

	g_unsetenv ("GO_DEBUG");
	g_assert_false (go_debug_flag ("alpha"));
	g_assert_false (go_debug_flag (""));

	g_setenv ("GO_DEBUG", "alpha", TRUE);
	g_assert_true (go_debug_flag ("alpha"));
	g_assert_false (go_debug_flag ("beta"));
	g_assert_false (go_debug_flag ("alph"));
	g_assert_false (go_debug_flag ("alphabet"));

	g_setenv ("GO_DEBUG", "alpha:beta", TRUE);
	g_assert_true (go_debug_flag ("alpha"));
	g_assert_true (go_debug_flag ("beta"));
	g_assert_false (go_debug_flag ("gamma"));

	g_setenv ("GO_DEBUG", "alpha,gamma beta", TRUE);
	g_assert_true (go_debug_flag ("alpha"));
	g_assert_true (go_debug_flag ("beta"));
	g_assert_true (go_debug_flag ("gamma"));
	g_assert_false (go_debug_flag ("delta"));

	g_setenv ("GO_DEBUG", "all", TRUE);
	g_assert_true (go_debug_flag ("anything"));

	g_setenv ("GO_DEBUG", "", TRUE);
	g_assert_false (go_debug_flag ("alpha"));

	if (saved)
		g_setenv ("GO_DEBUG", saved, TRUE);
	else
		g_unsetenv ("GO_DEBUG");
	g_free (saved);
}

static void
test_glib_extras_check_finalized_leak (void)
{
	if (g_test_subprocess ()) {
		/* Nobody releases the object */
		GObject *obj = g_object_new (G_TYPE_OBJECT, NULL);
		go_debug_check_finalized (obj, "leaky");
		_go_glib_extras_shutdown ();
		return;
	}
	g_test_trap_subprocess (NULL, 0, 0);
	g_test_trap_assert_passed ();
	g_test_trap_assert_stderr ("*GObject \"leaky\" at *not finalized.*");
}

static void
test_glib_extras_check_finalized_ok (void)
{
	if (g_test_subprocess ()) {
		GObject *obj = g_object_new (G_TYPE_OBJECT, NULL);
		GObject *obj2 = g_object_new (G_TYPE_OBJECT, NULL);
		go_debug_check_finalized (obj, "released");
		go_debug_check_finalized (obj2, "also released");
		/* Registering again just replaces the name */
		go_debug_check_finalized (obj2, "renamed");
		g_object_unref (obj);
		g_object_unref (obj2);
		_go_glib_extras_shutdown ();
		return;
	}
	g_test_trap_subprocess (NULL, 0, 0);
	g_test_trap_assert_passed ();
	g_test_trap_assert_stderr_unmatched ("*not finalized*");

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_debug_check_finalized*");
	go_debug_check_finalized (NULL, "nothing");
	g_test_assert_expected_messages ();
}

/* ---- memory chunks ---- */

static void
test_glib_extras_mem_chunk_basic (void)
{
	GOMemChunk *chunk = go_mem_chunk_new ("test", 24, 1000);
	gpointer ptrs[200];
	int i, j;

	/* Atoms are distinct, don't overlap, and keep their contents */
	for (i = 0; i < 200; i++) {
		ptrs[i] = go_mem_chunk_alloc (chunk);
		g_assert_nonnull (ptrs[i]);
		memset (ptrs[i], i + 1, 24);
	}
	for (i = 0; i < 200; i++)
		for (j = 0; j < 24; j++)
			g_assert_cmpint (((unsigned char *)ptrs[i])[j], ==, (i + 1) & 0xff);
	for (i = 0; i < 200; i++)
		for (j = 0; j < i; j++)
			g_assert_true (ptrs[i] != ptrs[j]);

	/* Suitable alignment for something of this size */
	for (i = 0; i < 200; i++)
		g_assert_cmpuint (GPOINTER_TO_SIZE (ptrs[i]) % 8, ==, 0);

	for (i = 0; i < 200; i++)
		go_mem_chunk_free (chunk, ptrs[i]);
	go_mem_chunk_destroy (chunk, FALSE);
}

static void
test_glib_extras_mem_chunk_alignment (void)
{
	/* The alignment promised follows the size of the atoms */
	gsize sizes[] = { 1, 2, 4, 8, 12, 16, 24, 40, 100 };
	unsigned k;

	for (k = 0; k < G_N_ELEMENTS (sizes); k++) {
		GOMemChunk *chunk = go_mem_chunk_new ("align", sizes[k], 500);
		gsize align = MIN (sizes[k] & (~sizes[k] + 1), sizeof (void *));
		gpointer ptrs[40];
		int i;

		for (i = 0; i < 40; i++) {
			ptrs[i] = go_mem_chunk_alloc (chunk);
			g_assert_cmpuint (GPOINTER_TO_SIZE (ptrs[i]) % align, ==, 0);
			memset (ptrs[i], 0xee, sizes[k]);
		}
		for (i = 0; i < 40; i++)
			go_mem_chunk_free (chunk, ptrs[i]);
		go_mem_chunk_destroy (chunk, FALSE);
	}
}

static void
test_glib_extras_mem_chunk_alloc0 (void)
{
	GOMemChunk *chunk = go_mem_chunk_new ("zero", 32, 256);
	gpointer p[10];
	int i, j;

	/* Dirty the memory, give it back, and get it again cleared */
	for (i = 0; i < 10; i++) {
		p[i] = go_mem_chunk_alloc (chunk);
		memset (p[i], 0xff, 32);
	}
	for (i = 0; i < 10; i++)
		go_mem_chunk_free (chunk, p[i]);
	for (i = 0; i < 10; i++) {
		p[i] = go_mem_chunk_alloc0 (chunk);
		for (j = 0; j < 32; j++)
			g_assert_cmpint (((unsigned char *)p[i])[j], ==, 0);
	}
	for (i = 0; i < 10; i++)
		go_mem_chunk_free (chunk, p[i]);
	go_mem_chunk_destroy (chunk, FALSE);
}

static void
test_glib_extras_mem_chunk_reuse (void)
{
	/* Mixed allocation and release, checking that live atoms never change */
	GOMemChunk *chunk = go_mem_chunk_new ("reuse", 16, 160);	/* a few atoms per block */
	GRand *rand = g_rand_new_with_seed (7);
	gpointer live[100] = { NULL };
	int tag[100];
	int i, k, j;

	for (i = 0; i < 20000; i++) {
		k = g_rand_int_range (rand, 0, 100);
		if (live[k]) {
			for (j = 0; j < 16; j++)
				g_assert_cmpint (((unsigned char *)live[k])[j], ==, tag[k] & 0xff);
			go_mem_chunk_free (chunk, live[k]);
			live[k] = NULL;
		} else {
			live[k] = go_mem_chunk_alloc (chunk);
			tag[k] = g_rand_int_range (rand, 1, 256);
			memset (live[k], tag[k], 16);
		}
	}
	/* No two live atoms are the same */
	for (i = 0; i < 100; i++)
		for (j = 0; j < i; j++)
			if (live[i] && live[j])
				g_assert_true (live[i] != live[j]);
	for (i = 0; i < 100; i++)
		if (live[i])
			go_mem_chunk_free (chunk, live[i]);
	go_mem_chunk_destroy (chunk, FALSE);
	g_rand_free (rand);
}

static void
cb_collect_leak (gpointer data, gpointer user)
{
	GHashTable *seen = user;
	g_assert_true (g_hash_table_add (seen, data));
}

static void
test_glib_extras_mem_chunk_leaks (void)
{
	GOMemChunk *chunk = go_mem_chunk_new ("leaks", 20, 200);
	gpointer p[50];
	GHashTable *seen = g_hash_table_new (g_direct_hash, g_direct_equal);
	int i;

	/* Nothing allocated, nothing leaked */
	go_mem_chunk_foreach_leak (chunk, cb_collect_leak, seen);
	g_assert_cmpuint (g_hash_table_size (seen), ==, 0);

	for (i = 0; i < 50; i++)
		p[i] = go_mem_chunk_alloc (chunk);

	/* Everything is outstanding */
	go_mem_chunk_foreach_leak (chunk, cb_collect_leak, seen);
	g_assert_cmpuint (g_hash_table_size (seen), ==, 50);
	for (i = 0; i < 50; i++)
		g_assert_true (g_hash_table_contains (seen, p[i]));

	/* Release every third; the others are what is left */
	g_hash_table_remove_all (seen);
	for (i = 0; i < 50; i += 3) {
		go_mem_chunk_free (chunk, p[i]);
		p[i] = NULL;
	}
	go_mem_chunk_foreach_leak (chunk, cb_collect_leak, seen);
	g_assert_cmpuint (g_hash_table_size (seen), ==, 50 - 17);
	for (i = 0; i < 50; i++)
		g_assert_true (g_hash_table_contains (seen, p[i]) == (p[i] != NULL));

	/* Releasing the lot gives the all clear again */
	for (i = 0; i < 50; i++)
		if (p[i])
			go_mem_chunk_free (chunk, p[i]);
	g_hash_table_remove_all (seen);
	go_mem_chunk_foreach_leak (chunk, cb_collect_leak, seen);
	g_assert_cmpuint (g_hash_table_size (seen), ==, 0);

	go_mem_chunk_destroy (chunk, FALSE);
	g_hash_table_destroy (seen);
}

static void
cb_free_from_chunk (gpointer data, gpointer user)
{
	go_mem_chunk_free (user, data);
}

static void
test_glib_extras_mem_chunk_leak_callback (void)
{
	/* It is safe to release atoms from the callback */
	GOMemChunk *chunk = go_mem_chunk_new ("cb", 16, 128);
	int i;

	for (i = 0; i < 30; i++)
		go_mem_chunk_alloc (chunk);
	go_mem_chunk_foreach_leak (chunk, cb_free_from_chunk, chunk);
	go_mem_chunk_destroy (chunk, FALSE);	/* no warning: all released */
}

static void
test_glib_extras_mem_chunk_destroy (void)
{
	GOMemChunk *chunk = go_mem_chunk_new ("lossy", 16, 128);
	GType t = go_mem_chunk_get_type ();
	GOMemChunk *c2;

	go_mem_chunk_alloc (chunk);
	go_mem_chunk_alloc (chunk);
	go_mem_chunk_alloc (chunk);

	/* Leaks are reported unless the caller says it expects them */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Leaked 3 nodes from lossy*");
	go_mem_chunk_destroy (chunk, FALSE);
	g_test_assert_expected_messages ();

	chunk = go_mem_chunk_new ("quiet", 16, 128);
	go_mem_chunk_alloc (chunk);
	go_mem_chunk_destroy (chunk, TRUE);

	/* Boxed type: copying is a reference, and the memory goes with the
	   last one */
	g_assert_true (G_TYPE_IS_BOXED (t));
	chunk = go_mem_chunk_new ("boxed", 16, 128);
	c2 = g_boxed_copy (t, chunk);
	g_assert_true (c2 == chunk);
	g_boxed_free (t, c2);
	go_mem_chunk_free (chunk, go_mem_chunk_alloc (chunk));	/* still usable */
	go_mem_chunk_destroy (chunk, FALSE);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_mem_chunk_destroy*");
	go_mem_chunk_destroy (NULL, FALSE);
	g_test_assert_expected_messages ();
}

/* ---- objects and properties ---- */

#define TEST_TYPE_OBJ (test_obj_get_type ())

typedef struct {
	GObject base;
	gboolean flag, ctor_flag;
	char *name;
	GODirection direction;
	int number;
} TestObj;

typedef struct {
	GObjectClass base;
} TestObjClass;

GType test_obj_get_type (void);
G_DEFINE_TYPE (TestObj, test_obj, G_TYPE_OBJECT)

enum { PROP_0, PROP_FLAG, PROP_CTOR_FLAG, PROP_RO_FLAG, PROP_NAME, PROP_DIRECTION, PROP_NUMBER };

static void
test_obj_set_property (GObject *obj, guint id, const GValue *value, GParamSpec *pspec)
{
	TestObj *t = (TestObj *)obj;
	switch (id) {
	case PROP_FLAG: t->flag = g_value_get_boolean (value); break;
	case PROP_CTOR_FLAG: t->ctor_flag = g_value_get_boolean (value); break;
	case PROP_NAME: g_free (t->name); t->name = g_value_dup_string (value); break;
	case PROP_DIRECTION: t->direction = g_value_get_enum (value); break;
	case PROP_NUMBER: t->number = g_value_get_int (value); break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, id, pspec);
	}
}

static void
test_obj_get_property (GObject *obj, guint id, GValue *value, GParamSpec *pspec)
{
	TestObj *t = (TestObj *)obj;
	switch (id) {
	case PROP_FLAG: g_value_set_boolean (value, t->flag); break;
	case PROP_CTOR_FLAG: g_value_set_boolean (value, t->ctor_flag); break;
	case PROP_RO_FLAG: g_value_set_boolean (value, TRUE); break;
	case PROP_NAME: g_value_set_string (value, t->name); break;
	case PROP_DIRECTION: g_value_set_enum (value, t->direction); break;
	case PROP_NUMBER: g_value_set_int (value, t->number); break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, id, pspec);
	}
}

static void
test_obj_finalize (GObject *obj)
{
	g_free (((TestObj *)obj)->name);
	G_OBJECT_CLASS (test_obj_parent_class)->finalize (obj);
}

static void
test_obj_class_init (TestObjClass *klass)
{
	GObjectClass *oc = G_OBJECT_CLASS (klass);
	oc->set_property = test_obj_set_property;
	oc->get_property = test_obj_get_property;
	oc->finalize = test_obj_finalize;
	g_object_class_install_property (oc, PROP_FLAG,
		g_param_spec_boolean ("flag", NULL, NULL, FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (oc, PROP_CTOR_FLAG,
		g_param_spec_boolean ("ctor-flag", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (oc, PROP_RO_FLAG,
		g_param_spec_boolean ("ro-flag", NULL, NULL, TRUE, G_PARAM_READABLE));
	g_object_class_install_property (oc, PROP_NAME,
		g_param_spec_string ("name", NULL, NULL, NULL, G_PARAM_READWRITE));
	g_object_class_install_property (oc, PROP_DIRECTION,
		g_param_spec_enum ("direction", NULL, NULL, GO_TYPE_DIRECTION,
				   GO_DIRECTION_UP, G_PARAM_READWRITE));
	g_object_class_install_property (oc, PROP_NUMBER,
		g_param_spec_int ("number", NULL, NULL, -1000, 1000, 0, G_PARAM_READWRITE));
}

static void
test_obj_init (TestObj *t)
{
	(void)t;
}

static void
test_glib_extras_object_toggle (void)
{
	GObject *obj = g_object_new (TEST_TYPE_OBJ, NULL);
	TestObj *t = (TestObj *)obj;

	g_assert_false (t->flag);
	go_object_toggle (obj, "flag");
	g_assert_true (t->flag);
	go_object_toggle (obj, "flag");
	g_assert_false (t->flag);

	/* Only boolean properties that can be read and written qualify */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*no boolean property*");
	go_object_toggle (obj, "name");
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*no boolean property*");
	go_object_toggle (obj, "ro-flag");
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*no boolean property*");
	go_object_toggle (obj, "ctor-flag");
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*no boolean property*");
	go_object_toggle (obj, "no-such-property");
	g_test_assert_expected_messages ();

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_object_toggle*");
	go_object_toggle (NULL, "flag");
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_object_toggle*");
	go_object_toggle (obj, NULL);
	g_test_assert_expected_messages ();

	g_object_unref (obj);
}

static void
test_glib_extras_set_property_boolean (void)
{
	GObject *obj = g_object_new (TEST_TYPE_OBJ, NULL);
	TestObj *t = (TestObj *)obj;
	GError *err = NULL;
	static const char *const yes[] = { "TRUE", "true", "True", "yes", "YES", "Yes", "1" };
	static const char *const no[] = { "FALSE", "false", "False", "no", "NO", "No", "0" };
	static const char *const bad[] = { "maybe", "2", "", "tru", "yess", "01", " 1" };
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (yes); i++) {
		t->flag = FALSE;
		g_assert_false (go_object_set_property (obj, "flag", "Flag", yes[i], &err, "Bad %s: %s"));
		g_assert_no_error (err);
		g_assert_true (t->flag);
	}
	for (i = 0; i < G_N_ELEMENTS (no); i++) {
		t->flag = TRUE;
		g_assert_false (go_object_set_property (obj, "flag", "Flag", no[i], &err, "Bad %s: %s"));
		g_assert_no_error (err);
		g_assert_false (t->flag);
	}

	/* Anything else is an error, reported using the template, and the
	   property is left alone */
	for (i = 0; i < G_N_ELEMENTS (bad); i++) {
		char *expected = g_strdup_printf ("Bad Flag: %s", bad[i]);
		t->flag = TRUE;
		g_assert_true (go_object_set_property (obj, "flag", "Flag", bad[i], &err, "Bad %s: %s"));
		g_assert_nonnull (err);
		g_assert_true (g_error_matches (err, go_error_invalid (), 0));
		g_assert_cmpstr (err->message, ==, expected);
		g_assert_true (t->flag);
		g_error_free (err);
		err = NULL;
		g_free (expected);
	}

	/* The error return is optional */
	g_assert_true (go_object_set_property (obj, "flag", "Flag", "maybe", NULL, "Bad %s: %s"));
	g_assert_false (go_object_set_property (obj, "flag", "Flag", "no", NULL, "Bad %s: %s"));

	g_object_unref (obj);
}

static void
test_glib_extras_set_property_other (void)
{
	GObject *obj = g_object_new (TEST_TYPE_OBJ, NULL);
	TestObj *t = (TestObj *)obj;
	GError *err = NULL;
	GEnumClass *klass = g_type_class_ref (GO_TYPE_DIRECTION);
	unsigned i;

	/* Strings are taken as they are */
	g_assert_false (go_object_set_property (obj, "name", "Name", "Some Name", &err, "Bad %s: %s"));
	g_assert_no_error (err);
	g_assert_cmpstr (t->name, ==, "Some Name");
	g_assert_false (go_object_set_property (obj, "name", "Name", "", &err, "Bad %s: %s"));
	g_assert_cmpstr (t->name, ==, "");

	/* Enumerations by value name or by nick */
	for (i = 0; i < klass->n_values; i++) {
		const GEnumValue *v = &klass->values[i];
		t->direction = (GODirection)-1;
		g_assert_false (go_object_set_property (obj, "direction", "Direction",
							v->value_name, &err, "Bad %s: %s"));
		g_assert_no_error (err);
		g_assert_cmpint (t->direction, ==, v->value);

		t->direction = (GODirection)-1;
		g_assert_false (go_object_set_property (obj, "direction", "Direction",
							v->value_nick, &err, "Bad %s: %s"));
		g_assert_no_error (err);
		g_assert_cmpint (t->direction, ==, v->value);
	}
	t->direction = GO_DIRECTION_DOWN;
	g_assert_true (go_object_set_property (obj, "direction", "Direction", "sideways", &err, "Bad %s: %s"));
	g_assert_cmpstr (err->message, ==, "Bad Direction: sideways");
	g_assert_cmpint (t->direction, ==, GO_DIRECTION_DOWN);
	g_clear_error (&err);
	g_type_class_unref (klass);

	/* Other kinds of properties are not supported */
	g_assert_true (go_object_set_property (obj, "number", "Number", "5", &err, "Bad %s: %s"));
	g_assert_cmpstr (err->message, ==, "Bad Number: 5");
	g_assert_cmpint (t->number, ==, 0);
	g_clear_error (&err);

	/* Invalid use */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_object_set_property*");
	g_assert_true (go_object_set_property (obj, "no-such-property", "X", "1", &err, "Bad %s: %s"));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_object_set_property*");
	g_assert_true (go_object_set_property (NULL, "flag", "X", "1", &err, "Bad %s: %s"));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_object_set_property*");
	g_assert_true (go_object_set_property (obj, "flag", "X", NULL, &err, "Bad %s: %s"));
	g_test_assert_expected_messages ();
	g_assert_null (err);

	g_object_unref (obj);
}

static void
cb_count_notify (G_GNUC_UNUSED GObject *obj, G_GNUC_UNUSED GParamSpec *pspec, gpointer user)
{
	(*(int *)user)++;
}

static void
test_glib_extras_object_properties (void)
{
	GObject *obj = g_object_new (TEST_TYPE_OBJ, "flag", TRUE, "name", "first",
				     "direction", GO_DIRECTION_LEFT, "number", 42,
				     "ctor-flag", TRUE, NULL);
	TestObj *t = (TestObj *)obj;
	GSList *props, *l;
	int notified = 0;
	int n_props = 0;

	props = go_object_properties_collect (obj);
	/* Alternating spec and value, for the read-write properties only
	   (not the read-only one and not the construct-only one) */
	g_assert_cmpuint (g_slist_length (props) % 2, ==, 0);
	for (l = props; l; l = l->next->next) {
		GParamSpec *pspec = l->data;
		GValue *value = l->next->data;
		g_assert_true (G_IS_PARAM_SPEC (pspec));
		g_assert_true (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec));
		g_assert_cmpstr (pspec->name, !=, "ro-flag");
		g_assert_cmpstr (pspec->name, !=, "ctor-flag");
		n_props++;
	}
	g_assert_cmpint (n_props, ==, 4);

	/* Change everything, then put it all back */
	t->flag = FALSE;
	g_free (t->name);
	t->name = g_strdup ("second");
	t->direction = GO_DIRECTION_RIGHT;
	t->number = -7;
	t->ctor_flag = FALSE;
	g_signal_connect (obj, "notify", G_CALLBACK (cb_count_notify), &notified);

	go_object_properties_apply (obj, props, FALSE);
	g_assert_cmpint (notified, ==, 4);
	g_assert_true (t->flag);
	g_assert_cmpstr (t->name, ==, "first");
	g_assert_cmpint (t->direction, ==, GO_DIRECTION_LEFT);
	g_assert_cmpint (t->number, ==, 42);
	g_assert_false (t->ctor_flag);	/* not collected, so not restored */

	/* Applying with changed_only touches only what is different */
	notified = 0;
	go_object_properties_apply (obj, props, TRUE);
	g_assert_cmpint (notified, ==, 0);

	t->number = 1;
	go_object_properties_apply (obj, props, TRUE);
	g_assert_cmpint (notified, ==, 1);
	g_assert_cmpint (t->number, ==, 42);

	g_free (t->name);
	t->name = g_strdup ("third");
	t->flag = FALSE;
	notified = 0;
	go_object_properties_apply (obj, props, TRUE);
	g_assert_cmpint (notified, ==, 2);
	g_assert_cmpstr (t->name, ==, "first");
	g_assert_true (t->flag);

	/* A snapshot is independent of later changes to the object */
	t->number = 100;
	g_assert_cmpint (t->number, ==, 100);
	go_object_properties_apply (obj, props, FALSE);
	g_assert_cmpint (t->number, ==, 42);

	go_object_properties_free (props);
	go_object_properties_free (NULL);
	g_object_unref (obj);
}

/* ---- key=value parsing ---- */

typedef struct {
	GString *log;
	int fail_at;	/* make the handler fail on the nth call (1-based), or 0 */
	int calls;
} KVState;

static gboolean
kv_handler (const char *name, const char *value, GError **err, gpointer user)
{
	KVState *st = user;
	st->calls++;
	g_string_append_printf (st->log, "[%s|%s]", name, value);
	if (st->fail_at == st->calls) {
		if (err)
			*err = g_error_new (go_error_invalid (), 0, "handler says no");
		return TRUE;
	}
	return FALSE;
}

/* Returns the result of go_parse_key_value; the handler log and the
   error message (or NULL) are stored in *log and *msg. */
static gboolean
kv_parse (const char *options, char **log, char **msg, int fail_at)
{
	KVState st;
	GError *err = NULL;
	gboolean res;

	st.log = g_string_new (NULL);
	st.fail_at = fail_at;
	st.calls = 0;
	res = go_parse_key_value (options, &err, kv_handler, &st);
	*log = g_string_free (st.log, FALSE);
	*msg = err ? g_strdup (err->message) : NULL;
	if (err) {
		g_assert_true (res);
		g_assert_true (g_error_matches (err, go_error_invalid (), 0));
		g_error_free (err);
	} else
		g_assert_false (res);
	return res;
}

static void
assert_kv (const char *options, const char *expected_log)
{
	char *log, *msg;
	g_assert_false (kv_parse (options, &log, &msg, 0));
	g_assert_null (msg);
	g_assert_cmpstr (log, ==, expected_log);
	g_free (log);
}

static void
assert_kv_error (const char *options, const char *expected_log, const char *expected_msg)
{
	char *log, *msg;
	g_assert_true (kv_parse (options, &log, &msg, 0));
	g_assert_cmpstr (msg, ==, expected_msg);
	g_assert_cmpstr (log, ==, expected_log);
	g_free (log);
	g_free (msg);
}

static void
test_glib_extras_parse_key_value (void)
{
	/* Nothing at all */
	assert_kv ("", "");
	assert_kv ("   ", "");
	assert_kv (" \t\n ", "");

	/* Pairs, separated by white space */
	assert_kv ("a=1", "[a|1]");
	assert_kv ("a=1 b=2", "[a|1][b|2]");
	assert_kv ("  a=1\t\tb=2\n c=3  ", "[a|1][b|2][c|3]");

	/* White space around the equal sign */
	assert_kv ("a = 1", "[a|1]");
	assert_kv ("a =1 b= 2", "[a|1][b|2]");
	assert_kv ("a   =   1", "[a|1]");

	/* Empty values */
	assert_kv ("a=", "[a|]");
	assert_kv ("a= b=", "[a|b=]");
	assert_kv ("a=\"\" b=''", "[a|][b|]");

	/* Unquoted values run to the next white space and may contain anything */
	assert_kv ("a=x=y,z:w;v", "[a|x=y,z:w;v]");
	assert_kv ("path=/usr/lib/x", "[path|/usr/lib/x]");

	/* Characters allowed in names */
	assert_kv ("k-1.x_y=v", "[k-1.x_y|v]");
	assert_kv ("a,b:c;d|e/f$g%h#i@j~k!l=1", "[a,b:c;d|e/f$g%h#i@j~k!l|1]");
	assert_kv ("h\xc3\xa9llo=w\xc3\xb6rld", "[h\xc3\xa9llo|w\xc3\xb6rld]");
	assert_kv ("x1=2", "[x1|2]");

	/* Quoted names and values may contain anything; backslash escapes */
	assert_kv ("\"my key\"='some value'", "[my key|some value]");
	assert_kv ("\"a=b\"=\"c d\"", "[a=b|c d]");
	assert_kv ("a=\"x\\\"y\"", "[a|x\"y]");
	assert_kv ("a='it\\'s'", "[a|it's]");
	assert_kv ("a=\"it's\"", "[a|it's]");
	assert_kv ("a=\"back\\\\slash\"", "[a|back\\slash]");
	assert_kv ("a=\"1\" b='2'c=3", "[a|1][b|2][c|3]");

	/* What go_strescape produces can be parsed */
	{
		GString *s = g_string_new ("k=");
		char *log, *msg;
		go_strescape (s, "va\"l\\ue 'x'");
		g_assert_false (kv_parse (s->str, &log, &msg, 0));
		g_assert_cmpstr (log, ==, "[k|va\"l\\ue 'x']");
		g_free (log);
		g_free (msg);
		g_string_free (s, TRUE);
	}
}

static void
test_glib_extras_parse_key_value_errors (void)
{
	/* Empty name, name without value, stray text */
	assert_kv_error ("=1", "", "Syntax error");
	assert_kv_error ("a b", "", "Syntax error");
	assert_kv_error ("a=1 =2", "[a|1]", "Syntax error");
	assert_kv_error ("a=1 (b)=2", "[a|1]", "Syntax error");
	assert_kv_error ("&=1", "", "Syntax error");
	assert_kv_error ("\"a\" b=1", "", "Syntax error");

	/* Unterminated quotes */
	assert_kv_error ("a='unterminated", "", "Quoted string not terminated");
	assert_kv_error ("\"a=1", "", "Quoted string not terminated");
	assert_kv_error ("a=\"x\\", "", "Quoted string not terminated");
	assert_kv_error ("a=1 b=\"x", "[a|1]", "Quoted string not terminated");
}

static void
test_glib_extras_parse_key_value_handler (void)
{
	char *log, *msg;

	/* The handler can stop the parsing, and its error is passed on */
	g_assert_true (kv_parse ("a=1 b=2 c=3", &log, &msg, 2));
	g_assert_cmpstr (log, ==, "[a|1][b|2]");
	g_assert_cmpstr (msg, ==, "handler says no");
	g_free (log);
	g_free (msg);

	g_assert_true (kv_parse ("a=1", &log, &msg, 1));
	g_assert_cmpstr (log, ==, "[a|1]");
	g_assert_cmpstr (msg, ==, "handler says no");
	g_free (log);
	g_free (msg);

	/* The error return is optional */
	{
		KVState st = { g_string_new (NULL), 0, 0 };
		g_assert_false (go_parse_key_value ("a=1 b=2", NULL, kv_handler, &st));
		g_assert_cmpstr (st.log->str, ==, "[a|1][b|2]");
		g_string_truncate (st.log, 0);
		g_assert_true (go_parse_key_value ("a=1 =", NULL, kv_handler, &st));
		g_assert_cmpstr (st.log->str, ==, "[a|1]");
		g_string_free (st.log, TRUE);
	}
}

static void
test_glib_extras_parse_key_value_end (void)
{
	/* A name that runs into the end of the string must stop there, not
	   carry on reading whatever happens to follow the terminator. */
	char buf[] = "abc\0xyz=1";
	char *log, *msg;

	g_assert_true (kv_parse (buf, &log, &msg, 0));
	g_assert_cmpstr (log, ==, "");
	g_assert_cmpstr (msg, ==, "Syntax error");
	g_free (log);
	g_free (msg);

	{
		char buf2[] = "a=1 key\0z=2";
		g_assert_true (kv_parse (buf2, &log, &msg, 0));
		g_assert_cmpstr (log, ==, "[a|1]");
		g_assert_cmpstr (msg, ==, "Syntax error");
		g_free (log);
		g_free (msg);
	}
}

static void
register_glib_extras_tests (void)
{
	g_test_add_func ("/utils/go-glib-extras/slist", test_glib_extras_slist);
	g_test_add_func ("/utils/go-glib-extras/list_macros", test_glib_extras_list_macros);
	g_test_add_func ("/utils/go-glib-extras/deprecated", test_glib_extras_deprecated);
	g_test_add_func ("/utils/go-glib-extras/list_index", test_glib_extras_list_index);
	g_test_add_func ("/utils/go-glib-extras/strsplit", test_glib_extras_strsplit);
	g_test_add_func ("/utils/go-glib-extras/str_compare", test_glib_extras_str_compare);
	g_test_add_func ("/utils/go-glib-extras/ascii_strcase", test_glib_extras_ascii_strcase);
	g_test_add_func ("/utils/go-glib-extras/collate_casefold", test_glib_extras_collate_casefold);
	g_test_add_func ("/utils/go-glib-extras/strcapital", test_glib_extras_strcapital);
	g_test_add_func ("/utils/go-glib-extras/strescape", test_glib_extras_strescape);
	g_test_add_func ("/utils/go-glib-extras/strunescape", test_glib_extras_strunescape);
	g_test_add_func ("/utils/go-glib-extras/gstring_append", test_glib_extras_gstring_append);
	g_test_add_func ("/utils/go-glib-extras/string_replace", test_glib_extras_string_replace);
	g_test_add_func ("/utils/go-glib-extras/string_replace/random", test_glib_extras_string_replace_random);
	g_test_add_func ("/utils/go-glib-extras/issign", test_glib_extras_issign);
	g_test_add_func ("/utils/go-glib-extras/memdup", test_glib_extras_memdup);
	g_test_add_func ("/utils/go-glib-extras/destroy_password", test_glib_extras_destroy_password);
	g_test_add_func ("/utils/go-glib-extras/guess_encoding", test_glib_extras_guess_encoding);
	g_test_add_func ("/utils/go-glib-extras/real_name", test_glib_extras_real_name);
	g_test_add_func ("/utils/go-glib-extras/debug_flag", test_glib_extras_debug_flag);
	g_test_add_func ("/utils/go-glib-extras/check_finalized/leak", test_glib_extras_check_finalized_leak);
	g_test_add_func ("/utils/go-glib-extras/check_finalized/ok", test_glib_extras_check_finalized_ok);
	g_test_add_func ("/utils/go-glib-extras/mem_chunk/basic", test_glib_extras_mem_chunk_basic);
	g_test_add_func ("/utils/go-glib-extras/mem_chunk/alignment", test_glib_extras_mem_chunk_alignment);
	g_test_add_func ("/utils/go-glib-extras/mem_chunk/alloc0", test_glib_extras_mem_chunk_alloc0);
	g_test_add_func ("/utils/go-glib-extras/mem_chunk/reuse", test_glib_extras_mem_chunk_reuse);
	g_test_add_func ("/utils/go-glib-extras/mem_chunk/leaks", test_glib_extras_mem_chunk_leaks);
	g_test_add_func ("/utils/go-glib-extras/mem_chunk/leak_callback", test_glib_extras_mem_chunk_leak_callback);
	g_test_add_func ("/utils/go-glib-extras/mem_chunk/destroy", test_glib_extras_mem_chunk_destroy);
	g_test_add_func ("/utils/go-glib-extras/object_toggle", test_glib_extras_object_toggle);
	g_test_add_func ("/utils/go-glib-extras/set_property/boolean", test_glib_extras_set_property_boolean);
	g_test_add_func ("/utils/go-glib-extras/set_property/other", test_glib_extras_set_property_other);
	g_test_add_func ("/utils/go-glib-extras/object_properties", test_glib_extras_object_properties);
	g_test_add_func ("/utils/go-glib-extras/parse_key_value", test_glib_extras_parse_key_value);
	g_test_add_func ("/utils/go-glib-extras/parse_key_value/errors", test_glib_extras_parse_key_value_errors);
	g_test_add_func ("/utils/go-glib-extras/parse_key_value/handler", test_glib_extras_parse_key_value_handler);
	g_test_add_func ("/utils/go-glib-extras/parse_key_value/end", test_glib_extras_parse_key_value_end);
}

/* ------------------------------------------------------------------ */
/* go-cairo.c                                                          */
/* ------------------------------------------------------------------ */

static void
test_cairo_clamp (void)
{
	g_assert_cmpfloat (GO_CAIRO_CLAMP (0.0), ==, 0.0);
	g_assert_cmpfloat (GO_CAIRO_CLAMP (123.5), ==, 123.5);
	g_assert_cmpfloat (GO_CAIRO_CLAMP (-7999999.5), ==, -7999999.5);
	g_assert_cmpfloat (GO_CAIRO_CLAMP (1e30), ==, 8000000);
	g_assert_cmpfloat (GO_CAIRO_CLAMP (-1e30), ==, -8000000);
	g_assert_cmpfloat (GO_CAIRO_CLAMP (go_pinf), ==, 8000000);
	g_assert_cmpfloat (GO_CAIRO_CLAMP (go_ninf), ==, -8000000);

	/* Snapping: to integers for even line widths, to half-integers for odd */
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (2.3, TRUE), ==, 2.0);
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (2.5, TRUE), ==, 3.0);
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (-2.3, TRUE), ==, -2.0);
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (2.3, FALSE), ==, 2.5);
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (2.9, FALSE), ==, 2.5);
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (-2.3, FALSE), ==, -2.5);
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (1e30, TRUE), ==, 8000000);
	g_assert_cmpfloat (GO_CAIRO_CLAMP_SNAP (-1e30, FALSE), ==, -8000000);
}

static void
test_cairo_surface_is_vector (void)
{
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);

	g_assert_false (go_cairo_surface_is_vector (surf));
	cairo_surface_destroy (surf);

#ifdef CAIRO_HAS_PDF_SURFACE
	surf = cairo_pdf_surface_create (NULL, 10, 10);
	g_assert_true (go_cairo_surface_is_vector (surf));
	cairo_surface_destroy (surf);
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
	surf = cairo_svg_surface_create (NULL, 10, 10);
	g_assert_true (go_cairo_surface_is_vector (surf));
	cairo_surface_destroy (surf);
#endif
#ifdef CAIRO_HAS_PS_SURFACE
	surf = cairo_ps_surface_create (NULL, 10, 10);
	g_assert_true (go_cairo_surface_is_vector (surf));
	cairo_surface_destroy (surf);
#endif
#ifdef CAIRO_HAS_RECORDING_SURFACE
	{
		/* Other kinds of surface aren't vector surfaces for this purpose */
		cairo_rectangle_t extents = { 0, 0, 10, 10 };
		surf = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, &extents);
		g_assert_false (go_cairo_surface_is_vector (surf));
		cairo_surface_destroy (surf);
	}
#endif
}

static void
test_cairo_rotate_deg (void)
{
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cairo_t *cr = cairo_create (surf);
	cairo_matrix_t m, expected;
	double x, y;
	int k;
	unsigned u;

	/* Multiples of 90 degrees are exact */
	static const struct { double deg, xx, yx; } exact[] = {
		{ 0, 1, 0 }, { 90, 0, 1 }, { 180, -1, 0 }, { 270, 0, -1 },
		{ 360, 1, 0 }, { -90, 0, -1 }, { -180, -1, 0 }, { 450, 0, 1 },
		{ 720, 1, 0 }, { -270, 0, 1 }
	};
	for (u = 0; u < G_N_ELEMENTS (exact); u++) {
		cairo_identity_matrix (cr);
		go_cairo_rotate_deg (cr, exact[u].deg);
		cairo_get_matrix (cr, &m);
		g_assert_cmpfloat (m.xx, ==, exact[u].xx);
		g_assert_cmpfloat (m.yx, ==, exact[u].yx);
		g_assert_cmpfloat (m.xy, ==, -exact[u].yx);
		g_assert_cmpfloat (m.yy, ==, exact[u].xx);
		g_assert_cmpfloat (m.x0, ==, 0);
		g_assert_cmpfloat (m.y0, ==, 0);
	}

	/* Same as cairo_rotate for other angles */
	for (k = -720; k <= 720; k += 37) {
		cairo_identity_matrix (cr);
		go_cairo_rotate_deg (cr, k);
		cairo_get_matrix (cr, &m);
		cairo_matrix_init_rotate (&expected, k * M_PI / 180);
		assert_close (m.xx, expected.xx);
		assert_close (m.yx, expected.yx);
		assert_close (m.xy, expected.xy);
		assert_close (m.yy, expected.yy);
	}

	/* Positive angles turn the x axis towards the y axis */
	cairo_identity_matrix (cr);
	go_cairo_rotate_deg (cr, 90);
	x = 1; y = 0;
	cairo_user_to_device (cr, &x, &y);
	assert_near (x, 0);
	assert_close (y, 1);

	/* It composes with what is already there: translate, then rotate
	   means the rotation is about the translated origin */
	cairo_identity_matrix (cr);
	cairo_translate (cr, 5, 3);
	go_cairo_rotate_deg (cr, 90);
	x = 2; y = 0;
	cairo_user_to_device (cr, &x, &y);
	assert_close (x, 5);
	assert_close (y, 5);

	/* Scale, then rotate */
	cairo_identity_matrix (cr);
	cairo_scale (cr, 2, 3);
	go_cairo_rotate_deg (cr, 90);
	x = 1; y = 0;
	cairo_user_to_device (cr, &x, &y);
	assert_near (x, 0);
	assert_close (y, 3);

	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_cairo_rotate_deg*");
	go_cairo_rotate_deg (NULL, 90);
	g_test_assert_expected_messages ();
}

static void
test_cairo_emit_svg_path (void)
{
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cairo_t *cr = cairo_create (surf);
	cairo_path_t *cp;

	/* A closed triangle */
	go_cairo_emit_svg_path (cr, "M 1 2 L 5 2 L 3 6 z");
	cp = cairo_copy_path (cr);
	g_assert_cmpint (cp->status, ==, CAIRO_STATUS_SUCCESS);
	g_assert_cmpint (cp->data[0].header.type, ==, CAIRO_PATH_MOVE_TO);
	assert_close (cp->data[1].point.x, 1);
	assert_close (cp->data[1].point.y, 2);
	g_assert_cmpint (cp->data[2].header.type, ==, CAIRO_PATH_LINE_TO);
	assert_close (cp->data[3].point.x, 5);
	assert_close (cp->data[3].point.y, 2);
	g_assert_cmpint (cp->data[4].header.type, ==, CAIRO_PATH_LINE_TO);
	assert_close (cp->data[5].point.x, 3);
	assert_close (cp->data[5].point.y, 6);
	g_assert_cmpint (cp->data[6].header.type, ==, CAIRO_PATH_CLOSE_PATH);
	cairo_path_destroy (cp);

	/* The path is added to whatever is in the context: here, a point
	   inside the triangle is filled */
	cairo_new_path (cr);
	cairo_set_source_rgb (cr, 1, 1, 1);
	go_cairo_emit_svg_path (cr, "M 0 0 L 10 0 L 10 10 L 0 10 z");
	cairo_fill (cr);
	g_assert_cmphex (pixel_at (surf, 5, 5), ==, 0xffffffffu);

	/* Curves */
	cairo_new_path (cr);
	go_cairo_emit_svg_path (cr, "M 0 0 C 1 2 3 4 5 6");
	cp = cairo_copy_path (cr);
	g_assert_cmpint (cp->data[2].header.type, ==, CAIRO_PATH_CURVE_TO);
	assert_close (cp->data[5].point.x, 5);
	assert_close (cp->data[5].point.y, 6);
	cairo_path_destroy (cp);

	/* No path, nothing happens */
	cairo_new_path (cr);
	go_cairo_emit_svg_path (cr, NULL);
	cp = cairo_copy_path (cr);
	g_assert_cmpint (cp->num_data, ==, 0);
	cairo_path_destroy (cp);

	/* An empty path adds nothing either */
	go_cairo_emit_svg_path (cr, "");
	cp = cairo_copy_path (cr);
	g_assert_cmpint (cp->num_data, ==, 0);
	cairo_path_destroy (cp);

	cairo_destroy (cr);
	cairo_surface_destroy (surf);
}

/* A w x h pixel image in the given layout, with each row padded by
   `pad' bytes that are set to a marker value */
typedef struct {
	int w, h, stride;
	guint8 *data;
} Pixels;

static Pixels *
pixels_new (int w, int h, int pad)
{
	Pixels *p = g_new (Pixels, 1);
	p->w = w;
	p->h = h;
	p->stride = 4 * w + pad;
	p->data = g_malloc (p->stride * h);
	memset (p->data, 0xaa, p->stride * h);
	return p;
}

static void
pixels_free (Pixels *p)
{
	g_free (p->data);
	g_free (p);
}

static guint8 *
pixels_at (Pixels *p, int x, int y)
{
	return p->data + y * p->stride + 4 * x;
}

static void
pixels_check_padding (Pixels *p)
{
	int y, k;
	for (y = 0; y < p->h; y++)
		for (k = 4 * p->w; k < p->stride; k++)
			g_assert_cmphex (p->data[y * p->stride + k], ==, 0xaa);
}

/* A pixbuf-format (R,G,B,A bytes) test image: a gradient in each
   channel over all combinations of value and alpha */
static Pixels *
pixbuf_pixels_256 (int pad)
{
	Pixels *p = pixels_new (256, 256, pad);
	int x, y;

	for (y = 0; y < 256; y++)
		for (x = 0; x < 256; x++) {
			guint8 *px = pixels_at (p, x, y);
			px[0] = x;		/* R */
			px[1] = 255 - x;	/* G */
			px[2] = (x * 7) & 0xff;	/* B */
			px[3] = y;		/* A */
		}
	return p;
}

static guint32
cairo_pixel (const Pixels *p, int x, int y)
{
	guint32 v;
	memcpy (&v, p->data + y * p->stride + 4 * x, 4);
	return v;
}

static void
test_cairo_pixbuf_to_cairo_opaque (void)
{
	Pixels *src = pixels_new (3, 2, 0), *dst = pixels_new (3, 2, 0);
	int x, y;

	for (y = 0; y < 2; y++)
		for (x = 0; x < 3; x++) {
			guint8 *px = pixels_at (src, x, y);
			px[0] = 10 + x; px[1] = 20 + y; px[2] = 30 + x + y; px[3] = 255;
		}
	go_cairo_convert_data_from_pixbuf (dst->data, src->data, 3, 2, src->stride);

	/* Opaque pixels are just rearranged into cairo's native ARGB32 */
	for (y = 0; y < 2; y++)
		for (x = 0; x < 3; x++)
			g_assert_cmphex (cairo_pixel (dst, x, y), ==,
					 0xff000000u | ((10 + x) << 16) | ((20 + y) << 8) | (30 + x + y));

	pixels_free (src);
	pixels_free (dst);
}

static void
test_cairo_pixbuf_to_cairo_alpha (void)
{
	Pixels *src = pixbuf_pixels_256 (0), *dst = pixels_new (256, 256, 0);
	int x, y;

	go_cairo_convert_data_from_pixbuf (dst->data, src->data, 256, 256, src->stride);

	for (y = 0; y < 256; y++) {
		for (x = 0; x < 256; x++) {
			const guint8 *s = pixels_at (src, x, y);
			guint32 v = cairo_pixel (dst, x, y);
			int a = v >> 24, r = (v >> 16) & 0xff, g = (v >> 8) & 0xff, b = v & 0xff;

			g_assert_cmpint (a, ==, y);
			/* Cairo's colors are premultiplied by alpha: the nearest
			   integer to value * alpha / 255, give or take rounding */
			g_assert_cmpint (abs (r - (int)rint (s[0] * y / 255.0)), <=, 1);
			g_assert_cmpint (abs (g - (int)rint (s[1] * y / 255.0)), <=, 1);
			g_assert_cmpint (abs (b - (int)rint (s[2] * y / 255.0)), <=, 1);
			/* No channel is ever brighter than alpha */
			g_assert_cmpint (r, <=, a);
			g_assert_cmpint (g, <=, a);
			g_assert_cmpint (b, <=, a);
		}
	}
	/* Fully transparent pixels have no color at all */
	for (x = 0; x < 256; x++)
		g_assert_cmphex (cairo_pixel (dst, x, 0), ==, 0);

	pixels_free (src);
	pixels_free (dst);
}

static void
test_cairo_pixbuf_to_cairo_inplace (void)
{
	Pixels *src = pixbuf_pixels_256 (0), *dst = pixels_new (256, 256, 0);
	Pixels *copy = pixbuf_pixels_256 (0);

	go_cairo_convert_data_from_pixbuf (dst->data, src->data, 256, 256, src->stride);

	/* In place, with the same pointer or with NULL for the source */
	go_cairo_convert_data_from_pixbuf (src->data, src->data, 256, 256, src->stride);
	g_assert_cmpint (memcmp (src->data, dst->data, 256 * 256 * 4), ==, 0);

	go_cairo_convert_data_from_pixbuf (copy->data, NULL, 256, 256, copy->stride);
	g_assert_cmpint (memcmp (copy->data, dst->data, 256 * 256 * 4), ==, 0);

	pixels_free (src);
	pixels_free (dst);
	pixels_free (copy);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_cairo_convert_data_from_pixbuf*");
	go_cairo_convert_data_from_pixbuf (NULL, (unsigned char *)"abcd", 1, 1, 4);
	g_test_assert_expected_messages ();
}

static void
test_cairo_pixbuf_rowstride (void)
{
	/* Rows may be padded; the padding is left alone, in both directions
	   and whether or not conversion is in place */
	int pad;

	for (pad = 0; pad <= 5; pad += 1) {
		Pixels *src = pixbuf_pixels_256 (pad), *dst = pixels_new (256, 256, pad);
		Pixels *plain = pixbuf_pixels_256 (0), *plain_dst = pixels_new (256, 256, 0);
		int x, y;

		go_cairo_convert_data_from_pixbuf (dst->data, src->data, 256, 256, src->stride);
		go_cairo_convert_data_from_pixbuf (plain_dst->data, plain->data, 256, 256, plain->stride);
		pixels_check_padding (dst);
		for (y = 0; y < 256; y++)
			for (x = 0; x < 256; x++)
				g_assert_cmphex (cairo_pixel (dst, x, y), ==, cairo_pixel (plain_dst, x, y));

		go_cairo_convert_data_from_pixbuf (src->data, src->data, 256, 256, src->stride);
		pixels_check_padding (src);
		for (y = 0; y < 256; y++)
			for (x = 0; x < 256; x++)
				g_assert_cmphex (cairo_pixel (src, x, y), ==, cairo_pixel (plain_dst, x, y));

		/* And back */
		{
			Pixels *back = pixels_new (256, 256, pad);
			go_cairo_convert_data_to_pixbuf (back->data, dst->data, 256, 256, dst->stride);
			pixels_check_padding (back);
			go_cairo_convert_data_to_pixbuf (dst->data, dst->data, 256, 256, dst->stride);
			pixels_check_padding (dst);
			for (y = 0; y < 256; y++)
				g_assert_cmpint (memcmp (pixels_at (back, 0, y), pixels_at (dst, 0, y), 4 * 256), ==, 0);
			pixels_free (back);
		}

		pixels_free (src);
		pixels_free (dst);
		pixels_free (plain);
		pixels_free (plain_dst);
	}
}

static void
test_cairo_cairo_to_pixbuf (void)
{
	Pixels *src = pixels_new (4, 1, 0), *dst = pixels_new (4, 1, 0);
	guint32 v[4];
	guint8 *px;

	/* Opaque: just rearranged */
	v[0] = 0xff0a141eu;	/* A R G B */
	/* Transparent: all zero in the result */
	v[1] = 0;
	/* A pixel which isn't premultiplied-valid must still not crash */
	v[2] = 0x00ffffffu;
	/* Half transparent white: premultiplied components are 128 */
	v[3] = 0x80808080u;
	memcpy (src->data, v, sizeof (v));

	go_cairo_convert_data_to_pixbuf (dst->data, src->data, 4, 1, src->stride);

	px = pixels_at (dst, 0, 0);
	g_assert_cmpint (px[0], ==, 0x0a);
	g_assert_cmpint (px[1], ==, 0x14);
	g_assert_cmpint (px[2], ==, 0x1e);
	g_assert_cmpint (px[3], ==, 0xff);

	px = pixels_at (dst, 1, 0);
	g_assert_cmpint (px[0], ==, 0);
	g_assert_cmpint (px[1], ==, 0);
	g_assert_cmpint (px[2], ==, 0);
	g_assert_cmpint (px[3], ==, 0);

	px = pixels_at (dst, 2, 0);
	g_assert_cmpint (px[0], ==, 0);
	g_assert_cmpint (px[1], ==, 0);
	g_assert_cmpint (px[2], ==, 0);
	g_assert_cmpint (px[3], ==, 0);

	px = pixels_at (dst, 3, 0);
	g_assert_cmpint (px[0], >=, 254);
	g_assert_cmpint (px[1], >=, 254);
	g_assert_cmpint (px[2], >=, 254);
	g_assert_cmpint (px[3], ==, 0x80);

	pixels_free (src);
	pixels_free (dst);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_cairo_convert_data_to_pixbuf*");
	go_cairo_convert_data_to_pixbuf (NULL, (unsigned char *)"abcd", 1, 1, 4);
	g_test_assert_expected_messages ();
}

static void
test_cairo_pixbuf_roundtrip (void)
{
	/* pixbuf -> cairo -> pixbuf loses precision for translucent pixels,
	   but never by more than one step of premultiplied color, which is
	   255/alpha in the original scale. */
	Pixels *orig = pixbuf_pixels_256 (0), *cairo = pixels_new (256, 256, 0);
	Pixels *back = pixels_new (256, 256, 0);
	int x, y, c;

	go_cairo_convert_data_from_pixbuf (cairo->data, orig->data, 256, 256, orig->stride);
	go_cairo_convert_data_to_pixbuf (back->data, cairo->data, 256, 256, cairo->stride);

	for (y = 1; y < 256; y++) {
		int tolerance = 255 / y + 1;
		for (x = 0; x < 256; x++) {
			const guint8 *o = pixels_at (orig, x, y), *b = pixels_at (back, x, y);
			g_assert_cmpint (b[3], ==, o[3]);
			for (c = 0; c < 3; c++)
				g_assert_cmpint (abs (b[c] - o[c]), <=, tolerance);
			/* Opaque pixels survive exactly */
			if (y == 255)
				for (c = 0; c < 4; c++)
					g_assert_cmpint (b[c], ==, o[c]);
		}
	}

	pixels_free (orig);
	pixels_free (cairo);
	pixels_free (back);
}

static void
test_cairo_pixbuf_vs_cairo_surface (void)
{
	/* The converted data is what cairo itself would have for the same
	   (non-premultiplied) color painted onto a transparent surface */
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 16, 1);
	cairo_t *cr = cairo_create (surf);
	Pixels *src = pixels_new (16, 1, 0), *dst = pixels_new (16, 1, 0);
	int x;

	for (x = 0; x < 16; x++) {
		guint8 *px = pixels_at (src, x, 0);
		px[0] = 255 - 16 * x;
		px[1] = 16 * x;
		px[2] = 100;
		px[3] = 16 * x + 15;

		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba (cr, px[0] / 255.0, px[1] / 255.0, px[2] / 255.0, px[3] / 255.0);
		cairo_rectangle (cr, x, 0, 1, 1);
		cairo_fill (cr);
	}
	go_cairo_convert_data_from_pixbuf (dst->data, src->data, 16, 1, src->stride);
	cairo_surface_flush (surf);

	for (x = 0; x < 16; x++) {
		guint32 a = pixel_at (surf, x, 0), b = cairo_pixel (dst, x, 0);
		int k;
		for (k = 0; k < 32; k += 8)
			g_assert_cmpint (abs ((int)((a >> k) & 0xff) - (int)((b >> k) & 0xff)), <=, 1);
	}

	pixels_free (src);
	pixels_free (dst);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);
}

static void
register_cairo_tests (void)
{
	g_test_add_func ("/utils/go-cairo/clamp", test_cairo_clamp);
	g_test_add_func ("/utils/go-cairo/surface_is_vector", test_cairo_surface_is_vector);
	g_test_add_func ("/utils/go-cairo/rotate_deg", test_cairo_rotate_deg);
	g_test_add_func ("/utils/go-cairo/emit_svg_path", test_cairo_emit_svg_path);
	g_test_add_func ("/utils/go-cairo/pixbuf_to_cairo/opaque", test_cairo_pixbuf_to_cairo_opaque);
	g_test_add_func ("/utils/go-cairo/pixbuf_to_cairo/alpha", test_cairo_pixbuf_to_cairo_alpha);
	g_test_add_func ("/utils/go-cairo/pixbuf_to_cairo/inplace", test_cairo_pixbuf_to_cairo_inplace);
	g_test_add_func ("/utils/go-cairo/pixbuf_rowstride", test_cairo_pixbuf_rowstride);
	g_test_add_func ("/utils/go-cairo/cairo_to_pixbuf", test_cairo_cairo_to_pixbuf);
	g_test_add_func ("/utils/go-cairo/pixbuf_roundtrip", test_cairo_pixbuf_roundtrip);
	g_test_add_func ("/utils/go-cairo/pixbuf_vs_cairo_surface", test_cairo_pixbuf_vs_cairo_surface);
}

/* ------------------------------------------------------------------ */
/* regutf8.c                                                           */
/* ------------------------------------------------------------------ */

static void
test_regex_compile_errors (void)
{
	static const struct {
		const char *pattern;
		int code;
	} cases[] = {
		{ "(", GO_REG_EPAREN },
		{ "a)", GO_REG_EPAREN },
		{ "[a", GO_REG_EBRACK },
		{ "*a", GO_REG_BADRPT },
		{ "a{2,1}", GO_REG_EBRACE },
		{ "[z-a]", GO_REG_ERANGE },
		{ "[[:foo:]]", GO_REG_ECTYPE },
		{ "a\\", GO_REG_EESCAPE },
		{ "(a)\\2", GO_REG_ESUBREG }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		GORegexp re;
		g_assert_cmpint (go_regcomp (&re, cases[i].pattern, 0), ==, cases[i].code);
	}

	/* Valid patterns of all sorts compile */
	{
		static const char *const good[] = {
			"", "a", "a*", "(a|b)+", "[a-z]{2,3}", "^$", "\\d+", "\\(", "[[:alpha:]]", "a.b",
			"h\xc3\xa9llo", "(?:x)", "a{2}"
		};
		for (i = 0; i < G_N_ELEMENTS (good); i++) {
			GORegexp re;
			g_assert_cmpint (go_regcomp (&re, good[i], 0), ==, GO_REG_NOERROR);
			go_regfree (&re);
		}
	}
}

static void
test_regex_exec (void)
{
	GORegexp re;
	GORegmatch m[4];
	int i;

	g_assert_cmpint (go_regcomp (&re, "b+", 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (re.re_nsub, ==, 0);
	g_assert_cmpint (go_regexec (&re, "aabbbcc", 1, m, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (m[0].rm_so, ==, 2);
	g_assert_cmpint (m[0].rm_eo, ==, 5);
	g_assert_cmpint (go_regexec (&re, "aaccc", 1, m, 0), ==, GO_REG_NOMATCH);
	g_assert_cmpint (go_regexec (&re, "", 1, m, 0), ==, GO_REG_NOMATCH);
	/* Nothing is wanted back */
	g_assert_cmpint (go_regexec (&re, "abc", 0, NULL, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "ac", 0, NULL, 0), ==, GO_REG_NOMATCH);
	go_regfree (&re);
	/* Freeing twice is fine */
	go_regfree (&re);

	/* Groups; ones that didn't take part, and ones that don't exist,
	   are reported as -1 */
	g_assert_cmpint (go_regcomp (&re, "a(x)?(b)", 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (re.re_nsub, ==, 2);
	g_assert_cmpint (go_regexec (&re, "zab", 4, m, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (m[0].rm_so, ==, 1);
	g_assert_cmpint (m[0].rm_eo, ==, 3);
	g_assert_cmpint (m[1].rm_so, ==, -1);
	g_assert_cmpint (m[1].rm_eo, ==, -1);
	g_assert_cmpint (m[2].rm_so, ==, 2);
	g_assert_cmpint (m[2].rm_eo, ==, 3);
	g_assert_cmpint (m[3].rm_so, ==, -1);
	g_assert_cmpint (m[3].rm_eo, ==, -1);
	g_assert_cmpint (go_regexec (&re, "zaxb", 3, m, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (m[1].rm_so, ==, 2);
	g_assert_cmpint (m[1].rm_eo, ==, 3);
	/* Fewer slots than groups is fine */
	g_assert_cmpint (go_regexec (&re, "zaxb", 2, m, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (m[1].rm_so, ==, 2);
	go_regfree (&re);

	/* Offsets are in bytes, and a dot matches a whole character */
	g_assert_cmpint (go_regcomp (&re, "a(.)b", 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "\xc3\xa9" "a\xc3\xb6" "b", 2, m, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (m[0].rm_so, ==, 2);
	g_assert_cmpint (m[0].rm_eo, ==, 6);
	g_assert_cmpint (m[1].rm_so, ==, 3);
	g_assert_cmpint (m[1].rm_eo, ==, 5);
	go_regfree (&re);

	/* NOSUB: no matches are reported, even if some are asked for */
	g_assert_cmpint (go_regcomp (&re, "(b)", GO_REG_NOSUB), ==, GO_REG_NOERROR);
	for (i = 0; i < 4; i++)
		m[i].rm_so = m[i].rm_eo = 77;
	g_assert_cmpint (go_regexec (&re, "abc", 2, m, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (m[0].rm_so, ==, 77);
	g_assert_cmpint (m[1].rm_eo, ==, 77);
	g_assert_cmpint (go_regexec (&re, "xyz", 2, m, 0), ==, GO_REG_NOMATCH);
	go_regfree (&re);
}

static void
test_regex_flags (void)
{
	GORegexp re;

	/* Case */
	g_assert_cmpint (go_regcomp (&re, "hello", 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "HELLO", 0, NULL, 0), ==, GO_REG_NOMATCH);
	go_regfree (&re);
	g_assert_cmpint (go_regcomp (&re, "hello", GO_REG_ICASE), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "HeLLo", 0, NULL, 0), ==, GO_REG_NOERROR);
	go_regfree (&re);
	g_assert_cmpint (go_regcomp (&re, "\xc3\xa9", GO_REG_ICASE), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "\xc3\x89", 0, NULL, 0), ==, GO_REG_NOERROR);
	go_regfree (&re);

	/* NEWLINE makes ^ and $ work at line boundaries */
	g_assert_cmpint (go_regcomp (&re, "^b$", 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "a\nb\nc", 0, NULL, 0), ==, GO_REG_NOMATCH);
	g_assert_cmpint (go_regexec (&re, "b", 0, NULL, 0), ==, GO_REG_NOERROR);
	go_regfree (&re);
	g_assert_cmpint (go_regcomp (&re, "^b$", GO_REG_NEWLINE), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "a\nb\nc", 0, NULL, 0), ==, GO_REG_NOERROR);
	go_regfree (&re);

	/* NOTBOL and NOTEOL: the ends of the string aren't the ends of the line */
	g_assert_cmpint (go_regcomp (&re, "^a", 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "abc", 0, NULL, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "abc", 0, NULL, GO_REG_NOTBOL), ==, GO_REG_NOMATCH);
	go_regfree (&re);
	g_assert_cmpint (go_regcomp (&re, "c$", 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "abc", 0, NULL, 0), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "abc", 0, NULL, GO_REG_NOTEOL), ==, GO_REG_NOMATCH);
	go_regfree (&re);

	/* Flags are independent: all of them at once */
	g_assert_cmpint (go_regcomp (&re, "^(x)$", GO_REG_ICASE | GO_REG_NEWLINE | GO_REG_NOSUB | GO_REG_EXTENDED), ==, GO_REG_NOERROR);
	g_assert_cmpint (go_regexec (&re, "a\nX\nb", 0, NULL, 0), ==, GO_REG_NOERROR);
	go_regfree (&re);
}

static void
test_regex_error_messages (void)
{
	char buf[200], small[8];
	int code, other;

	/* Every error has a message of its own */
	for (code = GO_REG_NOMATCH; code <= GO_REG_ERPAREN; code++) {
		size_t len = go_regerror (code, NULL, buf, sizeof (buf));
		g_assert_cmpuint (len, >, 0);
		g_assert_cmpuint (strlen (buf), ==, len);
		for (other = GO_REG_NOMATCH; other < code; other++) {
			char buf2[200];
			go_regerror (other, NULL, buf2, sizeof (buf2));
			g_assert_cmpstr (buf, !=, buf2);
		}
	}

	/* Success has a message too */
	g_assert_cmpuint (go_regerror (GO_REG_NOERROR, NULL, buf, sizeof (buf)), >, 0);

	/* Unknown codes get the message for a bad pattern */
	{
		char buf2[200];
		go_regerror (GO_REG_BADPAT, NULL, buf2, sizeof (buf2));
		g_assert_cmpuint (go_regerror (4711, NULL, buf, sizeof (buf)), ==, strlen (buf2));
		g_assert_cmpstr (buf, ==, buf2);
		go_regerror (-1, NULL, buf, sizeof (buf));
		g_assert_cmpstr (buf, ==, buf2);
	}

	/* A message that doesn't fit is cut short, but the full length is
	   what is returned */
	{
		size_t full = go_regerror (GO_REG_EPAREN, NULL, buf, sizeof (buf));
		size_t len;
		g_assert_cmpuint (full, >, sizeof (small));
		len = go_regerror (GO_REG_EPAREN, NULL, small, sizeof (small));
		g_assert_cmpuint (len, ==, full);
		g_assert_cmpuint (strlen (small), ==, sizeof (small) - 1);
		g_assert_cmpint (strncmp (small, buf, sizeof (small) - 1), ==, 0);

		/* A buffer of size 1 can only hold the terminator */
		small[0] = 'x';
		g_assert_cmpuint (go_regerror (GO_REG_EPAREN, NULL, small, 1), ==, full);
		g_assert_cmpint (small[0], ==, 0);

		/* No buffer at all */
		small[0] = 'x';
		g_assert_cmpuint (go_regerror (GO_REG_EPAREN, NULL, small, 0), ==, full);
		g_assert_cmpint (small[0], ==, 'x');
	}
}

static void
test_regexp_quote (void)
{
	GString *s = g_string_new (NULL);
	const char *rest;
	const char *specials = ".[\\*+{?^$(|)";
	const char *p;

	/* Characters that mean something to the regular expression engine
	   get a backslash in front of them */
	for (p = specials; *p; p++) {
		char one[2] = { *p, 0 };
		g_string_truncate (s, 0);
		rest = go_regexp_quote1 (s, one);
		g_assert_true (rest == one + 1);
		g_assert_cmpuint (s->len, ==, 2);
		g_assert_cmpint (s->str[0], ==, '\\');
		g_assert_cmpint (s->str[1], ==, *p);
	}

	/* Others don't */
	for (p = "abXY019 ,;:-_#%&@~!\"'<>=/]}"; *p; p++) {
		char one[2] = { *p, 0 };
		g_string_truncate (s, 0);
		rest = go_regexp_quote1 (s, one);
		g_assert_true (rest == one + 1);
		g_assert_cmpstr (s->str, ==, one);
	}

	/* Multibyte characters are taken whole */
	g_string_truncate (s, 0);
	rest = go_regexp_quote1 (s, "\xc3\xa9xyz");
	g_assert_cmpstr (s->str, ==, "\xc3\xa9");
	g_assert_cmpstr (rest, ==, "xyz");
	g_string_truncate (s, 0);
	rest = go_regexp_quote1 (s, "\xe2\x82\xac" "5");
	g_assert_cmpstr (s->str, ==, "\xe2\x82\xac");
	g_assert_cmpstr (rest, ==, "5");

	/* Nothing to do at the end of the string, and it appends */
	g_string_assign (s, "pre");
	rest = go_regexp_quote1 (s, "");
	g_assert_cmpstr (s->str, ==, "pre");
	g_assert_cmpstr (rest, ==, "");

	g_string_truncate (s, 0);
	go_regexp_quote (s, "a.b*c");
	g_assert_cmpstr (s->str, ==, "a\\.b\\*c");
	go_regexp_quote (s, "(x)");
	g_assert_cmpstr (s->str, ==, "a\\.b\\*c\\(x\\)");
	go_regexp_quote (s, "");
	g_assert_cmpstr (s->str, ==, "a\\.b\\*c\\(x\\)");

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_regexp_quote1*");
	g_assert_null (go_regexp_quote1 (NULL, "a"));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_regexp_quote1*");
	g_assert_null (go_regexp_quote1 (s, NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_regexp_quote*");
	go_regexp_quote (NULL, "a");
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_regexp_quote*");
	go_regexp_quote (s, NULL);
	g_test_assert_expected_messages ();

	g_string_free (s, TRUE);
}

static void
test_regexp_quote_matches_itself (void)
{
	/* Whatever the text, its quoted version as a regular expression
	   matches exactly that text */
	static const char *const units[] = {
		"a", "b", ".", "[", "]", "\\", "*", "+", "{", "}", "?", "^", "$",
		"(", ")", "|", "-", ",", " ", "\xc3\xa9", "\xe2\x82\xac"
	};
	GRand *rand = g_rand_new_with_seed (31337);
	int i;

	for (i = 0; i < 2000; i++) {
		GString *text = g_string_new (NULL), *quoted = g_string_new (NULL);
		GORegexp re;
		GORegmatch m;
		int n = g_rand_int_range (rand, 1, 8), k;
		char *haystack;

		for (k = 0; k < n; k++)
			g_string_append (text, units[g_rand_int_range (rand, 0, G_N_ELEMENTS (units))]);
		go_regexp_quote (quoted, text->str);

		g_assert_cmpint (go_regcomp (&re, quoted->str, 0), ==, GO_REG_NOERROR);
		haystack = g_strconcat ("zz", text->str, "zz", NULL);
		g_assert_cmpint (go_regexec (&re, haystack, 1, &m, 0), ==, GO_REG_NOERROR);
		g_assert_cmpint (m.rm_so, ==, 2);
		g_assert_cmpint (m.rm_eo, ==, 2 + (int)text->len);
		g_free (haystack);
		go_regfree (&re);

		g_string_free (text, TRUE);
		g_string_free (quoted, TRUE);
	}
	g_rand_free (rand);
}

static GOSearchReplace *
make_sr (const char *search, const char *replace, gboolean regexp,
	 gboolean ignore_case, gboolean preserve_case, gboolean words)
{
	return g_object_new (GO_TYPE_SEARCH_REPLACE,
			     "search-text", search,
			     "replace-text", replace,
			     "is-regexp", regexp,
			     "ignore-case", ignore_case,
			     "preserve-case", preserve_case,
			     "match-words", words,
			     NULL);
}

/* Replace using a search with the given options */
#define SR_PLAIN	0
#define SR_REGEXP	1
#define SR_ICASE	2
#define SR_PRESERVE	4
#define SR_WORDS	8

static char *
sr_replace (const char *search, const char *replace, int opts, const char *src)
{
	GOSearchReplace *sr = make_sr (search, replace, (opts & SR_REGEXP) != 0,
				       (opts & SR_ICASE) != 0, (opts & SR_PRESERVE) != 0,
				       (opts & SR_WORDS) != 0);
	char *res = go_search_replace_string (sr, src);
	g_object_unref (sr);
	return res;
}

static gboolean
sr_match (const char *search, int opts, const char *src)
{
	GOSearchReplace *sr = make_sr (search, "", (opts & SR_REGEXP) != 0,
				       (opts & SR_ICASE) != 0, FALSE, (opts & SR_WORDS) != 0);
	gboolean res = go_search_match_string (sr, src);
	g_object_unref (sr);
	return res;
}

static void
assert_replace (const char *search, const char *replace, int opts,
		const char *src, const char *expected)
{
	char *res = sr_replace (search, replace, opts, src);
	g_assert_cmpstr (res, ==, expected);
	g_free (res);
}

static void
test_search_replace_object (void)
{
	GOSearchReplace *sr = g_object_new (GO_TYPE_SEARCH_REPLACE, NULL);
	char *s1 = NULL, *s2 = NULL;
	gboolean b1, b2, b3, b4;

	g_assert_true (GO_IS_SEARCH_REPLACE (sr));
	g_assert_true (g_type_is_a (GO_TYPE_SEARCH_REPLACE, G_TYPE_OBJECT));

	/* Nothing is set to begin with */
	g_assert_null (sr->search_text);
	g_assert_null (sr->replace_text);
	g_assert_false (sr->is_regexp);
	g_assert_false (sr->ignore_case);
	g_assert_false (sr->preserve_case);
	g_assert_false (sr->match_words);

	g_object_set (sr, "search-text", "needle", "replace-text", "pin",
		      "is-regexp", TRUE, "ignore-case", TRUE,
		      "preserve-case", TRUE, "match-words", TRUE, NULL);
	g_assert_cmpstr (sr->search_text, ==, "needle");
	g_assert_cmpstr (sr->replace_text, ==, "pin");
	g_assert_true (sr->is_regexp);
	g_assert_true (sr->ignore_case);
	g_assert_true (sr->preserve_case);
	g_assert_true (sr->match_words);

	g_object_get (sr, "search-text", &s1, "replace-text", &s2,
		      "is-regexp", &b1, "ignore-case", &b2,
		      "preserve-case", &b3, "match-words", &b4, NULL);
	g_assert_cmpstr (s1, ==, "needle");
	g_assert_cmpstr (s2, ==, "pin");
	g_assert_true (b1 && b2 && b3 && b4);
	g_free (s1);
	g_free (s2);

	/* Strings are copied */
	{
		char *text = g_strdup ("copy me");
		g_object_set (sr, "search-text", text, NULL);
		g_free (text);
		g_assert_cmpstr (sr->search_text, ==, "copy me");
	}
	g_object_set (sr, "search-text", NULL, NULL);
	g_assert_null (sr->search_text);

	g_object_unref (sr);
}

static void
test_search_replace_verify (void)
{
	GOSearchReplace *sr;
	GError *err = NULL;
	GError *e1 = NULL, *e2 = NULL, *e3 = NULL;

	/* No search text */
	sr = g_object_new (GO_TYPE_SEARCH_REPLACE, NULL);
	g_assert_false (go_search_replace_verify (sr, FALSE, &err));
	g_assert_nonnull (err);
	g_assert_true (g_error_matches (err, go_search_replace_error_quark (), 0));
	g_assert_cmpuint (strlen (err->message), >, 0);
	e1 = err;
	err = NULL;
	g_object_set (sr, "search-text", "", NULL);
	g_assert_false (go_search_replace_verify (sr, FALSE, &err));
	g_assert_nonnull (err);
	g_error_free (err);
	err = NULL;
	/* The error return is optional */
	g_assert_false (go_search_replace_verify (sr, FALSE, NULL));

	/* A replacement is only needed if asked for */
	g_object_set (sr, "search-text", "x", NULL);
	g_assert_true (go_search_replace_verify (sr, FALSE, &err));
	g_assert_no_error (err);
	g_assert_false (go_search_replace_verify (sr, TRUE, &err));
	g_assert_true (g_error_matches (err, go_search_replace_error_quark (), 0));
	e2 = err;
	err = NULL;
	g_object_set (sr, "replace-text", "", NULL);
	g_assert_true (go_search_replace_verify (sr, TRUE, &err));
	g_assert_no_error (err);

	/* Invalid regular expressions */
	g_object_set (sr, "search-text", "(", "is-regexp", TRUE, NULL);
	g_assert_false (go_search_replace_verify (sr, FALSE, &err));
	g_assert_true (g_error_matches (err, go_search_replace_error_quark (), 0));
	e3 = err;
	err = NULL;
	g_assert_false (go_search_replace_verify (sr, TRUE, NULL));
	/* ... which are just text if not a regular expression */
	g_object_set (sr, "is-regexp", FALSE, NULL);
	g_assert_true (go_search_replace_verify (sr, TRUE, &err));
	g_assert_no_error (err);

	/* The three kinds of errors so far say different things */
	g_assert_cmpstr (e1->message, !=, e2->message);
	g_assert_cmpstr (e1->message, !=, e3->message);
	g_assert_cmpstr (e2->message, !=, e3->message);
	g_error_free (e1);
	g_error_free (e2);
	g_error_free (e3);
	g_object_unref (sr);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_search_replace_verify*");
	g_assert_false (go_search_replace_verify (NULL, FALSE, &err));
	g_test_assert_expected_messages ();
	g_assert_null (err);
}

static void
test_search_replace_verify_replacement (void)
{
	static const struct {
		const char *search, *replace;
		gboolean ok;
	} cases[] = {
		/* Plain replacement text */
		{ "(a)", "x", TRUE },
		{ "(a)", "", TRUE },
		/* $-references must name groups that exist */
		{ "(a)", "$1", TRUE },
		{ "(a)(b)", "$2$1", TRUE },
		{ "(a)", "[$1]", TRUE },
		{ "a", "$1", FALSE },
		{ "(a)", "$2", FALSE },
		{ "(a)", "$0", FALSE },
		{ "(a)", "$", FALSE },
		{ "(a)", "$x", FALSE },
		{ "(a)", "x$", FALSE },
		/* One digit only: $10 is $1 followed by 0 */
		{ "(a)", "$10", TRUE },
		{ "(a)(b)(c)(d)(e)(f)(g)(h)(i)", "$9", TRUE },
		{ "(a)(b)(c)(d)(e)(f)(g)(h)", "$9", FALSE },
		/* Backslash quotes the next character */
		{ "(a)", "\\$", TRUE },
		{ "(a)", "\\\\", TRUE },
		{ "(a)", "a\\", FALSE },
		{ "(a)", "\\", FALSE },
		{ "(a)", "\\$1", TRUE }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		GOSearchReplace *sr = make_sr (cases[i].search, cases[i].replace, TRUE, FALSE, FALSE, FALSE);
		GError *err = NULL;
		gboolean ok = go_search_replace_verify (sr, TRUE, &err);

		g_assert_cmpint (ok, ==, cases[i].ok);
		g_assert_true ((err == NULL) == ok);
		if (err) {
			g_assert_true (g_error_matches (err, go_search_replace_error_quark (), 0));
			g_error_free (err);
		}
		/* The replacement is only looked at if asked for */
		g_assert_true (go_search_replace_verify (sr, FALSE, NULL));
		g_object_unref (sr);
	}

	/* Without regular expressions, the replacement is literal */
	{
		GOSearchReplace *sr = make_sr ("(a)", "$1\\", FALSE, FALSE, FALSE, FALSE);
		g_assert_true (go_search_replace_verify (sr, TRUE, NULL));
		g_object_unref (sr);
	}
}

static void
test_search_match (void)
{
	/* Plain text; special characters have no special meaning */
	g_assert_true (sr_match ("needle", SR_PLAIN, "a needle in a haystack"));
	g_assert_true (sr_match ("needle", SR_PLAIN, "needle"));
	g_assert_false (sr_match ("needle", SR_PLAIN, "a Needle in a haystack"));
	g_assert_false (sr_match ("needle", SR_PLAIN, "needl"));
	g_assert_false (sr_match ("needle", SR_PLAIN, ""));
	g_assert_true (sr_match ("a.b", SR_PLAIN, "xa.by"));
	g_assert_false (sr_match ("a.b", SR_PLAIN, "xaxby"));
	g_assert_true (sr_match ("(x|y)*[z]+", SR_PLAIN, "see (x|y)*[z]+ here"));
	g_assert_false (sr_match ("(x|y)*[z]+", SR_PLAIN, "xyz"));
	g_assert_true (sr_match ("\\", SR_PLAIN, "a\\b"));
	g_assert_true (sr_match ("h\xc3\xa9llo", SR_PLAIN, "oh h\xc3\xa9llo"));

	/* Case */
	g_assert_true (sr_match ("needle", SR_ICASE, "a NeEdLe"));
	g_assert_true (sr_match ("\xc3\xa9", SR_ICASE, "\xc3\x89"));
	g_assert_false (sr_match ("needle", SR_ICASE, "a nedle"));

	/* Regular expressions */
	g_assert_true (sr_match ("a.b", SR_REGEXP, "xaxby"));
	g_assert_true (sr_match ("^abc$", SR_REGEXP, "abc"));
	g_assert_false (sr_match ("^abc$", SR_REGEXP, "abcd"));
	g_assert_true (sr_match ("[0-9]+", SR_REGEXP, "route 66"));
	g_assert_false (sr_match ("[0-9]+", SR_REGEXP, "route"));
	g_assert_true (sr_match ("(ab)+c", SR_REGEXP, "xababc"));
	g_assert_true (sr_match ("COLOU?R", SR_REGEXP | SR_ICASE, "color"));
	g_assert_true (sr_match ("", SR_REGEXP, "anything"));

	/* Properties can be changed between searches */
	{
		GOSearchReplace *sr = make_sr ("abc", "", FALSE, FALSE, FALSE, FALSE);
		g_assert_true (go_search_match_string (sr, "xabcx"));
		g_assert_false (go_search_match_string (sr, "xABCx"));
		g_object_set (sr, "ignore-case", TRUE, NULL);
		g_assert_true (go_search_match_string (sr, "xABCx"));
		g_object_set (sr, "search-text", "a.c", NULL);
		g_assert_false (go_search_match_string (sr, "xABCx"));
		g_object_set (sr, "is-regexp", TRUE, NULL);
		g_assert_true (go_search_match_string (sr, "xABCx"));
		g_object_unref (sr);
	}

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_search_match_string*");
	g_assert_false (go_search_match_string (NULL, "x"));
	g_test_assert_expected_messages ();
}

static void
test_search_match_words (void)
{
	/* Whole words only: not preceded or followed by a letter or digit */
	g_assert_true (sr_match ("cat", SR_WORDS, "cat"));
	g_assert_true (sr_match ("cat", SR_WORDS, "the cat sat"));
	g_assert_true (sr_match ("cat", SR_WORDS, "cat sat"));
	g_assert_true (sr_match ("cat", SR_WORDS, "the cat"));
	g_assert_true (sr_match ("cat", SR_WORDS, "cat, dog"));
	g_assert_true (sr_match ("cat", SR_WORDS, "(cat)"));
	g_assert_true (sr_match ("cat", SR_WORDS, "a-cat-b"));
	g_assert_true (sr_match ("cat", SR_WORDS, "_cat_"));
	g_assert_false (sr_match ("cat", SR_WORDS, "concatenate"));
	g_assert_false (sr_match ("cat", SR_WORDS, "cats"));
	g_assert_false (sr_match ("cat", SR_WORDS, "scat"));
	g_assert_false (sr_match ("cat", SR_WORDS, "cat1"));
	g_assert_false (sr_match ("cat", SR_WORDS, "2cat"));
	g_assert_false (sr_match ("cat", SR_WORDS, "\xc3\xa9" "cat"));
	g_assert_false (sr_match ("cat", SR_WORDS, "cat\xc3\xa9"));
	g_assert_false (sr_match ("cat", SR_WORDS, ""));

	/* A rejected match doesn't stop the search */
	g_assert_true (sr_match ("cat", SR_WORDS, "catcat cat"));
	g_assert_true (sr_match ("cat", SR_WORDS, "scat cats cat."));
	g_assert_false (sr_match ("cat", SR_WORDS, "scat cats concat"));

	/* The rejected match may itself end where the next one starts */
	g_assert_false (sr_match ("a", SR_WORDS, "aa"));
	g_assert_false (sr_match ("a", SR_WORDS, "aaa"));
	g_assert_true (sr_match ("a", SR_WORDS, "aa a"));
	g_assert_false (sr_match ("ab", SR_WORDS, "ababab"));
	g_assert_true (sr_match ("ab", SR_WORDS, "abab ab"));

	/* With the other options */
	g_assert_true (sr_match ("cat", SR_WORDS | SR_ICASE, "The CAT sat"));
	g_assert_false (sr_match ("cat", SR_WORDS | SR_ICASE, "CATS"));
	g_assert_true (sr_match ("c.t", SR_WORDS | SR_REGEXP, "cut cats"));
	g_assert_false (sr_match ("c.t", SR_WORDS | SR_REGEXP, "cuts cats"));

	/* The empty string is not a word, so these don't match anything */
	g_assert_false (sr_match ("x*", SR_WORDS | SR_REGEXP, "abc"));
	g_assert_false (sr_match ("x*", SR_WORDS | SR_REGEXP, ""));
	g_assert_true (sr_match ("x*", SR_WORDS | SR_REGEXP, "abc x"));
}

#ifdef G_OS_UNIX
/* A string placed so that the byte before it, or the one after its
   terminator, is on a page that cannot be read.  The first of the
   returned values is the string. */
static char *
guarded_string (const char *text, gboolean guard_before)
{
	long pagesize = sysconf (_SC_PAGESIZE);
	char *mem = mmap (NULL, 2 * pagesize, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	char *str;

	g_assert_true (mem != MAP_FAILED);
	if (guard_before) {
		g_assert_cmpint (mprotect (mem, pagesize, PROT_NONE), ==, 0);
		str = mem + pagesize;
	} else {
		g_assert_cmpint (mprotect (mem + pagesize, pagesize, PROT_NONE), ==, 0);
		str = mem + pagesize - (strlen (text) + 1);
	}
	memcpy (str, text, strlen (text) + 1);
	return str;
}

static void
test_search_words_memory_before (void)
{
	if (g_test_subprocess ()) {
		/* Nothing precedes the text and nothing may be looked at there */
		GOSearchReplace *sr = make_sr ("cat", "dog", FALSE, FALSE, FALSE, TRUE);
		char *res;

		g_assert_true (go_search_match_string (sr, guarded_string ("cat", TRUE)));
		res = go_search_replace_string (sr, guarded_string ("cat sat", TRUE));
		g_assert_cmpstr (res, ==, "dog sat");
		g_free (res);
		g_object_unref (sr);
		return;
	}
	g_test_trap_subprocess (NULL, 10 * G_USEC_PER_SEC, 0);
	g_test_trap_assert_passed ();
}

static void
test_search_words_memory_after (void)
{
	if (g_test_subprocess ()) {
		/* Looking for words with something that can match the empty
		   string must not carry on beyond the end of the text */
		GOSearchReplace *sr = make_sr ("x*", "-", TRUE, FALSE, FALSE, TRUE);
		char *res;

		g_assert_false (go_search_match_string (sr, guarded_string ("abc", FALSE)));
		res = go_search_replace_string (sr, guarded_string ("abc", FALSE));
		g_assert_null (res);
		g_object_unref (sr);
		return;
	}
	g_test_trap_subprocess (NULL, 10 * G_USEC_PER_SEC, 0);
	g_test_trap_assert_passed ();
}
#endif

static void
test_search_replace_plain (void)
{
	/* Nothing found, nothing returned */
	assert_replace ("x", "y", SR_PLAIN, "abc", NULL);
	assert_replace ("x", "y", SR_PLAIN, "", NULL);

	/* All occurrences are replaced */
	assert_replace ("a", "b", SR_PLAIN, "banana", "bbnbnb");
	assert_replace ("-", "+", SR_PLAIN, "a-b-c", "a+b+c");
	assert_replace ("ab", "X", SR_PLAIN, "ababab", "XXX");
	assert_replace ("abc", "", SR_PLAIN, "xabcyabcz", "xyz");
	assert_replace ("abc", "abc", SR_PLAIN, "xabc", "xabc");
	assert_replace ("a", "aa", SR_PLAIN, "aaa", "aaaaaa");
	assert_replace ("abc", "x", SR_PLAIN, "abc", "x");
	assert_replace ("a", "b", SR_PLAIN, "a", "b");

	/* Special characters in the search are plain text, and in the
	   replacement too if no regular expression is involved */
	assert_replace ("a.b", "X", SR_PLAIN, "axb a.b", "axb X");
	assert_replace ("(a)", "$1", SR_PLAIN, "(a)", "$1");
	assert_replace ("a", "\\", SR_PLAIN, "banana", "b\\n\\n\\");
	assert_replace ("a", "$", SR_PLAIN, "a", "$");
	assert_replace ("*", "+", SR_PLAIN, "2*3*4", "2+3+4");

	/* Case */
	assert_replace ("abc", "x", SR_PLAIN, "ABC", NULL);
	assert_replace ("abc", "x", SR_ICASE, "ABC abc AbC", "x x x");
	assert_replace ("\xc3\xa9", "E", SR_ICASE, "\xc3\x89t\xc3\xa9", "EtE");

	/* UTF-8 text */
	assert_replace ("\xc3\xb6", "oe", SR_PLAIN, "sch\xc3\xb6n", "schoen");
	assert_replace ("e", "\xe2\x82\xac", SR_PLAIN, "tee", "t\xe2\x82\xac\xe2\x82\xac");

	/* The search text is used as it is at the time of the call */
	{
		GOSearchReplace *sr = make_sr ("a", "b", FALSE, FALSE, FALSE, FALSE);
		char *res = go_search_replace_string (sr, "aXa");
		g_assert_cmpstr (res, ==, "bXb");
		g_free (res);
		g_object_set (sr, "search-text", "X", "replace-text", "Y", NULL);
		res = go_search_replace_string (sr, "aXa");
		g_assert_cmpstr (res, ==, "aYa");
		g_free (res);
		g_object_set (sr, "replace-text", "ZZ", NULL);
		res = go_search_replace_string (sr, "aXa");
		g_assert_cmpstr (res, ==, "aZZa");
		g_free (res);
		g_object_unref (sr);
	}

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_search_replace_string*");
	g_assert_null (go_search_replace_string (NULL, "x"));
	g_test_assert_expected_messages ();
	{
		GOSearchReplace *sr = make_sr ("a", NULL, FALSE, FALSE, FALSE, FALSE);
		g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_search_replace_string*");
		g_assert_null (go_search_replace_string (sr, "a"));
		g_test_assert_expected_messages ();
		g_object_unref (sr);
	}
}

static void
test_search_replace_regexp (void)
{
	/* Groups */
	assert_replace ("(\\w+)@(\\w+)", "$2 at $1", SR_REGEXP, "joe@host and amy@box", "host at joe and box at amy");
	assert_replace ("(a)(b)", "$2$1", SR_REGEXP, "abab", "baba");
	assert_replace ("(a)", "[$1$1]", SR_REGEXP, "banana", "b[aa]n[aa]n[aa]");
	assert_replace ("(a)|(b)", "[$1$2]", SR_REGEXP, "ab", "[a][b]");
	assert_replace ("(\\d+)", "<$1>", SR_REGEXP, "a1b22c333", "a<1>b<22>c<333>");

	/* Replacement text with a backslash quotes the character after it */
	assert_replace ("a", "\\$", SR_REGEXP, "banana", "b$n$n$");
	assert_replace ("a", "\\\\", SR_REGEXP, "banana", "b\\n\\n\\");
	assert_replace ("(a)", "\\$1", SR_REGEXP, "a", "$1");
	assert_replace ("a", "x\\y", SR_REGEXP, "a", "xy");

	/* Regular expressions with no groups and replacements without
	   special characters */
	assert_replace ("b+", "X", SR_REGEXP, "abbbcbd", "aXcXd");
	assert_replace ("[aeiou]", "", SR_REGEXP, "education", "dctn");
	assert_replace ("\\s+", " ", SR_REGEXP, "a  b \t c", "a b c");
	assert_replace ("x", "y", SR_REGEXP, "abc", NULL);

	/* Anchors and patterns that match everything or nothing in particular */
	assert_replace ("^", "> ", SR_REGEXP, "text", "> text");
	assert_replace ("$", "!", SR_REGEXP, "text", "text!");
	assert_replace (".*", "X", SR_REGEXP, "abc", "X");
	assert_replace ("^a", "b", SR_REGEXP, "aaa", "baa");
	assert_replace ("a$", "b", SR_REGEXP, "aaa", "aab");

	/* Case */
	assert_replace ("COLOU?R", "hue", SR_REGEXP | SR_ICASE, "Color COLOUR colour", "hue hue hue");

	/* Bytes vs characters */
	assert_replace ("(.)\xc3\xa9", "$1!", SR_REGEXP, "x\xc3\xa9 \xc3\xb6\xc3\xa9", "x! \xc3\xb6!");
}

static void
test_search_replace_preserve_case (void)
{
	/* The replacement follows the capitalization of what was found */
	assert_replace ("hello", "world", SR_ICASE | SR_PRESERVE, "hello",
			"world");
	assert_replace ("hello", "world", SR_ICASE | SR_PRESERVE, "Hello",
			"World");
	assert_replace ("hello", "world", SR_ICASE | SR_PRESERVE, "HELLO",
			"WORLD");
	assert_replace ("hello", "world", SR_ICASE | SR_PRESERVE, "Hello HELLO hello HeLLo",
			"World WORLD world world");
	assert_replace ("the search", "the replace", SR_ICASE | SR_PRESERVE,
			"The Search and THE SEARCH and the search",
			"The Replace and THE REPLACE and the replace");

	/* Without the option the replacement is used as it is */
	assert_replace ("hello", "world", SR_ICASE, "Hello HELLO", "world world");

	/* Non-ASCII letters take part */
	assert_replace ("\xc3\xa9t\xc3\xa9", "ete", SR_ICASE | SR_PRESERVE, "\xc3\x89t\xc3\xa9",
			"Ete");
	assert_replace ("\xc3\xa9t\xc3\xa9", "ete", SR_ICASE | SR_PRESERVE, "\xc3\x89T\xc3\x89",
			"ETE");

	/* Text without letters has no case to preserve */
	assert_replace ("123", "abc", SR_PRESERVE, "x123", "xabc");
}

static void
test_search_replace_words (void)
{
	assert_replace ("cat", "dog", SR_WORDS, "cat concat cats cat.", "dog concat cats dog.");
	assert_replace ("cat", "dog", SR_WORDS, "cat", "dog");
	assert_replace ("cat", "dog", SR_WORDS, "the cat", "the dog");
	assert_replace ("cat", "dog", SR_WORDS, "(cat)", "(dog)");
	assert_replace ("cat", "dog", SR_WORDS, "cat cat cat", "dog dog dog");

	/* Nothing to replace, nothing returned, even if there were matches
	   that didn't count */
	assert_replace ("cat", "dog", SR_WORDS, "concat", NULL);
	assert_replace ("cat", "dog", SR_WORDS, "cats scat", NULL);
	assert_replace ("a", "b", SR_WORDS, "aa", NULL);
	assert_replace ("a", "b", SR_WORDS, "xay", NULL);
	assert_replace ("a", "b", SR_WORDS, "aaa a aaa", "aaa b aaa");
	assert_replace ("a", "b", SR_WORDS, "a a", "b b");
	assert_replace ("a", "b", SR_WORDS, "a-a", "b-b");

	/* UTF-8 around the match */
	assert_replace ("cat", "dog", SR_WORDS, "\xc3\xa9" "cat cat\xc3\xa9 cat", "\xc3\xa9" "cat cat\xc3\xa9 dog");

	/* With the other options */
	assert_replace ("cat", "dog", SR_WORDS | SR_ICASE | SR_PRESERVE,
			"Cat CAT cat CATS", "Dog DOG dog CATS");
	assert_replace ("c(.)t", "d$1g", SR_WORDS | SR_REGEXP, "cat cut cart", "dag dug cart");

	/* An empty match isn't a word */
	assert_replace ("x*", "-", SR_WORDS | SR_REGEXP, "abc", NULL);
}

static void
register_regutf8_tests (void)
{
	g_test_add_func ("/utils/regutf8/regcomp/errors", test_regex_compile_errors);
	g_test_add_func ("/utils/regutf8/regexec", test_regex_exec);
	g_test_add_func ("/utils/regutf8/flags", test_regex_flags);
	g_test_add_func ("/utils/regutf8/regerror", test_regex_error_messages);
	g_test_add_func ("/utils/regutf8/regexp_quote", test_regexp_quote);
	g_test_add_func ("/utils/regutf8/regexp_quote/matches_itself", test_regexp_quote_matches_itself);
	g_test_add_func ("/utils/regutf8/search_replace/object", test_search_replace_object);
	g_test_add_func ("/utils/regutf8/search_replace/verify", test_search_replace_verify);
	g_test_add_func ("/utils/regutf8/search_replace/verify_replacement", test_search_replace_verify_replacement);
	g_test_add_func ("/utils/regutf8/search_match", test_search_match);
	g_test_add_func ("/utils/regutf8/search_match/words", test_search_match_words);
#ifdef G_OS_UNIX
	g_test_add_func ("/utils/regutf8/search_words/memory_before", test_search_words_memory_before);
	g_test_add_func ("/utils/regutf8/search_words/memory_after", test_search_words_memory_after);
#endif
	g_test_add_func ("/utils/regutf8/search_replace/plain", test_search_replace_plain);
	g_test_add_func ("/utils/regutf8/search_replace/regexp", test_search_replace_regexp);
	g_test_add_func ("/utils/regutf8/search_replace/preserve_case", test_search_replace_preserve_case);
	g_test_add_func ("/utils/regutf8/search_replace/words", test_search_replace_words);
}

/* ------------------------------------------------------------------ */
/* go-libxml-extras.c                                                  */
/* ------------------------------------------------------------------ */

static void
xml_quiet (G_GNUC_UNUSED void *ctx, G_GNUC_UNUSED const char *msg, ...)
{
}

/* Parse a document; the document is returned through *doc, the root
   element is the function result. */
static xmlNodePtr
xml_parse (const char *text, xmlDocPtr *doc)
{
	*doc = xmlReadMemory (text, strlen (text), NULL, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	g_assert_nonnull (*doc);
	return xmlDocGetRootElement (*doc);
}

/* A fresh document with an empty root element */
static xmlNodePtr
xml_new_root (xmlDocPtr *doc)
{
	xmlNodePtr root;
	*doc = xmlNewDoc (CC2XML ("1.0"));
	root = xmlNewDocNode (*doc, NULL, CC2XML ("root"), NULL);
	xmlDocSetRootElement (*doc, root);
	return root;
}

static void
test_xml_parse_file (void)
{
	char *name = NULL;
	int fd = g_file_open_tmp ("test-util-XXXXXX.xml", &name, NULL);
	xmlDocPtr doc;
	xmlNodePtr root;
	char *dir, *missing;

	g_assert_cmpint (fd, >=, 0);
	close (fd);

	g_assert_true (g_file_set_contents (name, "<?xml version=\"1.0\"?>\n<top a=\"1\"><child/></top>\n", -1, NULL));
	doc = go_xml_parse_file (name);
	g_assert_nonnull (doc);
	root = xmlDocGetRootElement (doc);
	g_assert_cmpstr (CXML2C (root->name), ==, "top");
	g_assert_nonnull (go_xml_get_child_by_name (root, "child"));
	xmlFreeDoc (doc);

	/* Things that aren't XML documents */
	xmlSetGenericErrorFunc (NULL, xml_quiet);
	g_assert_true (g_file_set_contents (name, "this is not XML", -1, NULL));
	g_assert_null (go_xml_parse_file (name));
	g_assert_true (g_file_set_contents (name, "<a><b></a>", -1, NULL));
	g_assert_null (go_xml_parse_file (name));
	g_assert_true (g_file_set_contents (name, "", 0, NULL));
	g_assert_null (go_xml_parse_file (name));
	xmlSetGenericErrorFunc (NULL, NULL);

	/* No such file, and a directory */
	g_remove (name);
	g_assert_null (go_xml_parse_file (name));
	dir = g_path_get_dirname (name);
	g_assert_null (go_xml_parse_file (dir));
	missing = g_build_filename (dir, "no", "such", "directory", "file.xml", NULL);
	g_assert_null (go_xml_parse_file (missing));
	g_free (missing);
	g_free (dir);
	g_free (name);
}

static void
test_xml_cstr (void)
{
	xmlDocPtr doc;
	xmlNodePtr root = xml_parse ("<r a=\"hello\" e=\"\" u=\"h\xc3\xa9llo\" x=\"1 &amp; 2 &lt; 3\">"
				     "text<c>inner</c> more</r>", &doc);
	xmlChar *s;

	s = go_xml_node_get_cstr (root, "a");
	g_assert_cmpstr (CXML2C (s), ==, "hello");
	xmlFree (s);
	s = go_xml_node_get_cstr (root, "e");
	g_assert_cmpstr (CXML2C (s), ==, "");
	xmlFree (s);
	s = go_xml_node_get_cstr (root, "u");
	g_assert_cmpstr (CXML2C (s), ==, "h\xc3\xa9llo");
	xmlFree (s);
	s = go_xml_node_get_cstr (root, "x");
	g_assert_cmpstr (CXML2C (s), ==, "1 & 2 < 3");
	xmlFree (s);
	g_assert_null (go_xml_node_get_cstr (root, "missing"));

	/* Without a name, the text of the node, including its children's */
	s = go_xml_node_get_cstr (root, NULL);
	g_assert_cmpstr (CXML2C (s), ==, "textinner more");
	xmlFree (s);
	xmlFreeDoc (doc);

	/* ... but only if there is any */
	root = xml_parse ("<r/>", &doc);
	g_assert_null (go_xml_node_get_cstr (root, NULL));
	xmlFreeDoc (doc);
	root = xml_parse ("<r></r>", &doc);
	g_assert_null (go_xml_node_get_cstr (root, NULL));
	xmlFreeDoc (doc);
	root = xml_parse ("<r>x</r>", &doc);
	s = go_xml_node_get_cstr (root, NULL);
	g_assert_cmpstr (CXML2C (s), ==, "x");
	xmlFree (s);
	xmlFreeDoc (doc);
}

static void
test_xml_set_cstr (void)
{
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	xmlChar *s;
	static const char *const values[] = {
		"plain", "", "with <angle> & \"quotes\" and 'apostrophes'", "h\xc3\xa9llo \xe2\x82\xac",
		"  spaces  ", "tab\there"
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (values); i++) {
		go_xml_node_set_cstr (root, "attr", values[i]);
		s = go_xml_node_get_cstr (root, "attr");
		g_assert_cmpstr (CXML2C (s), ==, values[i]);
		xmlFree (s);
	}

	/* Setting again replaces */
	go_xml_node_set_cstr (root, "a", "one");
	go_xml_node_set_cstr (root, "a", "two");
	s = go_xml_node_get_cstr (root, "a");
	g_assert_cmpstr (CXML2C (s), ==, "two");
	xmlFree (s);
	/* Other attributes are left alone */
	go_xml_node_set_cstr (root, "b", "bee");
	s = go_xml_node_get_cstr (root, "a");
	g_assert_cmpstr (CXML2C (s), ==, "two");
	xmlFree (s);

	/* Without a name it is the content; text and children are replaced */
	for (i = 0; i < G_N_ELEMENTS (values); i++) {
		xmlNewChild (root, NULL, CC2XML ("kid"), CC2XML ("x"));
		go_xml_node_set_cstr (root, NULL, values[i]);
		if (values[i][0]) {
			s = go_xml_node_get_cstr (root, NULL);
			g_assert_cmpstr (CXML2C (s), ==, values[i]);
			xmlFree (s);
		}
		g_assert_null (go_xml_get_child_by_name (root, "kid"));
	}

	/* The result survives serialization */
	go_xml_node_set_cstr (root, "attr", "a<b&\"c\"");
	{
		xmlChar *mem;
		int size;
		xmlDocPtr doc2;
		xmlNodePtr root2;

		xmlDocDumpMemory (doc, &mem, &size);
		doc2 = xmlReadMemory (CXML2C (mem), size, NULL, NULL, 0);
		xmlFree (mem);
		root2 = xmlDocGetRootElement (doc2);
		s = go_xml_node_get_cstr (root2, "attr");
		g_assert_cmpstr (CXML2C (s), ==, "a<b&\"c\"");
		xmlFree (s);
		xmlFreeDoc (doc2);
	}
	xmlFreeDoc (doc);
}

static void
test_xml_bool (void)
{
	static const struct { const char *text; gboolean value; } cases[] = {
		{ "1", TRUE }, { "true", TRUE }, { "TRUE", TRUE }, { "True", TRUE }, { "tRuE", TRUE },
		{ "0", FALSE }, { "false", FALSE }, { "FALSE", FALSE }, { "no", FALSE },
		{ "yes", FALSE }, { "2", FALSE }, { "", FALSE }, { "tru", FALSE }, { " 1", FALSE },
		{ "11", FALSE }, { "-1", FALSE }
	};
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	unsigned i;
	gboolean res;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		res = !cases[i].value;
		go_xml_node_set_cstr (root, "b", cases[i].text);
		/* Whatever the text, there is a value; it just isn't always true */
		g_assert_true (go_xml_node_get_bool (root, "b", &res));
		g_assert_cmpint (res, ==, cases[i].value);
	}

	/* No attribute: no value, and the result is left alone */
	res = 42;
	g_assert_false (go_xml_node_get_bool (root, "missing", &res));
	g_assert_cmpint (res, ==, 42);

	/* Writing and reading back */
	go_xml_node_set_bool (root, "b", TRUE);
	res = FALSE;
	g_assert_true (go_xml_node_get_bool (root, "b", &res));
	g_assert_true (res);
	go_xml_node_set_bool (root, "b", FALSE);
	res = TRUE;
	g_assert_true (go_xml_node_get_bool (root, "b", &res));
	g_assert_false (res);

	/* In the content of the node */
	go_xml_node_set_bool (root, NULL, TRUE);
	res = FALSE;
	g_assert_true (go_xml_node_get_bool (root, NULL, &res));
	g_assert_true (res);
	go_xml_node_set_bool (root, NULL, FALSE);
	res = TRUE;
	g_assert_true (go_xml_node_get_bool (root, NULL, &res));
	g_assert_false (res);

	xmlFreeDoc (doc);
}

static void
test_xml_int (void)
{
	static const struct { const char *text; gboolean ok; int value; } cases[] = {
		{ "0", TRUE, 0 }, { "42", TRUE, 42 }, { "-17", TRUE, -17 }, { "+5", TRUE, 5 },
		{ "007", TRUE, 7 }, { "2147483647", TRUE, G_MAXINT }, { "-2147483648", TRUE, G_MININT },
		{ "2147483648", FALSE, 0 }, { "-2147483649", FALSE, 0 },
		{ "99999999999999999999", FALSE, 0 }, { "-99999999999999999999", FALSE, 0 },
		{ "", FALSE, 0 }, { "abc", FALSE, 0 }, { "1x", FALSE, 0 }, { "x1", FALSE, 0 },
		{ "1.5", FALSE, 0 }, { "1 2", FALSE, 0 }, { "0x10", FALSE, 0 }, { "-", FALSE, 0 },
		{ "1e3", FALSE, 0 }
	};
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	unsigned i;
	int res;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		res = 123456;
		go_xml_node_set_cstr (root, "n", cases[i].text);
		g_assert_cmpint (go_xml_node_get_int (root, "n", &res), ==, cases[i].ok);
		if (cases[i].ok)
			g_assert_cmpint (res, ==, cases[i].value);
	}

	res = 5;
	g_assert_false (go_xml_node_get_int (root, "missing", &res));

	/* Round trip, including the extremes */
	{
		static const int values[] = { 0, 1, -1, 255, 65536, -65536, 123456789, G_MAXINT, G_MININT };
		for (i = 0; i < G_N_ELEMENTS (values); i++) {
			go_xml_node_set_int (root, "n", values[i]);
			res = 1;
			g_assert_true (go_xml_node_get_int (root, "n", &res));
			g_assert_cmpint (res, ==, values[i]);

			go_xml_node_set_int (root, NULL, values[i]);
			res = 1;
			g_assert_true (go_xml_node_get_int (root, NULL, &res));
			g_assert_cmpint (res, ==, values[i]);
		}
	}

	xmlFreeDoc (doc);
}

static void
test_xml_double (void)
{
	static const struct { const char *text; gboolean ok; double value; } cases[] = {
		{ "0", TRUE, 0 }, { "1.5", TRUE, 1.5 }, { "-2.25", TRUE, -2.25 }, { "+3", TRUE, 3 },
		{ ".5", TRUE, 0.5 }, { "5.", TRUE, 5 }, { "1e3", TRUE, 1000 }, { "1E-2", TRUE, 0.01 },
		{ "-1.5e+2", TRUE, -150 }, { "007.50", TRUE, 7.5 },
		{ "", FALSE, 0 }, { "abc", FALSE, 0 }, { "1.5x", FALSE, 0 }, { "1e", FALSE, 0 },
		{ "1,5", FALSE, 0 }, { "1 2", FALSE, 0 }, { ".", FALSE, 0 }, { "1e999", FALSE, 0 },
		{ "--1", FALSE, 0 }
	};
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	unsigned i;
	double res;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		res = 1234.5;
		go_xml_node_set_cstr (root, "d", cases[i].text);
		g_assert_cmpint (go_xml_node_get_double (root, "d", &res), ==, cases[i].ok);
		if (cases[i].ok)
			assert_close (res, cases[i].value);
	}

	g_assert_false (go_xml_node_get_double (root, "missing", &res));
	xmlFreeDoc (doc);
}

static void
test_xml_double_roundtrip (void)
{
	/* Whatever the size of the number it can be read back */
	static const double values[] = {
		0, 1, -1, 0.5, 0.1, 1.0 / 3, 123456.789, -98765.4321, 3.14159265358979,
		1e-5, 2e-5, 9.99e-6, 1.234e-7, -9.87e-12, 1e-100, 1e-300,
		1e8, 1e9, 2.5e9, -7.5e10, 123456789012345.0, 1.5e100, -2.5e200, 1.7e308
	};
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (values); i++) {
		double res = -1234.5;
		xmlChar *text;

		go_xml_node_set_double (root, "d", values[i], -1);
		text = go_xml_node_get_cstr (root, "d");
		/* A number text must be of reasonable length */
		g_assert_cmpuint (strlen (CXML2C (text)), <, 40);
		xmlFree (text);
		g_assert_true (go_xml_node_get_double (root, "d", &res));
		if (values[i] == 0)
			g_assert_cmpfloat (res, ==, 0);
		else
			g_assert_cmpfloat (fabs (res - values[i]) / fabs (values[i]), <, 1e-14);

		go_xml_node_set_double (root, NULL, values[i], -1);
		res = -1234.5;
		g_assert_true (go_xml_node_get_double (root, NULL, &res));
		if (values[i] == 0)
			g_assert_cmpfloat (res, ==, 0);
		else
			g_assert_cmpfloat (fabs (res - values[i]) / fabs (values[i]), <, 1e-14);
	}

	/* Precision limits the number of digits */
	{
		double res;
		xmlChar *a, *b;

		go_xml_node_set_double (root, "d", 3.14159265358979, 3);
		g_assert_true (go_xml_node_get_double (root, "d", &res));
		assert_close (res, 3.14);

		go_xml_node_set_double (root, "d", 123456.789, 2);
		g_assert_true (go_xml_node_get_double (root, "d", &res));
		assert_close (res, 120000);

		go_xml_node_set_double (root, "d", -0.00123456, 4);
		g_assert_true (go_xml_node_get_double (root, "d", &res));
		assert_close (res, -0.001235);

		go_xml_node_set_double (root, "d", 0.1, -1);
		a = go_xml_node_get_cstr (root, "d");
		g_assert_cmpstr (CXML2C (a), ==, "0.1");
		xmlFree (a);

		/* Too large precision is the largest sensible one */
		go_xml_node_set_double (root, "d", 1.0 / 3, DBL_DIG);
		a = go_xml_node_get_cstr (root, "d");
		go_xml_node_set_double (root, "d", 1.0 / 3, 100);
		b = go_xml_node_get_cstr (root, "d");
		g_assert_cmpstr (CXML2C (a), ==, CXML2C (b));
		xmlFree (b);
		xmlFree (a);
	}

	xmlFreeDoc (doc);
}

static void
test_xml_gocolor (void)
{
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	GOColor c;
	int v;

	/* The format is hexadecimal red:green:blue, 16 bits for each */
	go_xml_node_set_cstr (root, "c", "FFFF:8080:0");
	c = 0;
	g_assert_true (go_xml_node_get_gocolor (root, "c", &c));
	g_assert_cmphex (c, ==, GO_COLOR_FROM_RGBA (0xff, 0x80, 0, 0xff));

	go_xml_node_set_cstr (root, "c", "ffff:0:abcd");
	g_assert_true (go_xml_node_get_gocolor (root, "c", &c));
	g_assert_cmphex (c, ==, GO_COLOR_FROM_RGBA (0xff, 0, 0xab, 0xff));

	go_xml_node_set_cstr (root, "c", "0:0:0");
	g_assert_true (go_xml_node_get_gocolor (root, "c", &c));
	g_assert_cmphex (c, ==, GO_COLOR_BLACK);

	/* Not colors */
	g_assert_false (go_xml_node_get_gocolor (root, "missing", &c));
	go_xml_node_set_cstr (root, "c", "red");
	g_assert_false (go_xml_node_get_gocolor (root, "c", &c));
	go_xml_node_set_cstr (root, "c", "FF:FF");
	g_assert_false (go_xml_node_get_gocolor (root, "c", &c));
	go_xml_node_set_cstr (root, "c", "");
	g_assert_false (go_xml_node_get_gocolor (root, "c", &c));
	go_xml_node_set_cstr (root, "c", "zz:00:00");
	g_assert_false (go_xml_node_get_gocolor (root, "c", &c));

	/* What is written can be read back; alpha is not part of it and
	   always comes back opaque */
	for (v = 0; v < 256; v++) {
		GOColor orig = GO_COLOR_FROM_RGBA (v, 255 - v, (v * 37) & 0xff, 0xff);
		go_xml_node_set_gocolor (root, "c", orig);
		c = 0;
		g_assert_true (go_xml_node_get_gocolor (root, "c", &c));
		g_assert_cmphex (c, ==, orig);
	}
	go_xml_node_set_gocolor (root, "c", GO_COLOR_FROM_RGBA (1, 2, 3, 4));
	g_assert_true (go_xml_node_get_gocolor (root, "c", &c));
	g_assert_cmphex (c, ==, GO_COLOR_FROM_RGBA (1, 2, 3, 0xff));

	/* In the content of the node */
	go_xml_node_set_gocolor (root, NULL, GO_COLOR_FROM_RGB (10, 20, 30));
	c = 0;
	g_assert_true (go_xml_node_get_gocolor (root, NULL, &c));
	g_assert_cmphex (c, ==, GO_COLOR_FROM_RGB (10, 20, 30));

	xmlFreeDoc (doc);
}

static GType
fresh_enum_type (void)
{
	/* A type nobody has used yet, so that its class has not been created */
	static const GEnumValue values[] = {
		{ 1, "FRESH_ONE", "one" },
		{ 5, "FRESH_FIVE", "five" },
		{ 0, NULL, NULL }
	};
	static GType type = 0;
	if (!type)
		type = g_enum_register_static ("TestFreshEnum", values);
	return type;
}

static void
test_xml_enum (void)
{
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	GEnumClass *klass = g_type_class_ref (GO_TYPE_DIRECTION);
	gint res;
	unsigned i;

	for (i = 0; i < klass->n_values; i++) {
		const GEnumValue *v = &klass->values[i];
		char *num = g_strdup_printf ("%d", v->value);
		const char *spelled[3] = { v->value_name, v->value_nick, num };
		int k;

		/* Read by name, by nick and by number */
		for (k = 0; k < 3; k++) {
			go_xml_node_set_cstr (root, "e", spelled[k]);
			res = -1000;
			g_assert_true (go_xml_node_get_enum (root, "e", GO_TYPE_DIRECTION, &res));
			g_assert_cmpint (res, ==, v->value);
		}
		g_free (num);

		/* Written and read back */
		go_xml_node_set_enum (root, "e", GO_TYPE_DIRECTION, v->value);
		res = -1000;
		g_assert_true (go_xml_node_get_enum (root, "e", GO_TYPE_DIRECTION, &res));
		g_assert_cmpint (res, ==, v->value);
	}

	/* The value is stored by name */
	go_xml_node_set_enum (root, "e", GO_TYPE_DIRECTION, GO_DIRECTION_LEFT);
	{
		xmlChar *s = go_xml_node_get_cstr (root, "e");
		g_assert_cmpstr (CXML2C (s), ==, g_enum_get_value (klass, GO_DIRECTION_LEFT)->value_name);
		xmlFree (s);
	}

	/* Not values of the type */
	g_assert_false (go_xml_node_get_enum (root, "missing", GO_TYPE_DIRECTION, &res));
	go_xml_node_set_cstr (root, "e", "sideways");
	g_assert_false (go_xml_node_get_enum (root, "e", GO_TYPE_DIRECTION, &res));
	go_xml_node_set_cstr (root, "e", "99");
	g_assert_false (go_xml_node_get_enum (root, "e", GO_TYPE_DIRECTION, &res));
	go_xml_node_set_cstr (root, "e", "");
	g_assert_false (go_xml_node_get_enum (root, "e", GO_TYPE_DIRECTION, &res));

	/* Setting an invalid value warns and leaves the old one alone */
	go_xml_node_set_enum (root, "e", GO_TYPE_DIRECTION, GO_DIRECTION_UP);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Invalid value 99 for type GODirection*");
	go_xml_node_set_enum (root, "e", GO_TYPE_DIRECTION, 99);
	g_test_assert_expected_messages ();
	g_assert_true (go_xml_node_get_enum (root, "e", GO_TYPE_DIRECTION, &res));
	g_assert_cmpint (res, ==, GO_DIRECTION_UP);

	/* In the content of the node */
	go_xml_node_set_enum (root, NULL, GO_TYPE_DIRECTION, GO_DIRECTION_RIGHT);
	res = -1000;
	g_assert_true (go_xml_node_get_enum (root, NULL, GO_TYPE_DIRECTION, &res));
	g_assert_cmpint (res, ==, GO_DIRECTION_RIGHT);

	g_type_class_unref (klass);
	xmlFreeDoc (doc);
}

static void
test_xml_enum_fresh_type (void)
{
	/* The class of an enumeration need not exist before it is used here */
	xmlDocPtr doc;
	xmlNodePtr root = xml_new_root (&doc);
	GType t = fresh_enum_type ();
	gint res = 0;

	go_xml_node_set_enum (root, "e", t, 5);
	g_assert_true (go_xml_node_get_enum (root, "e", t, &res));
	g_assert_cmpint (res, ==, 5);

	go_xml_node_set_cstr (root, "e", "FRESH_ONE");
	g_assert_true (go_xml_node_get_enum (root, "e", t, &res));
	g_assert_cmpint (res, ==, 1);
	go_xml_node_set_cstr (root, "e", "five");
	g_assert_true (go_xml_node_get_enum (root, "e", t, &res));
	g_assert_cmpint (res, ==, 5);
	go_xml_node_set_cstr (root, "e", "2");
	g_assert_false (go_xml_node_get_enum (root, "e", t, &res));

	xmlFreeDoc (doc);
}

static void
test_xml_child_by_name (void)
{
	xmlDocPtr doc;
	xmlNodePtr root = xml_parse ("<root><a id=\"1\"/><b/><a id=\"2\"><x/></a>text<c/></root>", &doc);
	xmlNodePtr n;
	xmlChar *id;

	/* The first one with that name */
	n = go_xml_get_child_by_name (root, "a");
	g_assert_nonnull (n);
	id = go_xml_node_get_cstr (n, "id");
	g_assert_cmpstr (CXML2C (id), ==, "1");
	xmlFree (id);
	g_assert_nonnull (go_xml_get_child_by_name (root, "b"));
	g_assert_nonnull (go_xml_get_child_by_name (root, "c"));
	g_assert_cmpstr (CXML2C (go_xml_get_child_by_name (root, "c")->name), ==, "c");

	/* Only direct children count, and the name must match fully */
	g_assert_null (go_xml_get_child_by_name (root, "x"));
	g_assert_null (go_xml_get_child_by_name (root, "root"));
	g_assert_null (go_xml_get_child_by_name (root, "A"));
	g_assert_null (go_xml_get_child_by_name (root, "aa"));
	g_assert_null (go_xml_get_child_by_name (root, ""));
	{
		xmlNodePtr a2 = go_xml_get_child_by_name (root, "a")->next->next;
		g_assert_nonnull (go_xml_get_child_by_name (a2, "x"));
	}

	/* Elements without children */
	g_assert_null (go_xml_get_child_by_name (go_xml_get_child_by_name (root, "b"), "x"));

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_xml_get_child_by_name*");
	g_assert_null (go_xml_get_child_by_name (NULL, "a"));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_xml_get_child_by_name*");
	g_assert_null (go_xml_get_child_by_name (root, NULL));
	g_test_assert_expected_messages ();

	xmlFreeDoc (doc);
}

static char *
node_text (xmlNodePtr n)
{
	xmlChar *s;
	char *res;

	if (!n)
		return NULL;
	s = xmlNodeGetContent (n);
	res = g_strdup (CXML2C (s));
	xmlFree (s);
	return res;
}

static void
test_xml_child_by_name_no_lang (void)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	char *text;

	/* Elements marked as being in a particular language are skipped */
	root = xml_parse ("<root><t xml:lang=\"de\">DE</t><t>plain</t><t xml:lang=\"fr\">FR</t></root>", &doc);
	text = node_text (go_xml_get_child_by_name_no_lang (root, "t"));
	g_assert_cmpstr (text, ==, "plain");
	g_free (text);
	xmlFreeDoc (doc);

	root = xml_parse ("<root><t>first</t><t xml:lang=\"de\">DE</t></root>", &doc);
	text = node_text (go_xml_get_child_by_name_no_lang (root, "t"));
	g_assert_cmpstr (text, ==, "first");
	g_free (text);
	xmlFreeDoc (doc);

	/* If all are marked there is none */
	root = xml_parse ("<root><t xml:lang=\"de\">DE</t><t xml:lang=\"fr\">FR</t></root>", &doc);
	g_assert_null (go_xml_get_child_by_name_no_lang (root, "t"));
	/* ... and none of that name is none */
	g_assert_null (go_xml_get_child_by_name_no_lang (root, "u"));
	xmlFreeDoc (doc);

	root = xml_parse ("<root><u/><t>x</t></root>", &doc);
	g_assert_nonnull (go_xml_get_child_by_name_no_lang (root, "u"));
	g_assert_null (go_xml_get_child_by_name_no_lang (root, "ut"));
	xmlFreeDoc (doc);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_xml_get_child_by_name_no_lang*");
	g_assert_null (go_xml_get_child_by_name_no_lang (NULL, "t"));
	g_test_assert_expected_messages ();
	root = xml_parse ("<root/>", &doc);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_xml_get_child_by_name_no_lang*");
	g_assert_null (go_xml_get_child_by_name_no_lang (root, NULL));
	g_test_assert_expected_messages ();
	xmlFreeDoc (doc);
}

static void
test_xml_child_by_name_by_lang (void)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	char *saved = g_strdup (g_getenv ("LANGUAGE"));
	char *text;

	root = xml_parse ("<root><u lang=\"fr\">FR</u><u lang=\"de\">DE</u><u>none</u><t>only</t></root>", &doc);

	/* The best language wins, in the order the user prefers them */
	g_setenv ("LANGUAGE", "fr", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "FR");
	g_free (text);
	g_setenv ("LANGUAGE", "de", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "DE");
	g_free (text);
	g_setenv ("LANGUAGE", "de:fr", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "DE");
	g_free (text);
	g_setenv ("LANGUAGE", "fr:de", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "FR");
	g_free (text);
	/* A region is a refinement of the language */
	g_setenv ("LANGUAGE", "fr_FR.UTF-8", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "FR");
	g_free (text);

	/* No language that matches: the one with no language is the default */
	g_setenv ("LANGUAGE", "ja", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "none");
	g_free (text);

	/* One element with no language marking */
	text = node_text (go_xml_get_child_by_name_by_lang (root, "t"));
	g_assert_cmpstr (text, ==, "only");
	g_free (text);

	/* No such element */
	g_assert_null (go_xml_get_child_by_name_by_lang (root, "v"));
	xmlFreeDoc (doc);

	/* Only marked elements and none of the right language: nothing */
	root = xml_parse ("<root><u lang=\"fr\">FR</u><u lang=\"de\">DE</u></root>", &doc);
	g_assert_null (go_xml_get_child_by_name_by_lang (root, "u"));
	g_setenv ("LANGUAGE", "de", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "DE");
	g_free (text);
	xmlFreeDoc (doc);

	/* xml:lang counts as lang */
	root = xml_parse ("<root><u xml:lang=\"fr\">FR</u><u xml:lang=\"de\">DE</u><u>none</u></root>", &doc);
	g_setenv ("LANGUAGE", "fr", TRUE);
	text = node_text (go_xml_get_child_by_name_by_lang (root, "u"));
	g_assert_cmpstr (text, ==, "FR");
	g_free (text);
	xmlFreeDoc (doc);

	if (saved)
		g_setenv ("LANGUAGE", saved, TRUE);
	else
		g_unsetenv ("LANGUAGE");
	g_free (saved);

	root = xml_parse ("<root/>", &doc);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_xml_get_child_by_name_by_lang*");
	g_assert_null (go_xml_get_child_by_name_by_lang (NULL, "t"));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_xml_get_child_by_name_by_lang*");
	g_assert_null (go_xml_get_child_by_name_by_lang (root, NULL));
	g_test_assert_expected_messages ();
	xmlFreeDoc (doc);
}

/* Run @fn on a fresh GsfXMLOut writing to memory, and return the document
   element of what it produced. */
typedef void (*XmlOutFn) (GsfXMLOut *xml, gconstpointer data);

static xmlNodePtr
xml_out_run (XmlOutFn fn, gconstpointer data, xmlDocPtr *doc)
{
	GsfOutput *mem = gsf_output_memory_new ();
	GsfXMLOut *xml = gsf_xml_out_new (mem);
	const guint8 *bytes;
	gsf_off_t size;

	gsf_xml_out_start_element (xml, "e");
	fn (xml, data);
	gsf_xml_out_end_element (xml);
	g_object_unref (xml);
	gsf_output_close (mem);

	bytes = gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (mem));
	size = gsf_output_size (mem);
	*doc = xmlReadMemory ((const char *)bytes, size, NULL, NULL, 0);
	g_object_unref (mem);
	g_assert_nonnull (*doc);
	return xmlDocGetRootElement (*doc);
}

static void
xml_out_double_fn (GsfXMLOut *xml, gconstpointer data)
{
	go_xml_out_add_double (xml, "v", *(const double *)data);
}

static void
xml_out_double_content_fn (GsfXMLOut *xml, gconstpointer data)
{
	go_xml_out_add_double (xml, NULL, *(const double *)data);
}

static void
test_xml_out_double (void)
{
	static const double values[] = {
		0, 1, -1, 0.5, 0.1, 0.2 + 0.1, 1.0 / 3, M_PI, -M_E, 1e15, 1e16, 123456789012345678.0,
		1e-5, 1e22, 1e23, 1e300, 1e-300, 1.7976931348623157e308, 2.2250738585072014e-308,
		4.9406564584124654e-324, 0.30000000000000004, 5e-324
	};
	GRand *rand = g_rand_new_with_seed (2024);
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (values) + 3000; i++) {
		xmlDocPtr doc;
		xmlNodePtr root;
		xmlChar *text;
		double d, back;

		if (i < G_N_ELEMENTS (values))
			d = values[i];
		else {
			/* Any finite number, random bits */
			guint64 bits = ((guint64)g_rand_int (rand) << 32) | g_rand_int (rand);
			memcpy (&d, &bits, sizeof (d));
			if (!isfinite (d))
				continue;
		}

		root = xml_out_run (xml_out_double_fn, &d, &doc);
		text = xmlGetProp (root, CC2XML ("v"));
		g_assert_nonnull (text);
		/* Read back it must be the same number, exactly */
		back = g_ascii_strtod (CXML2C (text), NULL);
		g_assert_cmpfloat (back, ==, d);
		xmlFree (text);
		xmlFreeDoc (doc);
	}
	g_rand_free (rand);

	/* Without an attribute name it is the content */
	{
		xmlDocPtr doc;
		double d = 0.25;
		xmlNodePtr root = xml_out_run (xml_out_double_content_fn, &d, &doc);
		char *text = node_text (root);
		g_assert_cmpfloat (g_ascii_strtod (text, NULL), ==, 0.25);
		g_free (text);
		xmlFreeDoc (doc);
	}
}

static void
xml_out_color_fn (GsfXMLOut *xml, gconstpointer data)
{
	go_xml_out_add_color (xml, "c", *(const GOColor *)data);
}

static void
test_xml_out_color (void)
{
	static const GOColor colors[] = {
		0, 0xffffffffu, GO_COLOR_BLACK, GO_COLOR_RED, 0x12345678u, 0x80000001u,
		GO_COLOR_FROM_RGBA (1, 2, 3, 4)
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (colors); i++) {
		xmlDocPtr doc;
		xmlNodePtr root = xml_out_run (xml_out_color_fn, &colors[i], &doc);
		xmlChar *text = xmlGetProp (root, CC2XML ("c"));
		GOColor back = 12345;

		g_assert_nonnull (text);
		/* It is a color in the usual textual form */
		g_assert_true (go_color_from_str (CXML2C (text), &back));
		g_assert_cmphex (back, ==, colors[i]);
		xmlFree (text);
		xmlFreeDoc (doc);
	}
}

#ifdef GOFFICE_WITH_LONG_DOUBLE
static void
xml_out_long_double_fn (GsfXMLOut *xml, gconstpointer data)
{
	go_xml_out_add_long_double (xml, "v", *(const long double *)data);
}

static void
test_xml_out_long_double (void)
{
	static const long double values[] = {
		0, 1, -1, 0.1L, 1.0L / 3, 3.14159265358979323846264338327950288L, 1e100L, 1e-100L,
		1e300L, 1e-300L, 1e4000L, 1e-4000L
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (values); i++) {
		xmlDocPtr doc;
		xmlNodePtr root = xml_out_run (xml_out_long_double_fn, &values[i], &doc);
		xmlChar *text = xmlGetProp (root, CC2XML ("v"));
		long double back;

		g_assert_nonnull (text);
		back = strtold (CXML2C (text), NULL);
		g_assert_true (back == values[i]);
		xmlFree (text);
		xmlFreeDoc (doc);
	}
}
#endif

static GsfXMLInNode const xml_in_test_dtd[] = {
	GSF_XML_IN_NODE (START, START, -1, NULL, GSF_XML_NO_CONTENT, NULL, NULL),
	GSF_XML_IN_NODE_END
};

static void
test_xml_in_doc_dispose_on_exit (void)
{
	GsfXMLInDoc *doc1 = gsf_xml_in_doc_new (xml_in_test_dtd, NULL);
	GsfXMLInDoc *doc2 = gsf_xml_in_doc_new (xml_in_test_dtd, NULL);
	GsfXMLInDoc *other = gsf_xml_in_doc_new (xml_in_test_dtd, NULL);

	g_assert_nonnull (doc1);
	g_assert_nonnull (doc2);

	go_xml_in_doc_dispose_on_exit (&doc1);
	go_xml_in_doc_dispose_on_exit (&doc2);

	/* Registering does nothing by itself */
	g_assert_nonnull (doc1);
	g_assert_nonnull (doc2);

	/* At shutdown the registered ones are freed and their pointers cleared;
	   others are not touched */
	_go_libxml_extras_shutdown ();
	g_assert_null (doc1);
	g_assert_null (doc2);
	g_assert_nonnull (other);
	gsf_xml_in_doc_free (other);

	/* It can be done again, and with nothing registered it does nothing */
	_go_libxml_extras_shutdown ();
	doc1 = gsf_xml_in_doc_new (xml_in_test_dtd, NULL);
	go_xml_in_doc_dispose_on_exit (&doc1);
	_go_libxml_extras_shutdown ();
	g_assert_null (doc1);
	_go_libxml_extras_shutdown ();
}

static void
register_libxml_extras_tests (void)
{
	g_test_add_func ("/utils/go-libxml-extras/parse_file", test_xml_parse_file);
	g_test_add_func ("/utils/go-libxml-extras/cstr", test_xml_cstr);
	g_test_add_func ("/utils/go-libxml-extras/set_cstr", test_xml_set_cstr);
	g_test_add_func ("/utils/go-libxml-extras/bool", test_xml_bool);
	g_test_add_func ("/utils/go-libxml-extras/int", test_xml_int);
	g_test_add_func ("/utils/go-libxml-extras/double", test_xml_double);
	g_test_add_func ("/utils/go-libxml-extras/double/roundtrip", test_xml_double_roundtrip);
	g_test_add_func ("/utils/go-libxml-extras/gocolor", test_xml_gocolor);
	g_test_add_func ("/utils/go-libxml-extras/enum", test_xml_enum);
	g_test_add_func ("/utils/go-libxml-extras/enum/fresh_type", test_xml_enum_fresh_type);
	g_test_add_func ("/utils/go-libxml-extras/child_by_name", test_xml_child_by_name);
	g_test_add_func ("/utils/go-libxml-extras/child_by_name/no_lang", test_xml_child_by_name_no_lang);
	g_test_add_func ("/utils/go-libxml-extras/child_by_name/by_lang", test_xml_child_by_name_by_lang);
	g_test_add_func ("/utils/go-libxml-extras/out/double", test_xml_out_double);
	g_test_add_func ("/utils/go-libxml-extras/out/color", test_xml_out_color);
#ifdef GOFFICE_WITH_LONG_DOUBLE
	g_test_add_func ("/utils/go-libxml-extras/out/long_double", test_xml_out_long_double);
#endif
	g_test_add_func ("/utils/go-libxml-extras/in_doc_dispose_on_exit", test_xml_in_doc_dispose_on_exit);
}

/* ------------------------------------------------------------------ */
/* go-image.c                                                          */
/* ------------------------------------------------------------------ */

static void
test_image_format_info (void)
{
	int i, j;

	for (i = 0; i < GO_IMAGE_FORMAT_UNKNOWN; i++) {
		const GOImageFormatInfo *info = go_image_get_format_info ((GOImageFormat)i);

		g_assert_nonnull (info);
		g_assert_cmpint (info->format, ==, i);
		g_assert_nonnull (info->name);
		g_assert_cmpuint (strlen (info->name), >, 0);
		g_assert_nonnull (info->desc);
		g_assert_cmpuint (strlen (info->desc), >, 0);
		g_assert_nonnull (info->ext);
		g_assert_cmpuint (strlen (info->ext), >, 0);

		/* Names are distinct and can be turned back into the format */
		g_assert_cmpint (go_image_get_format_from_name (info->name), ==, i);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (info->name, !=, go_image_get_format_info ((GOImageFormat)j)->name);
	}

	/* Facts about the common formats */
	g_assert_true (go_image_get_format_info (GO_IMAGE_FORMAT_PNG)->has_pixbuf_saver);
	g_assert_true (go_image_get_format_info (GO_IMAGE_FORMAT_JPG)->has_pixbuf_saver);
	g_assert_false (go_image_get_format_info (GO_IMAGE_FORMAT_SVG)->has_pixbuf_saver);
	g_assert_false (go_image_get_format_info (GO_IMAGE_FORMAT_PDF)->has_pixbuf_saver);
	g_assert_true (go_image_get_format_info (GO_IMAGE_FORMAT_PNG)->alpha_support);
	g_assert_false (go_image_get_format_info (GO_IMAGE_FORMAT_JPG)->alpha_support);

	/* There is no information for an unknown format */
	g_assert_null (go_image_get_format_info (GO_IMAGE_FORMAT_UNKNOWN));

	/* The formats gdk-pixbuf knows come after the built-in ones */
	{
		GSList *fmts = gdk_pixbuf_get_formats (), *l;
		int k = 1;

		for (l = fmts; l; l = l->next, k++) {
			GdkPixbufFormat *pf = l->data;
			const GOImageFormatInfo *info =
				go_image_get_format_info ((GOImageFormat)(GO_IMAGE_FORMAT_UNKNOWN + k));
			char *name = gdk_pixbuf_format_get_name (pf);

			g_assert_nonnull (info);
			g_assert_cmpint (info->format, ==, GO_IMAGE_FORMAT_UNKNOWN + k);
			g_assert_cmpstr (info->name, ==, name);
			g_assert_nonnull (info->desc);
			g_assert_nonnull (info->ext);
			g_assert_cmpint (info->has_pixbuf_saver, ==, gdk_pixbuf_format_is_writable (pf));
			/* The name leads to a format with that name */
			{
				GOImageFormat f = go_image_get_format_from_name (name);
				g_assert_cmpint (f, !=, GO_IMAGE_FORMAT_UNKNOWN);
				g_assert_cmpstr (go_image_get_format_info (f)->name, ==, name);
			}
			g_free (name);
		}
		/* One past the last is an error */
		g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_format_info*");
		g_assert_null (go_image_get_format_info ((GOImageFormat)(GO_IMAGE_FORMAT_UNKNOWN + k)));
		g_test_assert_expected_messages ();
		g_slist_free (fmts);
	}
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_format_info*");
	g_assert_null (go_image_get_format_info ((GOImageFormat)100000));
	g_test_assert_expected_messages ();

	/* A boxed type for the information; there is nothing to copy */
	{
		GType t = go_image_format_info_get_type ();
		const GOImageFormatInfo *info = go_image_get_format_info (GO_IMAGE_FORMAT_PNG);
		g_assert_true (G_TYPE_IS_BOXED (t));
		g_assert_true (g_boxed_copy (t, info) == info);
		g_boxed_free (t, (gpointer)info);
		g_assert_cmpstr (go_image_get_format_info (GO_IMAGE_FORMAT_PNG)->name, !=, NULL);
	}
}

static void
test_image_format_from_name (void)
{
	g_assert_cmpint (go_image_get_format_from_name (NULL), ==, GO_IMAGE_FORMAT_UNKNOWN);
	g_assert_cmpint (go_image_get_format_from_name ("unknown"), ==, GO_IMAGE_FORMAT_UNKNOWN);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Unknown format name (bogus)*");
	g_assert_cmpint (go_image_get_format_from_name ("bogus"), ==, GO_IMAGE_FORMAT_UNKNOWN);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Unknown format name*");
	g_assert_cmpint (go_image_get_format_from_name (""), ==, GO_IMAGE_FORMAT_UNKNOWN);
	g_test_assert_expected_messages ();
}

static void
test_image_formats_with_pixbuf_saver (void)
{
	GSList *list = go_image_get_formats_with_pixbuf_saver (), *l;
	int i, n = 0;

	/* Exactly the built-in formats that say they can be saved */
	for (l = list; l; l = l->next) {
		GOImageFormat f = (GOImageFormat)GPOINTER_TO_UINT (l->data);
		g_assert_cmpint (f, <, GO_IMAGE_FORMAT_UNKNOWN);
		g_assert_true (go_image_get_format_info (f)->has_pixbuf_saver);
		n++;
	}
	for (i = 0; i < GO_IMAGE_FORMAT_UNKNOWN; i++) {
		gboolean in_list = g_slist_find (list, GUINT_TO_POINTER (i)) != NULL;
		g_assert_cmpint (in_list, ==, go_image_get_format_info ((GOImageFormat)i)->has_pixbuf_saver);
	}
	g_assert_cmpint (n, >, 0);
	g_slist_free (list);
}

static void
test_image_mime (void)
{
	static const char *const names[] = { "svg", "wmf", "emf", "pdf", "ps", "eps", "png", "jpeg" };
	unsigned i;

	/* Every format has mime types (a comma separated list), each of which
	   leads back to the format */
	for (i = 0; i < G_N_ELEMENTS (names); i++) {
		char *mimes = go_image_format_to_mime (names[i]);
		char **parts;
		int k;

		g_assert_nonnull (mimes);
		parts = g_strsplit (mimes, ",", -1);
		g_assert_cmpuint (g_strv_length (parts), >, 0);
		for (k = 0; parts[k]; k++) {
			char *back;
			g_assert_cmpuint (strlen (parts[k]), >, 0);
			g_assert_nonnull (strchr (parts[k], '/'));
			back = go_mime_to_image_format (parts[k]);
			g_assert_cmpstr (back, ==, names[i]);
			g_free (back);
		}
		g_strfreev (parts);
		g_free (mimes);
	}

	/* A few well known ones */
	{
		static const struct { const char *mime, *name; } known[] = {
			{ "image/png", "png" }, { "image/jpeg", "jpeg" }, { "image/svg+xml", "svg" },
			{ "application/pdf", "pdf" }, { "application/postscript", "ps" }
		};
		for (i = 0; i < G_N_ELEMENTS (known); i++) {
			char *name = go_mime_to_image_format (known[i].mime);
			g_assert_cmpstr (name, ==, known[i].name);
			g_free (name);
		}
	}

	/* Unknown things */
	{
		char *s = go_mime_to_image_format ("application/x-no-such-thing");
		g_assert_null (s);
		s = go_mime_to_image_format ("");
		g_assert_null (s);
		g_assert_null (go_image_format_to_mime (NULL));
		g_assert_null (go_image_format_to_mime ("no-such-format"));
		g_assert_null (go_image_format_to_mime (""));
	}
}

static void
test_image_type_for_format (void)
{
	GType t;
	GOImage *image;

	g_assert_true (go_image_type_for_format ("png") == GO_TYPE_PIXBUF);
	g_assert_true (go_image_type_for_format ("jpeg") == GO_TYPE_PIXBUF);
	g_assert_true (go_image_type_for_format ("emf") == GO_TYPE_EMF);
	g_assert_true (go_image_type_for_format ("wmf") == GO_TYPE_EMF);
	g_assert_true (go_image_type_for_format ("eps") == GO_TYPE_SPECTRE);

	/* Support for SVG depends on how goffice was built */
	t = go_image_type_for_format ("svg");
	g_assert_true (t == 0 || g_type_is_a (t, GO_TYPE_IMAGE));

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Unknown format name*");
	g_assert_cmpuint (go_image_type_for_format ("no-such-format"), ==, 0);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_type_for_format*");
	g_assert_cmpuint (go_image_type_for_format (NULL), ==, 0);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_type_for_format*");
	g_assert_cmpuint (go_image_type_for_format (""), ==, 0);
	g_test_assert_expected_messages ();

	/* new_for_format makes an instance of that type */
	image = go_image_new_for_format ("png");
	g_assert_true (GO_IS_PIXBUF (image));
	g_object_unref (image);
	image = go_image_new_for_format ("emf");
	g_assert_true (GO_IS_EMF (image));
	g_object_unref (image);
	image = go_image_new_for_format ("eps");
	g_assert_true (GO_IS_SPECTRE (image));
	g_object_unref (image);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Unknown format name*");
	g_assert_null (go_image_new_for_format ("no-such-format"));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_type_for_format*");
	g_assert_null (go_image_new_for_format (NULL));
	g_test_assert_expected_messages ();
}

/* ---- a minimal image class for testing the base class ---- */

#define TEST_TYPE_IMAGE (test_image_get_type ())

typedef struct {
	GOImage base;
	int id;
} TestImage;

typedef struct {
	GOImageClass base;
} TestImageClass;

GType test_image_get_type (void);
G_DEFINE_TYPE (TestImage, test_image, GO_TYPE_IMAGE)

static struct {
	int get_pixbuf_calls;
	int scaled_calls;
	int scaled_w, scaled_h;
	int draw_calls;
	int load_data_calls;
	GString *attrs;
} test_image_log;

static GdkPixbuf *
test_image_get_pixbuf (GOImage *image)
{
	test_image_log.get_pixbuf_calls++;
	return gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
			       MAX (1, (int)image->width), MAX (1, (int)image->height));
}

static GdkPixbuf *
test_image_get_scaled_pixbuf (G_GNUC_UNUSED GOImage *image, int width, int height)
{
	test_image_log.scaled_calls++;
	test_image_log.scaled_w = width;
	test_image_log.scaled_h = height;
	if (width < 1 || height < 1)
		return NULL;
	return gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
}

static void
test_image_vfunc_save (G_GNUC_UNUSED GOImage *image, GsfXMLOut *output)
{
	gsf_xml_out_add_cstr (output, "extra", "x");
}

static void
test_image_vfunc_load_attr (G_GNUC_UNUSED GOImage *image, const xmlChar *name, const xmlChar *value)
{
	g_string_append_printf (test_image_log.attrs, "%s=%s;", CXML2C (name), CXML2C (value));
}

static void
test_image_vfunc_load_data (G_GNUC_UNUSED GOImage *image, G_GNUC_UNUSED GsfXMLIn *xin)
{
	test_image_log.load_data_calls++;
}

static void
test_image_vfunc_draw (G_GNUC_UNUSED GOImage *image, G_GNUC_UNUSED cairo_t *cr)
{
	test_image_log.draw_calls++;
}

static gboolean
test_image_vfunc_differ (GOImage *a, GOImage *b)
{
	return ((TestImage *)a)->id != ((TestImage *)b)->id;
}

static void
test_image_class_init (TestImageClass *klass)
{
	GOImageClass *ic = (GOImageClass *)klass;
	ic->get_pixbuf = test_image_get_pixbuf;
	ic->get_scaled_pixbuf = test_image_get_scaled_pixbuf;
	ic->save = test_image_vfunc_save;
	ic->load_attr = test_image_vfunc_load_attr;
	ic->load_data = test_image_vfunc_load_data;
	ic->draw = test_image_vfunc_draw;
	ic->differ = test_image_vfunc_differ;
}

static void
test_image_init (G_GNUC_UNUSED TestImage *t)
{
}

static void
test_image_reset_log (void)
{
	if (test_image_log.attrs)
		g_string_free (test_image_log.attrs, TRUE);
	memset (&test_image_log, 0, sizeof (test_image_log));
	test_image_log.attrs = g_string_new (NULL);
}

static TestImage *
test_image_new (int w, int h)
{
	return g_object_new (TEST_TYPE_IMAGE, "width", w, "height", h, NULL);
}

static void
test_image_properties (void)
{
	TestImage *t = test_image_new (30, 20);
	GOImage *image = GO_IMAGE (t);
	guint w = 0, h = 0;
	gsize len = 99;

	g_assert_true (GO_IS_IMAGE (image));
	assert_close (go_image_get_width (image), 30);
	assert_close (go_image_get_height (image), 20);
	g_object_get (t, "width", &w, "height", &h, NULL);
	g_assert_cmpuint (w, ==, 30);
	g_assert_cmpuint (h, ==, 20);

	/* No data, no name to begin with */
	g_assert_null (go_image_get_data (image, &len));
	g_assert_cmpuint (len, ==, 0);
	g_assert_null (go_image_get_data (image, NULL));
	g_assert_null (go_image_get_name (image));

	/* Changing the size throws away the data (which is derived from it) */
	image->data = g_malloc (10);
	memset (image->data, 7, 10);
	image->data_length = 10;
	g_assert_nonnull (go_image_get_data (image, &len));
	g_assert_cmpuint (len, ==, 10);
	g_object_set (t, "width", 30, "height", 20, NULL);	/* same size */
	g_assert_nonnull (go_image_get_data (image, NULL));
	g_object_set (t, "width", 31, NULL);
	g_assert_null (go_image_get_data (image, NULL));
	assert_close (go_image_get_width (image), 31);
	image->data = g_malloc (10);
	g_object_set (t, "height", 21, NULL);
	g_assert_null (go_image_get_data (image, NULL));
	assert_close (go_image_get_height (image), 21);

	/* The size is limited to what fits in 16 bits */
	g_test_expect_message ("GLib-GObject", G_LOG_LEVEL_CRITICAL, "*is invalid or out of range*");
	g_object_set (t, "width", G_MAXUINT16 + 1, NULL);
	g_test_assert_expected_messages ();
	assert_close (go_image_get_width (image), 31);
	g_object_set (t, "width", G_MAXUINT16, NULL);
	assert_close (go_image_get_width (image), G_MAXUINT16);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_width*");
	assert_close (go_image_get_width (NULL), 0);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_height*");
	assert_close (go_image_get_height (NULL), 0);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_data*");
	g_assert_null (go_image_get_data (NULL, NULL));
	g_test_assert_expected_messages ();

	g_object_unref (t);
}

static void
test_image_name (void)
{
	TestImage *t = test_image_new (1, 1);
	GOImage *image = GO_IMAGE (t);
	char *original = g_strdup ("first name");

	go_image_set_name (image, original);
	g_free (original);	/* the name is a copy */
	g_assert_cmpstr (go_image_get_name (image), ==, "first name");
	go_image_set_name (image, "second");
	g_assert_cmpstr (go_image_get_name (image), ==, "second");
	go_image_set_name (image, "");
	g_assert_cmpstr (go_image_get_name (image), ==, "");
	go_image_set_name (image, NULL);
	g_assert_null (go_image_get_name (image));
	/* The name can be set to what it is now */
	go_image_set_name (image, go_image_get_name (image));
	g_assert_null (go_image_get_name (image));
	go_image_set_name (image, "x");
	go_image_set_name (image, go_image_get_name (image));
	g_assert_cmpstr (go_image_get_name (image), ==, "x");

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_set_name*");
	go_image_set_name (NULL, "x");
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_name*");
	g_assert_null (go_image_get_name (NULL));
	g_test_assert_expected_messages ();

	g_object_unref (t);
}

static void
test_image_pixbuf (void)
{
	TestImage *t = test_image_new (40, 30);
	GOImage *image = GO_IMAGE (t);
	GdkPixbuf *p1, *p2;

	test_image_reset_log ();

	/* The pixbuf is made when first asked for, and kept */
	p1 = go_image_get_pixbuf (image);
	g_assert_nonnull (p1);
	g_assert_cmpint (gdk_pixbuf_get_width (p1), ==, 40);
	g_assert_cmpint (gdk_pixbuf_get_height (p1), ==, 30);
	g_assert_cmpint (test_image_log.get_pixbuf_calls, ==, 1);
	p2 = go_image_get_pixbuf (image);
	g_assert_true (p1 == p2);
	g_assert_cmpint (test_image_log.get_pixbuf_calls, ==, 1);

	/* Each caller gets a reference of its own */
	g_object_unref (p1);
	g_object_unref (p2);
	p1 = go_image_get_pixbuf (image);
	g_assert_cmpint (gdk_pixbuf_get_width (p1), ==, 40);
	g_object_unref (p1);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_pixbuf*");
	g_assert_null (go_image_get_pixbuf (NULL));
	g_test_assert_expected_messages ();

	g_object_unref (t);
}

static void
test_image_scaled_pixbuf (void)
{
	/* image width, height, requested width, height, resulting size */
	static const struct { int w, h, rw, rh, ew, eh; } cases[] = {
		/* Too big: scaled to fit, keeping the proportions */
		{ 200, 100, 64, 64, 64, 32 },
		{ 100, 200, 64, 64, 32, 64 },
		{ 200, 100, 100, 100, 100, 50 },
		{ 200, 100, 200, 50, 100, 50 },
		{ 300, 300, 100, 50, 50, 50 },
		{ 65, 10, 64, 64, 64, 9 },
		{ 10, 65, 64, 64, 9, 64 },
		/* One direction too big is enough */
		{ 100, 10, 64, 64, 64, 6 },
		{ 10, 100, 64, 64, 6, 64 },
		/* Very wide and very tall images keep at least a pixel each way */
		{ 1000, 10, 64, 64, 64, 1 },
		{ 10, 1000, 64, 64, 1, 64 },
		{ 5000, 1, 64, 64, 64, 1 },
		/* Not too big: left as is, never enlarged */
		{ 30, 20, 64, 64, 30, 20 },
		{ 64, 64, 64, 64, 64, 64 },
		{ 10, 10, 1000, 1000, 10, 10 }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		TestImage *t = test_image_new (cases[i].w, cases[i].h);
		GdkPixbuf *pb;
		gboolean scaled = cases[i].w > cases[i].rw || cases[i].h > cases[i].rh;

		test_image_reset_log ();
		pb = go_image_get_scaled_pixbuf (GO_IMAGE (t), cases[i].rw, cases[i].rh);
		g_assert_nonnull (pb);
		g_assert_cmpint (gdk_pixbuf_get_width (pb), ==, cases[i].ew);
		g_assert_cmpint (gdk_pixbuf_get_height (pb), ==, cases[i].eh);
		if (scaled) {
			g_assert_cmpint (test_image_log.scaled_calls, ==, 1);
			g_assert_cmpint (test_image_log.scaled_w, ==, cases[i].ew);
			g_assert_cmpint (test_image_log.scaled_h, ==, cases[i].eh);
		} else {
			g_assert_cmpint (test_image_log.scaled_calls, ==, 0);
		}
		g_object_unref (pb);
		g_object_unref (t);
	}

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_scaled_pixbuf*");
	g_assert_null (go_image_get_scaled_pixbuf (NULL, 10, 10));
	g_test_assert_expected_messages ();
}

static void
test_image_thumbnail (void)
{
	TestImage *t = test_image_new (200, 100);
	GOImage *image = GO_IMAGE (t);
	const GdkPixbuf *th;

	test_image_reset_log ();

	/* Fits in 64 x 64, with the proportions of the image */
	th = go_image_get_thumbnail (image);
	g_assert_nonnull (th);
	g_assert_cmpint (gdk_pixbuf_get_width ((GdkPixbuf *)th), ==, 64);
	g_assert_cmpint (gdk_pixbuf_get_height ((GdkPixbuf *)th), ==, 32);
	g_assert_cmpint (test_image_log.scaled_calls, ==, 1);

	/* It is made once */
	g_assert_true (go_image_get_thumbnail (image) == th);
	g_assert_cmpint (test_image_log.scaled_calls, ==, 1);

	/* A change of the image makes it stale */
	_go_image_changed (image, 20, 10);
	assert_close (go_image_get_width (image), 20);
	assert_close (go_image_get_height (image), 10);
	th = go_image_get_thumbnail (image);
	g_assert_nonnull (th);
	g_assert_cmpint (gdk_pixbuf_get_width ((GdkPixbuf *)th), ==, 20);
	g_assert_cmpint (gdk_pixbuf_get_height ((GdkPixbuf *)th), ==, 10);

	/* Even for extreme proportions there is a thumbnail */
	_go_image_changed (image, 1000, 10);
	th = go_image_get_thumbnail (image);
	g_assert_nonnull (th);
	g_assert_cmpint (gdk_pixbuf_get_width ((GdkPixbuf *)th), ==, 64);
	g_assert_cmpint (gdk_pixbuf_get_height ((GdkPixbuf *)th), ==, 1);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_get_thumbnail*");
	g_assert_null (go_image_get_thumbnail (NULL));
	g_test_assert_expected_messages ();

	g_object_unref (t);
}

static void
test_image_differ (void)
{
	TestImage *a = test_image_new (10, 10), *b = test_image_new (10, 10);
	TestImage *c = test_image_new (11, 10), *d = test_image_new (10, 11);
	GdkPixbuf *pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 10, 10);
	GOImage *other = go_pixbuf_new_from_pixbuf (pb);

	a->id = b->id = 1;
	c->id = d->id = 1;
	g_assert_false (go_image_differ (GO_IMAGE (a), GO_IMAGE (a)));
	g_assert_false (go_image_differ (GO_IMAGE (a), GO_IMAGE (b)));

	/* Decided by the class if the size is the same */
	b->id = 2;
	g_assert_true (go_image_differ (GO_IMAGE (a), GO_IMAGE (b)));
	g_assert_true (go_image_differ (GO_IMAGE (b), GO_IMAGE (a)));
	b->id = 1;

	/* Different size, different image, whatever the class says */
	g_assert_true (go_image_differ (GO_IMAGE (a), GO_IMAGE (c)));
	g_assert_true (go_image_differ (GO_IMAGE (a), GO_IMAGE (d)));

	/* Different kinds of images always differ, even with the same size */
	g_assert_true (go_image_differ (GO_IMAGE (a), other));
	g_assert_true (go_image_differ (other, GO_IMAGE (a)));

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_differ*");
	g_assert_false (go_image_differ (NULL, GO_IMAGE (a)));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_differ*");
	g_assert_false (go_image_differ (GO_IMAGE (a), NULL));
	g_test_assert_expected_messages ();

	g_object_unref (other);
	g_object_unref (pb);
	g_object_unref (a);
	g_object_unref (b);
	g_object_unref (c);
	g_object_unref (d);
}

static void
test_image_draw (void)
{
	TestImage *t = test_image_new (10, 10);
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cairo_t *cr = cairo_create (surf);

	test_image_reset_log ();
	go_image_draw (GO_IMAGE (t), cr);
	g_assert_cmpint (test_image_log.draw_calls, ==, 1);
	go_image_draw (GO_IMAGE (t), cr);
	g_assert_cmpint (test_image_log.draw_calls, ==, 2);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_draw*");
	go_image_draw (NULL, cr);
	g_test_assert_expected_messages ();
	g_assert_cmpint (test_image_log.draw_calls, ==, 2);

	cairo_destroy (cr);
	cairo_surface_destroy (surf);
	g_object_unref (t);
}

static void
test_image_save_load (void)
{
	TestImage *t = test_image_new (12, 34);
	GOImage *image = GO_IMAGE (t);
	GsfOutput *mem = gsf_output_memory_new ();
	GsfXMLOut *xml = gsf_xml_out_new (mem);
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlChar *s;
	int v;

	/* Only images with a name are saved */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_save*");
	go_image_save (image, xml);
	g_test_assert_expected_messages ();

	go_image_set_name (image, "my image");
	go_image_save (image, xml);
	g_object_unref (xml);
	gsf_output_close (mem);
	doc = xmlReadMemory ((const char *)gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (mem)),
			     gsf_output_size (mem), NULL, NULL, 0);
	g_object_unref (mem);
	g_assert_nonnull (doc);
	root = xmlDocGetRootElement (doc);
	g_assert_cmpstr (CXML2C (root->name), ==, "GOImage");
	s = go_xml_node_get_cstr (root, "name");
	g_assert_cmpstr (CXML2C (s), ==, "my image");
	xmlFree (s);
	s = go_xml_node_get_cstr (root, "type");
	g_assert_cmpstr (CXML2C (s), ==, "TestImage");
	xmlFree (s);
	g_assert_true (go_xml_node_get_int (root, "width", &v));
	g_assert_cmpint (v, ==, 12);
	g_assert_true (go_xml_node_get_int (root, "height", &v));
	g_assert_cmpint (v, ==, 34);
	/* ... and what the class adds */
	s = go_xml_node_get_cstr (root, "extra");
	g_assert_cmpstr (CXML2C (s), ==, "x");
	xmlFree (s);
	xmlFreeDoc (doc);
	g_object_unref (t);

	/* Loading attributes: the size is taken care of by the base class,
	   everything else is passed on to the class */
	{
		TestImage *u = test_image_new (1, 1);
		const xmlChar *attrs[] = {
			CC2XML ("width"), CC2XML ("120"), CC2XML ("name"), CC2XML ("ignored here"),
			CC2XML ("height"), CC2XML ("45"), CC2XML ("custom"), CC2XML ("value"), NULL
		};

		test_image_reset_log ();
		go_image_load_attrs (GO_IMAGE (u), NULL, attrs);
		assert_close (go_image_get_width (GO_IMAGE (u)), 120);
		assert_close (go_image_get_height (GO_IMAGE (u)), 45);
		g_assert_cmpstr (test_image_log.attrs->str, ==, "name=ignored here;custom=value;");

		/* No attributes at all is fine */
		test_image_reset_log ();
		go_image_load_attrs (GO_IMAGE (u), NULL, NULL);
		g_assert_cmpstr (test_image_log.attrs->str, ==, "");
		{
			const xmlChar *none[] = { NULL };
			go_image_load_attrs (GO_IMAGE (u), NULL, none);
			g_assert_cmpstr (test_image_log.attrs->str, ==, "");
		}
		assert_close (go_image_get_width (GO_IMAGE (u)), 120);

		/* The data is loaded by the class */
		go_image_load_data (GO_IMAGE (u), NULL);
		g_assert_cmpint (test_image_log.load_data_calls, ==, 1);

		g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_image_load_attrs*");
		go_image_load_attrs (NULL, NULL, attrs);
		g_test_assert_expected_messages ();
		g_object_unref (u);
	}
}

static void
test_image_dpi (void)
{
	double x, y, ox, oy;

	go_image_get_default_dpi (&ox, &oy);
	g_assert_cmpfloat (ox, >, 0);
	g_assert_cmpfloat (oy, >, 0);

	go_image_set_default_dpi (300, 150);
	go_image_get_default_dpi (&x, &y);
	assert_close (x, 300);
	assert_close (y, 150);

	go_image_set_default_dpi (72.5, 72.5);
	go_image_get_default_dpi (&x, &y);
	assert_close (x, 72.5);
	assert_close (y, 72.5);

	go_image_set_default_dpi (ox, oy);
	go_image_get_default_dpi (&x, &y);
	assert_close (x, ox);
	assert_close (y, oy);
}

static void
test_image_get_info (void)
{
	TestImage *t = test_image_new (1, 1);
	GdkPixbuf *pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 4, 4);
	guint8 *png;
	gsize png_len;
	GOImage *image;
	const GOImageFormatInfo *info;

	/* The test class is none of the kinds it knows */
	g_assert_null (go_image_get_info (GO_IMAGE (t)));
	g_object_unref (t);

	g_assert_true (gdk_pixbuf_save_to_buffer (pb, (char **)&png, &png_len, "png", NULL, NULL));
	image = go_image_new_from_data ("png", png, png_len, NULL, NULL);
	g_assert_true (GO_IS_PIXBUF (image));
	info = go_image_get_info (image);
	g_assert_nonnull (info);
	g_assert_cmpint (info->format, ==, GO_IMAGE_FORMAT_PNG);
	g_object_unref (image);
	g_free (png);

	image = go_image_new_for_format ("emf");
	info = go_image_get_info (image);
	g_assert_nonnull (info);
	g_assert_cmpint (info->format, ==, GO_IMAGE_FORMAT_EMF);
	g_object_unref (image);
	image = go_image_new_for_format ("eps");
	info = go_image_get_info (image);
	g_assert_nonnull (info);
	g_assert_cmpint (info->format, ==, GO_IMAGE_FORMAT_EPS);
	g_object_unref (image);

	g_object_unref (pb);
}

static guint8 *
make_image_data (const char *type, int w, int h, gsize *len)
{
	GdkPixbuf *pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, w, h);
	guint8 *data;

	gdk_pixbuf_fill (pb, 0x336699ff);
	g_assert_true (gdk_pixbuf_save_to_buffer (pb, (char **)&data, len, type, NULL, NULL));
	g_object_unref (pb);
	return data;
}

static void
test_image_new_from_data (void)
{
	static const char *const types[] = { "png", "jpeg" };
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (types); i++) {
		gsize len;
		guint8 *data = make_image_data (types[i], 7, 5, &len);
		GOImage *image;
		char *format = NULL;
		GError *err = NULL;
		gsize got_len = 0;
		const guint8 *got;
		const GOImageFormatInfo *info;

		/* With the type given */
		image = go_image_new_from_data (types[i], data, len, &format, &err);
		g_assert_no_error (err);
		g_assert_true (GO_IS_PIXBUF (image));
		g_assert_cmpstr (format, ==, types[i]);
		assert_close (go_image_get_width (image), 7);
		assert_close (go_image_get_height (image), 5);
		got = go_image_get_data (image, &got_len);
		g_assert_cmpuint (got_len, ==, len);
		g_assert_cmpint (memcmp (got, data, len), ==, 0);
		info = go_image_get_info (image);
		g_assert_nonnull (info);
		g_assert_cmpstr (info->name, ==, types[i]);
		g_free (format);
		g_object_unref (image);

		/* Without it, detected from the data; the format is optional */
		format = NULL;
		image = go_image_new_from_data (NULL, data, len, &format, &err);
		g_assert_no_error (err);
		g_assert_true (GO_IS_PIXBUF (image));
		g_assert_cmpstr (format, ==, types[i]);
		assert_close (go_image_get_width (image), 7);
		g_free (format);
		g_object_unref (image);

		format = NULL;
		image = go_image_new_from_data ("", data, len, &format, &err);
		g_assert_true (GO_IS_PIXBUF (image));
		g_assert_cmpstr (format, ==, types[i]);
		g_free (format);
		g_object_unref (image);

		image = go_image_new_from_data (types[i], data, len, NULL, NULL);
		g_assert_true (GO_IS_PIXBUF (image));
		g_object_unref (image);

		/* The pixels are those of the picture */
		{
			GdkPixbuf *pb;
			const guint8 *px;
			image = go_image_new_from_data (types[i], data, len, NULL, NULL);
			pb = go_image_get_pixbuf (image);
			g_assert_cmpint (gdk_pixbuf_get_width (pb), ==, 7);
			g_assert_cmpint (gdk_pixbuf_get_height (pb), ==, 5);
			px = gdk_pixbuf_get_pixels (pb);
			/* (JPEG is lossy) */
			g_assert_cmpint (abs (px[0] - 0x33), <=, 6);
			g_assert_cmpint (abs (px[1] - 0x66), <=, 6);
			g_assert_cmpint (abs (px[2] - 0x99), <=, 6);
			g_object_unref (pb);
			g_object_unref (image);
		}

		g_free (data);
	}

	/* Data that is not an image of any known kind */
	{
		static const guint8 junk[] = "this is not a picture";
		char *format = NULL;
		GOImage *image;

		g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*unrecognized image format*");
		image = go_image_new_from_data (NULL, junk, sizeof (junk), &format, NULL);
		g_test_assert_expected_messages ();
		g_assert_null (image);
		g_assert_null (format);
	}

	/* Data that doesn't match the type it was said to have is an error,
	   which is reported */
	{
		static const guint8 junk[] = "this is not a PNG";
		GError *err = NULL;
		GOImage *image = go_image_new_from_data ("png", junk, sizeof (junk), NULL, &err);

		g_assert_nonnull (err);
		g_clear_error (&err);
		if (image)
			g_object_unref (image);
	}
}

static void
test_image_new_from_file (void)
{
	gsize len;
	guint8 *png = make_image_data ("png", 9, 6, &len);
	char *dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	char *good, *missing, *text;
	GOImage *image;
	GError *err = NULL;

	g_assert_nonnull (dir);
	good = g_build_filename (dir, "picture.png", NULL);
	missing = g_build_filename (dir, "missing.png", NULL);
	text = g_build_filename (dir, "notes.txt", NULL);
	g_assert_true (g_file_set_contents (good, (char *)png, len, NULL));
	g_assert_true (g_file_set_contents (text, "just some text\n", -1, NULL));

	image = go_image_new_from_file (good, &err);
	g_assert_no_error (err);
	g_assert_true (GO_IS_PIXBUF (image));
	assert_close (go_image_get_width (image), 9);
	assert_close (go_image_get_height (image), 6);
	g_object_unref (image);

	/* The error return is optional */
	image = go_image_new_from_file (good, NULL);
	g_assert_true (GO_IS_PIXBUF (image));
	g_object_unref (image);

	/* Nothing there, not a picture, no file name */
	g_assert_null (go_image_new_from_file (missing, NULL));
	g_assert_null (go_image_new_from_file (text, NULL));
	g_assert_null (go_image_new_from_file (NULL, NULL));
	g_assert_null (go_image_new_from_file (dir, NULL));

	g_remove (good);
	g_remove (text);
	g_remove (dir);
	g_free (good);
	g_free (missing);
	g_free (text);
	g_free (dir);
	g_free (png);
}

static void
register_image_tests (void)
{
	g_test_add_func ("/utils/go-image/format_info", test_image_format_info);
	g_test_add_func ("/utils/go-image/format_from_name", test_image_format_from_name);
	g_test_add_func ("/utils/go-image/formats_with_pixbuf_saver", test_image_formats_with_pixbuf_saver);
	g_test_add_func ("/utils/go-image/mime", test_image_mime);
	g_test_add_func ("/utils/go-image/type_for_format", test_image_type_for_format);
	g_test_add_func ("/utils/go-image/properties", test_image_properties);
	g_test_add_func ("/utils/go-image/name", test_image_name);
	g_test_add_func ("/utils/go-image/pixbuf", test_image_pixbuf);
	g_test_add_func ("/utils/go-image/scaled_pixbuf", test_image_scaled_pixbuf);
	g_test_add_func ("/utils/go-image/thumbnail", test_image_thumbnail);
	g_test_add_func ("/utils/go-image/differ", test_image_differ);
	g_test_add_func ("/utils/go-image/draw", test_image_draw);
	g_test_add_func ("/utils/go-image/save_load", test_image_save_load);
	g_test_add_func ("/utils/go-image/dpi", test_image_dpi);
	g_test_add_func ("/utils/go-image/get_info", test_image_get_info);
	g_test_add_func ("/utils/go-image/new_from_data", test_image_new_from_data);
	g_test_add_func ("/utils/go-image/new_from_file", test_image_new_from_file);
}

/* ------------------------------------------------------------------ */
/* go-font.c                                                           */
/* ------------------------------------------------------------------ */

static void
test_font_basic (void)
{
	const GOFont *f1, *f2, *f3, *f4;
	char *s;

	/* The same description gives the very same font */
	f1 = go_font_new_by_name ("Sans Bold 11");
	f2 = go_font_new_by_name ("Sans Bold 11");
	g_assert_nonnull (f1);
	g_assert_true (f1 == f2);
	g_assert_true (go_font_eq (f1, f2));

	/* A different one gives a different font */
	f3 = go_font_new_by_name ("Sans Bold 12");
	f4 = go_font_new_by_name ("Sans Italic 11");
	g_assert_true (f3 != f1);
	g_assert_true (f4 != f1);
	g_assert_true (f3 != f4);
	g_assert_false (go_font_eq (f1, f3));
	g_assert_false (go_font_eq (f1, f4));
	g_assert_false (go_font_eq (f3, f4));

	/* Fonts are described by pango font descriptions */
	g_assert_nonnull (f1->desc);
	g_assert_cmpint (pango_font_description_get_size (f1->desc), ==, 11 * PANGO_SCALE);
	g_assert_cmpint (pango_font_description_get_weight (f1->desc), ==, PANGO_WEIGHT_BOLD);
	g_assert_cmpint (pango_font_description_get_style (f4->desc), ==, PANGO_STYLE_ITALIC);

	/* By description, which is consumed */
	{
		PangoFontDescription *d = pango_font_description_from_string ("Sans Bold 11");
		const GOFont *f5 = go_font_new_by_desc (d);
		const GOFont *f6;

		g_assert_true (f5 == f1);
		go_font_unref (f5);

		d = pango_font_description_from_string ("Serif 33");
		f5 = go_font_new_by_desc (d);
		g_assert_nonnull (f5);
		g_assert_true (f5->desc == d);
		f6 = go_font_new_by_name ("Serif 33");
		g_assert_true (f6 == f5);
		go_font_unref (f6);
		go_font_unref (f5);
	}

	/* Each lookup has its own reference to release */
	go_font_unref (f1);
	go_font_unref (f2);
	f1 = go_font_new_by_name ("Sans Bold 11");
	s = go_font_as_str (f1);
	g_assert_nonnull (strstr (s, "Bold"));
	g_assert_nonnull (strstr (s, "11"));
	g_free (s);
	go_font_unref (f1);
	go_font_unref (f3);
	go_font_unref (f4);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_font_ref*");
	g_assert_null (go_font_ref (NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_font_unref*");
	go_font_unref (NULL);
	g_test_assert_expected_messages ();
}

static void
test_font_as_str (void)
{
	static const char *const names[] = {
		"Sans 8", "Sans 12", "Sans Bold 10", "Serif Italic 14", "Monospace Bold Italic 9",
		"Sans Condensed 11", "Sans Light 13", "DejaVu Sans Mono 10.5", "Sans Bold Italic 72"
	};
	unsigned i;
	char *s;

	for (i = 0; i < G_N_ELEMENTS (names); i++) {
		const GOFont *f = go_font_new_by_name (names[i]);
		const GOFont *g;

		/* The string can be turned back into the same font */
		s = go_font_as_str (f);
		g_assert_nonnull (s);
		g_assert_cmpuint (strlen (s), >, 0);
		g = go_font_new_by_name (s);
		g_assert_true (g == f);
		go_font_unref (g);
		go_font_unref (f);
		g_free (s);
	}

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_font_as_str*");
	s = go_font_as_str (NULL);
	g_test_assert_expected_messages ();
	g_assert_cmpstr (s, ==, "");
	g_free (s);
}

static void
test_font_index (void)
{
	const GOFont *f = go_font_new_by_name ("Sans Bold 17");
	const GOFont *g;
	const GOFont *def = go_font_new_by_index (0);
	char *s;

	/* The first font is the default one */
	g_assert_nonnull (def);
	s = go_font_as_str (def);
	g_assert_cmpstr (s, ==, "Sans 8");
	g_free (s);
	{
		const GOFont *by_name = go_font_new_by_name ("Sans 8");
		g_assert_true (by_name == def);
		go_font_unref (by_name);
	}
	go_font_unref (def);

	/* Fonts can be found by their index */
	g_assert_cmpint (f->font_index, >, 0);
	g = go_font_new_by_index (f->font_index);
	g_assert_true (g == f);
	go_font_unref (g);

	/* Indices are distinct for fonts alive at the same time */
	{
		const GOFont *other = go_font_new_by_name ("Sans Bold 18");
		g_assert_cmpint (other->font_index, !=, f->font_index);
		go_font_unref (other);
	}

	/* When a font is released its place becomes empty, and can be reused */
	{
		int idx;
		const GOFont *tmp = go_font_new_by_name ("Sans Italic 23");
		idx = tmp->font_index;
		go_font_unref (tmp);
		g_assert_null (go_font_new_by_index (idx));
		tmp = go_font_new_by_name ("Sans Italic 24");
		g_assert_cmpint (tmp->font_index, ==, idx);
		g_assert_true (go_font_new_by_index (idx) == tmp);
		go_font_unref (tmp);
		go_font_unref (tmp);
	}

	/* An index that has never been used */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_font_new_by_index*");
	g_assert_null (go_font_new_by_index (1000000));
	g_test_assert_expected_messages ();

	go_font_unref (f);
}

static void
test_font_boxed (void)
{
	GType t = go_font_get_type ();
	const GOFont *f = go_font_new_by_name ("Sans Bold 19");
	const GOFont *g;

	g_assert_true (G_TYPE_IS_BOXED (t));
	g = g_boxed_copy (t, f);
	g_assert_true (g == f);
	g_boxed_free (t, (gpointer)g);

	/* Still alive after the copy is gone */
	g_assert_true (go_font_new_by_index (f->font_index) == f);
	go_font_unref (f);
	go_font_unref (f);
}

typedef struct {
	int calls;
	const GOFont *font;
	gpointer user;
} FontWatch;

static void
font_watch_cb (gpointer font, gpointer data, gpointer user)
{
	FontWatch *w = user;
	w->calls++;
	w->font = font;
	w->user = data;
}

static void
test_font_cache_watch (void)
{
	FontWatch w = { 0, NULL, NULL };
	GClosure *closure = g_cclosure_new (G_CALLBACK (font_watch_cb), &w, NULL);
	const GOFont *f, *extra;

	g_closure_ref (closure);
	g_closure_sink (closure);
	go_font_cache_register (closure);

	/* Told when a font is no longer used by anyone but the cache */
	f = go_font_new_by_name ("Sans Bold 21");
	g_assert_cmpint (w.calls, ==, 0);
	go_font_unref (f);
	g_assert_cmpint (w.calls, ==, 1);
	g_assert_true (w.font == f);

	/* Not as long as someone holds on to it */
	w.calls = 0;
	f = go_font_new_by_name ("Sans Bold 22");
	extra = go_font_ref (f);
	go_font_unref (f);
	g_assert_cmpint (w.calls, ==, 0);
	go_font_unref (extra);
	g_assert_cmpint (w.calls, ==, 1);

	/* Lookups of existing fonts are not news */
	w.calls = 0;
	f = go_font_new_by_name ("Sans Bold 22");
	extra = go_font_new_by_name ("Sans Bold 22");
	go_font_unref (extra);
	g_assert_cmpint (w.calls, ==, 0);
	go_font_unref (f);
	g_assert_cmpint (w.calls, ==, 1);

	/* Unregistered watchers hear nothing */
	go_font_cache_unregister (closure);
	w.calls = 0;
	f = go_font_new_by_name ("Sans Bold 23");
	go_font_unref (f);
	g_assert_cmpint (w.calls, ==, 0);

	/* Unregistering what is not registered is harmless */
	go_font_cache_unregister (closure);
	g_closure_unref (closure);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_font_cache_register*");
	go_font_cache_register (NULL);
	g_test_assert_expected_messages ();
}

static void
test_font_sizes (void)
{
	GSList *sizes = go_fonts_list_sizes (), *l;
	guint prev = 0;
	int n = 0;

	for (l = sizes; l; l = l->next) {
		guint size = GPOINTER_TO_UINT (l->data);
		/* Positive, in whole points, and in order */
		g_assert_cmpuint (size, >, prev);
		g_assert_cmpuint (size % PANGO_SCALE, ==, 0);
		prev = size;
		n++;
	}
	g_assert_cmpint (n, >, 5);
	/* The sizes people use most are there */
	g_assert_nonnull (g_slist_find (sizes, GUINT_TO_POINTER (10 * PANGO_SCALE)));
	g_assert_nonnull (g_slist_find (sizes, GUINT_TO_POINTER (12 * PANGO_SCALE)));
	g_slist_free (sizes);
}

static PangoContext *
test_pango_context (void)
{
	return pango_font_map_create_context (pango_cairo_font_map_get_default ());
}

static void
test_font_families (void)
{
	PangoContext *ctx = test_pango_context ();
	GSList *families = go_fonts_list_families (ctx), *l;
	int n = 0;

	for (l = families; l; l = l->next) {
		const char *name = l->data;
		g_assert_nonnull (name);
		g_assert_true (g_utf8_validate (name, -1, NULL));
		g_assert_cmpuint (strlen (name), >, 0);
		/* Sorted */
		if (l->next)
			g_assert_cmpint (g_utf8_collate (name, l->next->data), <=, 0);
		n++;
	}

	/* The same ones that pango lists */
	{
		PangoFontFamily **fams;
		int count;
		pango_context_list_families (ctx, &fams, &count);
		g_assert_cmpint (n, ==, count);
		g_free (fams);
	}

	g_slist_free_full (families, g_free);
	g_object_unref (ctx);
}

static void
test_font_metrics (void)
{
	PangoContext *ctx = test_pango_context ();
	const GOFont *font = go_font_new_by_name ("Sans 12");
	const GOFont *big = go_font_new_by_name ("Sans 36");
	GOFontMetrics *m = go_font_metrics_new (ctx, font);
	GOFontMetrics *mb = go_font_metrics_new (ctx, big);
	int i;

	g_assert_nonnull (m);

	/* Digits have some width, at least a pixel and in order */
	for (i = 0; i <= 9; i++)
		g_assert_cmpint (m->digit_widths[i], >, 0);
	g_assert_cmpint (m->min_digit_width, >=, PANGO_SCALE);
	g_assert_cmpint (m->min_digit_width, <=, m->avg_digit_width);
	g_assert_cmpint (m->avg_digit_width, <=, m->max_digit_width);
	for (i = 0; i <= 9; i++) {
		g_assert_cmpint (m->digit_widths[i], >=, m->min_digit_width);
		g_assert_cmpint (m->digit_widths[i], <=, m->max_digit_width);
	}

	/* The other characters measured */
	g_assert_cmpint (m->hyphen_width, >, 0);
	g_assert_cmpint (m->minus_width, >, 0);
	g_assert_cmpint (m->plus_width, >, 0);
	g_assert_cmpint (m->E_width, >, 0);
	g_assert_cmpint (m->hash_width, >, 0);
	g_assert_cmpint (m->space_width, >, 0);

	/* The thin space, if there is one, is narrower than a space */
	if (m->thin_space) {
		g_assert_cmpint (m->thin_space_width, >, 0);
		g_assert_cmpint (m->thin_space_width, <, m->space_width);
		g_assert_true (g_unichar_isspace (m->thin_space));
	} else {
		g_assert_cmpint (m->thin_space_width, ==, 0);
	}

	/* A bigger font has bigger characters, roughly in proportion */
	g_assert_cmpint (mb->avg_digit_width, >, 2 * m->avg_digit_width);
	g_assert_cmpint (mb->avg_digit_width, <, 4 * m->avg_digit_width);
	g_assert_cmpint (mb->space_width, >, m->space_width);
	g_assert_cmpint (mb->hyphen_width, >, m->hyphen_width);

	/* The same font again measures the same */
	{
		GOFontMetrics *again = go_font_metrics_new (ctx, font);
		g_assert_cmpint (memcmp (again, m, sizeof (*m)), ==, 0);
		go_font_metrics_free (again);
	}

	go_font_metrics_free (m);
	go_font_metrics_free (mb);
	go_font_unref (font);
	go_font_unref (big);
	g_object_unref (ctx);
}

static void
test_font_metrics_boxed (void)
{
	PangoContext *ctx = test_pango_context ();
	const GOFont *font = go_font_new_by_name ("Sans 10");
	GOFontMetrics *m = go_font_metrics_new (ctx, font);
	GType t = go_font_metrics_get_type ();
	GOFontMetrics *copy;

	g_assert_true (G_TYPE_IS_BOXED (t));
	copy = g_boxed_copy (t, m);
	g_assert_true (copy != m);
	g_assert_cmpint (memcmp (copy, m, sizeof (*m)), ==, 0);
	/* Independent of the original */
	go_font_metrics_free (m);
	g_assert_cmpint (copy->digit_widths[3], >, 0);
	g_boxed_free (t, copy);

	go_font_unref (font);
	g_object_unref (ctx);
}

static void
test_font_metrics_unit (void)
{
	const GOFontMetrics *u = go_font_metrics_unit;
	int i;

	/* A made-up font in which everything is one unit wide */
	g_assert_nonnull (u);
	for (i = 0; i <= 9; i++)
		g_assert_cmpint (u->digit_widths[i], ==, 1);
	g_assert_cmpint (u->min_digit_width, ==, 1);
	g_assert_cmpint (u->max_digit_width, ==, 1);
	g_assert_cmpint (u->avg_digit_width, ==, 1);
	g_assert_cmpint (u->hyphen_width, ==, 1);
	g_assert_cmpint (u->minus_width, ==, 1);
	g_assert_cmpint (u->plus_width, ==, 1);
	g_assert_cmpint (u->E_width, ==, 1);
	g_assert_cmpint (u->hash_width, ==, 1);
	g_assert_cmpint (u->space_width, ==, 1);
	g_assert_cmpuint (u->thin_space, ==, 0);
}

static void
register_font_tests (void)
{
	g_test_add_func ("/utils/go-font/basic", test_font_basic);
	g_test_add_func ("/utils/go-font/as_str", test_font_as_str);
	g_test_add_func ("/utils/go-font/index", test_font_index);
	g_test_add_func ("/utils/go-font/boxed", test_font_boxed);
	g_test_add_func ("/utils/go-font/cache_watch", test_font_cache_watch);
	g_test_add_func ("/utils/go-font/sizes", test_font_sizes);
	g_test_add_func ("/utils/go-font/families", test_font_families);
	g_test_add_func ("/utils/go-font/metrics", test_font_metrics);
	g_test_add_func ("/utils/go-font/metrics_boxed", test_font_metrics_boxed);
	g_test_add_func ("/utils/go-font/metrics_unit", test_font_metrics_unit);
}

/* ------------------------------------------------------------------ */
/* go-locale.c                                                         */
/* ------------------------------------------------------------------ */

/* The locales used here have to be installed on the system; tests that
   need one that isn't there are skipped. */
static const char *const locale_de[] = { "de_DE.UTF-8", "de_DE.utf8", NULL };
static const char *const locale_en[] = { "en_US.UTF-8", "en_US.utf8", NULL };
static const char *const locale_ja[] = { "ja_JP.UTF-8", "ja_JP.utf8", NULL };
static const char *const locale_fr[] = { "fr_FR.UTF-8", "fr_FR.utf8", NULL };

/* Switch to the first available one; returns its name or NULL. */
static const char *
locale_enter (const char *const *names)
{
	int i;
	for (i = 0; names[i]; i++)
		if (go_setlocale (LC_ALL, names[i]))
			return names[i];
	return NULL;
}

static char *saved_locale;

static void
locale_save (void)
{
	saved_locale = g_strdup (setlocale (LC_ALL, NULL));
}

static void
locale_restore (void)
{
	go_setlocale (LC_ALL, saved_locale);
	g_free (saved_locale);
	saved_locale = NULL;
}

static void
test_locale_c (void)
{
	gboolean precedes = FALSE, space = TRUE;
	const GString *cur;

	locale_save ();
	g_assert_nonnull (go_setlocale (LC_ALL, "C"));

	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");
	g_assert_cmpstr (go_locale_get_thousand ()->str, ==, ",");
	cur = go_locale_get_currency (&precedes, &space);
	g_assert_cmpstr (cur->str, ==, "$");
	g_assert_true (precedes);
	g_assert_false (space);
	/* The two flags are optional */
	g_assert_true (go_locale_get_currency (NULL, NULL) == cur);
	g_assert_true (go_locale_get_currency (&precedes, NULL) == cur);
	g_assert_true (go_locale_get_currency (NULL, &space) == cur);

	g_assert_cmpint (go_locale_get_arg_sep (), ==, ',');
	g_assert_cmpint (go_locale_get_col_sep (), ==, ',');
	g_assert_cmpint (go_locale_get_row_sep (), ==, ';');

	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, "mm/dd/yy");
	g_assert_cmpstr (go_locale_get_time_format ()->str, ==, "hh:mm:ss");
	g_assert_cmpint (go_locale_month_before_day (), ==, 1);
	g_assert_true (go_locale_24h ());

	locale_restore ();
}

static void
test_locale_setlocale (void)
{
	const char *r;

	locale_save ();

	r = go_setlocale (LC_ALL, "C");
	g_assert_cmpstr (r, ==, "C");
	/* NULL asks for the current one */
	g_assert_cmpstr (go_setlocale (LC_ALL, NULL), ==, "C");
	g_assert_cmpstr (go_setlocale (LC_NUMERIC, NULL), ==, "C");

	/* The same as the C library's setlocale */
	g_assert_cmpstr (go_setlocale (LC_ALL, NULL), ==, setlocale (LC_ALL, NULL));

	/* A locale that doesn't exist is not set */
	g_assert_null (go_setlocale (LC_ALL, "no_SUCH.locale"));
	g_assert_cmpstr (go_setlocale (LC_ALL, NULL), ==, "C");
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");

	locale_restore ();
}

/* Things that must hold for every locale */
static void
check_locale_consistency (void)
{
	const GString *dec = go_locale_get_decimal ();
	const GString *th = go_locale_get_thousand ();
	const GString *df = go_locale_get_date_format ();
	const GString *tf = go_locale_get_time_format ();
	const char *p;
	int expected_order = -1;

	g_assert_true (g_utf8_validate (dec->str, -1, NULL));
	g_assert_cmpint (g_utf8_strlen (dec->str, -1), ==, 1);
	g_assert_true (g_utf8_validate (th->str, -1, NULL));
	g_assert_cmpint (g_utf8_strlen (th->str, -1), ==, 1);
	g_assert_cmpstr (th->str, !=, dec->str);
	g_assert_true (g_utf8_validate (go_locale_get_currency (NULL, NULL)->str, -1, NULL));
	g_assert_cmpuint (go_locale_get_currency (NULL, NULL)->len, >, 0);

	/* The separators of arguments and columns are chosen so as not to
	   clash with the decimal separator */
	g_assert_cmpint (go_locale_get_arg_sep (), ==, dec->str[0] == ',' ? ';' : ',');
	g_assert_cmpint (go_locale_get_col_sep (), ==, dec->str[0] == ',' ? '\\' : ',');
	g_assert_cmpint (go_locale_get_row_sep (), ==, ';');
	g_assert_cmpint (go_locale_get_arg_sep (), !=, dec->str[0]);

	g_assert_true (g_utf8_validate (df->str, -1, NULL));
	g_assert_cmpuint (df->len, >, 0);
	g_assert_nonnull (strstr (df->str, "y"));
	g_assert_nonnull (strstr (df->str, "m"));
	g_assert_nonnull (strstr (df->str, "d"));
	g_assert_true (g_utf8_validate (tf->str, -1, NULL));
	g_assert_cmpuint (tf->len, >, 0);
	g_assert_false (g_unichar_isspace (g_utf8_get_char (g_utf8_prev_char (tf->str + tf->len))));
	g_assert_nonnull (strstr (tf->str, "mm"));

	/* The order of day, month and year follows the date format */
	for (p = df->str; *p && expected_order < 0; p++) {
		if (*p == 'd')
			expected_order = 0;
		else if (*p == 'm')
			expected_order = 1;
		else if (*p == 'y')
			expected_order = 2;
	}
	g_assert_cmpint (go_locale_month_before_day (), ==, expected_order);

	/* The answers don't change from call to call */
	g_assert_true (go_locale_get_decimal () == dec);
	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, df->str);
}

static void
test_locale_consistency (void)
{
	const char *const *locales[] = { locale_de, locale_en, locale_ja, locale_fr };
	unsigned i;
	int tested = 0;

	locale_save ();
	g_assert_nonnull (go_setlocale (LC_ALL, "C"));
	check_locale_consistency ();
	for (i = 0; i < G_N_ELEMENTS (locales); i++) {
		if (locale_enter (locales[i])) {
			check_locale_consistency ();
			tested++;
		}
	}
	locale_restore ();
	if (tested == 0)
		g_test_skip ("no non-C locales installed");
}

static void
test_locale_specific (void)
{
	gboolean precedes, space;
	int tested = 0;

	locale_save ();

	if (locale_enter (locale_de)) {
		/* Decimal comma, dot for thousands, day first */
		g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ",");
		g_assert_cmpstr (go_locale_get_thousand ()->str, ==, ".");
		g_assert_cmpint (go_locale_get_arg_sep (), ==, ';');
		g_assert_cmpint (go_locale_get_col_sep (), ==, '\\');
		g_assert_cmpint (go_locale_month_before_day (), ==, 0);
		g_assert_true (go_locale_24h ());
		g_assert_cmpstr (go_locale_get_currency (&precedes, &space)->str, ==, "\xe2\x82\xac");
		g_assert_false (precedes);
		tested++;
	}
	if (locale_enter (locale_en)) {
		g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");
		g_assert_cmpstr (go_locale_get_thousand ()->str, ==, ",");
		g_assert_cmpint (go_locale_get_arg_sep (), ==, ',');
		g_assert_cmpint (go_locale_month_before_day (), ==, 1);
		g_assert_false (go_locale_24h ());
		g_assert_cmpstr (go_locale_get_currency (&precedes, &space)->str, ==, "$");
		g_assert_true (precedes);
		g_assert_nonnull (strstr (go_locale_get_time_format ()->str, "AM/PM"));
		tested++;
	}
	if (locale_enter (locale_ja)) {
		/* Year first, which is neither of the others */
		g_assert_cmpint (go_locale_month_before_day (), ==, 2);
		g_assert_true (go_locale_24h ());
		/* The date format has the year before the month before the day,
		   and keeps the characters that are not date codes */
		{
			const char *f = go_locale_get_date_format ()->str;
			const char *y = strstr (f, "yyyy"), *m = strstr (f, "mm"), *d = strstr (f, "dd");
			g_assert_nonnull (y);
			g_assert_nonnull (m);
			g_assert_nonnull (d);
			g_assert_true (y < m && m < d);
			g_assert_cmpint (strlen (f), >, 10);
		}
		tested++;
	}
	if (locale_enter (locale_fr)) {
		g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ",");
		g_assert_cmpint (go_locale_get_arg_sep (), ==, ';');
		g_assert_cmpint (go_locale_month_before_day (), ==, 0);
		tested++;
	}

	locale_restore ();
	if (tested == 0)
		g_test_skip ("no non-C locales installed");
}

static void
test_locale_caching (void)
{
	const char *de;

	locale_save ();
	g_assert_nonnull (go_setlocale (LC_ALL, "C"));
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");
	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, "mm/dd/yy");
	g_assert_cmpint (go_locale_month_before_day (), ==, 1);
	g_assert_true (go_locale_24h ());

	de = locale_enter (locale_de);
	if (!de) {
		locale_restore ();
		g_test_skip ("German locale not installed");
		return;
	}
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ",");
	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, "dd.mm.yyyy");
	g_assert_cmpint (go_locale_month_before_day (), ==, 0);

	/* The C library's setlocale doesn't tell us; what has been looked at
	   stays what it was until go_setlocale is used */
	g_assert_nonnull (setlocale (LC_ALL, "C"));
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ",");
	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, "dd.mm.yyyy");
	g_assert_cmpint (go_locale_month_before_day (), ==, 0);

	g_assert_nonnull (go_setlocale (LC_ALL, "C"));
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");
	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, "mm/dd/yy");
	g_assert_cmpint (go_locale_month_before_day (), ==, 1);

	/* Changing the locale for everything but numbers leaves the numbers */
	g_assert_nonnull (go_setlocale (LC_NUMERIC, de));
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ",");
	g_assert_nonnull (go_setlocale (LC_NUMERIC, "C"));
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");

	locale_restore ();
}

static void
test_locale_booleans (void)
{
	const char *t, *f;

	locale_save ();
	g_assert_nonnull (go_setlocale (LC_ALL, "C"));

	t = go_locale_boolean_name (TRUE);
	f = go_locale_boolean_name (FALSE);
	g_assert_nonnull (t);
	g_assert_nonnull (f);
	g_assert_cmpstr (t, !=, f);
	g_assert_cmpuint (strlen (t), >, 0);
	g_assert_cmpuint (strlen (f), >, 0);
	/* Without a translation they are the plain words */
	g_assert_cmpstr (t, ==, "TRUE");
	g_assert_cmpstr (f, ==, "FALSE");
	/* Anything non-zero is true */
	g_assert_true (go_locale_boolean_name (42) == t);
	g_assert_true (go_locale_boolean_name (-1) == t);
	g_assert_true (go_locale_boolean_name (0) == f);

	/* Fixing them to the untranslated ones; a change of locale undoes that */
	go_locale_untranslated_booleans ();
	g_assert_cmpstr (go_locale_boolean_name (TRUE), ==, "TRUE");
	g_assert_cmpstr (go_locale_boolean_name (FALSE), ==, "FALSE");
	g_assert_nonnull (go_setlocale (LC_ALL, "C"));
	g_assert_cmpstr (go_locale_boolean_name (TRUE), ==, "TRUE");
	g_assert_cmpstr (go_locale_boolean_name (FALSE), ==, "FALSE");

	locale_restore ();
}

static void
test_locale_shutdown (void)
{
	locale_save ();

	/* After shutdown everything can be asked for again, also when only
	   the date and time formats had been looked at */
	g_assert_nonnull (go_setlocale (LC_ALL, "C"));
	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, "mm/dd/yy");
	g_assert_cmpstr (go_locale_get_time_format ()->str, ==, "hh:mm:ss");
	_go_locale_shutdown ();
	g_assert_nonnull (go_locale_get_date_format ());
	g_assert_cmpstr (go_locale_get_date_format ()->str, ==, "mm/dd/yy");
	g_assert_nonnull (go_locale_get_time_format ());
	g_assert_cmpstr (go_locale_get_time_format ()->str, ==, "hh:mm:ss");

	/* ... and when the numbers had been looked at */
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");
	_go_locale_shutdown ();
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");
	g_assert_cmpstr (go_locale_get_thousand ()->str, ==, ",");
	g_assert_cmpstr (go_locale_get_currency (NULL, NULL)->str, ==, "$");

	/* Twice in a row, with nothing in between */
	_go_locale_shutdown ();
	_go_locale_shutdown ();
	g_assert_cmpstr (go_locale_get_decimal ()->str, ==, ".");

	locale_restore ();
}

static void
register_locale_tests (void)
{
	g_test_add_func ("/utils/go-locale/c", test_locale_c);
	g_test_add_func ("/utils/go-locale/setlocale", test_locale_setlocale);
	g_test_add_func ("/utils/go-locale/consistency", test_locale_consistency);
	g_test_add_func ("/utils/go-locale/specific", test_locale_specific);
	g_test_add_func ("/utils/go-locale/caching", test_locale_caching);
	g_test_add_func ("/utils/go-locale/booleans", test_locale_booleans);
	g_test_add_func ("/utils/go-locale/shutdown", test_locale_shutdown);
}

/* ------------------------------------------------------------------ */
/* go-style.c                                                          */
/* ------------------------------------------------------------------ */

#define TEST_TYPE_STYLE (test_style_get_type ())

typedef struct {
	GOStyle base;
} TestStyle;

typedef struct {
	GObjectClass base;
} TestStyleClass;

GType test_style_get_type (void);
G_DEFINE_TYPE (TestStyle, test_style, GO_TYPE_STYLE)

static void
test_style_class_init (G_GNUC_UNUSED TestStyleClass *klass)
{
}

static void
test_style_init (G_GNUC_UNUSED TestStyle *s)
{
}

/* All the "auto" flags, as pointers to the flags */
static void
style_auto_flags (GOStyle *s, gboolean **flags, int *n)
{
	int i = 0;
	flags[i++] = &s->marker.auto_shape;
	flags[i++] = &s->marker.auto_outline_color;
	flags[i++] = &s->marker.auto_fill_color;
	flags[i++] = &s->line.auto_dash;
	flags[i++] = &s->line.auto_color;
	flags[i++] = &s->line.auto_fore;
	flags[i++] = &s->line.auto_width;
	flags[i++] = &s->fill.auto_type;
	flags[i++] = &s->fill.auto_fore;
	flags[i++] = &s->fill.auto_back;
	flags[i++] = &s->fill.auto_pattern;
	flags[i++] = &s->fill.gradient.auto_dir;
	flags[i++] = &s->fill.gradient.auto_brightness;
	flags[i++] = &s->font.auto_scale;
	flags[i++] = &s->font.auto_font;
	flags[i++] = &s->font.auto_color;
	flags[i++] = &s->text_layout.auto_angle;
	*n = i;
}

static void
test_style_defaults (void)
{
	GOStyle *s = go_style_new ();
	GOColor c;
	const GOFont *def = go_font_new_by_index (0);
	gboolean *flags[32];
	int i, n;

	g_assert_true (GO_IS_STYLE (s));
	g_assert_true (GO_IS_PERSIST (s));
	g_assert_true (g_type_is_a (GO_TYPE_STYLE, G_TYPE_OBJECT));
	g_assert_cmpint (s->interesting_fields, ==, GO_STYLE_ALL);
	g_assert_cmpint (s->disable_theming, ==, 0);

	/* Everything is automatic to begin with */
	style_auto_flags (s, flags, &n);
	for (i = 0; i < n; i++)
		g_assert_true (*flags[i]);
	g_assert_true (go_style_is_auto (s));

	/* The line is solid and as thin as it gets, but not visible yet */
	g_assert_cmpint (s->line.dash_type, ==, GO_LINE_SOLID);
	assert_near (s->line.width, 0);
	g_assert_cmpint (s->line.cap, ==, CAIRO_LINE_CAP_BUTT);
	g_assert_cmpint (s->line.join, ==, CAIRO_LINE_JOIN_MITER);
	assert_close (s->line.miter_limit, 2);

	/* No fill, and a plain black one if asked for */
	g_assert_cmpint (s->fill.type, ==, GO_STYLE_FILL_NONE);
	g_assert_cmpfloat (s->fill.gradient.brightness, <, 0);
	g_assert_true (go_pattern_is_solid (&s->fill.pattern, &c));
	g_assert_cmphex (c, ==, GO_COLOR_BLACK);

	/* Default font, black text, no rotation */
	g_assert_true (s->font.font == def);
	g_assert_cmphex (s->font.color, ==, GO_COLOR_BLACK);
	assert_near (s->text_layout.angle, 0);

	/* A marker of its own, which is none */
	g_assert_true (GO_IS_MARKER (s->marker.mark));
	g_assert_true (go_style_get_marker (s) == s->marker.mark);
	g_assert_cmpint (go_marker_get_shape (s->marker.mark), ==, GO_MARKER_NONE);

	go_font_unref (def);
	g_object_unref (s);
}

static void
test_style_auto (void)
{
	GOStyle *s = go_style_new ();
	gboolean *flags[32];
	int i, n;
	GOMarker *old;

	style_auto_flags (s, flags, &n);

	/* Clearing turns everything off, and starts with a new marker */
	go_marker_set_shape (s->marker.mark, GO_MARKER_CIRCLE);
	old = s->marker.mark;
	go_style_clear_auto (s);
	for (i = 0; i < n; i++)
		g_assert_false (*flags[i]);
	g_assert_false (go_style_is_auto (s));
	g_assert_true (s->marker.mark != old);
	g_assert_cmpint (go_marker_get_shape (s->marker.mark), ==, GO_MARKER_NONE);

	/* Forcing turns everything on, again with a new marker */
	go_marker_set_shape (s->marker.mark, GO_MARKER_DIAMOND);
	go_style_force_auto (s);
	for (i = 0; i < n; i++)
		g_assert_true (*flags[i]);
	g_assert_true (go_style_is_auto (s));
	g_assert_cmpint (go_marker_get_shape (s->marker.mark), ==, GO_MARKER_NONE);

	/* Any one of these not being automatic is enough to not be automatic */
	{
		unsigned u;
		gboolean *relevant[] = {
			&s->marker.auto_shape, &s->marker.auto_outline_color, &s->marker.auto_fill_color,
			&s->line.auto_dash, &s->line.auto_color, &s->fill.auto_type, &s->fill.auto_fore,
			&s->fill.auto_back, &s->font.auto_scale, &s->font.auto_color, &s->font.auto_font,
			&s->text_layout.auto_angle
		};
		for (u = 0; u < G_N_ELEMENTS (relevant); u++) {
			*relevant[u] = FALSE;
			g_assert_false (go_style_is_auto (s));
			*relevant[u] = TRUE;
			g_assert_true (go_style_is_auto (s));
		}
	}

	/* What counts depends on the kind of fill */
	s->fill.type = GO_STYLE_FILL_PATTERN;
	g_assert_true (go_style_is_auto (s));
	s->fill.auto_pattern = FALSE;
	g_assert_false (go_style_is_auto (s));
	s->fill.type = GO_STYLE_FILL_NONE;
	g_assert_true (go_style_is_auto (s));
	s->fill.auto_pattern = TRUE;
	s->fill.type = GO_STYLE_FILL_GRADIENT;
	g_assert_true (go_style_is_auto (s));
	s->fill.gradient.auto_dir = FALSE;
	g_assert_false (go_style_is_auto (s));
	s->fill.gradient.auto_dir = TRUE;
	s->fill.gradient.auto_brightness = FALSE;
	g_assert_false (go_style_is_auto (s));
	s->fill.gradient.auto_brightness = TRUE;
	s->fill.type = GO_STYLE_FILL_NONE;

	g_object_unref (s);
}

/* A style in which every field has a value of its own */
static GOStyle *
make_custom_style (GType type)
{
	GOStyle *s = g_object_new (type, NULL);
	GOMarker *m;

	go_style_clear_auto (s);
	s->interesting_fields = GO_STYLE_LINE | GO_STYLE_FILL | GO_STYLE_FONT;
	s->disable_theming = GO_STYLE_FILL;

	s->line.width = 2.5;
	s->line.dash_type = GO_LINE_DASH;
	s->line.color = GO_COLOR_FROM_RGBA (1, 2, 3, 4);
	s->line.fore = GO_COLOR_FROM_RGBA (5, 6, 7, 8);
	s->line.pattern = GO_PATTERN_BRICKS;
	s->line.cap = CAIRO_LINE_CAP_ROUND;
	s->line.join = CAIRO_LINE_JOIN_BEVEL;
	s->line.miter_limit = 7.5;

	s->fill.type = GO_STYLE_FILL_PATTERN;
	s->fill.pattern.pattern = GO_PATTERN_HORIZ;
	s->fill.pattern.fore = GO_COLOR_FROM_RGBA (10, 20, 30, 40);
	s->fill.pattern.back = GO_COLOR_FROM_RGBA (50, 60, 70, 80);
	s->fill.invert_if_negative = TRUE;
	s->fill.gradient.dir = GO_GRADIENT_SW_TO_NE;
	s->fill.gradient.brightness = 33;

	m = go_marker_new ();
	go_marker_set_shape (m, GO_MARKER_CIRCLE);
	go_marker_set_size (m, 9);
	go_marker_set_outline_color (m, GO_COLOR_RED);
	go_marker_set_fill_color (m, GO_COLOR_GREEN);
	go_style_set_marker (s, m);

	go_style_set_font (s, go_font_new_by_name ("Serif Bold 14"));
	s->font.color = GO_COLOR_FROM_RGBA (11, 22, 33, 44);
	go_style_set_text_angle (s, 45);
	return s;
}

static void
assert_styles_identical (const GOStyle *a, const GOStyle *b)
{
	g_assert_cmpint (a->interesting_fields, ==, b->interesting_fields);
	g_assert_cmpint (a->disable_theming, ==, b->disable_theming);

	g_assert_cmpfloat (a->line.width, ==, b->line.width);
	g_assert_cmpint (a->line.dash_type, ==, b->line.dash_type);
	g_assert_cmpint (a->line.auto_dash, ==, b->line.auto_dash);
	g_assert_cmphex (a->line.color, ==, b->line.color);
	g_assert_cmphex (a->line.fore, ==, b->line.fore);
	g_assert_cmpint (a->line.auto_color, ==, b->line.auto_color);
	g_assert_cmpint (a->line.auto_fore, ==, b->line.auto_fore);
	g_assert_cmpint (a->line.auto_width, ==, b->line.auto_width);
	g_assert_cmpint (a->line.pattern, ==, b->line.pattern);
	g_assert_cmpint (a->line.cap, ==, b->line.cap);
	g_assert_cmpint (a->line.join, ==, b->line.join);
	g_assert_cmpfloat (a->line.miter_limit, ==, b->line.miter_limit);

	g_assert_cmpint (a->fill.type, ==, b->fill.type);
	g_assert_cmpint (a->fill.auto_type, ==, b->fill.auto_type);
	g_assert_cmpint (a->fill.auto_fore, ==, b->fill.auto_fore);
	g_assert_cmpint (a->fill.auto_back, ==, b->fill.auto_back);
	g_assert_cmpint (a->fill.auto_pattern, ==, b->fill.auto_pattern);
	g_assert_cmpint (a->fill.invert_if_negative, ==, b->fill.invert_if_negative);
	g_assert_cmpint (a->fill.pattern.pattern, ==, b->fill.pattern.pattern);
	g_assert_cmphex (a->fill.pattern.fore, ==, b->fill.pattern.fore);
	g_assert_cmphex (a->fill.pattern.back, ==, b->fill.pattern.back);
	g_assert_cmpint (a->fill.gradient.dir, ==, b->fill.gradient.dir);
	g_assert_cmpfloat (a->fill.gradient.brightness, ==, b->fill.gradient.brightness);
	g_assert_cmpint (a->fill.gradient.auto_dir, ==, b->fill.gradient.auto_dir);
	g_assert_cmpint (a->fill.gradient.auto_brightness, ==, b->fill.gradient.auto_brightness);

	g_assert_cmpint (a->marker.auto_shape, ==, b->marker.auto_shape);
	g_assert_cmpint (a->marker.auto_outline_color, ==, b->marker.auto_outline_color);
	g_assert_cmpint (a->marker.auto_fill_color, ==, b->marker.auto_fill_color);
	g_assert_cmpint (go_marker_get_shape (a->marker.mark), ==, go_marker_get_shape (b->marker.mark));
	g_assert_cmpint (go_marker_get_size (a->marker.mark), ==, go_marker_get_size (b->marker.mark));
	g_assert_cmphex (go_marker_get_outline_color (a->marker.mark), ==, go_marker_get_outline_color (b->marker.mark));
	g_assert_cmphex (go_marker_get_fill_color (a->marker.mark), ==, go_marker_get_fill_color (b->marker.mark));

	g_assert_true (go_font_eq (a->font.font, b->font.font));
	g_assert_cmphex (a->font.color, ==, b->font.color);
	g_assert_cmpint (a->font.auto_scale, ==, b->font.auto_scale);
	g_assert_cmpint (a->font.auto_font, ==, b->font.auto_font);
	g_assert_cmpint (a->font.auto_color, ==, b->font.auto_color);
	g_assert_cmpfloat (a->text_layout.angle, ==, b->text_layout.angle);
	g_assert_cmpint (a->text_layout.auto_angle, ==, b->text_layout.auto_angle);
}

static void
test_style_assign_dup (void)
{
	GOStyle *src = make_custom_style (GO_TYPE_STYLE);
	GOStyle *dup = go_style_dup (src);
	GOStyle *dst = go_style_new ();
	char *name;

	g_assert_true (dup != src);
	g_assert_true (GO_IS_STYLE (dup));
	assert_styles_identical (src, dup);

	/* The marker is a copy; the font is shared */
	g_assert_true (go_style_get_marker (dup) != go_style_get_marker (src));
	g_assert_true (dup->font.font == src->font.font);

	/* Changes to the copy don't reach the original */
	go_marker_set_size (dup->marker.mark, 3);
	dup->line.width = 9;
	g_assert_cmpint (go_marker_get_size (src->marker.mark), ==, 9);
	assert_close (src->line.width, 2.5);

	/* Assigning gives the same result, overwriting what was there */
	go_style_assign (dst, src);
	assert_styles_identical (src, dst);
	g_assert_true (go_style_get_marker (dst) != go_style_get_marker (src));
	go_style_assign (dst, src);
	assert_styles_identical (src, dst);
	go_style_assign (dst, dup);
	g_assert_cmpint (go_marker_get_size (dst->marker.mark), ==, 3);
	assert_close (dst->line.width, 9);

	/* Self-assignment changes nothing */
	go_style_assign (src, src);
	g_assert_cmpint (go_marker_get_size (src->marker.mark), ==, 9);

	g_object_unref (dup);
	g_object_unref (dst);
	g_object_unref (src);

	/* A copy of a style of a derived type is of that type */
	src = make_custom_style (TEST_TYPE_STYLE);
	dup = go_style_dup (src);
	name = g_strdup (G_OBJECT_TYPE_NAME (dup));
	g_assert_cmpstr (name, ==, "TestStyle");
	g_free (name);
	assert_styles_identical (src, dup);
	g_object_unref (dup);
	g_object_unref (src);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_dup*");
	g_assert_null (go_style_dup (NULL));
	g_test_assert_expected_messages ();
	dst = go_style_new ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_assign*");
	go_style_assign (dst, NULL);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_assign*");
	go_style_assign (NULL, dst);
	g_test_assert_expected_messages ();
	g_object_unref (dst);
}

static void
test_style_assign_image (void)
{
	/* Images are shared between the copies */
	TestImage *img = test_image_new (4, 4);
	GOStyle *a = go_style_new (), *b;
	GOStyle *c = go_style_new ();

	a->fill.type = GO_STYLE_FILL_IMAGE;
	a->fill.image.image = GO_IMAGE (img);	/* a owns our reference */
	a->fill.image.type = GO_IMAGE_CENTERED;

	b = go_style_dup (a);
	g_assert_cmpuint (G_OBJECT (img)->ref_count, ==, 2);
	g_assert_true (b->fill.image.image == GO_IMAGE (img));
	g_assert_cmpint (b->fill.image.type, ==, GO_IMAGE_CENTERED);

	/* Overwriting a style which had an image lets go of it */
	c->fill.type = GO_STYLE_FILL_IMAGE;
	c->fill.image.image = g_object_ref (GO_IMAGE (img));
	g_assert_cmpuint (G_OBJECT (img)->ref_count, ==, 3);
	go_style_assign (c, b);
	g_assert_cmpuint (G_OBJECT (img)->ref_count, ==, 3);	/* a, b, c */

	/* Assigning something else over it */
	{
		GOStyle *plain = go_style_new ();
		go_style_assign (c, plain);
		g_assert_cmpuint (G_OBJECT (img)->ref_count, ==, 2);
		g_object_unref (plain);
	}

	g_object_unref (b);
	g_assert_cmpuint (G_OBJECT (img)->ref_count, ==, 1);
	g_object_unref (c);
	g_object_unref (a);	/* the last reference */
}

static void
test_style_apply_theme (void)
{
	GOStyle *src = make_custom_style (GO_TYPE_STYLE);
	GOStyle *dst, *orig;
	GOMarker *m;

	/* Nothing is changed on things that aren't automatic */
	dst = go_style_new ();
	go_style_clear_auto (dst);
	orig = go_style_dup (dst);
	go_style_apply_theme (dst, src, GO_STYLE_ALL | GO_STYLE_MARKER | GO_STYLE_TEXT_LAYOUT);
	assert_styles_identical (orig, dst);
	g_object_unref (orig);
	g_object_unref (dst);

	/* Automatic things get the values of the theme, but only for the
	   kinds of fields asked for */
	dst = go_style_new ();
	go_style_apply_theme (dst, src, GO_STYLE_FILL);
	g_assert_cmpint (dst->fill.type, ==, GO_STYLE_FILL_PATTERN);
	g_assert_cmpint (dst->fill.pattern.pattern, ==, GO_PATTERN_HORIZ);
	g_assert_cmphex (dst->fill.pattern.fore, ==, src->fill.pattern.fore);
	g_assert_cmphex (dst->fill.pattern.back, ==, src->fill.pattern.back);
	g_assert_cmpint (dst->fill.gradient.dir, ==, GO_GRADIENT_SW_TO_NE);
	assert_close (dst->fill.gradient.brightness, 33);
	/* ... and nothing else */
	assert_near (dst->line.width, 0);
	g_assert_cmpint (dst->line.dash_type, ==, GO_LINE_SOLID);
	g_assert_cmpint (go_marker_get_shape (dst->marker.mark), ==, GO_MARKER_NONE);
	assert_near (dst->text_layout.angle, 0);
	g_assert_cmphex (dst->font.color, ==, GO_COLOR_BLACK);
	/* The flags themselves are not touched */
	g_assert_true (dst->fill.auto_type);

	/* Line and outline */
	go_style_apply_theme (dst, src, GO_STYLE_LINE);
	assert_close (dst->line.width, 2.5);
	g_assert_cmpint (dst->line.dash_type, ==, GO_LINE_DASH);
	g_assert_cmphex (dst->line.color, ==, src->line.color);
	g_object_unref (dst);
	dst = go_style_new ();
	go_style_apply_theme (dst, src, GO_STYLE_OUTLINE);
	assert_close (dst->line.width, 2.5);
	g_assert_cmpint (dst->line.dash_type, ==, GO_LINE_DASH);

	/* Marker */
	g_object_unref (dst);
	dst = go_style_new ();
	go_style_apply_theme (dst, src, GO_STYLE_MARKER);
	g_assert_cmpint (go_marker_get_shape (dst->marker.mark), ==, GO_MARKER_CIRCLE);
	g_assert_cmphex (go_marker_get_outline_color (dst->marker.mark), ==, GO_COLOR_RED);
	g_assert_cmphex (go_marker_get_fill_color (dst->marker.mark), ==, GO_COLOR_GREEN);
	/* (the size is not part of the theme) */
	g_assert_cmpint (go_marker_get_size (dst->marker.mark), !=, 9);
	/* Only the automatic parts */
	m = dst->marker.mark;
	dst->marker.auto_outline_color = FALSE;
	go_marker_set_outline_color (m, GO_COLOR_BLUE);
	go_style_apply_theme (dst, src, GO_STYLE_MARKER);
	g_assert_cmphex (go_marker_get_outline_color (dst->marker.mark), ==, GO_COLOR_BLUE);
	g_assert_cmphex (go_marker_get_fill_color (dst->marker.mark), ==, GO_COLOR_GREEN);

	/* Text */
	g_object_unref (dst);
	dst = go_style_new ();
	go_style_apply_theme (dst, src, GO_STYLE_TEXT_LAYOUT);
	assert_close (dst->text_layout.angle, 45);
	assert_near (dst->line.width, 0);
	go_style_apply_theme (dst, src, GO_STYLE_FONT);
	g_assert_cmphex (dst->font.color, ==, src->font.color);
	g_assert_true (dst->font.font == src->font.font);
	g_assert_true (go_font_eq (dst->font.font, src->font.font));

	/* Nothing is asked for, nothing is done */
	g_object_unref (dst);
	dst = go_style_new ();
	orig = go_style_dup (dst);
	go_style_apply_theme (dst, src, 0);
	assert_styles_identical (orig, dst);
	/* ... and the same style is a theme for itself */
	go_style_apply_theme (dst, dst, GO_STYLE_ALL);
	assert_styles_identical (orig, dst);
	g_object_unref (orig);
	g_object_unref (dst);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_apply_theme*");
	go_style_apply_theme (NULL, src, GO_STYLE_ALL);
	g_test_assert_expected_messages ();
	dst = go_style_new ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_apply_theme*");
	go_style_apply_theme (dst, NULL, GO_STYLE_ALL);
	g_test_assert_expected_messages ();
	g_object_unref (dst);

	g_object_unref (src);
}

static void
test_style_visibility (void)
{
	GOStyle *s = go_style_new ();

	/* Lines need a color that isn't transparent and a dash type that isn't none */
	s->line.color = 0;
	g_assert_false (go_style_is_line_visible (s));
	g_assert_false (go_style_is_outline_visible (s));
	s->line.color = GO_COLOR_FROM_RGBA (1, 2, 3, 1);
	g_assert_true (go_style_is_line_visible (s));
	g_assert_true (go_style_is_outline_visible (s));
	s->line.dash_type = GO_LINE_NONE;
	g_assert_false (go_style_is_line_visible (s));
	g_assert_false (go_style_is_outline_visible (s));
	s->line.dash_type = GO_LINE_DASH;
	g_assert_true (go_style_is_line_visible (s));
	s->line.color = GO_COLOR_FROM_RGBA (1, 2, 3, 0);
	g_assert_false (go_style_is_line_visible (s));

	/* Fills: whatever the type, as long as there is one */
	g_assert_false (go_style_is_fill_visible (s));
	s->fill.type = GO_STYLE_FILL_PATTERN;
	g_assert_true (go_style_is_fill_visible (s));
	s->fill.type = GO_STYLE_FILL_GRADIENT;
	g_assert_true (go_style_is_fill_visible (s));
	s->fill.type = GO_STYLE_FILL_IMAGE;
	g_assert_true (go_style_is_fill_visible (s));
	s->fill.type = GO_STYLE_FILL_NONE;
	g_assert_false (go_style_is_fill_visible (s));

	/* Markers need a shape, and an interest in them */
	g_assert_false (go_style_is_marker_visible (s));
	go_marker_set_shape (s->marker.mark, GO_MARKER_SQUARE);
	g_assert_true (go_style_is_marker_visible (s));
	s->interesting_fields &= ~GO_STYLE_MARKER;
	g_assert_false (go_style_is_marker_visible (s));
	s->interesting_fields |= GO_STYLE_MARKER;
	g_assert_true (go_style_is_marker_visible (s));
	go_marker_set_shape (s->marker.mark, GO_MARKER_NONE);
	g_assert_false (go_style_is_marker_visible (s));

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_is_line_visible*");
	g_assert_false (go_style_is_line_visible (NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_is_outline_visible*");
	g_assert_false (go_style_is_outline_visible (NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_is_fill_visible*");
	g_assert_false (go_style_is_fill_visible (NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_is_marker_visible*");
	g_assert_false (go_style_is_marker_visible (NULL));
	g_test_assert_expected_messages ();

	g_object_unref (s);
}

static void
test_style_different_size (void)
{
	GOStyle *a = go_style_new (), *b = go_style_new ();

	g_assert_false (go_style_is_different_size (a, a));
	g_assert_false (go_style_is_different_size (a, b));

	/* What takes up space */
	b->line.width = 3;
	g_assert_true (go_style_is_different_size (a, b));
	b->line.width = a->line.width;
	b->line.dash_type = GO_LINE_NONE;
	g_assert_true (go_style_is_different_size (a, b));
	b->line.dash_type = a->line.dash_type;
	b->fill.type = GO_STYLE_FILL_PATTERN;
	g_assert_true (go_style_is_different_size (a, b));
	b->fill.type = a->fill.type;
	go_style_set_text_angle (b, 30);
	g_assert_true (go_style_is_different_size (a, b));
	go_style_set_text_angle (b, 0);
	g_assert_false (go_style_is_different_size (a, b));
	go_style_set_font (b, go_font_new_by_name ("Serif Bold 31"));
	g_assert_true (go_style_is_different_size (a, b));
	g_assert_true (go_style_is_different_size (b, a));
	go_style_set_font (b, go_font_new_by_index (0));
	g_assert_false (go_style_is_different_size (a, b));

	/* What doesn't */
	b->line.color = GO_COLOR_RED;
	b->font.color = GO_COLOR_BLUE;
	b->fill.pattern.fore = GO_COLOR_GREEN;
	go_marker_set_shape (b->marker.mark, GO_MARKER_CROSS);
	g_assert_false (go_style_is_different_size (a, b));

	/* A missing style is different from anything */
	g_assert_true (go_style_is_different_size (a, NULL));
	g_assert_true (go_style_is_different_size (NULL, b));
	g_assert_true (go_style_is_different_size (NULL, NULL));

	g_object_unref (a);
	g_object_unref (b);
}

static void
test_style_setters (void)
{
	GOStyle *s = go_style_new ();
	GOMarker *m;
	const GOFont *f;

	/* The marker is taken over by the style */
	m = go_marker_new ();
	go_marker_set_shape (m, GO_MARKER_TRIANGLE_UP);
	go_style_set_marker (s, m);
	g_assert_true (go_style_get_marker (s) == m);
	/* Setting the same one again must not release it */
	g_object_ref (m);
	go_style_set_marker (s, m);
	g_assert_true (go_style_get_marker (s) == m);
	g_assert_cmpuint (G_OBJECT (m)->ref_count, ==, 2);
	g_object_unref (m);
	g_assert_cmpint (go_marker_get_shape (go_style_get_marker (s)), ==, GO_MARKER_TRIANGLE_UP);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_marker*");
	go_style_set_marker (s, NULL);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_marker*");
	go_style_set_marker (NULL, m);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_get_marker*");
	g_assert_null (go_style_get_marker (NULL));
	g_test_assert_expected_messages ();

	/* Fonts: given by font or by description, taken over */
	go_style_set_font (s, go_font_new_by_name ("Serif Italic 15"));
	f = go_font_new_by_name ("Serif Italic 15");
	g_assert_true (s->font.font == f);
	go_font_unref (f);
	go_style_set_font_desc (s, pango_font_description_from_string ("Monospace Bold 16"));
	f = go_font_new_by_name ("Monospace Bold 16");
	g_assert_true (s->font.font == f);
	go_font_unref (f);
	/* No font means no change */
	f = s->font.font;
	go_style_set_font (s, NULL);
	g_assert_true (s->font.font == f);
	/* Setting the font the style already has */
	go_style_set_font (s, go_font_ref (s->font.font));
	g_assert_true (s->font.font == f);

	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_font*");
	go_style_set_font (NULL, NULL);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_font_desc*");
	go_style_set_font_desc (NULL, NULL);
	g_test_assert_expected_messages ();

	/* Text angle in degrees, limited, and no longer automatic */
	g_assert_true (s->text_layout.auto_angle);
	go_style_set_text_angle (s, 45);
	assert_close (s->text_layout.angle, 45);
	g_assert_false (s->text_layout.auto_angle);
	go_style_set_text_angle (s, -90.5);
	assert_close (s->text_layout.angle, -90.5);
	go_style_set_text_angle (s, 180);
	assert_close (s->text_layout.angle, 180);
	go_style_set_text_angle (s, -180);
	assert_close (s->text_layout.angle, -180);
	go_style_set_text_angle (s, 270);
	assert_close (s->text_layout.angle, 180);
	go_style_set_text_angle (s, -1000);
	assert_close (s->text_layout.angle, -180);
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_text_angle*");
	go_style_set_text_angle (NULL, 5);
	g_test_assert_expected_messages ();

	g_object_unref (s);
}

static void
test_style_fill_brightness (void)
{
	GOStyle *s = go_style_new ();
	int step, k;
	GRand *rand = g_rand_new_with_seed (555);

	/* Only for gradients */
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_fill_brightness*");
	go_style_set_fill_brightness (s, 50);
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_fill_brightness*");
	go_style_set_fill_brightness (NULL, 50);
	g_test_assert_expected_messages ();

	s->fill.type = GO_STYLE_FILL_GRADIENT;

	/* From a white start color, the other end of the gradient goes from
	   black (brightness 0) to white (100) through grey */
	s->fill.pattern.back = GO_COLOR_WHITE;
	go_style_set_fill_brightness (s, 0);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_BLACK);
	go_style_set_fill_brightness (s, 100);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_WHITE);
	go_style_set_fill_brightness (s, 50);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_FROM_RGBA (128, 128, 128, 255));
	assert_close (s->fill.gradient.brightness, 50);

	/* ... and from black the same */
	s->fill.pattern.back = GO_COLOR_BLACK;
	go_style_set_fill_brightness (s, 0);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_BLACK);
	go_style_set_fill_brightness (s, 100);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_WHITE);
	go_style_set_fill_brightness (s, 50);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_FROM_RGBA (128, 128, 128, 255));

	/* The value is limited to 0..100 */
	go_style_set_fill_brightness (s, -50);
	assert_near (s->fill.gradient.brightness, 0);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_BLACK);
	go_style_set_fill_brightness (s, 500);
	assert_close (s->fill.gradient.brightness, 100);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_WHITE);

	/* For any start color, the extremes are black and white, and in
	   between the colors get brighter */
	for (k = 0; k < 200; k++) {
		int prev = -1;
		s->fill.pattern.back = GO_COLOR_FROM_RGBA (g_rand_int_range (rand, 0, 256),
							   g_rand_int_range (rand, 0, 256),
							   g_rand_int_range (rand, 0, 256), 255);
		go_style_set_fill_brightness (s, 0);
		g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_BLACK);
		go_style_set_fill_brightness (s, 100);
		g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_WHITE);
		for (step = 0; step <= 20; step++) {
			GOColor f;
			int sum;
			go_style_set_fill_brightness (s, 5 * step);
			f = s->fill.pattern.fore;
			sum = GO_COLOR_UINT_R (f) + GO_COLOR_UINT_G (f) + GO_COLOR_UINT_B (f);
			g_assert_cmpint (sum, >=, prev - 3);	/* (allowing for rounding) */
			prev = sum;
		}
	}

	g_rand_free (rand);
	g_object_unref (s);
}

static cairo_surface_t *
fill_surface (int w, int h, cairo_t **cr)
{
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
	*cr = cairo_create (surf);
	return surf;
}

static gboolean
path_is_empty (cairo_t *cr)
{
	cairo_path_t *p = cairo_copy_path (cr);
	gboolean empty = (p->num_data == 0);
	cairo_path_destroy (p);
	return empty;
}

static void
test_style_fill_basic (void)
{
	GOStyle *s = go_style_new ();
	cairo_t *cr;
	cairo_surface_t *surf = fill_surface (20, 20, &cr);

	/* No fill: nothing is drawn, and the path goes unless it is kept */
	cairo_rectangle (cr, 0, 0, 20, 20);
	go_style_fill (s, cr, TRUE);
	g_assert_false (path_is_empty (cr));
	g_assert_cmpuint (pixel_at (surf, 10, 10), ==, 0);
	go_style_fill (s, cr, FALSE);
	g_assert_true (path_is_empty (cr));
	g_assert_cmpuint (pixel_at (surf, 10, 10), ==, 0);

	/* A plain color */
	s->fill.type = GO_STYLE_FILL_PATTERN;
	go_pattern_set_solid (&s->fill.pattern, GO_COLOR_FROM_RGB (0, 255, 0));
	cairo_rectangle (cr, 0, 0, 10, 20);
	go_style_fill (s, cr, TRUE);
	g_assert_false (path_is_empty (cr));
	g_assert_cmphex (pixel_at (surf, 5, 10), ==, 0xff00ff00u);
	g_assert_cmpuint (pixel_at (surf, 15, 10), ==, 0);
	go_style_fill (s, cr, FALSE);
	g_assert_true (path_is_empty (cr));

	/* The foreground color, for the pattern that is all foreground */
	s->fill.pattern.pattern = GO_PATTERN_FOREGROUND_SOLID;
	s->fill.pattern.fore = GO_COLOR_RED;
	s->fill.pattern.back = GO_COLOR_BLUE;
	cairo_rectangle (cr, 10, 0, 10, 20);
	go_style_fill (s, cr, FALSE);
	g_assert_cmphex (pixel_at (surf, 15, 10), ==, 0xffff0000u);

	/* Nothing to fill: nothing happens, but the path still goes */
	s->fill.pattern.pattern = GO_PATTERN_SOLID;
	cairo_new_path (cr);
	cairo_rectangle (cr, 2, 2, 0, 10);
	go_style_fill (s, cr, FALSE);
	g_assert_true (path_is_empty (cr));
	cairo_rectangle (cr, 2, 2, 10, 0);
	go_style_fill (s, cr, TRUE);
	g_assert_false (path_is_empty (cr));
	cairo_new_path (cr);
	go_style_fill (s, cr, FALSE);
	g_assert_true (path_is_empty (cr));

	/* No style at all: nothing is drawn, and the path goes as usual */
	cairo_new_path (cr);
	cairo_rectangle (cr, 0, 0, 20, 20);
	go_style_fill (NULL, cr, TRUE);
	g_assert_false (path_is_empty (cr));
	go_style_fill (NULL, cr, FALSE);
	g_assert_true (path_is_empty (cr));

	cairo_destroy (cr);
	cairo_surface_destroy (surf);
	g_object_unref (s);
}

static void
test_style_fill_gradient (void)
{
	GOStyle *s = go_style_new ();
	cairo_t *cr;
	cairo_surface_t *surf;
	guint32 top, mid, bottom, left, right;

	s->fill.type = GO_STYLE_FILL_GRADIENT;
	s->fill.pattern.back = GO_COLOR_RED;
	s->fill.pattern.fore = GO_COLOR_BLUE;

	/* Top to bottom: starts with the first color and ends with the other */
	surf = fill_surface (20, 20, &cr);
	s->fill.gradient.dir = GO_GRADIENT_N_TO_S;
	cairo_rectangle (cr, 0, 0, 20, 20);
	go_style_fill (s, cr, FALSE);
	top = pixel_at (surf, 10, 0);
	bottom = pixel_at (surf, 10, 19);
	g_assert_cmpint ((top >> 16) & 0xff, >, 230);
	g_assert_cmpint (top & 0xff, <, 25);
	g_assert_cmpint (bottom & 0xff, >, 230);
	g_assert_cmpint ((bottom >> 16) & 0xff, <, 25);
	/* The same all along a row */
	g_assert_cmphex (pixel_at (surf, 0, 7), ==, pixel_at (surf, 19, 7));
	/* ... and in between a mix */
	mid = pixel_at (surf, 10, 10);
	g_assert_cmpint ((mid >> 16) & 0xff, >, 50);
	g_assert_cmpint (mid & 0xff, >, 50);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* The other way around */
	surf = fill_surface (20, 20, &cr);
	s->fill.gradient.dir = GO_GRADIENT_S_TO_N;
	cairo_rectangle (cr, 0, 0, 20, 20);
	go_style_fill (s, cr, FALSE);
	g_assert_cmpint (pixel_at (surf, 10, 0) & 0xff, >, 230);
	g_assert_cmpint ((pixel_at (surf, 10, 19) >> 16) & 0xff, >, 230);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Left to right */
	surf = fill_surface (20, 20, &cr);
	s->fill.gradient.dir = GO_GRADIENT_W_TO_E;
	cairo_rectangle (cr, 0, 0, 20, 20);
	go_style_fill (s, cr, FALSE);
	left = pixel_at (surf, 0, 10);
	right = pixel_at (surf, 19, 10);
	g_assert_cmpint ((left >> 16) & 0xff, >, 230);
	g_assert_cmpint (right & 0xff, >, 230);
	g_assert_cmphex (pixel_at (surf, 7, 0), ==, pixel_at (surf, 7, 19));
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Mirrored: there and back again, so both ends have the same color */
	surf = fill_surface (20, 20, &cr);
	s->fill.gradient.dir = GO_GRADIENT_N_TO_S_MIRRORED;
	cairo_rectangle (cr, 0, 0, 20, 20);
	go_style_fill (s, cr, FALSE);
	top = pixel_at (surf, 10, 0);
	mid = pixel_at (surf, 10, 10);
	bottom = pixel_at (surf, 10, 19);
	g_assert_cmpint ((top >> 16) & 0xff, >, 200);
	g_assert_cmpint (mid & 0xff, >, 200);
	g_assert_cmpint ((bottom >> 16) & 0xff, >, 200);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Corner to corner */
	surf = fill_surface (20, 20, &cr);
	s->fill.gradient.dir = GO_GRADIENT_NW_TO_SE;
	cairo_rectangle (cr, 0, 0, 20, 20);
	go_style_fill (s, cr, FALSE);
	g_assert_cmpint ((pixel_at (surf, 0, 0) >> 16) & 0xff, >, 230);
	g_assert_cmpint (pixel_at (surf, 19, 19) & 0xff, >, 230);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	/* Only what is in the path is filled */
	surf = fill_surface (20, 20, &cr);
	s->fill.gradient.dir = GO_GRADIENT_N_TO_S;
	cairo_rectangle (cr, 5, 5, 10, 10);
	go_style_fill (s, cr, FALSE);
	g_assert_cmpuint (pixel_at (surf, 2, 2), ==, 0);
	g_assert_cmpuint (pixel_at (surf, 17, 17), ==, 0);
	g_assert_cmpint ((pixel_at (surf, 10, 5) >> 16) & 0xff, >, 200);
	g_assert_cmpint (pixel_at (surf, 10, 14) & 0xff, >, 200);
	cairo_destroy (cr);
	cairo_surface_destroy (surf);

	g_object_unref (s);
}

static void
test_style_fill_image (void)
{
	static const struct {
		GOImageType type;
		int draws;
	} cases[] = {
		{ GO_IMAGE_STRETCHED, 1 },
		{ GO_IMAGE_CENTERED, 1 },
		{ GO_IMAGE_WALLPAPER, 9 },		/* 3 x 3 tiles of 10 starting at the corner */
		{ GO_IMAGE_CENTERED_WALLPAPER, 16 }	/* 4 x 4: partial ones all around */
	};
	unsigned i;
	int preserve;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		for (preserve = 0; preserve <= 1; preserve++) {
			GOStyle *s = go_style_new ();
			TestImage *img = test_image_new (10, 10);
			cairo_t *cr;
			cairo_surface_t *surf = fill_surface (30, 30, &cr);
			cairo_matrix_t m;

			s->fill.type = GO_STYLE_FILL_IMAGE;
			s->fill.image.image = GO_IMAGE (img);
			s->fill.image.type = cases[i].type;

			test_image_reset_log ();
			cairo_rectangle (cr, 0, 0, 25, 25);
			go_style_fill (s, cr, preserve);
			g_assert_cmpint (test_image_log.draw_calls, ==, cases[i].draws);
			g_assert_cmpint (path_is_empty (cr), ==, !preserve);

			/* The state of the drawing context is as it was */
			cairo_get_matrix (cr, &m);
			assert_close (m.xx, 1);
			assert_close (m.yy, 1);
			assert_near (m.x0, 0);
			assert_near (m.y0, 0);
			/* ... in particular there is no clipping left */
			cairo_new_path (cr);
			cairo_rectangle (cr, 0, 0, 30, 30);
			cairo_set_source_rgb (cr, 0, 0, 1);
			cairo_fill (cr);
			g_assert_cmphex (pixel_at (surf, 28, 28), ==, 0xff0000ffu);

			cairo_destroy (cr);
			cairo_surface_destroy (surf);
			g_object_unref (s);
		}
	}

	/* An image fill that has no image is white */
	{
		GOStyle *s = go_style_new ();
		cairo_t *cr;
		cairo_surface_t *surf = fill_surface (20, 20, &cr);

		s->fill.type = GO_STYLE_FILL_IMAGE;
		s->fill.image.image = NULL;
		cairo_rectangle (cr, 0, 0, 20, 20);
		go_style_fill (s, cr, TRUE);
		g_assert_cmphex (pixel_at (surf, 10, 10), ==, 0xffffffffu);
		g_assert_false (path_is_empty (cr));
		/* ... also when the path isn't kept */
		cairo_new_path (cr);
		cairo_rectangle (cr, 0, 0, 5, 5);
		go_style_fill (s, cr, FALSE);
		g_assert_true (path_is_empty (cr));
		g_assert_cmphex (pixel_at (surf, 2, 2), ==, 0xffffffffu);

		cairo_destroy (cr);
		cairo_surface_destroy (surf);
		g_object_unref (s);
	}

	/* Fresh surface: only the un-preserved fill, so nothing from before */
	{
		GOStyle *s = go_style_new ();
		cairo_t *cr;
		cairo_surface_t *surf = fill_surface (20, 20, &cr);

		s->fill.type = GO_STYLE_FILL_IMAGE;
		cairo_rectangle (cr, 4, 4, 8, 8);
		go_style_fill (s, cr, FALSE);
		g_assert_cmphex (pixel_at (surf, 8, 8), ==, 0xffffffffu);
		g_assert_cmpuint (pixel_at (surf, 1, 1), ==, 0);
		cairo_destroy (cr);
		cairo_surface_destroy (surf);
		g_object_unref (s);
	}
}

static void
test_style_cairo_line (void)
{
	GOStyle *s = go_style_new ();
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
	cairo_t *cr = cairo_create (surf);
	double r, g, b, a;
	cairo_pattern_t *src;

	/* No line: nothing is set */
	s->line.dash_type = GO_LINE_NONE;
	s->line.width = 7;
	assert_close (cairo_get_line_width (cr), 2);
	g_assert_false (go_style_set_cairo_line (s, cr));
	assert_close (cairo_get_line_width (cr), 2);

	/* Properties are handed over */
	s->line.dash_type = GO_LINE_SOLID;
	s->line.width = 3.5;
	s->line.cap = CAIRO_LINE_CAP_ROUND;
	s->line.join = CAIRO_LINE_JOIN_BEVEL;
	s->line.miter_limit = 5;
	s->line.color = GO_COLOR_FROM_RGBA (255, 0, 0, 128);
	g_assert_true (go_style_set_cairo_line (s, cr));
	assert_close (cairo_get_line_width (cr), 3.5);
	g_assert_cmpint (cairo_get_line_cap (cr), ==, CAIRO_LINE_CAP_ROUND);
	g_assert_cmpint (cairo_get_line_join (cr), ==, CAIRO_LINE_JOIN_BEVEL);
	assert_close (cairo_get_miter_limit (cr), 5);
	g_assert_cmpint (cairo_get_dash_count (cr), ==, 0);
	src = cairo_get_source (cr);
	g_assert_cmpint (cairo_pattern_get_type (src), ==, CAIRO_PATTERN_TYPE_SOLID);
	cairo_pattern_get_rgba (src, &r, &g, &b, &a);
	assert_close (r, 1);
	assert_close (g, 0);
	assert_close (b, 0);
	assert_close (a, 128 / 255.0);

	/* Foreground color pattern */
	s->line.pattern = GO_PATTERN_FOREGROUND_SOLID;
	s->line.fore = GO_COLOR_FROM_RGBA (0, 0, 255, 255);
	g_assert_true (go_style_set_cairo_line (s, cr));
	cairo_pattern_get_rgba (cairo_get_source (cr), &r, &g, &b, &a);
	assert_close (r, 0);
	assert_close (b, 1);
	assert_close (a, 1);

	/* Dashes: scaled by the width, and with butt caps dots (dashes of
	   length zero) would not be seen so they are given some length */
	s->line.pattern = GO_PATTERN_SOLID;
	s->line.dash_type = GO_LINE_S_DOT;
	s->line.width = 2;
	s->line.cap = CAIRO_LINE_CAP_ROUND;
	g_assert_true (go_style_set_cairo_line (s, cr));
	{
		int n = cairo_get_dash_count (cr);
		double dashes[16], offset;
		GOLineDashSequence *seq = go_line_dash_get_sequence (GO_LINE_S_DOT, 2);
		int i;

		g_assert_cmpint (n, ==, (int)seq->n_dash);
		cairo_get_dash (cr, dashes, &offset);
		assert_near (offset, seq->offset);
		for (i = 0; i < n; i++)
			assert_close (dashes[i], seq->dash[i]);
		/* Round caps give a dot out of a zero-length dash */
		g_assert_cmpfloat (dashes[0], ==, 0);
		go_line_dash_sequence_free (seq);

		s->line.cap = CAIRO_LINE_CAP_BUTT;
		g_assert_true (go_style_set_cairo_line (s, cr));
		n = cairo_get_dash_count (cr);
		cairo_get_dash (cr, dashes, &offset);
		for (i = 0; i < n; i += 2)
			g_assert_cmpfloat (dashes[i], >, 0);
	}

	/* Back to a solid line: no dashes */
	s->line.dash_type = GO_LINE_SOLID;
	g_assert_true (go_style_set_cairo_line (s, cr));
	g_assert_cmpint (cairo_get_dash_count (cr), ==, 0);

	/* Other patterns are tiled in device pixels, however things are scaled */
	s->line.pattern = GO_PATTERN_HORIZ;
	s->line.fore = GO_COLOR_RED;
	s->line.color = GO_COLOR_BLUE;
	cairo_scale (cr, 2, 3);
	g_assert_true (go_style_set_cairo_line (s, cr));
	src = cairo_get_source (cr);
	g_assert_cmpint (cairo_pattern_get_type (src), ==, CAIRO_PATTERN_TYPE_SURFACE);
	{
		cairo_matrix_t m;
		cairo_pattern_get_matrix (src, &m);
		assert_close (m.xx, 2);
		assert_close (m.yy, 3);
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surf);
	g_object_unref (s);

	/* Bad arguments */
	s = go_style_new ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_cairo_line*");
	g_assert_false (go_style_set_cairo_line (NULL, NULL));
	g_test_assert_expected_messages ();
	g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*go_style_set_cairo_line*");
	g_assert_false (go_style_set_cairo_line (s, NULL));
	g_test_assert_expected_messages ();
	g_object_unref (s);
}

static void
test_style_cairo_hairline (void)
{
	/* A width of zero is a line one device pixel wide */
	static const struct { double sx, sy; double expected; } cases[] = {
		{ 1, 1, 1 }, { 4, 4, 0.25 }, { 0.5, 0.5, 2 }, { 2, 8, 0.25 },
		/* Mirrored axes make no difference */
		{ 4, -4, 0.25 }, { -4, 4, 0.25 }, { -2, -2, 0.5 }, { 1, -1, 1 }
	};
	GOStyle *s = go_style_new ();
	unsigned i;

	s->line.color = GO_COLOR_BLACK;
	s->line.width = 0;
	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
		cairo_t *cr = cairo_create (surf);

		cairo_scale (cr, cases[i].sx, cases[i].sy);
		g_assert_true (go_style_set_cairo_line (s, cr));
		assert_close (cairo_get_line_width (cr), cases[i].expected);
		cairo_destroy (cr);
		cairo_surface_destroy (surf);
	}

	/* Rotating doesn't change it */
	{
		cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
		cairo_t *cr = cairo_create (surf);
		cairo_scale (cr, 5, 5);
		cairo_rotate (cr, 0.7);
		g_assert_true (go_style_set_cairo_line (s, cr));
		assert_close (cairo_get_line_width (cr), 0.2);
		cairo_destroy (cr);
		cairo_surface_destroy (surf);
	}

	/* Anything positive is used as it is */
	{
		cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
		cairo_t *cr = cairo_create (surf);
		s->line.width = 0.001;
		cairo_scale (cr, 100, -100);
		g_assert_true (go_style_set_cairo_line (s, cr));
		assert_close (cairo_get_line_width (cr), 0.001);
		cairo_destroy (cr);
		cairo_surface_destroy (surf);
	}

	g_object_unref (s);
}

/* ---- saving and loading ---- */

static void
style_xml_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	go_persist_prep_sax (GO_PERSIST (xin->user_state), xin, attrs);
}

static gboolean
style_from_xml (GOStyle *style, const char *xml)
{
	static GsfXMLInNode const dtd[] = {
		GSF_XML_IN_NODE (ROOT, ROOT, -1, "Root", GSF_XML_NO_CONTENT, NULL, NULL),
		GSF_XML_IN_NODE (ROOT, STYLE, -1, "Style", GSF_XML_NO_CONTENT, &style_xml_start, NULL),
		GSF_XML_IN_NODE_END
	};
	static GsfXMLInDoc *doc = NULL;
	GsfInput *in;
	gboolean ok;

	if (!doc) {
		doc = gsf_xml_in_doc_new (dtd, NULL);
		go_xml_in_doc_dispose_on_exit (&doc);
	}
	in = gsf_input_memory_new ((const guint8 *)xml, strlen (xml), FALSE);
	ok = gsf_xml_in_doc_parse (doc, in, style);
	g_object_unref (in);
	return ok;
}

/* Parse the inside of a style element */
static GOStyle *
style_from_body (const char *body)
{
	GOStyle *s = go_style_new ();
	char *xml = g_strdup_printf ("<Root><Style type=\"GOStyle\">%s</Style></Root>", body);
	g_assert_true (style_from_xml (s, xml));
	g_free (xml);
	return s;
}

static char *
style_to_xml (GOStyle *style)
{
	GsfOutput *mem = gsf_output_memory_new ();
	GsfXMLOut *xml = gsf_xml_out_new (mem);
	char *res;

	gsf_xml_out_start_element (xml, "Root");
	gsf_xml_out_start_element (xml, "Style");
	go_persist_sax_save (GO_PERSIST (style), xml);
	gsf_xml_out_end_element (xml);
	gsf_xml_out_end_element (xml);
	g_object_unref (xml);
	gsf_output_close (mem);
	res = g_strndup ((const char *)gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (mem)),
			 gsf_output_size (mem));
	g_object_unref (mem);
	return res;
}

static GOStyle *
style_roundtrip (GOStyle *style)
{
	char *xml = style_to_xml (style);
	GOStyle *res = go_style_new ();
	/* Things that depend on which fields are of interest */
	res->interesting_fields = style->interesting_fields;
	g_assert_true (style_from_xml (res, xml));
	g_free (xml);
	return res;
}

static void
test_style_persist_roundtrip (void)
{
	GOStyle *s = make_custom_style (GO_TYPE_STYLE);
	GOStyle *r;

	/* Line, fill, font, as chosen in the custom style */
	r = style_roundtrip (s);
	g_assert_cmpint (r->line.dash_type, ==, GO_LINE_DASH);
	assert_close (r->line.width, 2.5);
	g_assert_cmphex (r->line.color, ==, s->line.color);
	g_assert_false (r->line.auto_dash);
	g_assert_false (r->line.auto_width);
	g_assert_false (r->line.auto_color);

	g_assert_cmpint (r->fill.type, ==, GO_STYLE_FILL_PATTERN);
	g_assert_cmpint (r->fill.pattern.pattern, ==, GO_PATTERN_HORIZ);
	g_assert_cmphex (r->fill.pattern.fore, ==, s->fill.pattern.fore);
	g_assert_cmphex (r->fill.pattern.back, ==, s->fill.pattern.back);
	g_assert_false (r->fill.auto_type);
	g_assert_false (r->fill.auto_fore);
	g_assert_false (r->fill.auto_back);
	g_assert_false (r->fill.auto_pattern);

	g_assert_true (r->font.font == s->font.font);
	g_assert_cmphex (r->font.color, ==, s->font.color);
	g_assert_false (r->font.auto_font);
	g_assert_false (r->font.auto_color);
	g_assert_false (r->font.auto_scale);

	/* The marker and text layout were of no interest, so are not saved */
	g_assert_cmpint (go_marker_get_shape (r->marker.mark), ==, GO_MARKER_NONE);
	assert_near (r->text_layout.angle, 0);
	g_object_unref (r);

	/* All of them */
	s->interesting_fields = GO_STYLE_LINE | GO_STYLE_FILL | GO_STYLE_MARKER | GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT;
	r = style_roundtrip (s);
	g_assert_cmpint (go_marker_get_shape (r->marker.mark), ==, GO_MARKER_CIRCLE);
	g_assert_cmpint (go_marker_get_size (r->marker.mark), ==, 9);
	g_assert_cmphex (go_marker_get_outline_color (r->marker.mark), ==, GO_COLOR_RED);
	g_assert_cmphex (go_marker_get_fill_color (r->marker.mark), ==, GO_COLOR_GREEN);
	g_assert_false (r->marker.auto_shape);
	g_assert_false (r->marker.auto_outline_color);
	g_assert_false (r->marker.auto_fill_color);
	assert_close (r->text_layout.angle, 45);
	g_assert_false (r->text_layout.auto_angle);
	g_object_unref (r);

	/* Automatic things are saved as such, without their values */
	go_style_force_auto (s);
	s->interesting_fields = GO_STYLE_ALL;
	r = style_roundtrip (s);
	g_assert_true (go_style_is_auto (r));
	g_assert_true (r->line.auto_dash);
	g_assert_true (r->font.auto_font);
	g_assert_true (r->text_layout.auto_angle);
	g_object_unref (r);

	g_object_unref (s);
}

static void
test_style_persist_gradient (void)
{
	GOStyle *s = go_style_new (), *r;
	char *xml;

	go_style_clear_auto (s);
	s->fill.type = GO_STYLE_FILL_GRADIENT;
	s->fill.gradient.dir = GO_GRADIENT_E_TO_W_MIRRORED;
	s->fill.pattern.back = GO_COLOR_FROM_RGBA (200, 100, 50, 255);
	go_style_set_fill_brightness (s, 25);

	/* With a brightness, the second color is derived from it */
	xml = style_to_xml (s);
	g_assert_nonnull (strstr (xml, " brightness=\""));
	g_assert_null (strstr (xml, "end-color"));
	g_free (xml);
	r = style_roundtrip (s);
	g_assert_cmpint (r->fill.type, ==, GO_STYLE_FILL_GRADIENT);
	g_assert_cmpint (r->fill.gradient.dir, ==, GO_GRADIENT_E_TO_W_MIRRORED);
	assert_close (r->fill.gradient.brightness, 25);
	g_assert_cmphex (r->fill.pattern.back, ==, s->fill.pattern.back);
	g_assert_cmphex (r->fill.pattern.fore, ==, s->fill.pattern.fore);
	g_assert_false (r->fill.gradient.auto_dir);
	g_assert_false (r->fill.gradient.auto_brightness);
	g_object_unref (r);

	/* Without, both colors are saved */
	s->fill.gradient.brightness = -1;
	s->fill.pattern.fore = GO_COLOR_FROM_RGBA (1, 2, 3, 255);
	xml = style_to_xml (s);
	g_assert_nonnull (strstr (xml, "end-color"));
	g_assert_null (strstr (xml, " brightness=\""));
	g_free (xml);
	r = style_roundtrip (s);
	g_assert_cmpint (r->fill.type, ==, GO_STYLE_FILL_GRADIENT);
	g_assert_cmphex (r->fill.pattern.fore, ==, GO_COLOR_FROM_RGBA (1, 2, 3, 255));
	g_assert_cmpfloat (r->fill.gradient.brightness, <, 0);
	g_object_unref (r);

	/* No fill is saved as such */
	s->fill.type = GO_STYLE_FILL_NONE;
	r = style_roundtrip (s);
	g_assert_cmpint (r->fill.type, ==, GO_STYLE_FILL_NONE);
	g_object_unref (r);

	g_object_unref (s);
}

static void
test_style_persist_format (void)
{
	GOStyle *s = go_style_new ();
	char *xml;

	/* The type is saved, and the line as "line" or, if that is all
	   there is, "outline" */
	s->interesting_fields = GO_STYLE_LINE;
	xml = style_to_xml (s);
	g_assert_nonnull (strstr (xml, "type=\"GOStyle\""));
	g_assert_nonnull (strstr (xml, "<line "));
	g_assert_null (strstr (xml, "<outline"));
	g_assert_null (strstr (xml, "<fill"));
	g_assert_null (strstr (xml, "<marker"));
	g_assert_null (strstr (xml, "<font"));
	g_free (xml);

	s->interesting_fields = GO_STYLE_OUTLINE;
	xml = style_to_xml (s);
	g_assert_nonnull (strstr (xml, "<outline"));
	g_assert_null (strstr (xml, "<line "));
	g_free (xml);

	/* Only one of them even if both are wanted */
	s->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_LINE;
	xml = style_to_xml (s);
	g_assert_nonnull (strstr (xml, "<line "));
	g_assert_null (strstr (xml, "<outline"));
	g_free (xml);

	s->interesting_fields = GO_STYLE_FILL;
	xml = style_to_xml (s);
	g_assert_nonnull (strstr (xml, "<fill"));
	g_assert_null (strstr (xml, "<line"));
	g_free (xml);

	s->interesting_fields = 0;
	xml = style_to_xml (s);
	g_assert_null (strstr (xml, "<line"));
	g_assert_null (strstr (xml, "<fill"));
	g_assert_null (strstr (xml, "<marker"));
	g_assert_null (strstr (xml, "<font"));
	g_assert_null (strstr (xml, "<text_layout"));
	g_free (xml);

	/* The type of a derived class */
	g_object_unref (s);
	s = g_object_new (TEST_TYPE_STYLE, NULL);
	xml = style_to_xml (s);
	g_assert_nonnull (strstr (xml, "type=\"TestStyle\""));
	g_free (xml);
	g_object_unref (s);
}

static void
test_style_load_line (void)
{
	GOStyle *s;

	s = style_from_body ("<line dash=\"dash\" width=\"2.5\" auto-width=\"false\" color=\"10:20:30:40\" auto-color=\"false\" auto-dash=\"false\"/>");
	g_assert_cmpint (s->line.dash_type, ==, GO_LINE_DASH);
	assert_close (s->line.width, 2.5);
	g_assert_cmphex (s->line.color, ==, GO_COLOR_FROM_RGBA (0x10, 0x20, 0x30, 0x40));
	g_assert_false (s->line.auto_dash);
	g_assert_false (s->line.auto_width);
	g_assert_false (s->line.auto_color);
	g_object_unref (s);

	/* "outline" is the same thing */
	s = style_from_body ("<outline width=\"1.5\" auto-width=\"false\"/>");
	assert_close (s->line.width, 1.5);
	g_object_unref (s);

	/* Old files: a negative width was how to say there is no line */
	s = style_from_body ("<line width=\"-1\"/>");
	g_assert_cmpint (s->line.dash_type, ==, GO_LINE_NONE);
	assert_near (s->line.width, 0);
	g_object_unref (s);

	/* Old files: without auto-width, a width is automatic if it is zero */
	s = style_from_body ("<line width=\"2\"/>");
	assert_close (s->line.width, 2);
	g_assert_false (s->line.auto_width);
	g_object_unref (s);
	s = style_from_body ("<line width=\"0\"/>");
	g_assert_true (s->line.auto_width);
	g_object_unref (s);
	/* ... and what is said outright is believed */
	s = style_from_body ("<line width=\"2\" auto-width=\"true\"/>");
	g_assert_true (s->line.auto_width);
	g_object_unref (s);

	/* Nothing in the element leaves the defaults */
	s = style_from_body ("<line/>");
	g_assert_true (s->line.auto_dash);
	g_assert_true (s->line.auto_width);
	g_assert_cmpint (s->line.dash_type, ==, GO_LINE_SOLID);
	g_object_unref (s);
}

static void
test_style_load_booleans (void)
{
	static const struct { const char *text; gboolean value; } cases[] = {
		{ "true", TRUE }, { "True", TRUE }, { "TRUE", TRUE }, { "t", TRUE },
		{ "yes", TRUE }, { "Y", TRUE }, { "1", TRUE }, { "2", TRUE }, { "0x10", TRUE },
		{ "false", FALSE }, { "False", FALSE }, { "no", FALSE }, { "0", FALSE },
		{ "f", FALSE }, { "n", FALSE }, { "", FALSE }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		char *body = g_strdup_printf ("<line auto-dash=\"%s\"/>", cases[i].text);
		GOStyle *s = go_style_new ();
		char *xml = g_strdup_printf ("<Root><Style type=\"GOStyle\">%s</Style></Root>", body);

		s->line.auto_dash = !cases[i].value;
		g_assert_true (style_from_xml (s, xml));
		g_assert_cmpint (s->line.auto_dash, ==, cases[i].value);
		g_free (xml);
		g_free (body);
		g_object_unref (s);
	}
}

static void
test_style_load_fill (void)
{
	GOStyle *s;

	s = style_from_body ("<fill type=\"pattern\" auto-type=\"false\" is-auto=\"false\" auto-fore=\"false\">"
			     "<pattern type=\"bricks\" fore=\"FF:0:0:FF\" back=\"0:0:FF:FF\" auto-pattern=\"false\"/></fill>");
	g_assert_cmpint (s->fill.type, ==, GO_STYLE_FILL_PATTERN);
	g_assert_cmpint (s->fill.pattern.pattern, ==, GO_PATTERN_BRICKS);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_RED);
	g_assert_cmphex (s->fill.pattern.back, ==, GO_COLOR_BLUE);
	g_assert_false (s->fill.auto_type);
	g_assert_false (s->fill.auto_back);
	g_assert_false (s->fill.auto_fore);
	g_assert_false (s->fill.auto_pattern);
	g_object_unref (s);

	/* Without further qualification it is no longer automatic */
	s = style_from_body ("<fill type=\"none\"/>");
	g_assert_cmpint (s->fill.type, ==, GO_STYLE_FILL_NONE);
	g_assert_false (s->fill.auto_type);
	g_object_unref (s);

	/* A type nobody knows is a pattern */
	s = style_from_body ("<fill type=\"sparkles\"/>");
	g_assert_cmpint (s->fill.type, ==, GO_STYLE_FILL_PATTERN);
	g_object_unref (s);

	/* Gradients */
	{
		char *body = g_strdup_printf ("<fill type=\"gradient\"><gradient direction=\"%s\" "
					      "start-color=\"FF:FF:FF:FF\" end-color=\"0:0:0:FF\" auto-direction=\"false\"/></fill>",
					      go_gradient_dir_as_str (GO_GRADIENT_SE_TO_NW_MIRRORED));
		s = style_from_body (body);
		g_free (body);
	}
	g_assert_cmpint (s->fill.type, ==, GO_STYLE_FILL_GRADIENT);
	g_assert_cmpint (s->fill.gradient.dir, ==, GO_GRADIENT_SE_TO_NW_MIRRORED);
	g_assert_cmphex (s->fill.pattern.back, ==, GO_COLOR_WHITE);
	g_assert_cmphex (s->fill.pattern.fore, ==, GO_COLOR_BLACK);
	g_assert_false (s->fill.gradient.auto_dir);
	g_object_unref (s);

	/* An image can't be had without the image, and so is no fill at all */
	s = style_from_body ("<fill type=\"image\"/>");
	g_assert_cmpint (s->fill.type, ==, GO_STYLE_FILL_NONE);
	g_object_unref (s);
}

static void
test_style_load_font_marker_text (void)
{
	GOStyle *s;
	const GOFont *f;

	/* Font by description, with a family if there is none */
	s = style_from_body ("<font auto-font=\"false\" font=\"Serif Bold 14\" auto-color=\"false\" color=\"FF:0:0:FF\" auto-scale=\"false\"/>");
	f = go_font_new_by_name ("Serif Bold 14");
	g_assert_true (s->font.font == f);
	go_font_unref (f);
	g_assert_false (s->font.auto_font);
	g_assert_false (s->font.auto_color);
	g_assert_false (s->font.auto_scale);
	g_assert_cmphex (s->font.color, ==, GO_COLOR_RED);
	g_object_unref (s);

	s = style_from_body ("<font auto-font=\"false\" font=\"Bold 12\"/>");
	f = go_font_new_by_name ("Sans Bold 12");
	g_assert_true (s->font.font == f);
	go_font_unref (f);
	g_object_unref (s);

	/* Old files: no auto-color, so a color that is not black was chosen */
	s = style_from_body ("<font color=\"FF:0:0:FF\"/>");
	g_assert_false (s->font.auto_color);
	g_object_unref (s);
	s = style_from_body ("<font color=\"0:0:0:FF\"/>");
	g_assert_true (s->font.auto_color);
	g_object_unref (s);

	/* Old files: font and auto-font, always saved, and no real choice
	   unless the font was not the default one */
	s = style_from_body ("<font auto-font=\"true\" font=\"Sans 8\"/>");
	g_assert_true (s->font.auto_font);
	g_object_unref (s);
	s = style_from_body ("<font auto-font=\"true\" font=\"Serif 20\"/>");
	g_assert_false (s->font.auto_font);
	g_object_unref (s);

	/* Markers */
	s = style_from_body ("<marker auto-shape=\"false\" shape=\"diamond\" auto-outline=\"false\" "
			     "outline-color=\"FF:0:0:FF\" auto-fill=\"false\" fill-color=\"0:FF:0:FF\" size=\"11\"/>");
	g_assert_cmpint (go_marker_get_shape (s->marker.mark), ==, GO_MARKER_DIAMOND);
	g_assert_cmpint (go_marker_get_size (s->marker.mark), ==, 11);
	g_assert_cmphex (go_marker_get_outline_color (s->marker.mark), ==, GO_COLOR_RED);
	g_assert_cmphex (go_marker_get_fill_color (s->marker.mark), ==, GO_COLOR_GREEN);
	g_assert_false (s->marker.auto_shape);
	g_assert_false (s->marker.auto_outline_color);
	g_assert_false (s->marker.auto_fill_color);
	g_object_unref (s);

	/* Bad colors don't change the ones there are */
	s = style_from_body ("<marker outline-color=\"nonsense\" fill-color=\"also nonsense\"/>");
	{
		GOMarker *def = go_marker_new ();
		g_assert_cmphex (go_marker_get_outline_color (s->marker.mark), ==, go_marker_get_outline_color (def));
		g_assert_cmphex (go_marker_get_fill_color (s->marker.mark), ==, go_marker_get_fill_color (def));
		g_object_unref (def);
	}
	g_object_unref (s);

	/* Text layout */
	s = style_from_body ("<text_layout angle=\"45\"/>");
	assert_close (s->text_layout.angle, 45);
	g_assert_false (s->text_layout.auto_angle);
	g_object_unref (s);
	s = style_from_body ("<text_layout angle=\"1000\"/>");
	assert_close (s->text_layout.angle, 180);
	g_object_unref (s);
	s = style_from_body ("<text_layout/>");
	g_assert_true (s->text_layout.auto_angle);
	g_object_unref (s);
}

static void
register_style_tests (void)
{
	g_test_add_func ("/utils/go-style/defaults", test_style_defaults);
	g_test_add_func ("/utils/go-style/auto", test_style_auto);
	g_test_add_func ("/utils/go-style/assign_dup", test_style_assign_dup);
	g_test_add_func ("/utils/go-style/assign_image", test_style_assign_image);
	g_test_add_func ("/utils/go-style/apply_theme", test_style_apply_theme);
	g_test_add_func ("/utils/go-style/visibility", test_style_visibility);
	g_test_add_func ("/utils/go-style/different_size", test_style_different_size);
	g_test_add_func ("/utils/go-style/setters", test_style_setters);
	g_test_add_func ("/utils/go-style/fill_brightness", test_style_fill_brightness);
	g_test_add_func ("/utils/go-style/fill/basic", test_style_fill_basic);
	g_test_add_func ("/utils/go-style/fill/gradient", test_style_fill_gradient);
	g_test_add_func ("/utils/go-style/fill/image", test_style_fill_image);
	g_test_add_func ("/utils/go-style/cairo_line", test_style_cairo_line);
	g_test_add_func ("/utils/go-style/cairo_hairline", test_style_cairo_hairline);
	g_test_add_func ("/utils/go-style/persist/roundtrip", test_style_persist_roundtrip);
	g_test_add_func ("/utils/go-style/persist/gradient", test_style_persist_gradient);
	g_test_add_func ("/utils/go-style/persist/format", test_style_persist_format);
	g_test_add_func ("/utils/go-style/load/line", test_style_load_line);
	g_test_add_func ("/utils/go-style/load/booleans", test_style_load_booleans);
	g_test_add_func ("/utils/go-style/load/fill", test_style_load_fill);
	g_test_add_func ("/utils/go-style/load/font_marker_text", test_style_load_font_marker_text);
}

/* ------------------------------------------------------------------ */
/* go-gradient.c                                                       */
/* ------------------------------------------------------------------ */

static void
test_gradient_names (void)
{
	int i, j;

	/* Names round-trip and are distinct */
	for (i = 0; i < GO_GRADIENT_MAX; i++) {
		const char *name = go_gradient_dir_as_str ((GOGradientDirection)i);
		g_assert_nonnull (name);
		g_assert_cmpuint (strlen (name), >, 0);
		g_assert_cmpint (go_gradient_dir_from_str (name), ==, i);
		for (j = 0; j < i; j++)
			g_assert_cmpstr (name, !=, go_gradient_dir_as_str ((GOGradientDirection)j));
	}

	/* Names that are not any direction's give the default direction */
	g_assert_cmpint (go_gradient_dir_from_str ("no-such-direction"), ==, GO_GRADIENT_N_TO_S);
	g_assert_cmpint (go_gradient_dir_from_str (""), ==, GO_GRADIENT_N_TO_S);
	{
		/* Case matters */
		char *up = g_ascii_strup (go_gradient_dir_as_str (GO_GRADIENT_W_TO_E), -1);
		g_assert_cmpint (go_gradient_dir_from_str (up), ==, GO_GRADIENT_N_TO_S);
		g_free (up);
	}

	/* Anything out of range gets one placeholder name which is not the
	   name of a direction, so it too reads back as the default */
	{
		const char *placeholder = go_gradient_dir_as_str (GO_GRADIENT_MAX);
		g_assert_nonnull (placeholder);
		g_assert_cmpstr (go_gradient_dir_as_str ((GOGradientDirection)-1), ==, placeholder);
		g_assert_cmpstr (go_gradient_dir_as_str ((GOGradientDirection)1000), ==, placeholder);
		for (i = 0; i < GO_GRADIENT_MAX; i++)
			g_assert_cmpstr (go_gradient_dir_as_str ((GOGradientDirection)i), !=, placeholder);
		g_assert_cmpint (go_gradient_dir_from_str (placeholder), ==, GO_GRADIENT_N_TO_S);
	}
}

static void
register_gradient_tests (void)
{
	g_test_add_func ("/utils/go-gradient/names", test_gradient_names);
}

/* ------------------------------------------------------------------ */
/* go-rsm.c                                                           */
/* ------------------------------------------------------------------ */

static void
test_rsm_lookup_missing (void)
{
	const char *id = "go:test-util/rsm/missing";
	size_t len = 123;

	g_assert_null (go_rsm_lookup (id, NULL));
	g_assert_null (go_rsm_lookup (id, &len));
	g_assert_cmpuint (len, ==, 123);
}

static void
test_rsm_register_lookup (void)
{
	static const char data[] = { 'r', 's', 'm', 0, 1, 2 };
	static const char empty[] = "";
	const char *data_id = "go:test-util/rsm/data";
	const char *empty_id = "go:test-util/rsm/empty";
	gconstpointer ptr;
	size_t len;

	go_rsm_register_file (data_id, data, sizeof (data));
	go_rsm_register_file (empty_id, empty, 0);

	ptr = go_rsm_lookup (data_id, &len);
	g_assert_true (ptr == data);
	g_assert_cmpuint (len, ==, sizeof (data));
	g_assert_cmpint (memcmp (ptr, data, sizeof (data)), ==, 0);

	ptr = go_rsm_lookup (data_id, NULL);
	g_assert_true (ptr == data);

	len = 123;
	ptr = go_rsm_lookup (empty_id, &len);
	g_assert_true (ptr == empty);
	g_assert_cmpuint (len, ==, 0);

	ptr = go_rsm_lookup (empty_id, NULL);
	g_assert_true (ptr == empty);

	go_rsm_unregister_file (data_id);
	go_rsm_unregister_file (empty_id);
}

static void
test_rsm_unregister (void)
{
	static const char one[] = "one";
	static const char two[] = "two";
	const char *one_id = "go:test-util/rsm/unregister-one";
	const char *two_id = "go:test-util/rsm/unregister-two";

	go_rsm_register_file (one_id, one, sizeof (one));
	go_rsm_register_file (two_id, two, sizeof (two));

	go_rsm_unregister_file (one_id);
	g_assert_null (go_rsm_lookup (one_id, NULL));
	g_assert_true (go_rsm_lookup (two_id, NULL) == two);

	go_rsm_unregister_file (two_id);
	g_assert_null (go_rsm_lookup (two_id, NULL));
	g_assert_null (go_rsm_lookup (one_id, NULL));
}

static void
test_rsm_copies_id (void)
{
	char id[64];
	const char *original = "go:test-util/rsm/copied-id";
	const char *changed = "go:test-util/rsm/changed-id";
	static const char data[] = "copied";

	g_assert_cmpuint (strlen (original) + 1, <=, sizeof (id));
	strcpy (id, original);
	go_rsm_register_file (id, data, sizeof (data));

	strcpy (id, changed);
	g_assert_null (go_rsm_lookup (id, NULL));
	g_assert_true (go_rsm_lookup (original, NULL) == data);

	go_rsm_unregister_file (original);
}

static void
register_rsm_tests (void)
{
	g_test_add_func ("/utils/go-rsm/lookup-missing", test_rsm_lookup_missing);
	g_test_add_func ("/utils/go-rsm/register-lookup", test_rsm_register_lookup);
	g_test_add_func ("/utils/go-rsm/unregister", test_rsm_unregister);
	g_test_add_func ("/utils/go-rsm/copies-id", test_rsm_copies_id);
}

/* ------------------------------------------------------------------ */
/* go-persist.c                                                       */
/* ------------------------------------------------------------------ */

typedef struct {
	GObject parent;
	int prepare_count;
	int save_count;
	GOPersist const *last_persist;
	gpointer prep_user_state;
	char *prepared_key;
	GsfXMLOut *last_xml_out;
	const char *message;
} TestPersist;

typedef struct {
	GObjectClass parent_class;
} TestPersistClass;

GType test_persist_get_type (void);
#define TEST_PERSIST(o) ((TestPersist *)(o))

static void test_persist_interface_init (GOPersistClass *iface);

G_DEFINE_TYPE_WITH_CODE (TestPersist, test_persist, G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (GO_TYPE_PERSIST, test_persist_interface_init))

static void
test_persist_iface_prep_sax (GOPersist *gp, GsfXMLIn *xin,
			     xmlChar const **attrs)
{
	TestPersist *self = TEST_PERSIST (gp);
	unsigned i;

	self->prepare_count++;
	self->last_persist = gp;
	self->prep_user_state = xin->user_state;

	if (!attrs)
		return;
	for (i = 0; attrs[i] != NULL; i += 2) {
		g_assert_nonnull (attrs[i + 1]);
		if (strcmp (CXML2C (attrs[i]), "key") == 0)
			self->prepared_key = g_strdup (CXML2C (attrs[i + 1]));
	}
}

static void
test_persist_iface_sax_save (GOPersist const *gp, GsfXMLOut *output)
{
	TestPersist *self = (TestPersist *)gp;

	self->save_count++;
	self->last_persist = gp;
	self->last_xml_out = output;
	gsf_xml_out_simple_element (output, "Message", self->message);
}

static void
test_persist_interface_init (GOPersistClass *iface)
{
	iface->prep_sax = test_persist_iface_prep_sax;
	iface->sax_save = test_persist_iface_sax_save;
}

static void
test_persist_class_init (TestPersistClass *klass)
{
	(void)klass;
}

static void
test_persist_init (TestPersist *self)
{
	self->message = "hello";
}

static void
test_persist_interface_type (void)
{
	GType type = go_persist_get_type ();
	TestPersist *obj;

	g_assert_true (type != G_TYPE_INVALID);
	g_assert_true (G_TYPE_IS_INTERFACE (type));
	g_assert_cmpstr (g_type_name (type), ==, "GOPersist");
	g_assert_true (go_persist_get_type () == type);

	obj = g_object_new (test_persist_get_type (), NULL);
	g_assert_true (GO_IS_PERSIST (obj));
	g_assert_true (G_TYPE_CHECK_INSTANCE_TYPE (obj, type));
	g_object_unref (obj);
}

static void
test_persist_sax_save (void)
{
	TestPersist *obj = g_object_new (test_persist_get_type (), NULL);
	GsfOutput *mem = gsf_output_memory_new ();
	GsfXMLOut *xml = gsf_xml_out_new (mem);
	char *saved;

	gsf_xml_out_start_element (xml, "Root");
	go_persist_sax_save (GO_PERSIST (obj), xml);
	gsf_xml_out_end_element (xml);

	g_assert_cmpint (obj->save_count, ==, 1);
	g_assert_true (obj->last_persist == GO_PERSIST (obj));
	g_assert_true (obj->last_xml_out == xml);

	g_object_unref (xml);
	gsf_output_close (mem);
	saved = g_strndup ((const char *)gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (mem)),
			   gsf_output_size (mem));
	g_assert_nonnull (strstr (saved, "<Message>hello</Message>"));

	g_free (saved);
	g_object_unref (mem);
	g_object_unref (obj);
}

static void
test_persist_xml_start (GsfXMLIn *xin, xmlChar const **attrs)
{
	go_persist_prep_sax (GO_PERSIST (xin->user_state), xin, attrs);
}

static void
test_persist_prep_sax (void)
{
	static GsfXMLInNode const dtd[] = {
		GSF_XML_IN_NODE (ROOT, ROOT, -1, "Root",
				 GSF_XML_NO_CONTENT, NULL, NULL),
		GSF_XML_IN_NODE (ROOT, PERSIST, -1, "Persist",
				 GSF_XML_NO_CONTENT, test_persist_xml_start, NULL),
		GSF_XML_IN_NODE_END
	};
	const char *xml = "<Root><Persist key=\"value\"/></Root>";
	GsfXMLInDoc *doc = gsf_xml_in_doc_new (dtd, NULL);
	GsfInput *in = gsf_input_memory_new ((const guint8 *)xml, strlen (xml), FALSE);
	TestPersist *obj = g_object_new (test_persist_get_type (), NULL);

	g_assert_true (gsf_xml_in_doc_parse (doc, in, obj));
	g_assert_cmpint (obj->prepare_count, ==, 1);
	g_assert_true (obj->last_persist == GO_PERSIST (obj));
	g_assert_true (obj->prep_user_state == obj);
	g_assert_cmpstr (obj->prepared_key, ==, "value");

	g_free (obj->prepared_key);
	g_object_unref (in);
	gsf_xml_in_doc_free (doc);
	g_object_unref (obj);
}

static void
register_persist_tests (void)
{
	g_test_add_func ("/utils/go-persist/interface-type",
			 test_persist_interface_type);
	g_test_add_func ("/utils/go-persist/sax-save", test_persist_sax_save);
	g_test_add_func ("/utils/go-persist/prep-sax", test_persist_prep_sax);
}

/* ------------------------------------------------------------------ */
/* go-pango-extras.c                                                  */
/* ------------------------------------------------------------------ */

typedef struct {
	int start, end;
} TestPangoRange;

static void
insert_pango_attr (PangoAttrList *list, PangoAttribute *attr,
		   int start, int end)
{
	attr->start_index = start;
	attr->end_index = end;
	pango_attr_list_insert (list, attr);
}

static void
add_pango_int_attr (PangoAttrList *list, PangoAttrType type, int value,
		    int start, int end)
{
	PangoAttribute *attr;

	switch (type) {
	case PANGO_ATTR_STYLE:
		attr = pango_attr_style_new (value);
		break;
	case PANGO_ATTR_WEIGHT:
		attr = pango_attr_weight_new (value);
		break;
	case PANGO_ATTR_UNDERLINE:
		attr = pango_attr_underline_new (value);
		break;
	case PANGO_ATTR_STRIKETHROUGH:
		attr = pango_attr_strikethrough_new (value);
		break;
	case PANGO_ATTR_RISE:
		attr = pango_attr_rise_new (value);
		break;
	case PANGO_ATTR_VARIANT:
		attr = pango_attr_variant_new (value);
		break;
	case PANGO_ATTR_LETTER_SPACING:
		attr = pango_attr_letter_spacing_new (value);
		break;
	case PANGO_ATTR_FALLBACK:
		attr = pango_attr_fallback_new (value);
		break;
	case PANGO_ATTR_GRAVITY:
		attr = pango_attr_gravity_new (value);
		break;
	case PANGO_ATTR_GRAVITY_HINT:
		attr = pango_attr_gravity_hint_new (value);
		break;
	default:
		g_assert_not_reached ();
	}
	insert_pango_attr (list, attr, start, end);
}

typedef struct {
	PangoAttrType type;
	GArray *ranges;
} CollectRangesData;

static gboolean
collect_pango_range (PangoAttribute *attr, gpointer data)
{
	CollectRangesData *cd = data;

	if (attr->klass->type == cd->type) {
		TestPangoRange range = { attr->start_index, attr->end_index };
		g_array_append_val (cd->ranges, range);
	}
	return FALSE;
}

static void
assert_pango_ranges (PangoAttrList *list, PangoAttrType type,
		     TestPangoRange const *expected, gsize n)
{
	CollectRangesData data;
	PangoAttrList *copy;
	gsize i;

	data.type = type;
	data.ranges = g_array_new (FALSE, FALSE, sizeof (TestPangoRange));
	copy = pango_attr_list_copy (list);
	(void)pango_attr_list_filter (copy, collect_pango_range, &data);
	pango_attr_list_unref (copy);

	g_assert_cmpuint (data.ranges->len, ==, n);
	for (i = 0; i < n; i++) {
		TestPangoRange const *got =
			&g_array_index (data.ranges, TestPangoRange, i);
		g_assert_cmpint (got->start, ==, expected[i].start);
		g_assert_cmpint (got->end, ==, expected[i].end);
	}
	g_array_free (data.ranges, TRUE);
}

typedef struct {
	PangoAttrType type;
	gboolean found;
	int ival;
	double dval;
	int start, end;
} FindAttrData;

static gboolean
find_pango_attr_value (PangoAttribute *attr, gpointer data)
{
	FindAttrData *fd = data;

	if (attr->klass->type != fd->type)
		return FALSE;

	fd->found = TRUE;
	fd->start = attr->start_index;
	fd->end = attr->end_index;
	if (fd->type == PANGO_ATTR_SCALE)
		fd->dval = ((PangoAttrFloat *)attr)->value;
	else
		fd->ival = ((PangoAttrInt *)attr)->value;
	return FALSE;
}

static FindAttrData
find_pango_attr (PangoAttrList *list, PangoAttrType type)
{
	FindAttrData data = { type, FALSE, 0, 0, 0, 0 };
	PangoAttrList *copy = pango_attr_list_copy (list);

	(void)pango_attr_list_filter (copy, find_pango_attr_value, &data);
	pango_attr_list_unref (copy);
	return data;
}

typedef struct {
	PangoAttrType type;
	gboolean found;
} FindTypeData;

static gboolean
find_pango_attr_type (PangoAttribute *attr, gpointer data)
{
	FindTypeData *fd = data;

	if (attr->klass->type == fd->type)
		fd->found = TRUE;
	return FALSE;
}

static gboolean
pango_list_has_type (PangoAttrList *list, PangoAttrType type)
{
	FindTypeData data = { type, FALSE };
	PangoAttrList *copy = pango_attr_list_copy (list);

	(void)pango_attr_list_filter (copy, find_pango_attr_type, &data);
	pango_attr_list_unref (copy);
	return data.found;
}

static PangoAttrList *
make_pango_splice_list (void)
{
	PangoAttrList *list = pango_attr_list_new ();

	add_pango_int_attr (list, PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, 0, 10);
	add_pango_int_attr (list, PANGO_ATTR_UNDERLINE,
			    PANGO_UNDERLINE_SINGLE, 2, 5);
	add_pango_int_attr (list, PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC, 6, 8);
	add_pango_int_attr (list, PANGO_ATTR_STRIKETHROUGH, TRUE, 0, 2);
	return list;
}

static void
test_pango_attr_types (void)
{
	PangoAttrType sub = go_pango_attr_subscript_get_attr_type ();
	PangoAttrType sup = go_pango_attr_superscript_get_attr_type ();
	PangoAttribute *attr, *copy;

	g_assert_true (sub != PANGO_ATTR_INVALID);
	g_assert_true (sup != PANGO_ATTR_INVALID);
	g_assert_true (sub != sup);
	g_assert_true (go_pango_attr_subscript_get_attr_type () == sub);
	g_assert_true (go_pango_attr_superscript_get_attr_type () == sup);

	attr = go_pango_attr_subscript_new (TRUE);
	g_assert_cmpint (attr->klass->type, ==, sub);
	g_assert_true (((GOPangoAttrSubscript *)attr)->val);
	copy = pango_attribute_copy (attr);
	g_assert_true (copy != attr);
	g_assert_true (pango_attribute_equal (attr, copy));
	((GOPangoAttrSubscript *)copy)->val = FALSE;
	g_assert_false (pango_attribute_equal (attr, copy));
	pango_attribute_destroy (copy);
	pango_attribute_destroy (attr);

	attr = go_pango_attr_superscript_new (FALSE);
	g_assert_cmpint (attr->klass->type, ==, sup);
	g_assert_false (((GOPangoAttrSuperscript *)attr)->val);
	copy = pango_attribute_copy (attr);
	g_assert_true (pango_attribute_equal (attr, copy));
	((GOPangoAttrSuperscript *)copy)->val = TRUE;
	g_assert_false (pango_attribute_equal (attr, copy));
	pango_attribute_destroy (copy);
	pango_attribute_destroy (attr);
}

static void
test_pango_attr_list_is_empty (void)
{
	PangoAttrList *list = pango_attr_list_new ();

	g_assert_true (go_pango_attr_list_is_empty (NULL));
	g_assert_true (go_pango_attr_list_is_empty (list));

	add_pango_int_attr (list, PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, 0, 3);
	g_assert_false (go_pango_attr_list_is_empty (list));

	pango_attr_list_unref (list);
}

static void
test_pango_open_hole (void)
{
	PangoAttrList *list = make_pango_splice_list ();
	TestPangoRange const weight[] = { { 0, 4 }, { 7, 13 } };
	TestPangoRange const underline[] = { { 2, 4 }, { 7, 8 } };
	TestPangoRange const italic[] = { { 9, 11 } };
	TestPangoRange const strike[] = { { 0, 2 } };

	go_pango_attr_list_open_hole (list, 4, 3);

	assert_pango_ranges (list, PANGO_ATTR_WEIGHT,
			     weight, G_N_ELEMENTS (weight));
	assert_pango_ranges (list, PANGO_ATTR_UNDERLINE,
			     underline, G_N_ELEMENTS (underline));
	assert_pango_ranges (list, PANGO_ATTR_STYLE,
			     italic, G_N_ELEMENTS (italic));
	assert_pango_ranges (list, PANGO_ATTR_STRIKETHROUGH,
			     strike, G_N_ELEMENTS (strike));

	pango_attr_list_unref (list);
}

static void
test_pango_erase (void)
{
	PangoAttrList *list = make_pango_splice_list ();
	TestPangoRange const weight[] = { { 0, 7 } };
	TestPangoRange const underline[] = { { 2, 4 } };
	TestPangoRange const italic[] = { { 4, 5 } };
	TestPangoRange const strike[] = { { 0, 2 } };

	go_pango_attr_list_erase (list, 4, 3);

	assert_pango_ranges (list, PANGO_ATTR_WEIGHT,
			     weight, G_N_ELEMENTS (weight));
	assert_pango_ranges (list, PANGO_ATTR_UNDERLINE,
			     underline, G_N_ELEMENTS (underline));
	assert_pango_ranges (list, PANGO_ATTR_STYLE,
			     italic, G_N_ELEMENTS (italic));
	assert_pango_ranges (list, PANGO_ATTR_STRIKETHROUGH,
			     strike, G_N_ELEMENTS (strike));

	pango_attr_list_unref (list);
}

static void
test_pango_unset (void)
{
	PangoAttrList *list = make_pango_splice_list ();
	TestPangoRange const weight[] = { { 0, 3 }, { 6, 10 } };
	TestPangoRange const underline[] = { { 2, 5 } };
	TestPangoRange const italic[] = { { 6, 8 } };
	TestPangoRange const strike[] = { { 0, 2 } };

	go_pango_attr_list_unset (list, 3, 6, PANGO_ATTR_WEIGHT);

	assert_pango_ranges (list, PANGO_ATTR_WEIGHT,
			     weight, G_N_ELEMENTS (weight));
	assert_pango_ranges (list, PANGO_ATTR_UNDERLINE,
			     underline, G_N_ELEMENTS (underline));
	assert_pango_ranges (list, PANGO_ATTR_STYLE,
			     italic, G_N_ELEMENTS (italic));
	assert_pango_ranges (list, PANGO_ATTR_STRIKETHROUGH,
			     strike, G_N_ELEMENTS (strike));

	pango_attr_list_unref (list);
}

static void
test_pango_translate_attributes (void)
{
	PangoAttrList *attrs, *translated;
	PangoAttribute *attr;
	FindAttrData scale, rise;

	attrs = pango_attr_list_new ();
	attr = go_pango_attr_superscript_new (TRUE);
	insert_pango_attr (attrs, attr, 1, 3);
	translated = go_pango_translate_attributes (attrs);

	g_assert_true (translated != attrs);
	g_assert_true (pango_list_has_type (attrs,
					    go_pango_attr_superscript_get_attr_type ()));
	g_assert_false (pango_list_has_type (translated,
					     go_pango_attr_superscript_get_attr_type ()));
	scale = find_pango_attr (translated, PANGO_ATTR_SCALE);
	rise = find_pango_attr (translated, PANGO_ATTR_RISE);
	g_assert_true (scale.found);
	g_assert_true (rise.found);
	g_assert_cmpint (scale.start, ==, 1);
	g_assert_cmpint (scale.end, ==, 3);
	assert_close (scale.dval, GO_SUPERSCRIPT_SCALE);
	g_assert_cmpint (rise.start, ==, 1);
	g_assert_cmpint (rise.end, ==, 3);
	g_assert_cmpint (rise.ival, ==, GO_SUPERSCRIPT_RISE);

	pango_attr_list_unref (translated);
	pango_attr_list_unref (attrs);

	attrs = pango_attr_list_new ();
	attr = go_pango_attr_subscript_new (TRUE);
	insert_pango_attr (attrs, attr, 1, 3);
	translated = go_pango_translate_attributes (attrs);

	scale = find_pango_attr (translated, PANGO_ATTR_SCALE);
	rise = find_pango_attr (translated, PANGO_ATTR_RISE);
	g_assert_true (scale.found);
	g_assert_true (rise.found);
	assert_close (scale.dval, GO_SUBSCRIPT_SCALE);
	g_assert_cmpint (rise.ival, ==, GO_SUBSCRIPT_RISE);

	pango_attr_list_unref (translated);
	pango_attr_list_unref (attrs);

	attrs = pango_attr_list_new ();
	add_pango_int_attr (attrs, PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, 0, 3);
	translated = go_pango_translate_attributes (attrs);
	g_assert_true (translated == attrs);
	pango_attr_list_unref (translated);
	pango_attr_list_unref (attrs);
}

static void
test_pango_translate_layout (void)
{
	PangoContext *context = test_pango_context ();
	PangoLayout *layout = pango_layout_new (context);
	PangoAttrList *attrs = pango_attr_list_new ();
	FindAttrData rise;

	insert_pango_attr (attrs, go_pango_attr_subscript_new (TRUE), 1, 2);
	pango_layout_set_text (layout, "abc", -1);
	pango_layout_set_attributes (layout, attrs);
	pango_attr_list_unref (attrs);

	go_pango_translate_layout (layout);

	attrs = pango_layout_get_attributes (layout);
	g_assert_nonnull (attrs);
	g_assert_false (pango_list_has_type (attrs,
					     go_pango_attr_subscript_get_attr_type ()));
	rise = find_pango_attr (attrs, PANGO_ATTR_RISE);
	g_assert_true (rise.found);
	g_assert_cmpint (rise.ival, ==, GO_SUBSCRIPT_RISE);

	g_object_unref (layout);
	g_object_unref (context);
}

static void
test_pango_attrs_to_markup (void)
{
	static struct {
		PangoAttrType type;
		int value;
		char const *markup;
	} const cases[] = {
		{ PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
		  "<span font_style=\"italic\">abc</span>" },
		{ PANGO_ATTR_STYLE, PANGO_STYLE_OBLIQUE,
		  "<span font_style=\"oblique\">abc</span>" },
		{ PANGO_ATTR_STYLE, PANGO_STYLE_NORMAL,
		  "<span font_style=\"normal\">abc</span>" },
		{ PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
		  "<span font_weight=\"700\">abc</span>" },
		{ PANGO_ATTR_STRIKETHROUGH, TRUE,
		  "<span strikethrough=\"true\">abc</span>" },
		{ PANGO_ATTR_STRIKETHROUGH, FALSE,
		  "<span strikethrough=\"false\">abc</span>" },
		{ PANGO_ATTR_UNDERLINE, PANGO_UNDERLINE_SINGLE,
		  "<span underline=\"single\">abc</span>" },
		{ PANGO_ATTR_UNDERLINE, PANGO_UNDERLINE_NONE,
		  "<span underline=\"none\">abc</span>" },
		{ PANGO_ATTR_RISE, 1234,
		  "<span rise=\"1234\">abc</span>" },
		{ PANGO_ATTR_VARIANT, PANGO_VARIANT_SMALL_CAPS,
		  "<span font_variant=\"smallcaps\">abc</span>" },
		{ PANGO_ATTR_LETTER_SPACING, 100,
		  "<span letter_spacing=\"100\">abc</span>" },
		{ PANGO_ATTR_FALLBACK, FALSE,
		  "<span fallback=\"false\">abc</span>" },
		{ PANGO_ATTR_GRAVITY, PANGO_GRAVITY_EAST,
		  "<span gravity=\"east\">abc</span>" },
		{ PANGO_ATTR_GRAVITY_HINT, PANGO_GRAVITY_HINT_STRONG,
		  "<span gravity_hint=\"strong\">abc</span>" }
	};
	PangoAttrList *attrs;
	PangoFontDescription *desc;
	PangoColor color = { 0xffff, 0, 0 };
	char *markup, *expected, *str;
	unsigned i;

	g_assert_null (go_pango_attrs_to_markup (NULL, NULL));

	attrs = pango_attr_list_new ();
	markup = go_pango_attrs_to_markup (attrs, "a<b&c");
	g_assert_cmpstr (markup, ==, "a&lt;b&amp;c");
	g_free (markup);
	pango_attr_list_unref (attrs);

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		attrs = pango_attr_list_new ();
		add_pango_int_attr (attrs, cases[i].type, cases[i].value, 0, 3);
		markup = go_pango_attrs_to_markup (attrs, "abc");
		g_assert_cmpstr (markup, ==, cases[i].markup);
		g_free (markup);
		pango_attr_list_unref (attrs);
	}

	attrs = pango_attr_list_new ();
	insert_pango_attr (attrs, pango_attr_family_new ("Serif <&>"), 0, 3);
	markup = go_pango_attrs_to_markup (attrs, "abc");
	g_assert_cmpstr (markup, ==,
			 "<span font_family=\"Serif &lt;&amp;&gt;\">abc</span>");
	g_free (markup);
	pango_attr_list_unref (attrs);

	attrs = pango_attr_list_new ();
	insert_pango_attr (attrs, pango_attr_size_new (12 * PANGO_SCALE), 0, 3);
	markup = go_pango_attrs_to_markup (attrs, "abc");
	expected = g_strdup_printf ("<span font_size=\"%i\">abc</span>",
				    12 * PANGO_SCALE);
	g_assert_cmpstr (markup, ==, expected);
	g_free (expected);
	g_free (markup);
	pango_attr_list_unref (attrs);

	attrs = pango_attr_list_new ();
	insert_pango_attr (attrs, pango_attr_foreground_new (color.red,
							     color.green,
							     color.blue), 0, 3);
	markup = go_pango_attrs_to_markup (attrs, "abc");
	str = pango_color_to_string (&color);
	expected = g_strdup_printf ("<span foreground=\"%s\">abc</span>", str);
	g_free (str);
	g_assert_cmpstr (markup, ==, expected);
	g_free (expected);
	g_free (markup);
	pango_attr_list_unref (attrs);

	attrs = pango_attr_list_new ();
	desc = pango_font_description_from_string ("Sans Bold 12");
	insert_pango_attr (attrs, pango_attr_font_desc_new (desc), 0, 3);
	str = pango_font_description_to_string (desc);
	{
		char *escaped = g_markup_escape_text (str, -1);
		expected = g_strdup_printf ("<span font_desc=\"%s\">abc</span>",
					    escaped);
		g_free (escaped);
	}
	g_free (str);
	markup = go_pango_attrs_to_markup (attrs, "abc");
	g_assert_cmpstr (markup, ==, expected);
	g_free (expected);
	g_free (markup);
	pango_font_description_free (desc);
	pango_attr_list_unref (attrs);

	/* Unsupported attributes are ignored.  */
	attrs = pango_attr_list_new ();
	insert_pango_attr (attrs, pango_attr_scale_new (0.5), 0, 3);
	markup = go_pango_attrs_to_markup (attrs, "abc");
	g_assert_cmpstr (markup, ==, "abc");
	g_free (markup);
	pango_attr_list_unref (attrs);

	attrs = pango_attr_list_new ();
	insert_pango_attr (attrs, go_pango_attr_superscript_new (TRUE), 0, 3);
	markup = go_pango_attrs_to_markup (attrs, "abc");
	g_assert_cmpstr (markup, ==, "abc");
	g_free (markup);
	pango_attr_list_unref (attrs);
}

static void
test_pango_measure_string (void)
{
	PangoContext *context = test_pango_context ();
	PangoFontDescription *desc = pango_font_description_from_string ("Sans 12");
	int empty = go_pango_measure_string (context, desc, "");
	int one = go_pango_measure_string (context, desc, "i");
	int many = go_pango_measure_string (context, desc, "iiiiiiii");

	g_assert_cmpint (empty, >=, 0);
	g_assert_cmpint (empty, <, one);
	g_assert_cmpint (many, >, one);

	pango_font_description_free (desc);
	g_object_unref (context);
}

#ifdef GOFFICE_WITH_GTK
static void
test_pango_gtk_buffer (void)
{
	GtkTextBuffer *buffer;
	GtkTextTagTable *table;
	GtkTextTag *bold, *italic;
	GtkTextIter iter;
	PangoAttrList *attrs = pango_attr_list_new ();

	buffer = gtk_text_buffer_new (NULL);
	table = gtk_text_buffer_get_tag_table (buffer);

	go_create_std_tags_for_buffer (buffer);
	bold = gtk_text_tag_table_lookup (table, "PANGO_WEIGHT_BOLD");
	italic = gtk_text_tag_table_lookup (table, "PANGO_STYLE_ITALIC");
	g_assert_nonnull (bold);
	g_assert_nonnull (italic);

	/* "a\303\251bc": bytes 1..3 are the single character "é".  */
	gtk_text_buffer_set_text (buffer, "a\303\251bc", -1);
	insert_pango_attr (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD),
			   1, 3);
	insert_pango_attr (attrs, pango_attr_style_new (PANGO_STYLE_ITALIC),
			   0, 1);
	go_load_pango_attributes_into_buffer (attrs, buffer, "a\303\251bc");

	gtk_text_buffer_get_iter_at_offset (buffer, &iter, 1);
	g_assert_true (gtk_text_iter_has_tag (&iter, bold));
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
	g_assert_true (gtk_text_iter_has_tag (&iter, italic));

	pango_attr_list_unref (attrs);
	g_object_unref (buffer);
}
#endif

static void
register_pango_extras_tests (void)
{
	g_test_add_func ("/utils/go-pango-extras/attr-types",
			 test_pango_attr_types);
	g_test_add_func ("/utils/go-pango-extras/attr-list-is-empty",
			 test_pango_attr_list_is_empty);
	g_test_add_func ("/utils/go-pango-extras/open-hole", test_pango_open_hole);
	g_test_add_func ("/utils/go-pango-extras/erase", test_pango_erase);
	g_test_add_func ("/utils/go-pango-extras/unset", test_pango_unset);
	g_test_add_func ("/utils/go-pango-extras/translate-attributes",
			 test_pango_translate_attributes);
	g_test_add_func ("/utils/go-pango-extras/translate-layout",
			 test_pango_translate_layout);
	g_test_add_func ("/utils/go-pango-extras/attrs-to-markup",
			 test_pango_attrs_to_markup);
	g_test_add_func ("/utils/go-pango-extras/measure-string",
			 test_pango_measure_string);
#ifdef GOFFICE_WITH_GTK
	g_test_add_func ("/utils/go-pango-extras/gtk-buffer",
			 test_pango_gtk_buffer);
#endif
}

/* ------------------------------------------------------------------ */
/* go-file.c                                                          */
/* ------------------------------------------------------------------ */

static char *
test_file_make_uri (char const *dir, char const *base, char const *contents,
		    char **path_out)
{
	char *path = g_build_filename (dir, base, NULL);
	char *uri;

	g_assert_true (g_file_set_contents (path, contents, -1, NULL));
	uri = go_filename_to_uri (path);
	g_assert_nonnull (uri);
	*path_out = path;
	return uri;
}

static void
test_file_filename_simplify (void)
{
	char *s, *dir, *sub, *path, *other, *expected;

	s = go_filename_simplify ("/a/b//./c", GO_DOTDOT_SYNTACTIC, FALSE);
	g_assert_cmpstr (s, ==, "/a/b/c");
	g_free (s);

	s = go_filename_simplify ("/../a", GO_DOTDOT_SYNTACTIC, FALSE);
	g_assert_cmpstr (s, ==, "/a");
	g_free (s);

	s = go_filename_simplify ("/a/b/../c", GO_DOTDOT_SYNTACTIC, FALSE);
	g_assert_cmpstr (s, ==, "/a/c");
	g_free (s);

	s = go_filename_simplify ("/a/b/../c", GO_DOTDOT_LEAVE, FALSE);
	g_assert_cmpstr (s, ==, "/a/b/../c");
	g_free (s);

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	sub = g_build_filename (dir, "sub", NULL);
	g_assert_cmpint (g_mkdir (sub, 0700), ==, 0);

	path = g_build_filename (dir, "sub", "..", "file", NULL);
	expected = g_build_filename (dir, "file", NULL);
	s = go_filename_simplify (path, GO_DOTDOT_TEST, FALSE);
	g_assert_cmpstr (s, ==, expected);
	g_free (s);
	g_free (expected);
	g_free (path);

	other = g_build_filename (dir, "missing", "..", "file", NULL);
	s = go_filename_simplify (other, GO_DOTDOT_TEST, FALSE);
	g_assert_cmpstr (s, ==, other);
	g_free (s);
	g_free (other);

	g_rmdir (sub);
	g_rmdir (dir);
	g_free (sub);
	g_free (dir);

#ifndef G_OS_WIN32
	{
		char *cwd = g_get_current_dir ();
		expected = g_build_filename (cwd, "x", "y", NULL);
		s = go_filename_simplify ("x//./y", GO_DOTDOT_LEAVE, TRUE);
		g_assert_cmpstr (s, ==, expected);
		g_free (s);
		g_free (expected);
		g_free (cwd);
	}
#endif
}

static void
test_file_url_simplify (void)
{
	static struct {
		char const *uri;
		char const *result;
	} const cases[] = {
		{ "http://host/./a", "http://host/a" },
		{ "http://host/././a", "http://host/a" },
		{ "HTTP://example.com/a/./b", "http://example.com/a/b" },
		{ "https://EXAMPLE.com/./a/./b", "https://EXAMPLE.com/a/b" },
		{ "FTP://host/a//b/./c", "ftp://host/a/b/c" },
		{ "file:///tmp/./x", "file:///tmp/x" },
		{ "file:///tmp//./x", "file:///tmp/x" },
		/* ".." is left alone: an intermediate component may be a symlink.  */
		{ "http://host/../a", "http://host/../a" },
		{ "http://host/a/../b", "http://host/a/../b" }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		char *result = go_url_simplify (cases[i].uri);
		g_assert_cmpstr (result, ==, cases[i].result);
		g_free (result);
	}
}

static void
test_file_url_relative (void)
{
	static struct {
		char const *ref, *rel, *result;
	} const resolve_cases[] = {
		{ "http://example.com/a/b/c", "./d",
		  "http://example.com/a/b/d" },
		{ "http://example.com/a/b/c", "d",
		  "http://example.com/a/b/d" },
		{ "http://example.com/a/b/c", "../d",
		  "http://example.com/a/b/../d" },
		{ "a/b", "c", "a/c" }
	};
	static struct {
		char const *uri, *ref, *result;
	} const relative_cases[] = {
		{ "http://example.com/a/b/c", "http://example.com/a/x/y",
		  "../b/c" },
		{ "http://example.com/a/b/c", "http://example.com/a/b/c",
		  "c" },
		{ "file:///a/b/c", "file:///a/x/y", "../b/c" },
		{ "http://example.com/a", "http://other.com/a", NULL },
		{ "https://example.com/a", "http://example.com/a", NULL }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (resolve_cases); i++) {
		char *result = go_url_resolve_relative (resolve_cases[i].ref,
							resolve_cases[i].rel);
		g_assert_cmpstr (result, ==, resolve_cases[i].result);
		g_free (result);
	}

	for (i = 0; i < G_N_ELEMENTS (relative_cases); i++) {
		char *result = go_url_make_relative (relative_cases[i].uri,
						    relative_cases[i].ref);
		g_assert_cmpstr (result, ==, relative_cases[i].result);
		g_free (result);
	}
}

static void
test_file_uri_conversions (void)
{
	char *dir, *path, *uri, *expected, *roundtrip;

#ifndef G_OS_WIN32
	uri = go_filename_from_uri ("file:///tmp/a%20b.txt");
	g_assert_cmpstr (uri, ==, "/tmp/a b.txt");
	g_free (uri);
#endif

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	path = g_build_filename (dir, "plain.txt", NULL);

	uri = go_filename_to_uri (path);
	expected = g_filename_to_uri (path, NULL, NULL);
	g_assert_cmpstr (uri, ==, expected);
	roundtrip = go_filename_from_uri (uri);
	g_assert_cmpstr (roundtrip, ==, path);

	g_free (roundtrip);
	g_free (expected);
	g_free (uri);
	g_free (path);
	g_rmdir (dir);
	g_free (dir);
}

static void
test_file_shell_arg_to_uri (void)
{
	char *uri, *dir, *expected;

	uri = go_shell_arg_to_uri ("fd://17");
	g_assert_cmpstr (uri, ==, "fd://17");
	g_free (uri);

	uri = go_shell_arg_to_uri ("fd://17/");
	g_assert_cmpstr (uri, ==, "fd://17/");
	g_free (uri);

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = go_shell_arg_to_uri (dir);
	expected = go_filename_to_uri (dir);
	g_assert_cmpstr (uri, ==, expected);
	g_free (expected);
	g_free (uri);
	g_rmdir (dir);
	g_free (dir);
}

static void
test_file_uri_parts (void)
{
	char *dir, *path, *uri, *result, *expected;

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = test_file_make_uri (dir, "a b.txt", "some text\n", &path);

	result = go_basename_from_uri (uri);
	g_assert_cmpstr (result, ==, "a b.txt");
	g_free (result);

	result = go_dirname_from_uri (uri, TRUE);
	g_assert_cmpstr (result, ==, dir);
	g_free (result);

	expected = g_strconcat ("file://", dir, NULL);
	result = go_dirname_from_uri (uri, FALSE);
	g_assert_cmpstr (result, ==, expected);
	g_free (expected);
	g_free (result);

	g_remove (path);
	g_rmdir (dir);
	g_free (path);
	g_free (uri);
	g_free (dir);
}

static void
test_file_open_create (void)
{
	static char const text[] = "go-file test data\n";
	char *dir, *path, *uri, *missing_path, *missing_uri;
	gsize len = sizeof (text) - 1;
	GError *err = NULL;
	GsfOutput *output;
	GsfInput *input;
	guint8 const *data;

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = test_file_make_uri (dir, "data.txt", text, &path);

	output = go_file_create (uri, &err);
	g_assert_no_error (err);
	g_assert_nonnull (output);
	g_assert_true (gsf_output_write (output, len, (guint8 const *)text));
	g_assert_true (gsf_output_close (output));
	g_object_unref (output);

	input = go_file_open (uri, &err);
	g_assert_no_error (err);
	g_assert_nonnull (input);
	g_assert_cmpint ((gint64)gsf_input_size (input), ==, (gint64)len);
	data = gsf_input_read (input, len, NULL);
	g_assert_nonnull (data);
	g_assert_cmpmem (data, len, text, len);
	g_object_unref (input);

	missing_path = g_build_filename (dir, "missing.txt", NULL);
	missing_uri = go_filename_to_uri (missing_path);
	input = go_file_open (missing_uri, &err);
	g_assert_null (input);
	g_assert_nonnull (err);
	g_clear_error (&err);

	g_free (missing_uri);
	g_free (missing_path);
	g_remove (path);
	g_rmdir (dir);
	g_free (path);
	g_free (uri);
	g_free (dir);
}

static void
test_file_split_urls (void)
{
	static char const data[] =
		"# a comment\r\n"
		"  http://example.com/a  \r\n"
		"\n"
		"file:///tmp/b\n"
		"   \n"
		"http://example.com/c";
	GSList *urls = go_file_split_urls (data);

	g_assert_cmpuint (g_slist_length (urls), ==, 3);
	g_assert_cmpstr (g_slist_nth_data (urls, 0), ==, "http://example.com/a");
	g_assert_cmpstr (g_slist_nth_data (urls, 1), ==, "file:///tmp/b");
	g_assert_cmpstr (g_slist_nth_data (urls, 2), ==, "http://example.com/c");
	g_slist_free_full (urls, g_free);
}

static void
test_file_owner_group (void)
{
	char *dir, *path, *uri, *owner, *group;

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = test_file_make_uri (dir, "owner.txt", "x", &path);

	owner = go_file_get_owner_name (uri);
	group = go_file_get_group_name (uri);

	if (owner == NULL && group == NULL) {
		g_test_skip ("owner and group names are unavailable");
	} else {
		if (owner != NULL) {
			g_assert_true (g_utf8_validate (owner, -1, NULL));
			g_assert_cmpstr (owner, ==, g_get_user_name ());
		}
		if (group != NULL) {
			g_assert_true (g_utf8_validate (group, -1, NULL));
			g_assert_cmpuint (strlen (group), >, 0);
		}
	}

	g_free (group);
	g_free (owner);
	g_remove (path);
	g_rmdir (dir);
	g_free (path);
	g_free (uri);
	g_free (dir);
}

static void
test_file_permissions (void)
{
	GOFilePermissions boxed = {
		TRUE, FALSE, TRUE,
		FALSE, TRUE, FALSE,
		TRUE, FALSE, TRUE
	};
	GOFilePermissions wanted = {
		TRUE, TRUE, FALSE,
		TRUE, FALSE, FALSE,
		FALSE, FALSE, FALSE
	};
	GType type = go_file_permissions_get_type ();
	GValue value = G_VALUE_INIT, copy = G_VALUE_INIT;
	GOFilePermissions *got;
	char *dir, *path, *uri;

	g_assert_true (G_TYPE_IS_BOXED (type));
	g_value_init (&value, type);
	g_value_set_boxed (&value, &boxed);
	g_value_init (&copy, type);
	g_value_copy (&value, &copy);
	got = g_value_get_boxed (&copy);
	g_assert_nonnull (got);
	g_assert_true (got->owner_read);
	g_assert_false (got->owner_write);
	g_assert_true (got->owner_execute);
	g_assert_false (got->group_read);
	g_assert_true (got->group_write);
	g_assert_false (got->group_execute);
	g_assert_true (got->others_read);
	g_assert_false (got->others_write);
	g_assert_true (got->others_execute);
	g_value_unset (&copy);
	g_value_unset (&value);

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = test_file_make_uri (dir, "perms.txt", "x", &path);

	go_set_file_permissions (uri, &wanted);
	got = go_get_file_permissions (uri);
	g_assert_nonnull (got);
	g_assert_true (got->owner_read);
	g_assert_true (got->owner_write);
	g_assert_false (got->owner_execute);
#ifndef G_OS_WIN32
	g_assert_true (got->group_read);
	g_assert_false (got->group_write);
	g_assert_false (got->group_execute);
	g_assert_false (got->others_read);
	g_assert_false (got->others_write);
	g_assert_false (got->others_execute);
#endif
	g_free (got);

	g_remove (path);
	g_rmdir (dir);
	g_free (path);
	g_free (uri);
	g_free (dir);
}

static void
test_file_dates (void)
{
	char *dir, *path, *uri, *missing_path, *missing_uri;
	time_t before = time (NULL) - 1;
	time_t accessed, modified, changed;
	GDateTime *modtime;
	gint64 modtime_unix;

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = test_file_make_uri (dir, "dates.txt", "x", &path);

	modtime = go_file_get_modtime (uri);
	g_assert_nonnull (modtime);
	modtime_unix = g_date_time_to_unix (modtime);
	g_assert_cmpint (modtime_unix, >=, (gint64)before);

	accessed = go_file_get_date_accessed (uri);
	modified = go_file_get_date_modified (uri);
	changed = go_file_get_date_changed (uri);
	g_assert_cmpint ((gint64)accessed, >=, (gint64)before);
	g_assert_cmpint ((gint64)modified, >=, (gint64)before);
	g_assert_cmpint ((gint64)changed, >=, (gint64)before);
	g_assert_cmpint ((gint64)modified, ==, modtime_unix);

	g_date_time_unref (modtime);

	missing_path = g_build_filename (dir, "missing.txt", NULL);
	missing_uri = go_filename_to_uri (missing_path);
	g_assert_null (go_file_get_modtime (missing_uri));
	g_assert_cmpint ((gint64)go_file_get_date_accessed (missing_uri), ==, -1);
	g_assert_cmpint ((gint64)go_file_get_date_modified (missing_uri), ==, -1);
	g_assert_cmpint ((gint64)go_file_get_date_changed (missing_uri), ==, -1);

	g_free (missing_uri);
	g_free (missing_path);
	g_remove (path);
	g_rmdir (dir);
	g_free (path);
	g_free (uri);
	g_free (dir);
}

static void
test_file_access (void)
{
	char *dir, *path, *uri, *missing_path, *missing_uri;

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = test_file_make_uri (dir, "access.txt", "x", &path);

	g_assert_cmpint (go_file_access (uri, GO_F_OK), ==, 0);
	g_assert_cmpint (go_file_access (uri, GO_R_OK), ==, 0);
	g_assert_cmpint (go_file_access (uri, GO_W_OK), ==, 0);

	missing_path = g_build_filename (dir, "missing.txt", NULL);
	missing_uri = go_filename_to_uri (missing_path);
	g_assert_cmpint (go_file_access (missing_uri, GO_F_OK), ==, -1);
	g_assert_cmpint (go_file_access ("http://example.com/a", GO_F_OK), ==, -1);

	g_free (missing_uri);
	g_free (missing_path);
	g_remove (path);
	g_rmdir (dir);
	g_free (path);
	g_free (uri);
	g_free (dir);
}

static void
test_file_url_encode (void)
{
	static struct {
		int type;
		char const *text;
		char const *result;
	} const cases[] = {
		{ 0, "a b/c@d+e", "a%20b%2Fc@d%2Be" },
		{ 1, "a b/c@d+e", "a%20b/c@d+e" }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		char *result = go_url_encode (cases[i].text, cases[i].type);
		g_assert_cmpstr (result, ==, cases[i].result);
		g_free (result);
	}
}

static void
test_file_check_extension (void)
{
	static struct {
		char const *uri;
		char const *std_ext;
		gboolean result;
		char const *new_uri;
	} const cases[] = {
		{ "file:///tmp/a", "txt", TRUE, "file:///tmp/a.txt" },
		{ "file:///tmp/a.txt", "txt", TRUE, "file:///tmp/a.txt" },
		{ "file:///tmp/a.TXT", "txt", TRUE, "file:///tmp/a.TXT" },
		{ "file:///tmp/a.odt", "txt", FALSE, "file:///tmp/a.odt" },
		{ "file:///tmp/a", "", TRUE, "file:///tmp/a" },
		{ "file:///tmp/a.txt", NULL, TRUE, "file:///tmp/a.txt" }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		char *new_uri = NULL;
		gboolean result = go_url_check_extension (cases[i].uri,
							  cases[i].std_ext,
							  &new_uri);
		g_assert_cmpint (result, ==, cases[i].result);
		g_assert_cmpstr (new_uri, ==, cases[i].new_uri);
		g_free (new_uri);

		/* The output argument is optional.  */
		result = go_url_check_extension (cases[i].uri,
						 cases[i].std_ext, NULL);
		g_assert_cmpint (result, ==, cases[i].result);
	}
}

static void
test_file_mime_type (void)
{
	static char const text[] = "hello world\n";
	char *dir, *path, *uri, *mime, *content_type, *expected, *description;

	dir = g_dir_make_tmp ("test-util-XXXXXX", NULL);
	g_assert_nonnull (dir);
	uri = test_file_make_uri (dir, "notes.txt", text, &path);

	mime = go_get_mime_type (uri);
	g_assert_nonnull (mime);
	g_assert_true (g_utf8_validate (mime, -1, NULL));
	g_assert_cmpuint (strlen (mime), >, 0);

#ifndef G_OS_WIN32
	content_type = g_content_type_guess (NULL, (guchar const *)text,
					     sizeof (text) - 1, NULL);
	expected = content_type ? g_content_type_get_mime_type (content_type) : NULL;
	g_free (content_type);
	if (expected != NULL) {
		g_free (mime);
		mime = go_get_mime_type_for_data (text, sizeof (text) - 1);
		g_assert_cmpstr (mime, ==, expected);
	}
	g_free (expected);
#endif
	g_free (mime);

	content_type = g_content_type_from_mime_type ("text/plain");
	expected = content_type ? g_content_type_get_description (content_type) : NULL;
	g_free (content_type);
	description = go_mime_type_get_description ("text/plain");
	g_assert_cmpstr (description, ==, expected ? expected : "text/plain");
	g_free (description);
	g_free (expected);

	g_remove (path);
	g_rmdir (dir);
	g_free (path);
	g_free (uri);
	g_free (dir);
}

static void
test_file_shell_argv (void)
{
	gchar const *argv[] = { "one", "two", NULL };
	gchar const **args = go_shell_argv_to_glib_encoding (2, argv);

#ifdef G_OS_WIN32
	g_assert_cmpstr (args[0], ==, "one");
	g_assert_cmpstr (args[1], ==, "two");
	g_assert_null (args[2]);
#else
	g_assert_true (args == argv);
#endif
	go_shell_argv_to_glib_encoding_free ();
}

static void
register_file_tests (void)
{
	g_test_add_func ("/utils/go-file/filename-simplify",
			 test_file_filename_simplify);
	g_test_add_func ("/utils/go-file/url-simplify", test_file_url_simplify);
	g_test_add_func ("/utils/go-file/url-relative",
			 test_file_url_relative);
	g_test_add_func ("/utils/go-file/uri-conversions",
			 test_file_uri_conversions);
	g_test_add_func ("/utils/go-file/shell-arg-to-uri",
			 test_file_shell_arg_to_uri);
	g_test_add_func ("/utils/go-file/uri-parts", test_file_uri_parts);
	g_test_add_func ("/utils/go-file/open-create", test_file_open_create);
	g_test_add_func ("/utils/go-file/split-urls", test_file_split_urls);
	g_test_add_func ("/utils/go-file/owner-group", test_file_owner_group);
	g_test_add_func ("/utils/go-file/permissions", test_file_permissions);
	g_test_add_func ("/utils/go-file/dates", test_file_dates);
	g_test_add_func ("/utils/go-file/access", test_file_access);
	g_test_add_func ("/utils/go-file/url-encode", test_file_url_encode);
	g_test_add_func ("/utils/go-file/check-extension",
			 test_file_check_extension);
	g_test_add_func ("/utils/go-file/mime-type", test_file_mime_type);
	g_test_add_func ("/utils/go-file/shell-argv", test_file_shell_argv);
}

/* ------------------------------------------------------------------ */

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);
	libgoffice_init ();
#ifdef GOFFICE_WITH_GTK
	/*
	 * Using GtkText* requires GTK to be initialized first.  In particular,
	 * GtkTextTag's deprecated GdkColor properties are looked up by name
	 * when the class is initialized, and gtk_init_check() registers that
	 * boxed type before any GtkTextTag is created.
	 */
	(void)gtk_init_check (NULL, NULL);
#endif

	register_geometry_tests ();
	register_color_tests ();
	register_unit_tests ();
	register_datetime_tests ();
	register_string_tests ();
	register_line_tests ();
	register_undo_tests ();
	register_path_tests ();
	register_bezier_tests ();
	register_marker_tests ();
	register_pattern_tests ();
	register_glib_extras_tests ();
	register_cairo_tests ();
	register_regutf8_tests ();
	register_libxml_extras_tests ();
	register_image_tests ();
	register_font_tests ();
	register_locale_tests ();
	register_style_tests ();
	register_gradient_tests ();
	register_rsm_tests ();
	register_persist_tests ();
	register_pango_extras_tests ();
	register_file_tests ();

	int res = g_test_run ();
	libgoffice_shutdown ();
	return res;
}
