/*****************************************************************************
for MinGW32

OK, I got rid of _alloca() by compiling with gcc -mno-stack-arg-probe ...
Now how do I get rid of __main()?
*****************************************************************************/
#ifdef __WIN32__
int __main(void) { return 0; }
#endif
