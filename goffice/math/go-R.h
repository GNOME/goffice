/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-R.h
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_R_H
#define GO_R_H

G_BEGIN_DECLS

double go_trunc (double);
double go_dnorm (double x, double mu, double sigma, gboolean give_log);
double go_pnorm (double x, double mu, double sigma, gboolean lower_tail, gboolean log_p);
void go_pnorm_both (double x, double *cum, double *ccum, int i_tail, gboolean log_p);
double go_qnorm (double p, double mu, double sigma, gboolean lower_tail, gboolean log_p);
double go_dlnorm (double x, double meanlog, double sdlog, gboolean give_log);
double go_plnorm (double x, double logmean, double logsd, gboolean lower_tail, gboolean log_p);
double go_qlnorm (double p, double logmean, double logsd, gboolean lower_tail, gboolean log_p);
double go_dweibull (double x, double shape, double scale, gboolean give_log);
double go_pweibull (double x, double shape, double scale, gboolean lower_tail, gboolean log_p);
double go_qweibull (double p, double shape, double scale, gboolean lower_tail, gboolean log_p);
double go_dcauchy (double x, double location, double scale, gboolean give_log);
double go_pcauchy (double x, double location, double scale, gboolean lower_tail, gboolean log_p);
double go_qcauchy (double p, double location, double scale, gboolean lower_tail, gboolean log_p);

#ifdef GOFFICE_WITH_LONG_DOUBLE

long double go_truncl (long double);
long double go_dnorml (long double x, long double mu, long double sigma, gboolean give_log);
long double go_pnorml (long double x, long double mu, long double sigma, gboolean lower_tail, gboolean log_p);
void go_pnorm_bothl (long double x, long double *cum, long double *ccum, int i_tail, gboolean log_p);
long double go_qnorml (long double p, long double mu, long double sigma, gboolean lower_tail, gboolean log_p);
long double go_dlnorml (long double x, long double meanlog, long double sdlog, gboolean give_log);
long double go_plnorml (long double x, long double logmean, long double logsd, gboolean lower_tail, gboolean log_p);
long double go_qlnorml (long double p, long double logmean, long double logsd, gboolean lower_tail, gboolean log_p);
long double go_dweibulll (long double x, long double shape, long double scale, gboolean give_log);
long double go_pweibulll (long double x, long double shape, long double scale, gboolean lower_tail, gboolean log_p);
long double go_qweibulll (long double p, long double shape, long double scale, gboolean lower_tail, gboolean log_p);
long double go_dcauchyl (long double x, long double location, long double scale, gboolean give_log);
long double go_pcauchyl (long double x, long double location, long double scale, gboolean lower_tail, gboolean log_p);
long double go_qcauchyl (long double p, long double location, long double scale, gboolean lower_tail, gboolean log_p);

#endif

G_END_DECLS

#endif /* GO_R_H */
