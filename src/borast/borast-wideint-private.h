/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2004 Keith Packard
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Keith Packard
 *
 * Contributor(s):
 *	Keith R. Packard <keithp@keithp.com>
 *
 */

#ifndef BORAST_WIDEINT_H
#define BORAST_WIDEINT_H

#include "borast-wideint-type-private.h"

#include "borast-compiler-private.h"

/*
 * 64-bit datatypes.  Two separate implementations, one using
 * built-in 64-bit signed/unsigned types another implemented
 * as a pair of 32-bit ints
 */

#define I borast_private borast_const

#if !HAVE_UINT64_T

borast_uquorem64_t I
_borast_uint64_divrem (borast_uint64_t num, borast_uint64_t den);

borast_uint64_t I	_borast_uint32_to_uint64 (uint32_t i);
#define			_borast_uint64_to_uint32(a)  ((a).lo)
borast_uint64_t I	_borast_uint64_add (borast_uint64_t a, borast_uint64_t b);
borast_uint64_t I	_borast_uint64_sub (borast_uint64_t a, borast_uint64_t b);
borast_uint64_t I	_borast_uint64_mul (borast_uint64_t a, borast_uint64_t b);
borast_uint64_t I	_borast_uint32x32_64_mul (uint32_t a, uint32_t b);
borast_uint64_t I	_borast_uint64_lsl (borast_uint64_t a, int shift);
borast_uint64_t I	_borast_uint64_rsl (borast_uint64_t a, int shift);
borast_uint64_t I	_borast_uint64_rsa (borast_uint64_t a, int shift);
int	       I	_borast_uint64_lt (borast_uint64_t a, borast_uint64_t b);
int	       I	_borast_uint64_cmp (borast_uint64_t a, borast_uint64_t b);
int	       I	_borast_uint64_eq (borast_uint64_t a, borast_uint64_t b);
borast_uint64_t I	_borast_uint64_negate (borast_uint64_t a);
#define			_borast_uint64_is_zero(a) ((a).hi == 0 && (a).lo == 0)
#define			_borast_uint64_negative(a)   (((int32_t) ((a).hi)) < 0)
borast_uint64_t I	_borast_uint64_not (borast_uint64_t a);

#define			_borast_uint64_to_int64(i)   (i)
#define			_borast_int64_to_uint64(i)   (i)

borast_int64_t  I	_borast_int32_to_int64(int32_t i);
#define			_borast_int64_to_int32(a)    ((int32_t) _borast_uint64_to_uint32(a))
#define			_borast_int64_add(a,b)	    _borast_uint64_add (a,b)
#define			_borast_int64_sub(a,b)	    _borast_uint64_sub (a,b)
#define			_borast_int64_mul(a,b)	    _borast_uint64_mul (a,b)
borast_int64_t  I	_borast_int32x32_64_mul (int32_t a, int32_t b);
int	       I	_borast_int64_lt (borast_int64_t a, borast_int64_t b);
int	       I	_borast_int64_cmp (borast_int64_t a, borast_int64_t b);
#define			_borast_int64_is_zero(a)	    _borast_uint64_is_zero (a)
#define			_borast_int64_eq(a,b)	    _borast_uint64_eq (a,b)
#define			_borast_int64_lsl(a,b)	    _borast_uint64_lsl (a,b)
#define			_borast_int64_rsl(a,b)	    _borast_uint64_rsl (a,b)
#define			_borast_int64_rsa(a,b)	    _borast_uint64_rsa (a,b)
#define			_borast_int64_negate(a)	    _borast_uint64_negate(a)
#define			_borast_int64_negative(a)    (((int32_t) ((a).hi)) < 0)
#define			_borast_int64_not(a)	    _borast_uint64_not(a)

#else

static inline borast_uquorem64_t
_borast_uint64_divrem (borast_uint64_t num, borast_uint64_t den)
{
    borast_uquorem64_t	qr;

    qr.quo = num / den;
    qr.rem = num % den;
    return qr;
}

#define			_borast_uint32_to_uint64(i)  ((uint64_t) (i))
#define			_borast_uint64_to_uint32(i)  ((uint32_t) (i))
#define			_borast_uint64_add(a,b)	    ((a) + (b))
#define			_borast_uint64_sub(a,b)	    ((a) - (b))
#define			_borast_uint64_mul(a,b)	    ((a) * (b))
#define			_borast_uint32x32_64_mul(a,b)	((uint64_t) (a) * (b))
#define			_borast_uint64_lsl(a,b)	    ((a) << (b))
#define			_borast_uint64_rsl(a,b)	    ((uint64_t) (a) >> (b))
#define			_borast_uint64_rsa(a,b)	    ((uint64_t) ((int64_t) (a) >> (b)))
#define			_borast_uint64_lt(a,b)	    ((a) < (b))
#define                 _borast_uint64_cmp(a,b)       ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define			_borast_uint64_is_zero(a)    ((a) == 0)
#define			_borast_uint64_eq(a,b)	    ((a) == (b))
#define			_borast_uint64_negate(a)	    ((uint64_t) -((int64_t) (a)))
#define			_borast_uint64_negative(a)   ((int64_t) (a) < 0)
#define			_borast_uint64_not(a)	    (~(a))

#define			_borast_uint64_to_int64(i)   ((int64_t) (i))
#define			_borast_int64_to_uint64(i)   ((uint64_t) (i))

#define			_borast_int32_to_int64(i)    ((int64_t) (i))
#define			_borast_int64_to_int32(i)    ((int32_t) (i))
#define			_borast_int64_add(a,b)	    ((a) + (b))
#define			_borast_int64_sub(a,b)	    ((a) - (b))
#define			_borast_int64_mul(a,b)	    ((a) * (b))
#define			_borast_int32x32_64_mul(a,b) ((int64_t) (a) * (b))
#define			_borast_int64_lt(a,b)	    ((a) < (b))
#define                 _borast_int64_cmp(a,b)       ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define			_borast_int64_is_zero(a)     ((a) == 0)
#define			_borast_int64_eq(a,b)	    ((a) == (b))
#define			_borast_int64_lsl(a,b)	    ((a) << (b))
#define			_borast_int64_rsl(a,b)	    ((int64_t) ((uint64_t) (a) >> (b)))
#define			_borast_int64_rsa(a,b)	    ((int64_t) (a) >> (b))
#define			_borast_int64_negate(a)	    (-(a))
#define			_borast_int64_negative(a)    ((a) < 0)
#define			_borast_int64_not(a)	    (~(a))

#endif

/*
 * 64-bit comparisions derived from lt or eq
 */
#define			_borast_uint64_le(a,b)	    (!_borast_uint64_gt(a,b))
#define			_borast_uint64_ne(a,b)	    (!_borast_uint64_eq(a,b))
#define			_borast_uint64_ge(a,b)	    (!_borast_uint64_lt(a,b))
#define			_borast_uint64_gt(a,b)	    _borast_uint64_lt(b,a)

#define			_borast_int64_le(a,b)	    (!_borast_int64_gt(a,b))
#define			_borast_int64_ne(a,b)	    (!_borast_int64_eq(a,b))
#define			_borast_int64_ge(a,b)	    (!_borast_int64_lt(a,b))
#define			_borast_int64_gt(a,b)	    _borast_int64_lt(b,a)

/*
 * As the C implementation always computes both, create
 * a function which returns both for the 'native' type as well
 */

static inline borast_quorem64_t
_borast_int64_divrem (borast_int64_t num, borast_int64_t den)
{
    int			num_neg = _borast_int64_negative (num);
    int			den_neg = _borast_int64_negative (den);
    borast_uquorem64_t	uqr;
    borast_quorem64_t	qr;

    if (num_neg)
	num = _borast_int64_negate (num);
    if (den_neg)
	den = _borast_int64_negate (den);
    uqr = _borast_uint64_divrem (num, den);
    if (num_neg)
	qr.rem = _borast_int64_negate (uqr.rem);
    else
	qr.rem = uqr.rem;
    if (num_neg != den_neg)
	qr.quo = (borast_int64_t) _borast_int64_negate (uqr.quo);
    else
	qr.quo = (borast_int64_t) uqr.quo;
    return qr;
}

#if 0
static inline int32_t
_borast_int64_32_div (borast_int64_t num, int32_t den)
{
    return num / den;
}
#endif

static inline int32_t
_borast_int64_32_div (borast_int64_t num, int32_t den)
{
  borast_quorem64_t quorem;
  borast_int64_t den64;

  den64 = _borast_int32_to_int64 (den);
  quorem = _borast_int64_divrem (num, den64);

  return _borast_int64_to_int32 (quorem.quo);
}

/*
 * 128-bit datatypes.  Again, provide two implementations in
 * case the machine has a native 128-bit datatype.  GCC supports int128_t
 * on ia64
 */

#if !HAVE_UINT128_T

borast_uint128_t I	_borast_uint32_to_uint128 (uint32_t i);
borast_uint128_t I	_borast_uint64_to_uint128 (borast_uint64_t i);
#define			_borast_uint128_to_uint64(a)	((a).lo)
#define			_borast_uint128_to_uint32(a)	_borast_uint64_to_uint32(_borast_uint128_to_uint64(a))
borast_uint128_t I	_borast_uint128_add (borast_uint128_t a, borast_uint128_t b);
borast_uint128_t I	_borast_uint128_sub (borast_uint128_t a, borast_uint128_t b);
borast_uint128_t I	_borast_uint128_mul (borast_uint128_t a, borast_uint128_t b);
borast_uint128_t I	_borast_uint64x64_128_mul (borast_uint64_t a, borast_uint64_t b);
borast_uint128_t I	_borast_uint128_lsl (borast_uint128_t a, int shift);
borast_uint128_t I	_borast_uint128_rsl (borast_uint128_t a, int shift);
borast_uint128_t I	_borast_uint128_rsa (borast_uint128_t a, int shift);
int	        I	_borast_uint128_lt (borast_uint128_t a, borast_uint128_t b);
int	        I	_borast_uint128_cmp (borast_uint128_t a, borast_uint128_t b);
int	        I	_borast_uint128_eq (borast_uint128_t a, borast_uint128_t b);
#define			_borast_uint128_is_zero(a) (_borast_uint64_is_zero ((a).hi) && _borast_uint64_is_zero ((a).lo))
borast_uint128_t I	_borast_uint128_negate (borast_uint128_t a);
#define			_borast_uint128_negative(a)  (_borast_uint64_negative(a.hi))
borast_uint128_t I	_borast_uint128_not (borast_uint128_t a);

#define			_borast_uint128_to_int128(i)	(i)
#define			_borast_int128_to_uint128(i)	(i)

borast_int128_t  I	_borast_int32_to_int128 (int32_t i);
borast_int128_t  I	_borast_int64_to_int128 (borast_int64_t i);
#define			_borast_int128_to_int64(a)   ((borast_int64_t) (a).lo)
#define			_borast_int128_to_int32(a)   _borast_int64_to_int32(_borast_int128_to_int64(a))
#define			_borast_int128_add(a,b)	    _borast_uint128_add(a,b)
#define			_borast_int128_sub(a,b)	    _borast_uint128_sub(a,b)
#define			_borast_int128_mul(a,b)	    _borast_uint128_mul(a,b)
borast_int128_t I _borast_int64x64_128_mul (borast_int64_t a, borast_int64_t b);
#define                 _borast_int64x32_128_mul(a, b) _borast_int64x64_128_mul(a, _borast_int32_to_int64(b))
#define			_borast_int128_lsl(a,b)	    _borast_uint128_lsl(a,b)
#define			_borast_int128_rsl(a,b)	    _borast_uint128_rsl(a,b)
#define			_borast_int128_rsa(a,b)	    _borast_uint128_rsa(a,b)
int 	        I	_borast_int128_lt (borast_int128_t a, borast_int128_t b);
int	        I	_borast_int128_cmp (borast_int128_t a, borast_int128_t b);
#define			_borast_int128_is_zero(a)    _borast_uint128_is_zero (a)
#define			_borast_int128_eq(a,b)	    _borast_uint128_eq (a,b)
#define			_borast_int128_negate(a)	    _borast_uint128_negate(a)
#define			_borast_int128_negative(a)   (_borast_uint128_negative(a))
#define			_borast_int128_not(a)	    _borast_uint128_not(a)

#else	/* !HAVE_UINT128_T */

#define			_borast_uint32_to_uint128(i) ((uint128_t) (i))
#define			_borast_uint64_to_uint128(i) ((uint128_t) (i))
#define			_borast_uint128_to_uint64(i) ((uint64_t) (i))
#define			_borast_uint128_to_uint32(i) ((uint32_t) (i))
#define			_borast_uint128_add(a,b)	    ((a) + (b))
#define			_borast_uint128_sub(a,b)	    ((a) - (b))
#define			_borast_uint128_mul(a,b)	    ((a) * (b))
#define			_borast_uint64x64_128_mul(a,b)	((uint128_t) (a) * (b))
#define			_borast_uint128_lsl(a,b)	    ((a) << (b))
#define			_borast_uint128_rsl(a,b)	    ((uint128_t) (a) >> (b))
#define			_borast_uint128_rsa(a,b)	    ((uint128_t) ((int128_t) (a) >> (b)))
#define			_borast_uint128_lt(a,b)	    ((a) < (b))
#define			_borast_uint128_cmp(a,b)	    ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define			_borast_uint128_is_zero(a)   ((a) == 0)
#define			_borast_uint128_eq(a,b)	    ((a) == (b))
#define			_borast_uint128_negate(a)    ((uint128_t) -((int128_t) (a)))
#define			_borast_uint128_negative(a)  ((int128_t) (a) < 0)
#define			_borast_uint128_not(a)	    (~(a))

#define			_borast_uint128_to_int128(i) ((int128_t) (i))
#define			_borast_int128_to_uint128(i) ((uint128_t) (i))

#define			_borast_int32_to_int128(i)   ((int128_t) (i))
#define			_borast_int64_to_int128(i)   ((int128_t) (i))
#define			_borast_int128_to_int64(i)   ((int64_t) (i))
#define			_borast_int128_to_int32(i)   ((int32_t) (i))
#define			_borast_int128_add(a,b)	    ((a) + (b))
#define			_borast_int128_sub(a,b)	    ((a) - (b))
#define			_borast_int128_mul(a,b)	    ((a) * (b))
#define			_borast_int64x64_128_mul(a,b) ((int128_t) (a) * (b))
#define                 _borast_int64x32_128_mul(a, b) _borast_int64x64_128_mul(a, _borast_int32_to_int64(b))
#define			_borast_int128_lt(a,b)	    ((a) < (b))
#define			_borast_int128_cmp(a,b)	    ((a) == (b) ? 0 : (a) < (b) ? -1 : 1)
#define			_borast_int128_is_zero(a)    ((a) == 0)
#define			_borast_int128_eq(a,b)	    ((a) == (b))
#define			_borast_int128_lsl(a,b)	    ((a) << (b))
#define			_borast_int128_rsl(a,b)	    ((int128_t) ((uint128_t) (a) >> (b)))
#define			_borast_int128_rsa(a,b)	    ((int128_t) (a) >> (b))
#define			_borast_int128_negate(a)	    (-(a))
#define			_borast_int128_negative(a)   ((a) < 0)
#define			_borast_int128_not(a)	    (~(a))

#endif	/* HAVE_UINT128_T */

borast_uquorem128_t I
_borast_uint128_divrem (borast_uint128_t num, borast_uint128_t den);

borast_quorem128_t I
_borast_int128_divrem (borast_int128_t num, borast_int128_t den);

borast_uquorem64_t I
_borast_uint_96by64_32x64_divrem (borast_uint128_t num,
				 borast_uint64_t  den);

borast_quorem64_t I
_borast_int_96by64_32x64_divrem (borast_int128_t num,
				borast_int64_t  den);

#define			_borast_uint128_le(a,b)	    (!_borast_uint128_gt(a,b))
#define			_borast_uint128_ne(a,b)	    (!_borast_uint128_eq(a,b))
#define			_borast_uint128_ge(a,b)	    (!_borast_uint128_lt(a,b))
#define			_borast_uint128_gt(a,b)	    _borast_uint128_lt(b,a)

#define			_borast_int128_le(a,b)	    (!_borast_int128_gt(a,b))
#define			_borast_int128_ne(a,b)	    (!_borast_int128_eq(a,b))
#define			_borast_int128_ge(a,b)	    (!_borast_int128_lt(a,b))
#define			_borast_int128_gt(a,b)	    _borast_int128_lt(b,a)

#undef I

#endif /* BORAST_WIDEINT_H */
