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
 */

#include "borast-wideint-private.h"

#if HAVE_UINT64_T

#define uint64_lo32(i)	((i) & 0xffffffff)
#define uint64_hi32(i)	((i) >> 32)
#define uint64_lo(i)	((i) & 0xffffffff)
#define uint64_hi(i)	((i) >> 32)
#define uint64_shift32(i)   ((i) << 32)
#define uint64_carry32	(((uint64_t) 1) << 32)

#define _borast_uint32s_to_uint64(h,l) ((uint64_t) (h) << 32 | (l))

#else

#define uint64_lo32(i)	((i).lo)
#define uint64_hi32(i)	((i).hi)

static borast_uint64_t
uint64_lo (borast_uint64_t i)
{
    borast_uint64_t  s;

    s.lo = i.lo;
    s.hi = 0;
    return s;
}

static borast_uint64_t
uint64_hi (borast_uint64_t i)
{
    borast_uint64_t  s;

    s.lo = i.hi;
    s.hi = 0;
    return s;
}

static borast_uint64_t
uint64_shift32 (borast_uint64_t i)
{
    borast_uint64_t  s;

    s.lo = 0;
    s.hi = i.lo;
    return s;
}

static const borast_uint64_t uint64_carry32 = { 0, 1 };

borast_uint64_t
_borast_uint32_to_uint64 (uint32_t i)
{
    borast_uint64_t	q;

    q.lo = i;
    q.hi = 0;
    return q;
}

borast_int64_t
_borast_int32_to_int64 (int32_t i)
{
    borast_uint64_t	q;

    q.lo = i;
    q.hi = i < 0 ? -1 : 0;
    return q;
}

static borast_uint64_t
_borast_uint32s_to_uint64 (uint32_t h, uint32_t l)
{
    borast_uint64_t	q;

    q.lo = l;
    q.hi = h;
    return q;
}

borast_uint64_t
_borast_uint64_add (borast_uint64_t a, borast_uint64_t b)
{
    borast_uint64_t	s;

    s.hi = a.hi + b.hi;
    s.lo = a.lo + b.lo;
    if (s.lo < a.lo)
	s.hi++;
    return s;
}

borast_uint64_t
_borast_uint64_sub (borast_uint64_t a, borast_uint64_t b)
{
    borast_uint64_t	s;

    s.hi = a.hi - b.hi;
    s.lo = a.lo - b.lo;
    if (s.lo > a.lo)
	s.hi--;
    return s;
}

#define uint32_lo(i)	((i) & 0xffff)
#define uint32_hi(i)	((i) >> 16)
#define uint32_carry16	((1) << 16)

borast_uint64_t
_borast_uint32x32_64_mul (uint32_t a, uint32_t b)
{
    borast_uint64_t  s;

    uint16_t	ah, al, bh, bl;
    uint32_t	r0, r1, r2, r3;

    al = uint32_lo (a);
    ah = uint32_hi (a);
    bl = uint32_lo (b);
    bh = uint32_hi (b);

    r0 = (uint32_t) al * bl;
    r1 = (uint32_t) al * bh;
    r2 = (uint32_t) ah * bl;
    r3 = (uint32_t) ah * bh;

    r1 += uint32_hi(r0);    /* no carry possible */
    r1 += r2;		    /* but this can carry */
    if (r1 < r2)	    /* check */
	r3 += uint32_carry16;

    s.hi = r3 + uint32_hi(r1);
    s.lo = (uint32_lo (r1) << 16) + uint32_lo (r0);
    return s;
}

borast_int64_t
_borast_int32x32_64_mul (int32_t a, int32_t b)
{
    borast_int64_t s;
    s = _borast_uint32x32_64_mul ((uint32_t) a, (uint32_t) b);
    if (a < 0)
	s.hi -= b;
    if (b < 0)
	s.hi -= a;
    return s;
}

borast_uint64_t
_borast_uint64_mul (borast_uint64_t a, borast_uint64_t b)
{
    borast_uint64_t	s;

    s = _borast_uint32x32_64_mul (a.lo, b.lo);
    s.hi += a.lo * b.hi + a.hi * b.lo;
    return s;
}

borast_uint64_t
_borast_uint64_lsl (borast_uint64_t a, int shift)
{
    if (shift >= 32)
    {
	a.hi = a.lo;
	a.lo = 0;
	shift -= 32;
    }
    if (shift)
    {
	a.hi = a.hi << shift | a.lo >> (32 - shift);
	a.lo = a.lo << shift;
    }
    return a;
}

borast_uint64_t
_borast_uint64_rsl (borast_uint64_t a, int shift)
{
    if (shift >= 32)
    {
	a.lo = a.hi;
	a.hi = 0;
	shift -= 32;
    }
    if (shift)
    {
	a.lo = a.lo >> shift | a.hi << (32 - shift);
	a.hi = a.hi >> shift;
    }
    return a;
}

#define _borast_uint32_rsa(a,n)	((uint32_t) (((int32_t) (a)) >> (n)))

borast_int64_t
_borast_uint64_rsa (borast_int64_t a, int shift)
{
    if (shift >= 32)
    {
	a.lo = a.hi;
	a.hi = _borast_uint32_rsa (a.hi, 31);
	shift -= 32;
    }
    if (shift)
    {
	a.lo = a.lo >> shift | a.hi << (32 - shift);
	a.hi = _borast_uint32_rsa (a.hi, shift);
    }
    return a;
}

int
_borast_uint64_lt (borast_uint64_t a, borast_uint64_t b)
{
    return (a.hi < b.hi ||
	    (a.hi == b.hi && a.lo < b.lo));
}

int
_borast_uint64_eq (borast_uint64_t a, borast_uint64_t b)
{
    return a.hi == b.hi && a.lo == b.lo;
}

int
_borast_int64_lt (borast_int64_t a, borast_int64_t b)
{
    if (_borast_int64_negative (a) && !_borast_int64_negative (b))
	return 1;
    if (!_borast_int64_negative (a) && _borast_int64_negative (b))
	return 0;
    return _borast_uint64_lt (a, b);
}

int
_borast_uint64_cmp (borast_uint64_t a, borast_uint64_t b)
{
    if (a.hi < b.hi)
	return -1;
    else if (a.hi > b.hi)
	return 1;
    else if (a.lo < b.lo)
	return -1;
    else if (a.lo > b.lo)
	return 1;
    else
	return 0;
}

int
_borast_int64_cmp (borast_int64_t a, borast_int64_t b)
{
    if (_borast_int64_negative (a) && !_borast_int64_negative (b))
	return -1;
    if (!_borast_int64_negative (a) && _borast_int64_negative (b))
	return 1;

    return _borast_uint64_cmp (a, b);
}

borast_uint64_t
_borast_uint64_not (borast_uint64_t a)
{
    a.lo = ~a.lo;
    a.hi = ~a.hi;
    return a;
}

borast_uint64_t
_borast_uint64_negate (borast_uint64_t a)
{
    a.lo = ~a.lo;
    a.hi = ~a.hi;
    if (++a.lo == 0)
	++a.hi;
    return a;
}

/*
 * Simple bit-at-a-time divide.
 */
borast_uquorem64_t
_borast_uint64_divrem (borast_uint64_t num, borast_uint64_t den)
{
    borast_uquorem64_t	qr;
    borast_uint64_t	bit;
    borast_uint64_t	quo;

    bit = _borast_uint32_to_uint64 (1);

    /* normalize to make den >= num, but not overflow */
    while (_borast_uint64_lt (den, num) && (den.hi & 0x80000000) == 0)
    {
	bit = _borast_uint64_lsl (bit, 1);
	den = _borast_uint64_lsl (den, 1);
    }
    quo = _borast_uint32_to_uint64 (0);

    /* generate quotient, one bit at a time */
    while (bit.hi | bit.lo)
    {
	if (_borast_uint64_le (den, num))
	{
	    num = _borast_uint64_sub (num, den);
	    quo = _borast_uint64_add (quo, bit);
	}
	bit = _borast_uint64_rsl (bit, 1);
	den = _borast_uint64_rsl (den, 1);
    }
    qr.quo = quo;
    qr.rem = num;
    return qr;
}

#endif /* !HAVE_UINT64_T */

#if HAVE_UINT128_T
borast_uquorem128_t
_borast_uint128_divrem (borast_uint128_t num, borast_uint128_t den)
{
    borast_uquorem128_t	qr;

    qr.quo = num / den;
    qr.rem = num % den;
    return qr;
}

#else

borast_uint128_t
_borast_uint32_to_uint128 (uint32_t i)
{
    borast_uint128_t	q;

    q.lo = _borast_uint32_to_uint64 (i);
    q.hi = _borast_uint32_to_uint64 (0);
    return q;
}

borast_int128_t
_borast_int32_to_int128 (int32_t i)
{
    borast_int128_t	q;

    q.lo = _borast_int32_to_int64 (i);
    q.hi = _borast_int32_to_int64 (i < 0 ? -1 : 0);
    return q;
}

borast_uint128_t
_borast_uint64_to_uint128 (borast_uint64_t i)
{
    borast_uint128_t	q;

    q.lo = i;
    q.hi = _borast_uint32_to_uint64 (0);
    return q;
}

borast_int128_t
_borast_int64_to_int128 (borast_int64_t i)
{
    borast_int128_t	q;

    q.lo = i;
    q.hi = _borast_int32_to_int64 (_borast_int64_negative(i) ? -1 : 0);
    return q;
}

borast_uint128_t
_borast_uint128_add (borast_uint128_t a, borast_uint128_t b)
{
    borast_uint128_t	s;

    s.hi = _borast_uint64_add (a.hi, b.hi);
    s.lo = _borast_uint64_add (a.lo, b.lo);
    if (_borast_uint64_lt (s.lo, a.lo))
	s.hi = _borast_uint64_add (s.hi, _borast_uint32_to_uint64 (1));
    return s;
}

borast_uint128_t
_borast_uint128_sub (borast_uint128_t a, borast_uint128_t b)
{
    borast_uint128_t	s;

    s.hi = _borast_uint64_sub (a.hi, b.hi);
    s.lo = _borast_uint64_sub (a.lo, b.lo);
    if (_borast_uint64_gt (s.lo, a.lo))
	s.hi = _borast_uint64_sub (s.hi, _borast_uint32_to_uint64(1));
    return s;
}

borast_uint128_t
_borast_uint64x64_128_mul (borast_uint64_t a, borast_uint64_t b)
{
    borast_uint128_t	s;
    uint32_t		ah, al, bh, bl;
    borast_uint64_t	r0, r1, r2, r3;

    al = uint64_lo32 (a);
    ah = uint64_hi32 (a);
    bl = uint64_lo32 (b);
    bh = uint64_hi32 (b);

    r0 = _borast_uint32x32_64_mul (al, bl);
    r1 = _borast_uint32x32_64_mul (al, bh);
    r2 = _borast_uint32x32_64_mul (ah, bl);
    r3 = _borast_uint32x32_64_mul (ah, bh);

    r1 = _borast_uint64_add (r1, uint64_hi (r0));    /* no carry possible */
    r1 = _borast_uint64_add (r1, r2);	    	    /* but this can carry */
    if (_borast_uint64_lt (r1, r2))		    /* check */
	r3 = _borast_uint64_add (r3, uint64_carry32);

    s.hi = _borast_uint64_add (r3, uint64_hi(r1));
    s.lo = _borast_uint64_add (uint64_shift32 (r1),
				uint64_lo (r0));
    return s;
}

borast_int128_t
_borast_int64x64_128_mul (borast_int64_t a, borast_int64_t b)
{
    borast_int128_t  s;
    s = _borast_uint64x64_128_mul (_borast_int64_to_uint64(a),
				  _borast_int64_to_uint64(b));
    if (_borast_int64_negative (a))
	s.hi = _borast_uint64_sub (s.hi,
				  _borast_int64_to_uint64 (b));
    if (_borast_int64_negative (b))
	s.hi = _borast_uint64_sub (s.hi,
				  _borast_int64_to_uint64 (a));
    return s;
}

borast_uint128_t
_borast_uint128_mul (borast_uint128_t a, borast_uint128_t b)
{
    borast_uint128_t	s;

    s = _borast_uint64x64_128_mul (a.lo, b.lo);
    s.hi = _borast_uint64_add (s.hi,
				_borast_uint64_mul (a.lo, b.hi));
    s.hi = _borast_uint64_add (s.hi,
				_borast_uint64_mul (a.hi, b.lo));
    return s;
}

borast_uint128_t
_borast_uint128_lsl (borast_uint128_t a, int shift)
{
    if (shift >= 64)
    {
	a.hi = a.lo;
	a.lo = _borast_uint32_to_uint64 (0);
	shift -= 64;
    }
    if (shift)
    {
	a.hi = _borast_uint64_add (_borast_uint64_lsl (a.hi, shift),
				    _borast_uint64_rsl (a.lo, (64 - shift)));
	a.lo = _borast_uint64_lsl (a.lo, shift);
    }
    return a;
}

borast_uint128_t
_borast_uint128_rsl (borast_uint128_t a, int shift)
{
    if (shift >= 64)
    {
	a.lo = a.hi;
	a.hi = _borast_uint32_to_uint64 (0);
	shift -= 64;
    }
    if (shift)
    {
	a.lo = _borast_uint64_add (_borast_uint64_rsl (a.lo, shift),
				    _borast_uint64_lsl (a.hi, (64 - shift)));
	a.hi = _borast_uint64_rsl (a.hi, shift);
    }
    return a;
}

borast_uint128_t
_borast_uint128_rsa (borast_int128_t a, int shift)
{
    if (shift >= 64)
    {
	a.lo = a.hi;
	a.hi = _borast_uint64_rsa (a.hi, 64-1);
	shift -= 64;
    }
    if (shift)
    {
	a.lo = _borast_uint64_add (_borast_uint64_rsl (a.lo, shift),
				    _borast_uint64_lsl (a.hi, (64 - shift)));
	a.hi = _borast_uint64_rsa (a.hi, shift);
    }
    return a;
}

int
_borast_uint128_lt (borast_uint128_t a, borast_uint128_t b)
{
    return (_borast_uint64_lt (a.hi, b.hi) ||
	    (_borast_uint64_eq (a.hi, b.hi) &&
	     _borast_uint64_lt (a.lo, b.lo)));
}

int
_borast_int128_lt (borast_int128_t a, borast_int128_t b)
{
    if (_borast_int128_negative (a) && !_borast_int128_negative (b))
	return 1;
    if (!_borast_int128_negative (a) && _borast_int128_negative (b))
	return 0;
    return _borast_uint128_lt (a, b);
}

int
_borast_uint128_cmp (borast_uint128_t a, borast_uint128_t b)
{
    int cmp;

    cmp = _borast_uint64_cmp (a.hi, b.hi);
    if (cmp)
	return cmp;
    return _borast_uint64_cmp (a.lo, b.lo);
}

int
_borast_int128_cmp (borast_int128_t a, borast_int128_t b)
{
    if (_borast_int128_negative (a) && !_borast_int128_negative (b))
	return -1;
    if (!_borast_int128_negative (a) && _borast_int128_negative (b))
	return 1;

    return _borast_uint128_cmp (a, b);
}

int
_borast_uint128_eq (borast_uint128_t a, borast_uint128_t b)
{
    return (_borast_uint64_eq (a.hi, b.hi) &&
	    _borast_uint64_eq (a.lo, b.lo));
}

#if HAVE_UINT64_T
#define _borast_msbset64(q)  (q & ((uint64_t) 1 << 63))
#else
#define _borast_msbset64(q)  (q.hi & ((uint32_t) 1 << 31))
#endif

borast_uquorem128_t
_borast_uint128_divrem (borast_uint128_t num, borast_uint128_t den)
{
    borast_uquorem128_t	qr;
    borast_uint128_t	bit;
    borast_uint128_t	quo;

    bit = _borast_uint32_to_uint128 (1);

    /* normalize to make den >= num, but not overflow */
    while (_borast_uint128_lt (den, num) && !_borast_msbset64(den.hi))
    {
	bit = _borast_uint128_lsl (bit, 1);
	den = _borast_uint128_lsl (den, 1);
    }
    quo = _borast_uint32_to_uint128 (0);

    /* generate quotient, one bit at a time */
    while (_borast_uint128_ne (bit, _borast_uint32_to_uint128(0)))
    {
	if (_borast_uint128_le (den, num))
	{
	    num = _borast_uint128_sub (num, den);
	    quo = _borast_uint128_add (quo, bit);
	}
	bit = _borast_uint128_rsl (bit, 1);
	den = _borast_uint128_rsl (den, 1);
    }
    qr.quo = quo;
    qr.rem = num;
    return qr;
}

borast_int128_t
_borast_int128_negate (borast_int128_t a)
{
    a.lo = _borast_uint64_not (a.lo);
    a.hi = _borast_uint64_not (a.hi);
    return _borast_uint128_add (a, _borast_uint32_to_uint128 (1));
}

borast_int128_t
_borast_int128_not (borast_int128_t a)
{
    a.lo = _borast_uint64_not (a.lo);
    a.hi = _borast_uint64_not (a.hi);
    return a;
}

#endif /* !HAVE_UINT128_T */

borast_quorem128_t
_borast_int128_divrem (borast_int128_t num, borast_int128_t den)
{
    int			num_neg = _borast_int128_negative (num);
    int			den_neg = _borast_int128_negative (den);
    borast_uquorem128_t	uqr;
    borast_quorem128_t	qr;

    if (num_neg)
	num = _borast_int128_negate (num);
    if (den_neg)
	den = _borast_int128_negate (den);
    uqr = _borast_uint128_divrem (num, den);
    if (num_neg)
	qr.rem = _borast_int128_negate (uqr.rem);
    else
	qr.rem = uqr.rem;
    if (num_neg != den_neg)
	qr.quo = _borast_int128_negate (uqr.quo);
    else
	qr.quo = uqr.quo;
    return qr;
}

/**
 * _borast_uint_96by64_32x64_divrem:
 *
 * Compute a 32 bit quotient and 64 bit remainder of a 96 bit unsigned
 * dividend and 64 bit divisor.  If the quotient doesn't fit into 32
 * bits then the returned remainder is equal to the divisor, and the
 * quotient is the largest representable 64 bit integer.  It is an
 * error to call this function with the high 32 bits of @num being
 * non-zero. */
borast_uquorem64_t
_borast_uint_96by64_32x64_divrem (borast_uint128_t num,
				 borast_uint64_t den)
{
    borast_uquorem64_t result;
    borast_uint64_t B = _borast_uint32s_to_uint64 (1, 0);

    /* These are the high 64 bits of the *96* bit numerator.  We're
     * going to represent the numerator as xB + y, where x is a 64,
     * and y is a 32 bit number. */
    borast_uint64_t x = _borast_uint128_to_uint64 (_borast_uint128_rsl(num, 32));

    /* Initialise the result to indicate overflow. */
    result.quo = _borast_uint32s_to_uint64 (-1U, -1U);
    result.rem = den;

    /* Don't bother if the quotient is going to overflow. */
    if (_borast_uint64_ge (x, den)) {
	return /* overflow */ result;
    }

    if (_borast_uint64_lt (x, B)) {
	/* When the final quotient is known to fit in 32 bits, then
	 * num < 2^64 if and only if den < 2^32. */
	return _borast_uint64_divrem (_borast_uint128_to_uint64 (num), den);
    }
    else {
	/* Denominator is >= 2^32. the numerator is >= 2^64, and the
	 * division won't overflow: need two divrems.  Write the
	 * numerator and denominator as
	 *
	 *	num = xB + y		x : 64 bits, y : 32 bits
	 *	den = uB + v		u, v : 32 bits
	 */
	uint32_t y = _borast_uint128_to_uint32 (num);
	uint32_t u = uint64_hi32 (den);
	uint32_t v = _borast_uint64_to_uint32 (den);

	/* Compute a lower bound approximate quotient of num/den
	 * from x/(u+1).  Then we have
	 *
	 * x	= q(u+1) + r	; q : 32 bits, r <= u : 32 bits.
	 *
	 * xB + y	= q(u+1)B	+ (rB+y)
	 *		= q(uB + B + v - v) + (rB+y)
	 *		= q(uB + v)	+ qB - qv + (rB+y)
	 *		= q(uB + v)	+ q(B-v) + (rB+y)
	 *
	 * The true quotient of num/den then is q plus the
	 * contribution of q(B-v) + (rB+y).  The main contribution
	 * comes from the term q(B-v), with the term (rB+y) only
	 * contributing at most one part.
	 *
	 * The term q(B-v) must fit into 64 bits, since q fits into 32
	 * bits on account of being a lower bound to the true
	 * quotient, and as B-v <= 2^32, we may safely use a single
	 * 64/64 bit division to find its contribution. */

	borast_uquorem64_t quorem;
	borast_uint64_t remainder; /* will contain final remainder */
	uint32_t quotient;	/* will contain final quotient. */
	uint32_t q;
	uint32_t r;

	/* Approximate quotient by dividing the high 64 bits of num by
	 * u+1. Watch out for overflow of u+1. */
	if (u+1) {
	    quorem = _borast_uint64_divrem (x, _borast_uint32_to_uint64 (u+1));
	    q = _borast_uint64_to_uint32 (quorem.quo);
	    r = _borast_uint64_to_uint32 (quorem.rem);
	}
	else {
	    q = uint64_hi32 (x);
	    r = _borast_uint64_to_uint32 (x);
	}
	quotient = q;

	/* Add the main term's contribution to quotient.  Note B-v =
	 * -v as an uint32 (unless v = 0) */
	if (v)
	    quorem = _borast_uint64_divrem (_borast_uint32x32_64_mul (q, -v), den);
	else
	    quorem = _borast_uint64_divrem (_borast_uint32s_to_uint64 (q, 0), den);
	quotient += _borast_uint64_to_uint32 (quorem.quo);

	/* Add the contribution of the subterm and start computing the
	 * true remainder. */
	remainder = _borast_uint32s_to_uint64 (r, y);
	if (_borast_uint64_ge (remainder, den)) {
	    remainder = _borast_uint64_sub (remainder, den);
	    quotient++;
	}

	/* Add the contribution of the main term's remainder. The
	 * funky test here checks that remainder + main_rem >= den,
	 * taking into account overflow of the addition. */
	remainder = _borast_uint64_add (remainder, quorem.rem);
	if (_borast_uint64_ge (remainder, den) ||
	    _borast_uint64_lt (remainder, quorem.rem))
	{
	    remainder = _borast_uint64_sub (remainder, den);
	    quotient++;
	}

	result.quo = _borast_uint32_to_uint64 (quotient);
	result.rem = remainder;
    }
    return result;
}

borast_quorem64_t
_borast_int_96by64_32x64_divrem (borast_int128_t num, borast_int64_t den)
{
    int			num_neg = _borast_int128_negative (num);
    int			den_neg = _borast_int64_negative (den);
    borast_uint64_t	nonneg_den;
    borast_uquorem64_t	uqr;
    borast_quorem64_t	qr;

    if (num_neg)
	num = _borast_int128_negate (num);
    if (den_neg)
	nonneg_den = _borast_int64_negate (den);
    else
	nonneg_den = den;

    uqr = _borast_uint_96by64_32x64_divrem (num, nonneg_den);
    if (_borast_uint64_eq (uqr.rem, nonneg_den)) {
	/* bail on overflow. */
	qr.quo = _borast_uint32s_to_uint64 (0x7FFFFFFF, -1U);;
	qr.rem = den;
	return qr;
    }

    if (num_neg)
	qr.rem = _borast_int64_negate (uqr.rem);
    else
	qr.rem = uqr.rem;
    if (num_neg != den_neg)
	qr.quo = _borast_int64_negate (uqr.quo);
    else
	qr.quo = uqr.quo;
    return qr;
}
