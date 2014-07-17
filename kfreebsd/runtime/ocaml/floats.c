/***********************************************************************/
/*                                                                     */
/*                                OCaml                                */
/*                                                                     */
/*            Xavier Leroy, projet Cristal, INRIA Rocquencourt         */
/*                                                                     */
/*  Copyright 1996 Institut National de Recherche en Informatique et   */
/*  en Automatique.  All rights reserved.  This file is distributed    */
/*  under the terms of the GNU Library General Public License, with    */
/*  the special exception on linking described in file ../LICENSE.     */
/*                                                                     */
/***********************************************************************/

/* The interface of this file is in "mlvalues.h" and "alloc.h" */

#if defined(__FreeBSD__) && defined(_KERNEL)
#include <fixmath.h>
#else
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "alloc.h"
#include "fail.h"
#include "memory.h"
#include "mlvalues.h"
#include "misc.h"
#include "reverse.h"
#include "stacks.h"

#ifdef _MSC_VER
#include <float.h>
#define isnan _isnan
#define isfinite _finite
#endif

CAMLprim value caml_format_float(value fmt, value arg);
/*CAMLprim*/ value caml_float_of_substring(value vs, value idx, value l);
CAMLprim value caml_float_of_string(value vs);
CAMLprim value caml_int_of_float(value f);
CAMLprim value caml_float_of_int(value n);
CAMLprim value caml_neg_float(value f);
CAMLprim value caml_abs_float(value f);
CAMLprim value caml_add_float(value f, value g);
CAMLprim value caml_sub_float(value f, value g);
CAMLprim value caml_mul_float(value f, value g);
CAMLprim value caml_div_float(value f, value g);
CAMLprim value caml_exp_float(value f);
CAMLprim value caml_floor_float(value f);
CAMLprim value caml_fmod_float(value f1, value f2);
CAMLprim value caml_frexp_float(value f);
CAMLprim value caml_ldexp_float(value f, value i);
CAMLprim value caml_log_float(value f);
CAMLprim value caml_log10_float(value f);
CAMLprim value caml_modf_float(value f);
CAMLprim value caml_sqrt_float(value f);
CAMLprim value caml_power_float(value f, value g);
CAMLprim value caml_sin_float(value f);
CAMLprim value caml_sinh_float(value f);
CAMLprim value caml_cos_float(value f);
CAMLprim value caml_cosh_float(value f);
CAMLprim value caml_tan_float(value f);
CAMLprim value caml_tanh_float(value f);
CAMLprim value caml_asin_float(value f);
CAMLprim value caml_acos_float(value f);
CAMLprim value caml_atan_float(value f);
CAMLprim value caml_atan2_float(value f, value g);
CAMLprim value caml_ceil_float(value f);
CAMLprim value caml_hypot_float(value f, value g);
CAMLprim value caml_expm1_float(value f);
CAMLprim value caml_log1p_float(value f);
CAMLprim value caml_copysign_float(value f, value g);
CAMLprim value caml_eq_float(value f, value g);
CAMLprim value caml_neq_float(value f, value g);
CAMLprim value caml_le_float(value f, value g);
CAMLprim value caml_lt_float(value f, value g);
CAMLprim value caml_ge_float(value f, value g);
CAMLprim value caml_gt_float(value f, value g);
CAMLprim value caml_float_compare(value vf, value vg);
CAMLprim value caml_classify_float(value vd);
void caml_init_ieee_floats(void);

#ifdef ARCH_ALIGN_DOUBLE

CAMLexport __double caml_Double_val(value val)
{
  union { value v[2]; __double d; } buffer;

  Assert(sizeof(__double) == 2 * sizeof(value));
  buffer.v[0] = Field(val, 0);
  buffer.v[1] = Field(val, 1);
  return buffer.d;
}

CAMLexport void caml_Store_double_val(value val, __double dbl)
{
  union { value v[2]; __double d; } buffer;

  Assert(sizeof(__double) == 2 * sizeof(value));
  buffer.d = dbl;
  Field(val, 0) = buffer.v[0];
  Field(val, 1) = buffer.v[1];
}

#endif

CAMLexport value caml_copy_double(__double d)
{
  value res;

#define Setup_for_gc
#define Restore_after_gc
  Alloc_small(res, Double_wosize, Double_tag);
#undef Setup_for_gc
#undef Restore_after_gc
  Store_double_val(res, d);
  return res;
}

CAMLprim value caml_format_float(value fmt, value arg)
{
#define MAX_DIGITS 350
/* Max number of decimal digits in a "natural" (not artificially padded)
   representation of a float. Can be quite big for %f format.
   Max exponent for IEEE format is 308 decimal digits.
   Rounded up for good measure. */
  char format_buffer[MAX_DIGITS + 20];
  int prec, i;
  char * p;
  char * dest;
  value res;
  __double d = Double_val(arg);

#ifdef HAS_BROKEN_PRINTF
  if (isfinite(d)) {
#endif
  prec = MAX_DIGITS;
  for (p = String_val(fmt); *p != 0; p++) {
    if (*p >= '0' && *p <= '9') {
      i = atoi(p) + MAX_DIGITS;
      if (i > prec) prec = i;
      break;
    }
  }
  for( ; *p != 0; p++) {
    if (*p == '.') {
      i = atoi(p+1) + MAX_DIGITS;
      if (i > prec) prec = i;
      break;
    }
  }
  if (prec < sizeof(format_buffer)) {
    dest = format_buffer;
  } else {
    dest = caml_stat_alloc(prec);
  }
#if defined(__FreeBSD__) && defined(_KERNEL)
  fixpt_to_str(d, dest, (prec > MAX_DIGITS) ? prec - MAX_DIGITS : MAX_DIGITS);
#else
  sprintf(dest, String_val(fmt), d);
#endif
  res = caml_copy_string(dest);
  if (dest != format_buffer) {
    caml_stat_free(dest);
  }
#ifdef HAS_BROKEN_PRINTF
  } else {
    if (isnan(d))
    {
      res = caml_copy_string("nan");
    }
    else
    {
      if (d > 0)
      {
        res = caml_copy_string("inf");
      }
      else
      {
        res = caml_copy_string("-inf");
      }
    }
  }
#endif
  return res;
}

/*CAMLprim*/ value caml_float_of_substring(value vs, value idx, value l)
{
  char parse_buffer[64];
  char * buf, * src, * dst, * end;
  mlsize_t len, lenvs;
  __double d;
  intnat flen = Long_val(l);
  intnat fidx = Long_val(idx);

  lenvs = caml_string_length(vs);
  len =
    fidx >= 0 && fidx < lenvs && flen > 0 && flen <= lenvs - fidx
    ? flen : 0;
  buf = len < sizeof(parse_buffer) ? parse_buffer : caml_stat_alloc(len + 1);
  src = String_val(vs) + fidx;
  dst = buf;
  while (len--) {
    char c = *src++;
    if (c != '_') *dst++ = c;
  }
  *dst = 0;
  if (dst == buf) goto error;
#if defined(__FreeBSD__) && defined(_KERNEL)
  d = fixpt_strtod((const char *) buf, (const char **) &end);
#else
  d = strtod((const char *) buf, &end);
#endif
  if (end != dst) goto error;
  if (buf != parse_buffer) caml_stat_free(buf);
  return caml_copy_double(d);
 error:
  if (buf != parse_buffer) caml_stat_free(buf);
  caml_failwith("float_of_string");
#if defined(__FreeBSD__) && defined(_KERNEL)
  return 0;
#endif
}

CAMLprim value caml_float_of_string(value vs)
{
  char parse_buffer[64];
  char * buf, * src, * dst, * end;
  mlsize_t len;
  __double d;

  len = caml_string_length(vs);
  buf = len < sizeof(parse_buffer) ? parse_buffer : caml_stat_alloc(len + 1);
  src = String_val(vs);
  dst = buf;
  while (len--) {
    char c = *src++;
    if (c != '_') *dst++ = c;
  }
  *dst = 0;
  if (dst == buf) goto error;
#if defined(__FreeBSD__) && defined(_KERNEL)
  d = fixpt_strtod((const char *) buf, (const char **) &end);
#else
  d = strtod((const char *) buf, &end);
#endif
  if (end != dst) goto error;
  if (buf != parse_buffer) caml_stat_free(buf);
  return caml_copy_double(d);
 error:
  if (buf != parse_buffer) caml_stat_free(buf);
  caml_failwith("float_of_string");
#if defined(__FreeBSD__) && defined(_KERNEL)
  return 0;
#endif
}

CAMLprim value caml_int_of_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return Val_long((intnat) fixpt_to_int(Double_val(f)));
#else
  return Val_long((intnat) Double_val(f));
#endif
}

CAMLprim value caml_float_of_int(value n)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double((fixpt_from_int(Long_val(n))));
#else
  return caml_copy_double((double) Long_val(n));
#endif
}

CAMLprim value caml_neg_float(value f)
{
  return caml_copy_double(- Double_val(f));
}

CAMLprim value caml_abs_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_abs(Double_val(f)));
#else
  return caml_copy_double(fabs(Double_val(f)));
#endif
}

CAMLprim value caml_add_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_add(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(Double_val(f) + Double_val(g));
#endif
}

CAMLprim value caml_sub_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_sub(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(Double_val(f) - Double_val(g));
#endif
}

CAMLprim value caml_mul_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_mul(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(Double_val(f) * Double_val(g));
#endif
}

CAMLprim value caml_div_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_div(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(Double_val(f) / Double_val(g));
#endif
}

CAMLprim value caml_exp_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_exp(Double_val(f)));
#else
  return caml_copy_double(exp(Double_val(f)));
#endif
}

CAMLprim value caml_floor_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_floor(Double_val(f)));
#else
  return caml_copy_double(floor(Double_val(f)));
#endif
}

CAMLprim value caml_fmod_float(value f1, value f2)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_mod(Double_val(f1), Double_val(f2)));
#else
  return caml_copy_double(fmod(Double_val(f1), Double_val(f2)));
#endif
}

CAMLprim value caml_frexp_float(value f)
{
  CAMLparam1 (f);
  CAMLlocal2 (res, mantissa);
  int exponent;

#if defined(__FreeBSD__) && defined(_KERNEL)
  mantissa = caml_copy_double(fixpt_frexp(Double_val(f), &exponent));
#else
  mantissa = caml_copy_double(frexp (Double_val(f), &exponent));
#endif
  res = caml_alloc_tuple(2);
  Field(res, 0) = mantissa;
  Field(res, 1) = Val_int(exponent);
  CAMLreturn (res);
}

CAMLprim value caml_ldexp_float(value f, value i)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_ldexp(Double_val(f), Int_val(i)));
#else
  return caml_copy_double(ldexp(Double_val(f), Int_val(i)));
#endif
}

CAMLprim value caml_log_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_log(Double_val(f)));
#else
  return caml_copy_double(log(Double_val(f)));
#endif
}

CAMLprim value caml_log10_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_log10(Double_val(f)));
#else
  return caml_copy_double(log10(Double_val(f)));
#endif
}

CAMLprim value caml_modf_float(value f)
{
  __double frem;

  CAMLparam1 (f);
  CAMLlocal3 (res, quo, rem);

#if defined(__FreeBSD__) && defined(_KERNEL)
  quo = caml_copy_double(fixpt_modf(Double_val(f), &frem));
  rem = caml_copy_double(frem);
#else
  quo = caml_copy_double(modf (Double_val(f), &frem));
  rem = caml_copy_double(frem);
#endif
  res = caml_alloc_tuple(2);
  Field(res, 0) = quo;
  Field(res, 1) = rem;
  CAMLreturn (res);
}

CAMLprim value caml_sqrt_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_sqrt(Double_val(f)));
#else
  return caml_copy_double(sqrt(Double_val(f)));
#endif
}

CAMLprim value caml_power_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_pow(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(pow(Double_val(f), Double_val(g)));
#endif
}

CAMLprim value caml_sin_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_sin(Double_val(f)));
#else
  return caml_copy_double(sin(Double_val(f)));
#endif
}

CAMLprim value caml_sinh_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_sinh(Double_val(f)));
#else
  return caml_copy_double(sinh(Double_val(f)));
#endif
}

CAMLprim value caml_cos_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_cos(Double_val(f)));
#else
  return caml_copy_double(cos(Double_val(f)));
#endif
}

CAMLprim value caml_cosh_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_cosh(Double_val(f)));
#else
  return caml_copy_double(cosh(Double_val(f)));
#endif
}

CAMLprim value caml_tan_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_tan(Double_val(f)));
#else
  return caml_copy_double(tan(Double_val(f)));
#endif
}

CAMLprim value caml_tanh_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_tanh(Double_val(f)));
#else
  return caml_copy_double(tanh(Double_val(f)));
#endif
}

CAMLprim value caml_asin_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_asin(Double_val(f)));
#else
  return caml_copy_double(asin(Double_val(f)));
#endif
}

CAMLprim value caml_acos_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_acos(Double_val(f)));
#else
  return caml_copy_double(acos(Double_val(f)));
#endif
}

CAMLprim value caml_atan_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_atan(Double_val(f)));
#else
  return caml_copy_double(atan(Double_val(f)));
#endif
}

CAMLprim value caml_atan2_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_atan2(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(atan2(Double_val(f), Double_val(g)));
#endif
}

CAMLprim value caml_ceil_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_ceil(Double_val(f)));
#else
  return caml_copy_double(ceil(Double_val(f)));
#endif
}

#if !defined(__FreeBSD__) && !defined(_KERNEL)
CAMLexport double caml_hypot(double x, double y)
{
#ifdef HAS_C99_FLOAT_OPS
  return hypot(x, y);
#else
  double tmp, ratio;
  if (x != x) return x;  /* NaN */
  if (y != y) return y;  /* NaN */
  x = fabs(x); y = fabs(y);
  if (x < y) { tmp = x; x = y; y = tmp; }
  if (x == 0.0) return 0.0;
  ratio = y / x;
  return x * sqrt(1.0 + ratio * ratio);
#endif
}
#endif /* __FreeBSD__ && _KERNEL */

CAMLprim value caml_hypot_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_hypot(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(caml_hypot(Double_val(f), Double_val(g)));
#endif
}

#if !defined(__FreeBSD__) && !defined(_KERNEL)
/* These emulations of expm1() and log1p() are due to William Kahan.
   See http://www.plunk.org/~hatch/rightway.php */
CAMLexport double caml_expm1(double x)
{
#ifdef HAS_C99_FLOAT_OPS
  return expm1(x);
#else
  double u = exp(x);
  if (u == 1.)
    return x;
  if (u - 1. == -1.)
    return -1.;
  return (u - 1.) * x / log(u);
#endif
}

CAMLexport double caml_log1p(double x)
{
#ifdef HAS_C99_FLOAT_OPS
  return log1p(x);
#else
  double u = 1. + x;
  if (u == 1.)
    return x;
  else
    return log(u) * x / (u - 1.);
#endif
}
#endif /* __FreeBSD__ && _KERNEL */

CAMLprim value caml_expm1_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_expm1(Double_val(f)));
#else
  return caml_copy_double(caml_expm1(Double_val(f)));
#endif
}

CAMLprim value caml_log1p_float(value f)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_log1p(Double_val(f)));
#else
  return caml_copy_double(caml_log1p(Double_val(f)));
#endif
}

union double_as_two_int32 {
    double d;
#if defined(ARCH_BIG_ENDIAN) || (defined(__arm__) && !defined(__ARM_EABI__))
    struct { uint32 h; uint32 l; } i;
#else
    struct { uint32 l; uint32 h; } i;
#endif
};

#if !defined(__FreeBSD__) && !defined(_KERNEL)
CAMLexport double caml_copysign(double x, double y)
{
#ifdef HAS_C99_FLOAT_OPS
  return copysign(x, y);
#else
  union double_as_two_int32 ux, uy;
  ux.d = x;
  uy.d = y;
  ux.i.h &= 0x7FFFFFFFU;
  ux.i.h |= (uy.i.h & 0x80000000U);
  return ux.d;
#endif
}
#endif /* __FreeBSD__ && _KERNEL */

CAMLprim value caml_copysign_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return caml_copy_double(fixpt_copysign(Double_val(f), Double_val(g)));
#else
  return caml_copy_double(caml_copysign(Double_val(f), Double_val(g)));
#endif
}

CAMLprim value caml_eq_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return Val_bool(Double_val(f) == Double_val(g));
#else
  return Val_bool(Double_val(f) == Double_val(g));
#endif
}

CAMLprim value caml_neq_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return Val_bool(Double_val(f) != Double_val(g));
#else
  return Val_bool(Double_val(f) != Double_val(g));
#endif
}

CAMLprim value caml_le_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return Val_bool(Double_val(f) <= Double_val(g));
#else
  return Val_bool(Double_val(f) <= Double_val(g));
#endif
}

CAMLprim value caml_lt_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return Val_bool(Double_val(f) <= Double_val(g));
#else
  return Val_bool(Double_val(f) < Double_val(g));
#endif
}

CAMLprim value caml_ge_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return Val_bool(Double_val(f) >= Double_val(g));
#else
  return Val_bool(Double_val(f) >= Double_val(g));
#endif
}

CAMLprim value caml_gt_float(value f, value g)
{
#if defined(__FreeBSD__) && defined(_KERNEL)
  return Val_bool(Double_val(f) > Double_val(g));
#else
  return Val_bool(Double_val(f) > Double_val(g));
#endif
}

CAMLprim value caml_float_compare(value vf, value vg)
{
  __double f = Double_val(vf);
  __double g = Double_val(vg);
  if (f == g) return Val_int(0);
  if (f < g) return Val_int(-1);
  if (f > g) return Val_int(1);
  /* One or both of f and g is NaN.  Order according to the
     convention NaN = NaN and NaN < x for all other floats x. */
#if 0
  if (f == f) return Val_int(1);  /* f is not NaN, g is NaN */
  if (g == g) return Val_int(-1); /* g is not NaN, f is NaN */
#endif
  return Val_int(0);              /* both f and g are NaN */
}

enum { FP_normal, FP_subnormal, FP_zero, FP_infinite, FP_nan };

CAMLprim value caml_classify_float(value vd)
{
  /* Cygwin 1.3 has problems with fpclassify (PR#1293), so don't use it */
#if defined(fpclassify) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
  switch (fpclassify(Double_val(vd))) {
  case FP_NAN:
    return Val_int(FP_nan);
  case FP_INFINITE:
    return Val_int(FP_infinite);
  case FP_ZERO:
    return Val_int(FP_zero);
  case FP_SUBNORMAL:
    return Val_int(FP_subnormal);
  default: /* case FP_NORMAL */
    return Val_int(FP_normal);
  }
#elif defined(__FreeBSD__) && defined(_KERNEL)
  __double d;

  d = Double_val(vd);
  if (d == 0)
    return Val_int(FP_zero);
  return Val_int(FP_normal);
#else
  union double_as_two_int32 u;
  uint32 h, l;

  u.d = Double_val(vd);
  h = u.i.h;  l = u.i.l;
  l = l | (h & 0xFFFFF);
  h = h & 0x7FF00000;
  if ((h | l) == 0)
    return Val_int(FP_zero);
  if (h == 0)
    return Val_int(FP_subnormal);
  if (h == 0x7FF00000) {
    if (l == 0)
      return Val_int(FP_infinite);
    else
      return Val_int(FP_nan);
  }
  return Val_int(FP_normal);
#endif
}

/* The [caml_init_ieee_float] function should initialize floating-point hardware
   so that it behaves as much as possible like the IEEE standard.
   In particular, return special numbers like Infinity and NaN instead
   of signalling exceptions.  Currently, everyone is in IEEE mode
   at program startup, except FreeBSD prior to 4.0R. */

#ifdef __FreeBSD__
#include <sys/param.h>
#if (__FreeBSD_version < 400017)
#include <floatingpoint.h>
#endif
#endif

void caml_init_ieee_floats(void)
{
#if defined(__FreeBSD__) && (__FreeBSD_version < 400017) && !defined(_KERNEL)
  fpsetmask(0);
#endif
}
