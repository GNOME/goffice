/*
 * Documentation for those functions where inline documentation is
 * not appropriate, either because the source code was imported
 * from elsewhere or because it's basically a copy of documentation
 * for a symbol for another number system.
 */


// --- BEGIN AUTO-GENERATED DOCUMENTATION MARKER ---

/**
 * go_accumulator_addD: (skip)
 **/

/**
 * go_accumulator_add_quadD: (skip)
 **/

/**
 * go_accumulator_add_quadl: (skip)
 **/

/**
 * go_accumulator_addl: (skip)
 **/

/**
 * go_accumulator_clearD: (skip)
 **/

/**
 * go_accumulator_clearl: (skip)
 **/

/**
 * go_accumulator_freeD: (skip)
 **/

/**
 * go_accumulator_freel: (skip)
 **/

/**
 * go_accumulator_newD: (skip)
 **/

/**
 * go_accumulator_newl: (skip)
 **/

/**
 * go_accumulator_startD: (skip)
 **/

/**
 * go_accumulator_startl: (skip)
 **/

/**
 * go_accumulator_valueD: (skip)
 **/

/**
 * go_accumulator_valuel: (skip)
 **/

/**
 * go_add_epsilonD:
 * @x: a number
 *
 * Returns the next-larger representable value, except that zero and
 * infinites are returned unchanged.
 */

/**
 * go_add_epsilonl:
 * @x: a number
 *
 * Returns the next-larger representable value, except that zero and
 * infinites are returned unchanged.
 */

/**
 * go_atan2piD:
 * @y: a number
 * @x: a number
 *
 * Returns: the polar angle of the point (@x,@y) in radians divided by Pi.
 * The result is a number between -1 and +1.
 */

/**
 * go_atan2pil:
 * @y: a number
 * @x: a number
 *
 * Returns: the polar angle of the point (@x,@y) in radians divided by Pi.
 * The result is a number between -1 and +1.
 */

/**
 * go_atanpiD:
 * @x: a number
 *
 * Returns: the arc tangent of @x in radians divided by Pi.  The result is a
 * number between -1 and +1.
 */

/**
 * go_atanpil:
 * @x: a number
 *
 * Returns: the arc tangent of @x in radians divided by Pi.  The result is a
 * number between -1 and +1.
 */

/**
 * go_cospiD:
 * @x: a number
 *
 * Returns: the cosine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

/**
 * go_cospil:
 * @x: a number
 *
 * Returns: the cosine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

/**
 * go_cotpiD:
 * @x: a number
 *
 * Returns: the cotangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

/**
 * go_cotpil:
 * @x: a number
 *
 * Returns: the cotangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

/**
 * go_cspline_destroyD:
 * @sp: a spline structure returned by go_cspline_init.
 *
 * Frees the spline structure when done.
 */

/**
 * go_cspline_destroyl:
 * @sp: a spline structure returned by go_cspline_init.
 *
 * Frees the spline structure when done.
 */

/**
 * go_cspline_get_derivD:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: the value
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 *
 * Returns: the interpolated derivative at x, or 0 if an error occurred.
 */

/**
 * go_cspline_get_derivl:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: the value
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 *
 * Returns: the interpolated derivative at x, or 0 if an error occurred.
 */

/**
 * go_cspline_get_derivsD:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of the n interpolated derivatives which
 * should be destroyed by a call to g_free when not anymore needed, or %NULL if
 * an error occurred.
 */

/**
 * go_cspline_get_derivsl:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of the n interpolated derivatives which
 * should be destroyed by a call to g_free when not anymore needed, or %NULL if
 * an error occurred.
 */

/**
 * go_cspline_get_integralsD:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of the n-1 integrals on the intervals
 * between two consecutive values stored in x. which should be destroyed by
 * a call to g_free when not anymore needed, or %NULL if  an error occurred.
 */

/**
 * go_cspline_get_integralsl:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of the n-1 integrals on the intervals
 * between two consecutive values stored in x. which should be destroyed by
 * a call to g_free when not anymore needed, or %NULL if  an error occurred.
 */

/**
 * go_cspline_get_valueD:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: The value
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 *
 * Returns: the interpolated value for x, or 0 if an error occurred.
 */

/**
 * go_cspline_get_valuel:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: The value
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 *
 * Returns: the interpolated value for x, or 0 if an error occurred.
 */

/**
 * go_cspline_get_valuesD:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of interpolated values which should
 * be destroyed by a call to g_free when not anymore needed, or %NULL if
 * an error occurred.
 */

/**
 * go_cspline_get_valuesl:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of interpolated values which should
 * be destroyed by a call to g_free when not anymore needed, or %NULL if
 * an error occurred.
 */

/**
 * go_cspline_initD: (skip)
 * @x: the x values
 * @y: the y values
 * @n: the number of x and y values
 * @limits: how the limits must be treated, four values are allowed:
 *	GO_CSPLINE_NATURAL: first and least second derivatives are 0.
 *	GO_CSPLINE_PARABOLIC: the curve will be a parabolic arc outside of the limits.
 *	GO_CSPLINE_CUBIC: the curve will be cubic outside of the limits.
 *	GO_CSPLINE_CLAMPED: the first and last derivatives are imposed.
 * @c0: the first derivative when using clamped splines, not used in the
 *      other limit types.
 * @cn: the first derivative when using clamped splines, not used in the
 *      other limit types.
 *
 * Creates a spline structure, and computes the coefficients associated with the
 * polynomials. The ith polynomial (between x[i-1] and x[i] is:
 * y(x) = y[i-1] + (c[i-1] + (b[i-1] + a[i] * (x - x[i-1])) * (x - x[i-1])) * (x - x[i-1])
 * where a[i-1], b[i-1], c[i-1], x[i-1] and y[i-1] are the corresponding
 * members of the new structure.
 *
 * Returns: a newly created GOCSpline instance which should be
 * destroyed by a call to go_cspline_destroy.
 */

/**
 * go_cspline_initl: (skip)
 * @x: the x values
 * @y: the y values
 * @n: the number of x and y values
 * @limits: how the limits must be treated, four values are allowed:
 *	GO_CSPLINE_NATURAL: first and least second derivatives are 0.
 *	GO_CSPLINE_PARABOLIC: the curve will be a parabolic arc outside of the limits.
 *	GO_CSPLINE_CUBIC: the curve will be cubic outside of the limits.
 *	GO_CSPLINE_CLAMPED: the first and last derivatives are imposed.
 * @c0: the first derivative when using clamped splines, not used in the
 *      other limit types.
 * @cn: the first derivative when using clamped splines, not used in the
 *      other limit types.
 *
 * Creates a spline structure, and computes the coefficients associated with the
 * polynomials. The ith polynomial (between x[i-1] and x[i] is:
 * y(x) = y[i-1] + (c[i-1] + (b[i-1] + a[i] * (x - x[i-1])) * (x - x[i-1])) * (x - x[i-1])
 * where a[i-1], b[i-1], c[i-1], x[i-1] and y[i-1] are the corresponding
 * members of the new structure.
 *
 * Returns: a newly created GOCSpline instance which should be
 * destroyed by a call to go_cspline_destroy.
 */

/**
 * go_exponential_regressionD:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points.
 * Fits to "y = b * m1^x1 * ... * md^xd " or equivalently to
 * "log y = log b + x1 * log m1 + ... + xd * log md".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_exponential_regression_as_logD:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points as
 * go_exponential_regression, but returns the logarithm of the coefficients instead
 * or the coefficients themselves.
 * Fits to "y = b * exp (m1*x1) * ... * exp (md*xd) " or equivalently to
 * "ln y = ln b + x1 * m1 + ... + xd * md".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_exponential_regression_as_logl:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points as
 * go_exponential_regression, but returns the logarithm of the coefficients instead
 * or the coefficients themselves.
 * Fits to "y = b * exp (m1*x1) * ... * exp (md*xd) " or equivalently to
 * "ln y = ln b + x1 * m1 + ... + xd * md".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_exponential_regressionl:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points.
 * Fits to "y = b * m1^x1 * ... * md^xd " or equivalently to
 * "log y = log b + x1 * log m1 + ... + xd * log md".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_fake_ceilD:
 * @x: value to ceil
 *
 * Returns: the ceiling of @x, ie., the smallest integer that is not smaller
 * than @x.  However, this variant applies a 1 ulp grace interval for
 * values that are just a hair larger than an integer.
 */

/**
 * go_fake_ceill:
 * @x: value to ceil
 *
 * Returns: the ceiling of @x, ie., the smallest integer that is not smaller
 * than @x.  However, this variant applies a 1 ulp grace interval for
 * values that are just a hair larger than an integer.
 */

/**
 * go_fake_floorD:
 * @x: value to floor
 *
 * Returns: the floor of @x, ie., the largest integer that is not larger
 * than @x.  However, this variant applies a 1 ulp grace interval for
 * values that are just a hair less than an integer.
 */

/**
 * go_fake_floorl:
 * @x: value to floor
 *
 * Returns: the floor of @x, ie., the largest integer that is not larger
 * than @x.  However, this variant applies a 1 ulp grace interval for
 * values that are just a hair less than an integer.
 */

/**
 * go_format_specializeD:
 * @fmt: the format to specialize
 * @val: the value to use
 * @type: the type of value; 'F' for numeric, 'B' for boolean, 'S' for string.
 * @inhibit_minus: (out): set to %TRUE if the format dictates that a minus
 * should be inhibited when rendering negative values.
 *
 * Returns: (transfer none): @fmt format, presumably a conditional format,
 * specialized to @value of @type.
 */

/**
 * go_format_specializel:
 * @fmt: the format to specialize
 * @val: the value to use
 * @type: the type of value; 'F' for numeric, 'B' for boolean, 'S' for string.
 * @inhibit_minus: (out): set to %TRUE if the format dictates that a minus
 * should be inhibited when rendering negative values.
 *
 * Returns: (transfer none): @fmt format, presumably a conditional format,
 * specialized to @value of @type.
 */

/**
 * go_format_valueD:
 * @fmt: a #GOFormat
 * @val: value to format
 *
 * Converts @val into a string using format specified by @fmt.
 *
 * Returns: (transfer full): formatted value.
 **/

/**
 * go_format_value_gstringD:
 * @layout: Optional PangoLayout, probably preseeded with font attribute.
 * @str: a GString to store (not append!) the resulting string in.
 * @measure: (scope call): Function to measure width of string/layout.
 * @metrics: Font metrics corresponding to @measure.
 * @fmt: #GOFormat
 * @val: floating-point value.  Must be finite.
 * @type: a format character
 * @sval: a string to append to @str after @val
 * @go_color: a color to render
 * @col_width: intended max width of layout in pango units.  -1 means
 *             no restriction.
 * @date_conv: #GODateConventions
 * @unicode_minus: Use unicode minuses, not hyphens.
 *
 * Render a floating-point value into @layout in such a way that the
 * layouting width does not needlessly exceed @col_width.  Optionally
 * use unicode minus instead of hyphen.
 * Returns: a #GOFormatNumberError
 **/

/**
 * go_format_value_gstringl:
 * @layout: Optional PangoLayout, probably preseeded with font attribute.
 * @str: a GString to store (not append!) the resulting string in.
 * @measure: (scope call): Function to measure width of string/layout.
 * @metrics: Font metrics corresponding to @measure.
 * @fmt: #GOFormat
 * @val: floating-point value.  Must be finite.
 * @type: a format character
 * @sval: a string to append to @str after @val
 * @go_color: a color to render
 * @col_width: intended max width of layout in pango units.  -1 means
 *             no restriction.
 * @date_conv: #GODateConventions
 * @unicode_minus: Use unicode minuses, not hyphens.
 *
 * Render a floating-point value into @layout in such a way that the
 * layouting width does not needlessly exceed @col_width.  Optionally
 * use unicode minus instead of hyphen.
 * Returns: a #GOFormatNumberError
 **/

/**
 * go_format_valuel:
 * @fmt: a #GOFormat
 * @val: value to format
 *
 * Converts @val into a string using format specified by @fmt.
 *
 * Returns: (transfer full): formatted value.
 **/

/**
 * go_linear_regressionD:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors.
 * @ys: y-vector.  (Dependent data.)
 * @n: number of data points.
 * @affine: if true, a non-zero constant is allowed.
 * @res: (out): place for constant[0] and slope1[1], slope2[2],... There will be dim+1 results.
 * @stat_: (out): storage for additional results.
 *
 * Performs multi-dimensional linear regressions on the input points.
 * Fits to "y = b + a1 * x1 + ... ad * xd".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_linear_regressionl:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors.
 * @ys: y-vector.  (Dependent data.)
 * @n: number of data points.
 * @affine: if true, a non-zero constant is allowed.
 * @res: (out): place for constant[0] and slope1[1], slope2[2],... There will be dim+1 results.
 * @stat_: (out): storage for additional results.
 *
 * Performs multi-dimensional linear regressions on the input points.
 * Fits to "y = b + a1 * x1 + ... ad * xd".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_logarithmic_fitD:
 * @xs: x-vector (i.e. independent data)
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @res: output place for sign[0], a[1], b[2], c[3], and sum of squared residuals[4].
 *
 * Performs a two-dimensional non-linear fitting on the input points.
 * Fits to "y = a + b * ln (sign * (x - c))", with sign in {-1, +1}.
 * The graph is a logarithmic curve moved horizontally by c and possibly
 * mirrored across the y-axis (if sign = -1).
 *
 * Fits c (and sign) by iterative trials, but seems to be fast enough even
 * for automatic recomputation.
 *
 * Adapts c until a local minimum of squared residuals is reached. For each
 * new c tried out the corresponding a and b are calculated by linear
 * regression. If no local minimum is found, an error is returned. If there
 * is more than one local minimum, the one found is not necessarily the
 * smallest (i.e., there might be cases in which the returned fit is not the
 * best possible). If the shape of the point cloud is to different from
 * ``logarithmic'', either sign can not be determined (error returned) or no
 * local minimum will be found.
 *
 * (Requires: at least 3 different x values, at least 3 different y values.)
 *
 * Returns: #GORegressionResult as above.
 */

/**
 * go_logarithmic_fitl:
 * @xs: x-vector (i.e. independent data)
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @res: output place for sign[0], a[1], b[2], c[3], and sum of squared residuals[4].
 *
 * Performs a two-dimensional non-linear fitting on the input points.
 * Fits to "y = a + b * ln (sign * (x - c))", with sign in {-1, +1}.
 * The graph is a logarithmic curve moved horizontally by c and possibly
 * mirrored across the y-axis (if sign = -1).
 *
 * Fits c (and sign) by iterative trials, but seems to be fast enough even
 * for automatic recomputation.
 *
 * Adapts c until a local minimum of squared residuals is reached. For each
 * new c tried out the corresponding a and b are calculated by linear
 * regression. If no local minimum is found, an error is returned. If there
 * is more than one local minimum, the one found is not necessarily the
 * smallest (i.e., there might be cases in which the returned fit is not the
 * best possible). If the shape of the point cloud is to different from
 * ``logarithmic'', either sign can not be determined (error returned) or no
 * local minimum will be found.
 *
 * (Requires: at least 3 different x values, at least 3 different y values.)
 *
 * Returns: #GORegressionResult as above.
 */

/**
 * go_logarithmic_regressionD:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-zero constant is allowed
 * @res: output place for constant[0] and factor1[1], factor2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * This is almost a copy of linear_regression and produces multi-dimensional
 * linear regressions on the input points after transforming xss to ln(xss).
 * Fits to "y = b + a1 * z1 + ... ad * zd" with "zi = ln (xi)".
 * Problems with arrays in the calling function: see comment to
 * gnumeric_linest, which is also valid for gnumeric_logreg.
 *
 * (Errors: less than two points, all points on a vertical line, non-positive x data.)
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_logarithmic_regressionl:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-zero constant is allowed
 * @res: output place for constant[0] and factor1[1], factor2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * This is almost a copy of linear_regression and produces multi-dimensional
 * linear regressions on the input points after transforming xss to ln(xss).
 * Fits to "y = b + a1 * z1 + ... ad * zd" with "zi = ln (xi)".
 * Problems with arrays in the calling function: see comment to
 * gnumeric_linest, which is also valid for gnumeric_logreg.
 *
 * (Errors: less than two points, all points on a vertical line, non-positive x data.)
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_non_linear_regressionD:
 * @f: (scope call): the model function
 * @xvals: independent values.
 * @par: model parameters.
 * @yvals: dependent values.
 * @sigmas: stahdard deviations for the dependent values.
 * @x_dim: Number of data points.
 * @p_dim: Number of parameters.
 * @chi: Chi Squared of the final result.  This value is not very
 * meaningful without the sigmas.
 * @errors: MUST ALREADY BE ALLOCATED.  These are the approximated standard
 * deviation for each parameter.
 *
 * SYNOPSIS:
 *   result = non_linear_regression (f, xvals, par, yvals, sigmas,
 *                                   x_dim, p_dim, &chi, errors)
 * Non linear regression.
 * Returns: the results of the non-linear regression from the given initial
 * values.
 * The resulting parameters are placed back into @par.
 **/

/**
 * go_non_linear_regressionl:
 * @f: (scope call): the model function
 * @xvals: independent values.
 * @par: model parameters.
 * @yvals: dependent values.
 * @sigmas: stahdard deviations for the dependent values.
 * @x_dim: Number of data points.
 * @p_dim: Number of parameters.
 * @chi: Chi Squared of the final result.  This value is not very
 * meaningful without the sigmas.
 * @errors: MUST ALREADY BE ALLOCATED.  These are the approximated standard
 * deviation for each parameter.
 *
 * SYNOPSIS:
 *   result = non_linear_regression (f, xvals, par, yvals, sigmas,
 *                                   x_dim, p_dim, &chi, errors)
 * Non linear regression.
 * Returns: the results of the non-linear regression from the given initial
 * values.
 * The resulting parameters are placed back into @par.
 **/

/**
 * go_pow10D:
 * @n: exponent
 *
 * Computes 10 to the power of @n.  This is fast and accurate (under the
 * reasonable assumption that the compiler is accurate).
 */

/**
 * go_pow10l:
 * @n: exponent
 *
 * Computes 10 to the power of @n.  This is fast and accurate (under the
 * reasonable assumption that the compiler is accurate).
 */

/**
 * go_pow2D:
 * @n: exponent
 *
 * Computes 2 to the power of @n.  This is fast and accurate.
 */

/**
 * go_pow2l:
 * @n: exponent
 *
 * Computes 2 to the power of @n.  This is fast and accurate.
 */

/**
 * go_power_regressionD:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points.
 * Fits to "y = b * x1^m1 * ... * xd^md " or equivalently to
 * "log y = log b + m1 * log x1 + ... + md * log xd".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_power_regressionl:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_: (out) (optional): storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points.
 * Fits to "y = b * x1^m1 * ... * xd^md " or equivalently to
 * "log y = log b + m1 * log x1 + ... + md * log xd".
 *
 * Returns: #GORegressionResult as above.
 **/

/**
 * go_quad_absD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the absolute value of @a, storing the result in @res.
 **/

/**
 * go_quad_absl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the absolute value of @a, storing the result in @res.
 **/

/**
 * go_quad_acosD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc cosine of @a, storing the result in @res.
 **/

/**
 * go_quad_acosl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc cosine of @a, storing the result in @res.
 **/

/**
 * go_quad_addD:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function adds @a and @b, storing the result in @res.
 **/

/**
 * go_quad_addl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function adds @a and @b, storing the result in @res.
 **/

/**
 * go_quad_asinD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc sine of @a, storing the result in @res.
 **/

/**
 * go_quad_asinl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc sine of @a, storing the result in @res.
 **/

/**
 * go_quad_atan2D:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y), storing
 * the result in @res.
 **/

/**
 * go_quad_atan2l:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y), storing
 * the result in @res.
 **/

/**
 * go_quad_atan2piD:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y) divided
 * by pi, storing the result in @res.
 **/

/**
 * go_quad_atan2pil:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y) divided
 * by pi, storing the result in @res.
 **/

/**
 * go_quad_constant8D:
 * @res: (out): result location
 * @data: (array length=n): vector of digits
 * @base: base of vector's elements
 * @n: length of digit vector.
 * @scale: scaling value after interpreting digits
 *
 * This function interprets a vector of digits in a given base as a
 * quad-precision value.  It is mostly meant for internal use.
 **/

/**
 * go_quad_constant8l:
 * @res: (out): result location
 * @data: (array length=n): vector of digits
 * @base: base of vector's elements
 * @n: length of digit vector.
 * @scale: scaling value after interpreting digits
 *
 * This function interprets a vector of digits in a given base as a
 * quad-precision value.  It is mostly meant for internal use.
 **/

/**
 * go_quad_cosD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a, storing the result in @res.
 **/

/**
 * go_quad_cosl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a, storing the result in @res.
 **/

/**
 * go_quad_cospiD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/

/**
 * go_quad_cospil:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/

/**
 * go_quad_divD:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function divides @a and @b, storing the result in @res.
 **/

/**
 * go_quad_divl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function divides @a and @b, storing the result in @res.
 **/

/**
 * go_quad_dot_productD:
 * @res: (out): result location
 * @a: (array length=n): vector of quad-precision values
 * @b: (array length=n): vector of quad-precision values
 * @n: length of vectors.
 **/

/**
 * go_quad_dot_productl:
 * @res: (out): result location
 * @a: (array length=n): vector of quad-precision values
 * @b: (array length=n): vector of quad-precision values
 * @n: length of vectors.
 **/

/**
 * go_quad_endD:
 * @state: state pointer from go_quad_start.
 *
 * This ends a section of quad precision arithmetic.
 **/

/**
 * go_quad_endl:
 * @state: state pointer from go_quad_start.
 *
 * This ends a section of quad precision arithmetic.
 **/

/**
 * go_quad_expD:
 * @res: (out): result location
 * @expb: (out): (allow-none): power-of-base result scaling location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a, storing the result
 * in @res.  If the optional @expb is supplied, it is used to return a
 * power of radix by which the result should be scaled.  This is useful to
 * represent results much, much bigger than double precision can handle.
 **/

/**
 * go_quad_expl:
 * @res: (out): result location
 * @expb: (out): (allow-none): power-of-base result scaling location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a, storing the result
 * in @res.  If the optional @expb is supplied, it is used to return a
 * power of radix by which the result should be scaled.  This is useful to
 * represent results much, much bigger than double precision can handle.
 **/

/**
 * go_quad_expm1D:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a with 1 subtracted,
 * storing the difference in @res.
 **/

/**
 * go_quad_expm1l:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a with 1 subtracted,
 * storing the difference in @res.
 **/

/**
 * go_quad_floorD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the floor of @a, storing the result in @res.
 **/

/**
 * go_quad_floorl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the floor of @a, storing the result in @res.
 **/

/**
 * go_quad_hypotD:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function computes the square root of @a^2 plus @b^2, storing the
 * result in @res.
 **/

/**
 * go_quad_hypotl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function computes the square root of @a^2 plus @b^2, storing the
 * result in @res.
 **/

/**
 * go_quad_initD:
 * @res: (out): result location
 * @h: a double precision value
 *
 * This stores the value @h in @res.  As an exception, this may be called
 * outside go_quad_start and go_quad_end sections.
 **/

/**
 * go_quad_initl:
 * @res: (out): result location
 * @h: a double precision value
 *
 * This stores the value @h in @res.  As an exception, this may be called
 * outside go_quad_start and go_quad_end sections.
 **/

/**
 * go_quad_logD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the natural logarithm at @a, storing the result
 * in @res.
 **/

/**
 * go_quad_logl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the natural logarithm at @a, storing the result
 * in @res.
 **/

/**
 * go_quad_matrix_back_solveD:
 * @R: An upper triangular matrix.
 * @x: (out): Result vector.
 * @b: Input vector.
 * @allow_degenerate: If %TRUE, then degenerate dimensions are ignored other
 * than being given a zero result.  A degenerate dimension is one whose
 * diagonal entry is zero.
 *
 * Returns: %TRUE on error.
 *
 * This function solves the triangular system R*x=b.
 **/

/**
 * go_quad_matrix_back_solvel:
 * @R: An upper triangular matrix.
 * @x: (out): Result vector.
 * @b: Input vector.
 * @allow_degenerate: If %TRUE, then degenerate dimensions are ignored other
 * than being given a zero result.  A degenerate dimension is one whose
 * diagonal entry is zero.
 *
 * Returns: %TRUE on error.
 *
 * This function solves the triangular system R*x=b.
 **/

/**
 * go_quad_matrix_copyD:
 * @A: (out): Destination matrix.
 * @B: (transfer none): Source matrix.
 *
 * Copies B to A.
 **/

/**
 * go_quad_matrix_copyl:
 * @A: (out): Destination matrix.
 * @B: (transfer none): Source matrix.
 *
 * Copies B to A.
 **/

/**
 * go_quad_matrix_dupD: (skip)
 * @A: Matrix to duplicate
 *
 * Returns: a new matrix.
 **/

/**
 * go_quad_matrix_dupl: (skip)
 * @A: Matrix to duplicate
 *
 * Returns: a new matrix.
 **/

/**
 * go_quad_matrix_eigen_rangeD:
 * @A: Triangular matrix.
 * @emin: (out): Smallest absolute eigen value.
 * @emax: (out): Largest absolute eigen value.
 **/

/**
 * go_quad_matrix_eigen_rangel:
 * @A: Triangular matrix.
 * @emin: (out): Smallest absolute eigen value.
 * @emax: (out): Largest absolute eigen value.
 **/

/**
 * go_quad_matrix_fwd_solveD:
 * @R: An upper triangular matrix.
 * @x: (out): Result vector.
 * @b: Input vector.
 * @allow_degenerate: If %TRUE, then degenerate dimensions are ignored other
 * than being given a zero result.  A degenerate dimension is one whose
 * diagonal entry is zero.
 *
 * Returns: %TRUE on error.
 *
 * This function solves the triangular system RT*x=b.
 **/

/**
 * go_quad_matrix_fwd_solvel:
 * @R: An upper triangular matrix.
 * @x: (out): Result vector.
 * @b: Input vector.
 * @allow_degenerate: If %TRUE, then degenerate dimensions are ignored other
 * than being given a zero result.  A degenerate dimension is one whose
 * diagonal entry is zero.
 *
 * Returns: %TRUE on error.
 *
 * This function solves the triangular system RT*x=b.
 **/

/**
 * go_quad_matrix_inverseD: (skip)
 * @A: Source matrix.
 * @threshold: condition number threshold.
 *
 * Returns: The inverse matrix of A.  If any eigenvalues divided by the largest
 * eigenvalue is less than or equal to the given threshold, %NULL is returned
 * indicating a matrix that cannot be inverted.  (Note: this doesn't actually
 * use the eigenvalues of A, but of A after an orthogonal transformation.)
 **/

/**
 * go_quad_matrix_inversel: (skip)
 * @A: Source matrix.
 * @threshold: condition number threshold.
 *
 * Returns: The inverse matrix of A.  If any eigenvalues divided by the largest
 * eigenvalue is less than or equal to the given threshold, %NULL is returned
 * indicating a matrix that cannot be inverted.  (Note: this doesn't actually
 * use the eigenvalues of A, but of A after an orthogonal transformation.)
 **/

/**
 * go_quad_matrix_multiplyD:
 * @C: (out): Destination matrix.
 * @A: Source matrix.
 * @B: Source matrix.
 *
 * Multiplies A*B and stores the result in C.
 **/

/**
 * go_quad_matrix_multiplyl:
 * @C: (out): Destination matrix.
 * @A: Source matrix.
 * @B: Source matrix.
 *
 * Multiplies A*B and stores the result in C.
 **/

/**
 * go_quad_matrix_newD: (skip)
 * @m: number of rows
 * @n: number of columns
 *
 * Returns: a new zero matrix.
 **/

/**
 * go_quad_matrix_newl: (skip)
 * @m: number of rows
 * @n: number of columns
 *
 * Returns: a new zero matrix.
 **/

/**
 * go_quad_matrix_pseudo_inverseD: (skip)
 * @A: An arbitrary matrix.
 * @threshold: condition number threshold.
 *
 * Returns: @A's pseudo-inverse.
 **/

/**
 * go_quad_matrix_pseudo_inversel: (skip)
 * @A: An arbitrary matrix.
 * @threshold: condition number threshold.
 *
 * Returns: @A's pseudo-inverse.
 **/

/**
 * go_quad_matrix_transposeD:
 * @A: (out): Destination matrix.
 * @B: (transfer none): Source matrix.
 *
 * Transposes B into A.
 **/

/**
 * go_quad_matrix_transposel:
 * @A: (out): Destination matrix.
 * @B: (transfer none): Source matrix.
 *
 * Transposes B into A.
 **/

/**
 * go_quad_mul12D:
 * @res: (out): result location
 * @x: double precision value
 * @y: double precision value
 *
 * This function multiplies @x and @y, storing the result in @res with full
 * quad precision.
 **/

/**
 * go_quad_mul12l:
 * @res: (out): result location
 * @x: double precision value
 * @y: double precision value
 *
 * This function multiplies @x and @y, storing the result in @res with full
 * quad precision.
 **/

/**
 * go_quad_mulD:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function multiplies @a and @b, storing the result in @res.
 **/

/**
 * go_quad_mull:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function multiplies @a and @b, storing the result in @res.
 **/

/**
 * go_quad_negateD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function negates @a and stores the result in @res.
 **/

/**
 * go_quad_negatel:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function negates @a and stores the result in @res.
 **/

/**
 * go_quad_powD:
 * @res: (out): result location
 * @expb: (out): (allow-none): power-of-base result scaling location
 * @x: quad-precision value
 * @y: quad-precision value
 *
 * This function computes @x to the power of @y, storing the result in @res.
 * If the optional @expb is supplied, it is used to return a power of radix
 * by which the result should be scaled.  Such scaling can be done with the
 * scalbn function, typically after combining multiple such terms.  This is
 * useful to represent results much, much bigger than double precision can
 * handle.
 **/

/**
 * go_quad_powl:
 * @res: (out): result location
 * @expb: (out): (allow-none): power-of-base result scaling location
 * @x: quad-precision value
 * @y: quad-precision value
 *
 * This function computes @x to the power of @y, storing the result in @res.
 * If the optional @expb is supplied, it is used to return a power of radix
 * by which the result should be scaled.  Such scaling can be done with the
 * scalbn function, typically after combining multiple such terms.  This is
 * useful to represent results much, much bigger than double precision can
 * handle.
 **/

/**
 * go_quad_qr_mark_degenerateD: (skip)
 * @qr: A QR decomposition.
 * @i: a dimension
 *
 * Marks dimension i of the qr decomposition as degenerate.  In practice
 * this means setting the i-th eigenvalue of R to zero.
 **/

/**
 * go_quad_qr_mark_degeneratel: (skip)
 * @qr: A QR decomposition.
 * @i: a dimension
 *
 * Marks dimension i of the qr decomposition as degenerate.  In practice
 * this means setting the i-th eigenvalue of R to zero.
 **/

/**
 * go_quad_qr_multiply_qtD:
 * @qr: A QR decomposition.
 * @x: (inout): a vector.
 *
 * Replaces @x by Q^t * x
 **/

/**
 * go_quad_qr_multiply_qtl:
 * @qr: A QR decomposition.
 * @x: (inout): a vector.
 *
 * Replaces @x by Q^t * x
 **/

/**
 * go_quad_qr_newD: (skip)
 * @A: Source matrix.
 *
 * QR decomposition of a matrix using Householder matrices.
 *
 * A (input) is an m-times-n matrix.  A[0...m-1][0..n-1]
 * If qAT is TRUE, this parameter is transposed.
 *
 * V is a pre-allocated output m-times-n matrix.  V will contrain
 * n vectors of different lengths: n, n-1, ..., 1.  These are the
 * Householder vectors (or null for the degenerate case).  The
 * matrix Q of size m-times-m is implied from V.
 *
 * R is a matrix of size n-times-n.  (To get the m-times-n version
 * of R, simply add m-n null rows.)
 * Returns: (transfer full): a new #GOQuadQR.
 **/

/**
 * go_quad_qr_newl: (skip)
 * @A: Source matrix.
 *
 * QR decomposition of a matrix using Householder matrices.
 *
 * A (input) is an m-times-n matrix.  A[0...m-1][0..n-1]
 * If qAT is TRUE, this parameter is transposed.
 *
 * V is a pre-allocated output m-times-n matrix.  V will contrain
 * n vectors of different lengths: n, n-1, ..., 1.  These are the
 * Householder vectors (or null for the degenerate case).  The
 * matrix Q of size m-times-m is implied from V.
 *
 * R is a matrix of size n-times-n.  (To get the m-times-n version
 * of R, simply add m-n null rows.)
 * Returns: (transfer full): a new #GOQuadQR.
 **/

/**
 * go_quad_qr_rD:
 * @qr: A QR decomposition.
 *
 * Returns: the small R from the decomposition, i.e., a square matrix
 * of size n.  To get the large R, if needed, add m-n zero rows.
 **/

/**
 * go_quad_qr_rl:
 * @qr: A QR decomposition.
 *
 * Returns: the small R from the decomposition, i.e., a square matrix
 * of size n.  To get the large R, if needed, add m-n zero rows.
 **/

/**
 * go_quad_sinD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a, storing the result in @res.
 **/

/**
 * go_quad_sinl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a, storing the result in @res.
 **/

/**
 * go_quad_sinpiD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/

/**
 * go_quad_sinpil:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/

/**
 * go_quad_sqrtD:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the square root of @a, storing the result in @res.
 **/

/**
 * go_quad_sqrtl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the square root of @a, storing the result in @res.
 **/

/**
 * go_quad_startD:
 *
 * Initializes #GOQuad arithmetic. Any use of #GOQuad must occur between calls
 * to go_quad_startD() and go_quad_end().
 * Returns: (transfer full): a pointer to pass to go_quad_end() when done.
 **/

/**
 * go_quad_startl:
 *
 * Initializes #GOQuad arithmetic. Any use of #GOQuad must occur between calls
 * to go_quad_startl() and go_quad_end().
 * Returns: (transfer full): a pointer to pass to go_quad_end() when done.
 **/

/**
 * go_quad_subD:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function subtracts @a and @b, storing the result in @res.
 **/

/**
 * go_quad_subl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function subtracts @a and @b, storing the result in @res.
 **/

/**
 * go_quad_valueD:
 * @a: quad-precision value
 *
 * Returns: closest double precision value to @a.  As an exception,
 * this may be called outside go_quad_start and go_quad_end sections.
 **/

/**
 * go_quad_valuel:
 * @a: quad-precision value
 *
 * Returns: closest double precision value to @a.  As an exception,
 * this may be called outside go_quad_start and go_quad_end sections.
 **/

/**
 * go_range_averageD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The average of
 * the input values will be stored in @res.
 */

/**
 * go_range_averagel:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The average of
 * the input values will be stored in @res.
 */

/**
 * go_range_constantD:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is constant, 0 otherwise.
 */

/**
 * go_range_constantl:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is constant, 0 otherwise.
 */

/**
 * go_range_decreasingD:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is decreasing, 0 otherwise.
 */

/**
 * go_range_decreasingl:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is decreasing, 0 otherwise.
 */

/**
 * go_range_devsqD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The sum of the input
 * values deviation from the mean will be stored in @res.
 */

/**
 * go_range_devsql:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The sum of the input
 * values deviation from the mean will be stored in @res.
 */

/**
 * go_range_fractile_interD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 * @f: fractile
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated fractile given by @f and stores it in @res.
 */

/**
 * go_range_fractile_inter_sortedD:
 * @xs: (array length=n): values, which must be sorted.
 * @n: number of values
 * @res: (out): result.
 * @f: fractile
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated fractile given by @f and stores it in @res.
 */

/**
 * go_range_fractile_inter_sortedl:
 * @xs: (array length=n): values, which must be sorted.
 * @n: number of values
 * @res: (out): result.
 * @f: fractile
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated fractile given by @f and stores it in @res.
 */

/**
 * go_range_fractile_interl:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 * @f: fractile
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated fractile given by @f and stores it in @res.
 */

/**
 * go_range_increasingD:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is increasing, 0 otherwise.
 */

/**
 * go_range_increasingl:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is increasing, 0 otherwise.
 */

/**
 * go_range_maxD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The maximum of
 * the input values will be stored in @res.
 */

/**
 * go_range_maxabsD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The maximum of the absolute
 * values of the input values will be stored in @res.
 */

/**
 * go_range_maxabsl:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The maximum of the absolute
 * values of the input values will be stored in @res.
 */

/**
 * go_range_maxl:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The maximum of
 * the input values will be stored in @res.
 */

/**
 * go_range_median_interD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated median and stores it in @res.
 */

/**
 * go_range_median_inter_sortedD:
 * @xs: (array length=n): values, which must be sorted.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated median and stores it in @res.
 */

/**
 * go_range_median_inter_sortedl:
 * @xs: (array length=n): values, which must be sorted.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated median and stores it in @res.
 */

/**
 * go_range_median_interl:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  This function computes
 * the interpolated median and stores it in @res.
 */

/**
 * go_range_minD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The minimum of
 * the input values will be stored in @res.
 */

/**
 * go_range_minl:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The minimum of
 * the input values will be stored in @res.
 */

/**
 * go_range_sumD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The arithmetic sum of the input
 * values will be stored in @res.
 */

/**
 * go_range_suml:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The arithmetic sum of the input
 * values will be stored in @res.
 */

/**
 * go_range_sumsqD:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The arithmetic sum of the squares
 * of the input values will be stored in @res.
 */

/**
 * go_range_sumsql:
 * @xs: (array length=n): values.
 * @n: number of values
 * @res: (out): result.
 *
 * Returns: 0 unless an error occurred.  The arithmetic sum of the squares
 * of the input values will be stored in @res.
 */

/**
 * go_range_vary_uniformlyD:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is either decreasing or increasing, 0 otherwise.
 */

/**
 * go_range_vary_uniformlyl:
 * @xs: (array length=n): values.
 * @n: number of values
 *
 * Returns: 1 if range is either decreasing or increasing, 0 otherwise.
 */

/**
 * go_render_generalD:
 * @layout: Optional #PangoLayout, probably preseeded with font attribute.
 * @str: a GString to store (not append!) the resulting string in.
 * @measure: (scope call): Function to measure width of string/layout.
 * @metrics: Font metrics corresponding to @measure.
 * @val: floating-point value.  Must be finite.
 * @col_width: intended max width of layout in the units that @measure uses.
 * A width of -1 means no restriction.
 * @unicode_minus: Use unicode minuses, not hyphens.
 * @numeral_shape: numeral shape identifier.
 * @custom_shape_flags: flags for using @numeral_shape.
 *
 * Render a floating-point value into @layout in such a way that the
 * layouting width does not needlessly exceed @col_width.  Optionally
 * use unicode minus instead of hyphen.
 **/

/**
 * go_render_generall:
 * @layout: Optional #PangoLayout, probably preseeded with font attribute.
 * @str: a GString to store (not append!) the resulting string in.
 * @measure: (scope call): Function to measure width of string/layout.
 * @metrics: Font metrics corresponding to @measure.
 * @val: floating-point value.  Must be finite.
 * @col_width: intended max width of layout in the units that @measure uses.
 * A width of -1 means no restriction.
 * @unicode_minus: Use unicode minuses, not hyphens.
 * @numeral_shape: numeral shape identifier.
 * @custom_shape_flags: flags for using @numeral_shape.
 *
 * Render a floating-point value into @layout in such a way that the
 * layouting width does not needlessly exceed @col_width.  Optionally
 * use unicode minus instead of hyphen.
 **/

/**
 * go_sinpiD:
 * @x: a number
 *
 * Returns: the sine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

/**
 * go_sinpil:
 * @x: a number
 *
 * Returns: the sine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

/**
 * go_sub_epsilonD:
 * @x: a number
 *
 * Returns the next-smaller representable value, except that zero and
 * infinites are returned unchanged.
 */

/**
 * go_sub_epsilonl:
 * @x: a number
 *
 * Returns the next-smaller representable value, except that zero and
 * infinites are returned unchanged.
 */

/**
 * go_tanpiD:
 * @x: a number
 *
 * Returns: the tangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

/**
 * go_tanpil:
 * @x: a number
 *
 * Returns: the tangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */

// --- END AUTO-GENERATED DOCUMENTATION MARKER ---
