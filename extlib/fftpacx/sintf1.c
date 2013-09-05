/* sintf1.f -- translated by f2c (version 20100827).
   You must link the resulting object file with libf2c:
	on Microsoft Windows system, link with libf2c.lib;
	on Linux or Unix systems, link with .../path/to/libf2c.a -lm
	or, if you install libf2c.a in a standard place, with -lf2c -lm
	-- in that order, at the end of the command line, as in
		cc *.o -lf2c -lm
	Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

		http://www.netlib.org/f2c/libf2c.zip
*/

#include "f2c.h"

/* Table of constant values */

static integer c__1 = 1;
static integer c_n5 = -5;

/*     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*     *                                                               * */
/*     *                  copyright (c) 2011 by UCAR                   * */
/*     *                                                               * */
/*     *       University Corporation for Atmospheric Research         * */
/*     *                                                               * */
/*     *                      all rights reserved                      * */
/*     *                                                               * */
/*     *                     FFTPACK  version 5.1                      * */
/*     *                                                               * */
/*     *                 A Fortran Package of Fast Fourier             * */
/*     *                                                               * */
/*     *                Subroutines and Example Programs               * */
/*     *                                                               * */
/*     *                             by                                * */
/*     *                                                               * */
/*     *               Paul Swarztrauber and Dick Valent               * */
/*     *                                                               * */
/*     *                             of                                * */
/*     *                                                               * */
/*     *         the National Center for Atmospheric Research          * */
/*     *                                                               * */
/*     *                Boulder, Colorado  (80307)  U.S.A.             * */
/*     *                                                               * */
/*     *                   which is sponsored by                       * */
/*     *                                                               * */
/*     *              the National Science Foundation                  * */
/*     *                                                               * */
/*     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Subroutine */ int sintf1_(integer *n, integer *inc, doublereal *x, 
	doublereal *wsave, doublereal *xh, doublereal *work, integer *ier)
{
    /* System generated locals */
    integer x_dim1, x_offset, i__1;

    /* Builtin functions */
    double sqrt(doublereal), log(doublereal);

    /* Local variables */
    integer i__, k;
    doublereal t1, t2;
    integer kc, np1, ns2, ier1, modn;
    doublereal dsum;
    integer lnxh, lnwk, lnsv;
    doublereal sfnp1, xhold;
    extern /* Subroutine */ int rfft1f_(integer *, integer *, doublereal *, 
	    integer *, doublereal *, integer *, doublereal *, integer *, 
	    integer *);
    doublereal ssqrt3;
    extern /* Subroutine */ int xerfft_(char *, integer *, ftnlen);

    /* Parameter adjustments */
    x_dim1 = *inc;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    --wsave;
    --xh;

    /* Function Body */
    *ier = 0;
    if ((i__1 = *n - 2) < 0) {
	goto L200;
    } else if (i__1 == 0) {
	goto L102;
    } else {
	goto L103;
    }
L102:
    ssqrt3 = 1. / sqrt(3.);
    xhold = ssqrt3 * (x[x_dim1 + 1] + x[(x_dim1 << 1) + 1]);
    x[(x_dim1 << 1) + 1] = ssqrt3 * (x[x_dim1 + 1] - x[(x_dim1 << 1) + 1]);
    x[x_dim1 + 1] = xhold;
    goto L200;
L103:
    np1 = *n + 1;
    ns2 = *n / 2;
    i__1 = ns2;
    for (k = 1; k <= i__1; ++k) {
	kc = np1 - k;
	t1 = x[k * x_dim1 + 1] - x[kc * x_dim1 + 1];
	t2 = wsave[k] * (x[k * x_dim1 + 1] + x[kc * x_dim1 + 1]);
	xh[k + 1] = t1 + t2;
	xh[kc + 1] = t2 - t1;
/* L104: */
    }
    modn = *n % 2;
    if (modn == 0) {
	goto L124;
    }
    xh[ns2 + 2] = x[(ns2 + 1) * x_dim1 + 1] * 4.;
L124:
    xh[1] = 0.;
    lnxh = np1;
    lnsv = np1 + (integer) (log((doublereal) np1) / log(2.)) + 4;
    lnwk = np1;

    rfft1f_(&np1, &c__1, &xh[1], &lnxh, &wsave[ns2 + 1], &lnsv, work, &lnwk, &
	    ier1);
    if (ier1 != 0) {
	*ier = 20;
	xerfft_("SINTF1", &c_n5, (ftnlen)6);
	goto L200;
    }

    if (np1 % 2 != 0) {
	goto L30;
    }
    xh[np1] += xh[np1];
L30:
    sfnp1 = 1. / (doublereal) np1;
    x[x_dim1 + 1] = xh[1] * .5;
    dsum = x[x_dim1 + 1];
    i__1 = *n;
    for (i__ = 3; i__ <= i__1; i__ += 2) {
	x[(i__ - 1) * x_dim1 + 1] = xh[i__] * .5;
	dsum += xh[i__ - 1] * .5;
	x[i__ * x_dim1 + 1] = dsum;
/* L105: */
    }
    if (modn != 0) {
	goto L200;
    }
    x[*n * x_dim1 + 1] = xh[*n + 1] * .5;
L200:
    return 0;
} /* sintf1_ */

