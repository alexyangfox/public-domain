/* This software is in the public domain and has no warrantee */
#include <stdio.h>
#include <string.h>
#include "libbitap.h"

int main (void)
{
  char *tests[] = {
    "misdo", "micfo", "micro", "midro",
    "abc", "xxx", "bc", "c",
    "abc|def", "bcd", "abc|def", "abc",
    "abc|def", "cde", "abc|def", "def",
    "a+b", "bx", "a+",      "aaxaa",
    "a*b", "xx", "a*b",      "bx",
    "aa(bb)+", "xabbb", "aa(bb)+", "xabbbb",
    "ba{3}", "xaaaa", "ba{3}", "xaaa",
    "ba{2,}", "xa", "ba{2,}", "xaaaaaa",
    "ba{3,4}", "xaaaaa", "ba{3,4}", "xaaa",
    "b[xyz]", "x1", "b[xyz]", "xy",
    "b[a-z]", "x1", "b[a-z]", "xy",
    "b\\.", "xy", "\\.b", ".x",
    "ba?", "xbaa", "bca?", "xbc",
    "(01){17}a", "010101010101010101010101010101010xx", /* Test multiple */
    "(01){17}a", "0101010101010101010101010101010101x", /*   words */
    ".*bab.*",  "caac",
    ".*nstein.*", "Levenshtein",
    NULL,
  };
  int i, e = 1, cnt;
  bitapType b;
  const char *begin, *end;
  
  printf ("Matching, allowing for %d error%s\n", e, e == 1 ? "" : "s");
  for (i = 0; tests[i] != NULL; i += 2) {
    NewBitap (&b, tests[i]);
    printf ("%s\t%s\t", tests[i], tests[i+1]);
    if ((NULL != (end = FindWithBitap (&b, tests[i + 1],
      strlen (tests[i + 1]), e,	&cnt, &begin))) == (i % 4 == 0)) {
      printf ("\tFailure!\n");
      return 1;
    }
    if (end) printf ("%d '%.*s'", cnt, end - begin, begin);
    printf ("\tTest passed\n");
    DeleteBitap (&b);
  }
  return 0;
}
