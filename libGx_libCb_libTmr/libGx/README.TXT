The configure script is not GNU autoconf related.  This code is
designed to be integrated into your project (the benefit of public
domain code), so configure can/should be rewritten to do exactly what
you the user wants

LibGx has dependencies on libCb and libTmr.  Current versions of all
these libraries may be downloaded from www.johnculp.net.

To compile libGx, its tests and demos, you will have to either:

#1 extract libCb and libTmr directly in the libGx directory (simpler)
#2 create a symbolic link in the libGx directory to the libCb and
   libTmr directories (better)

In either case, you will need to run ./configure and make for both
libCb and libTmr.


LibGx is licensed public-domain and has been designed to be directly
integrated into applications. The simplest way to get started with
libGx is to download and extract libGx, libCb and libTmr into the top
level folder of your application file tree, then have your makefile
traverse into the library folder with a $(MAKE) -C in the process of
compiling or cleaning your project.

For additional information and documentation, please see
www.johnculp.net.

