/* LibTomFloat, multiple-precision floating-point library
 *
 * LibTomFloat is a library that provides multiple-precision
 * floating-point artihmetic as well as trigonometric functionality.
 *
 * This library requires the public domain LibTomMath to be installed.
 * 
 * This library is free for all purposes without any express
 * gurantee it works
 *
 * Tom St Denis, tomstdenis@iahu.ca, http://float.libtomcrypt.org
 */
#include <tomfloat.h>

int  mpf_copy(mp_float *src, mp_float *dest)
{
   if (src == dest) {
      return MP_OKAY;
   }
   dest->radix = src->radix;
   dest->exp   = src->exp;
   return mp_copy(&(src->mantissa), &(dest->mantissa));
}
