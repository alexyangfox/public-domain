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

int  mpf_const_ln_d(mp_float *a, long b)
{
   int err;

   /* test input */
   if (b < 0) {
      return MP_VAL;
   }

   if (b == 0) {
      return mpf_const_d(a, 1);
   }

   if (b == 1) {
      return mpf_const_d(a, 0);
   }

   if ((err = mpf_const_d(a, b)) != MP_OKAY) {
      return err;
   }
   return mpf_ln(a, a);
}
                /* a = ln b    */

