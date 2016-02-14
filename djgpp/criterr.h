/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             April 1, 1992
 * Modified:         July 21, 1997 Jason Hood
 *
 * This program is released into the public domain, Frank Davis.
 *   You may distribute it freely.
 */


static const char * const criterr_screen[] = {
"�����������������������������������������������������������������ͻ",
"�                        TDE Signal Handler                       �",
"�����������������������������������������������������������������Ķ",
"�                                                                 �",
"�  Signal:                                                        �",
"�   info1:                                                        �",
"�   info2:                                                        �",
"�                                                                 �",
"�����������������������������������������������������������������Ķ",
"�       Please enter action:  (C)ontinue, (E)xit or (A)bort       �",
"�����������������������������������������������������������������ͼ"
};


static const char * const sigabrt[] = {
   "SIGABRT",
   "an assertion has failed",
   "error in TDE - call Jason Hood <jadoxa@yahoo.com.au>"
};


static const char * const sigfpe[] = {
   "SIGFPE",
   "arithmetic exception, divide by 0",
   "No floats are used in TDE.  Error in djgpp??"
};


static const char * const sigill[] = {
   "SIGILL",
   "unknown or invalid exception",
   "possible error in TDE"
};


static const char * const sigsegv[] = {
   "SIGSEGV",
   "segment violation:  invalid memory reference",
   "error in TDE - call Jason Hood <jadoxa@yahoo.com.au>"
};
