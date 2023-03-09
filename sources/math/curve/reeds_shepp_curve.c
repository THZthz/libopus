/**
 * @file reeds_sheep_curve.c
 *     Author:    _              _
 *               / \   _ __ ___ (_)  __ _ ___
 *              / _ \ | '_ ` _ \| |/ _` / __|
 *             / ___ \| | | | | | | (_| \__ \
 *            /_/   \_\_| |_| |_|_|\__,_|___/  2022/8/24
 *
 * @example
 *
 * @development_log
 *
 */


#include <math.h>
#include <stdlib.h>

#include "reeds_shepp_curve.h"
#include "data_structure/array.h"

enum opus_rs_seg_type RS_seg_type[18][5] = {
        {RS_L, RS_R, RS_L, RS_N, RS_N},
        {RS_R, RS_L, RS_R, RS_N, RS_N},
        {RS_L, RS_R, RS_L, RS_R, RS_N},
        {RS_R, RS_L, RS_R, RS_L, RS_N},
        {RS_L, RS_R, RS_S, RS_L, RS_N},
        {RS_R, RS_L, RS_S, RS_R, RS_N},
        {RS_L, RS_S, RS_R, RS_L, RS_N},
        {RS_R, RS_S, RS_L, RS_R, RS_N},
        {RS_L, RS_R, RS_S, RS_R, RS_N},
        {RS_R, RS_L, RS_S, RS_L, RS_N},
        {RS_R, RS_S, RS_R, RS_L, RS_N},
        {RS_L, RS_S, RS_L, RS_R, RS_N},
        {RS_L, RS_S, RS_R, RS_N, RS_N},
        {RS_R, RS_S, RS_L, RS_N, RS_N},
        {RS_L, RS_S, RS_L, RS_N, RS_N},
        {RS_R, RS_S, RS_R, RS_N, RS_N},
        {RS_L, RS_R, RS_S, RS_L, RS_R},
        {RS_R, RS_L, RS_S, RS_R, RS_L}};

void opus_rs_curve_set_data(rs_path_t *path, opus_rs_seg_type_t type[5], double t, double u, double v, double w, double x)
{
	path->length[0]    = t;
	path->length[1]    = u;
	path->length[2]    = v;
	path->length[3]    = w;
	path->length[4]    = x;
	path->total_length = fabs(t) + fabs(u) + fabs(v) + fabs(w) + fabs(x);
	path->type         = type;
}

/**
 * @param type default RS_seg_type[0]
 * @param t default DBL_MAX
 * others all default to 0.0
 */
void rs_path_create_data(rs_path_t *path, opus_rs_seg_type_t type[5], double t, double u, double v, double w, double x)
{
	opus_rs_curve_set_data(path, type, t, u, v, w, x);
}

double rs_path_mod_2_pi(double x)
{
	double v = fmod(x, 2 * OPUS_PI);

	if (v < -OPUS_PI) {
		v += 2.0 * OPUS_PI;
	} else if (v > OPUS_PI) {
		v -= 2.0 * OPUS_PI;
	}

	return v;
}

void rs_path_polar(double x, double y, double *r, double *theta)
{
	*r     = sqrt(x * x + y * y);
	*theta = atan2(y, x);
}

void rs_path_tau_omega(double u, double v, double xi, double eta, double phi, double *tau, double *omega)
{
	double delta = rs_path_mod_2_pi(u - v);
	double A     = sin(u) - sin(delta);
	double B     = cos(u) - cos(delta) - 1.0;
	double t1    = atan2(eta * A - xi * B, xi * A + eta * B);
	double t2    = 2.0 * (cos(delta) - cos(v) - cos(u)) + 3.0;
	*tau         = (t2 < 0.0) ? rs_path_mod_2_pi(t1 + OPUS_PI) : rs_path_mod_2_pi(t1);
	*omega       = rs_path_mod_2_pi(*tau - u + v - phi);
}

/* Paper P390, formula 8.1 */
int rs_path_LpSpLp(double x, double y, double phi, double *t, double *u, double *v)
{
	rs_path_polar(x - sin(phi), y - 1.0 + cos(phi), u, t);

	if (*t >= 0.0) {
		*v = rs_path_mod_2_pi(phi - *t);
		if (*v >= 0.0) {
			return 1;
		}
	}

	return 0;
}

/* Paper P390, formula 8.2 */
int rs_path_LpSpRp(double x, double y, double phi, double *t, double *u, double *v)
{
	double t1, u1, theta;
	rs_path_polar(x + sin(phi), y - 1 - cos(phi), &u1, &t1);

	u1 = pow(u1, 2);

	if (u1 < 4.0) {
		return 0;
	}

	*u    = sqrt(u1 - 4.0);
	theta = atan2(2.0, *u);
	*t    = rs_path_mod_2_pi(t1 + theta);
	*v    = rs_path_mod_2_pi(*t - phi);

	return 1;
}

rs_path_t *rs_path_CSC(double x, double y, double phi, rs_path_t *path)
{
	double t, u, v, length_min = path->total_length, L;

	if (rs_path_LpSpLp(x, y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[14], t, u, v, 0.0, 0.0);
		length_min = L;
	}
	if (rs_path_LpSpLp(-x, y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[14], -t, -u, -v, 0.0, 0.0);
		length_min = L;
	}
	if (rs_path_LpSpLp(x, -y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[15], t, u, v, 0.0, 0.0);
		length_min = L;
	}
	if (rs_path_LpSpLp(-x, -y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[15], -t, -u, -v, 0.0, 0.0);
		length_min = L;
	}
	if (rs_path_LpSpRp(x, y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[12], t, u, v, 0.0, 0.0);
		length_min = L;
	}
	if (rs_path_LpSpRp(-x, y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[12], -t, -u, -v, 0.0, 0.0);
		length_min = L;
	}
	if (rs_path_LpSpRp(x, -y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[13], t, u, v, 0.0, 0.0);
		length_min = L;
	}
	if (rs_path_LpSpRp(-x, -y, phi, &t, &u, &v) && length_min > fabs(t) + fabs(u) + fabs(v)) {
		opus_rs_curve_set_data(path, RS_seg_type[13], -t, -u, -v, 0.0, 0.0);
	}

	return path;
}

/*
 * Paper P390 formula 8.3 and 8.4.
 * There is an error in the deduction of the formula in the paper.
 * It can be deduced by itself according to the inscribed circle.
 */
int rs_path_LpRmL(double x, double y, double phi, double *t, double *u, double *v)
{
	double xi  = x - sin(phi);
	double eta = y - 1.0 + cos(phi);
	double u1, theta;
	rs_path_polar(xi, eta, &u1, &theta);

	if (u1 > 4.0) {
		return 0;
	}

	*u = -2.0 * asin(0.25 * u1);
	*t = rs_path_mod_2_pi(theta + 0.5 * *u + OPUS_PI);
	*v = rs_path_mod_2_pi(phi - *t + *u);

	return 1;
}

rs_path_t *rs_path_CCC(double x, double y, double phi, rs_path_t *path)
{
	double t, u, v, L, xb, yb;
	double length_min = path->total_length;

	if (rs_path_LpRmL(x, y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[0], t, u, v, 0.0, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmL(-x, y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[0], -t, -u, -v, 0.0, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmL(x, -y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[1], t, u, v, 0.0, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmL(-x, -y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[1], -t, -u, -v, 0.0, 0.0);
		length_min = L;
	}

	xb = x * cos(phi) + y * sin(phi);
	yb = x * sin(phi) - y * cos(phi);
	if (rs_path_LpRmL(xb, yb, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[0], v, u, t, 0.0, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmL(-xb, yb, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[0], -v, -u, -t, 0.0, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmL(xb, -yb, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[1], v, u, t, 0.0, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmL(-xb, -yb, phi, &t, &u, &v) && length_min > fabs(t) + fabs(u) + fabs(v)) {
		opus_rs_curve_set_data(path, RS_seg_type[1], -v, -u, -t, 0.0, 0.0);
	}

	return path;
}

/* Paper P391 formula 8.7 */
int rs_path_LpRupLumRm(double x, double y, double phi, double *t, double *u, double *v)
{
	double xi  = x + sin(phi);
	double eta = y - 1.0 - cos(phi);
	double rho = 0.25 * (2.0 + sqrt(xi * xi + eta * eta));

	if (rho > 1.0) {
		return 0;
	}

	*u = acos(rho);
	rs_path_tau_omega(*u, -*u, xi, eta, phi, t, v);

	return 1;
}

/* Paper P391 formula 8.8 */
int rs_path_LpRumLumRp(double x, double y, double phi, double *t, double *u, double *v)
{
	double xi  = x + sin(phi);
	double eta = y - 1.0 - cos(phi);
	double rho = (20.0 - xi * xi - eta * eta) / 16.0;

	if (rho >= 0.0 && rho <= 1.0) {
		*u = -acos(rho);
		if (*u >= -OPUS_PI2) {
			rs_path_tau_omega(*u, *u, xi, eta, phi, t, v);
			return 1;
		}
	}

	return 0;
}

rs_path_t *rs_path_CCCC(double x, double y, double phi, rs_path_t *path)
{
	double t, u, v, L;
	double length_min = path->total_length;

	if (rs_path_LpRupLumRm(x, y, phi, &t, &u, &v) && length_min > (L = fabs(t) + 2.0 * fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[2], t, u, -u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRupLumRm(-x, y, -phi, &t, &u, &v) &&
	    length_min > (L = fabs(t) + 2.0 * fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[2], -t, -u, u, -v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRupLumRm(x, -y, -phi, &t, &u, &v) &&
	    length_min > (L = fabs(t) + 2.0 * fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[3], t, u, -u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRupLumRm(-x, -y, phi, &t, &u, &v) &&
	    length_min > (L = fabs(t) + 2.0 * fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[3], -t, -u, u, -v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRumLumRp(x, y, phi, &t, &u, &v) &&
	    length_min > (L = fabs(t) + 2.0 * fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[2], t, u, u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRumLumRp(-x, y, -phi, &t, &u, &v) &&
	    length_min > (L = fabs(t) + 2.0 * fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[2], -t, -u, -u, -v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRumLumRp(x, -y, -phi, &t, &u, &v) &&
	    length_min > (L = fabs(t) + 2.0 * fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[3], t, u, u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRumLumRp(-x, -y, phi, &t, &u, &v) && length_min > fabs(t) + 2.0 * fabs(u) + fabs(v)) {
		opus_rs_curve_set_data(path, RS_seg_type[3], -t, -u, -u, -v, 0.0);
	}

	return path;
}

/* Paper P391 formula 8.9 */
int rs_path_LpRmSmLm(double x, double y, double phi, double *t, double *u, double *v)
{
	double xi  = x - sin(phi);
	double eta = y - 1.0 + cos(phi);
	double rho, theta, r;

	rs_path_polar(xi, eta, &rho, &theta);

	if (rho < 2.0) {
		return 0;
	}

	r  = sqrt(rho * rho - 4.0);
	*u = 2.0 - r;
	*t = rs_path_mod_2_pi(theta + atan2(r, -2.0));
	*v = rs_path_mod_2_pi(phi - OPUS_PI2 - *t);

	return 1;
}

/* Paper P391 formula 8.10 */
int rs_path_LpRmSmRm(double x, double y, double phi, double *t, double *u, double *v)
{
	double xi  = x + sin(phi);
	double eta = y - 1.0 - cos(phi);
	double rho, theta;

	rs_path_polar(-eta, xi, &rho, &theta);

	if (rho < 2.0) {
		return 0;
	}

	*t = theta;
	*u = 2.0 - rho;
	*v = rs_path_mod_2_pi(*t + OPUS_PI2 - phi);

	return 1;
}

rs_path_t *rs_path_CCSC(double x, double y, double phi, rs_path_t *path)
{
	double t, u, v, L, xb, yb;
	double length_min = path->total_length - OPUS_PI2;

	if (rs_path_LpRmSmLm(x, y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[4], t, -OPUS_PI2, u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmLm(-x, y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[4], -t, OPUS_PI2, -u, -v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmLm(x, -y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[5], t, -OPUS_PI2, u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmLm(-x, -y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[5], -t, OPUS_PI2, -u, -v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(x, y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[8], t, -OPUS_PI2, u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(-x, y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[8], -t, OPUS_PI2, -u, -v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(x, -y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[9], t, -OPUS_PI2, u, v, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(-x, -y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[9], -t, OPUS_PI2, -u, -v, 0.0);
		length_min = L;
	}

	xb = x * cos(phi) + y * sin(phi);
	yb = x * sin(phi) - y * cos(phi);
	if (rs_path_LpRmSmLm(xb, yb, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[6], v, u, -OPUS_PI2, t, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmLm(-xb, yb, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[6], -v, -u, OPUS_PI2, -t, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmLm(xb, -yb, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[7], v, u, -OPUS_PI2, t, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmLm(-xb, -yb, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[7], -v, -u, OPUS_PI2, -t, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(xb, yb, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[10], v, u, -OPUS_PI2, t, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(-xb, yb, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[10], -v, -u, OPUS_PI2, -t, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(xb, -yb, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[11], v, u, -OPUS_PI2, t, 0.0);
		length_min = L;
	}

	if (rs_path_LpRmSmRm(-xb, -yb, phi, &t, &u, &v) && length_min > fabs(t) + fabs(u) + fabs(v)) {
		opus_rs_curve_set_data(path, RS_seg_type[11], -v, -u, OPUS_PI2, -t, 0.0);
	}

	return path;
}

/*
 * Paper P391 formula 8.11
 * There is an error in the deduction of the formula in the paper.
 * It can be deduced by itself according to the inscribed circle.
 */
int rs_path_LpRmSLmRp(double x, double y, double phi, double *t, double *u, double *v)
{
	double xi  = x + sin(phi);
	double eta = y - 1.0 - cos(phi);
	double rho, theta;

	rs_path_polar(xi, eta, &rho, &theta);

	if (rho >= 2.0) {
		*u = 4.0 - sqrt(rho * rho - 4.0);

		if (*u <= 0.0) {
			*t = rs_path_mod_2_pi(atan2((4.0 - *u) * xi - 2.0 * eta, -2.0 * xi + (*u - 4.0) * eta));
			*v = rs_path_mod_2_pi(*t - phi);

			return 1;
		}
	}

	return 0;
}

rs_path_t *rs_path_CCSCC(double x, double y, double phi, rs_path_t *path)
{
	double t, u, v, L;
	double length_min = path->total_length - OPUS_PI;

	if (rs_path_LpRmSLmRp(x, y, phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[16], t, -OPUS_PI2, u, -OPUS_PI2, v);
		length_min = L;
	}

	if (rs_path_LpRmSLmRp(-x, y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[16], -t, OPUS_PI2, -u, OPUS_PI2, -v);
		length_min = L;
	}

	if (rs_path_LpRmSLmRp(x, -y, -phi, &t, &u, &v) && length_min > (L = fabs(t) + fabs(u) + fabs(v))) {
		opus_rs_curve_set_data(path, RS_seg_type[17], t, -OPUS_PI2, u, -OPUS_PI2, v);
		length_min = L;
	}

	if (rs_path_LpRmSLmRp(-x, -y, phi, &t, &u, &v) && length_min > fabs(t) + fabs(u) + fabs(v)) {
		opus_rs_curve_set_data(path, RS_seg_type[17], -t, OPUS_PI2, -u, OPUS_PI2, -v);
	}

	return path;
}

void opus_rs_curve_calc(rs_path_t *path, double x_0, double y_0, double yaw_0, double x_1, double y_1, double yaw_1, double turning_radius)
{
	/* translation */
	double dx = x_1 - x_0;
	double dy = y_1 - y_0;

	/* rotate */
	double c   = cos(yaw_0); /* 2d rotation matrix */
	double s   = sin(yaw_0);
	double x   = c * dx + s * dy;
	double y   = -s * dx + c * dy;
	double phi = yaw_1 - yaw_0;

	opus_rs_curve_set_data(path, RS_seg_type[0], DBL_MAX, 0.0, 0.0, 0.0, 0.0);

	path->start_state.x  = x_0;
	path->start_state.y  = y_0;
	path->start_state.z  = yaw_0;
	path->goal_state.x   = x_1;
	path->goal_state.y   = y_1;
	path->goal_state.z   = yaw_1;
	path->turning_radius = turning_radius;

	x /= path->turning_radius;
	y /= path->turning_radius;

	path = rs_path_CSC(x, y, phi, path);
	path = rs_path_CCC(x, y, phi, path);
	path = rs_path_CCCC(x, y, phi, path);
	path = rs_path_CCSC(x, y, phi, path);
	path = rs_path_CCSCC(x, y, phi, path);
}

double opus_rs_curve_distance(rs_path_t *path)
{
	return path->turning_radius * path->total_length;
}

opus_vec3 *opus_rs_curve_build_trajectory(rs_path_t *rs_path, double step_size)
{
	double       path_length          = opus_rs_curve_distance(rs_path);
	double       interpolation_number = (unsigned int) (path_length / step_size);
	double       phi;
	unsigned int i, j;
	opus_vec3   *path_poses;

	opus_arr_create(path_poses, sizeof(opus_vec3));

	for (i = 0; i <= interpolation_number; ++i) {
		double v;
		double t   = i * 1.0 / interpolation_number;
		double seg = t * rs_path->total_length;

		opus_vec3 temp_pose = {0.0, 0.0, 0.0};
		opus_vec3 pose; /* final position calculated */

		temp_pose.z = rs_path->start_state.z;
		for (j = 0; j < 5u && seg > 0; ++j) {
			if (rs_path->length[j] < 0.0) {
				v = opus_max(-seg, rs_path->length[j]);
				seg += v;
			} else {
				v = opus_min(seg, rs_path->length[j]);
				seg -= v;
			}

			phi = temp_pose.z;
			switch (rs_path->type[j]) {
				case RS_L:
					temp_pose.x = sin(phi + v) - sin(phi) + temp_pose.x;
					temp_pose.y = -cos(phi + v) + cos(phi) + temp_pose.y;
					temp_pose.z = phi + v;
					break;
				case RS_R:
					temp_pose.x = -sin(phi - v) + sin(phi) + temp_pose.x;
					temp_pose.y = cos(phi - v) - cos(phi) + temp_pose.y;
					temp_pose.z = phi - v;
					break;
				case RS_S:
					temp_pose.x = v * cos(phi) + temp_pose.x;
					temp_pose.y = v * sin(phi) + temp_pose.y;
					temp_pose.z = phi;
					break;
				case RS_N:
					break;
			}
		}

		pose.x = temp_pose.x * rs_path->turning_radius + rs_path->start_state.x;
		pose.y = temp_pose.y * rs_path->turning_radius + rs_path->start_state.y;
		pose.z = temp_pose.z;

		opus_arr_push(path_poses, &pose);
	}

	return path_poses;
}
