/* sintmi.f -- translated by f2c (version 20100827).
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

static integer c__3 = 3;
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

/* Subroutine */ int sintmi_(integer *n, doublereal *wsave, integer *lensav, 
	integer *ier)
{
    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    double log(doublereal), atan(doublereal), sin(doublereal);

    /* Local variables */
    integer k;
    doublereal dt, pi;
    integer np1, ns2, ier1, lnsv;
    extern /* Subroutine */ int rfftmi_(integer *, doublereal *, integer *, 
	    integer *), xerfft_(char *, integer *, ftnlen);


    /* Parameter adjustments */
    --wsave;

    /* Function Body */
    *ier = 0;

    if (*lensav < *n / 2 + *n + (integer) (log((doublereal) (*n)) / log(2.)) 
	    + 4) {
	*ier = 2;
	xerfft_("SINTMI", &c__3, (ftnlen)6);
	goto L300;
    }

    pi = atan(1.) * 4.;
    if (*n <= 1) {
	return 0;
    }
    ns2 = *n / 2;
    np1 = *n + 1;
    dt = pi / (doublereal) np1;
    i__1 = ns2;
    for (k = 1; k <= i__1; ++k) {
	wsave[k] = sin(k * dt) * 2.;
/* L101: */
    }
    lnsv = np1 + (integer) (log((doublereal) np1) / log(2.)) + 4;
    rfftmi_(&np1, &wsave[ns2 + 1], &lnsv, &ier1);
    if (ier1 != 0) {
	*ier = 20;
	xerfft_("SINTMI", &c_n5, (ftnlen)6);
    }

L300:
    return 0;
} /* sintmi_ */

