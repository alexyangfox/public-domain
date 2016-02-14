# Don't edit Makefile! Use conf-* for configuration.

SHELL=/bin/sh

default: it

4c0.c: \
idea/c0.c pentium/c0.c ppro/c0.c sparc/c0.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/c0.c > 4c0.c

4c0.o: \
compile 4c0.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4c0.c

4c1.c: \
idea/c1.c pentium/c1.c ppro/c1.c sparc/c1.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/c1.c > 4c1.c

4c1.o: \
compile 4c1.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4c1.c

4c2.c: \
idea/c2.c pentium/c2.c ppro/c2.c sparc/c2.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/c2.c > 4c2.c

4c2.o: \
compile 4c2.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4c2.c

4c3.c: \
idea/c3.c pentium/c3.c ppro/c3.c sparc/c3.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/c3.c > 4c3.c

4c3.o: \
compile 4c3.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4c3.c

4c4.c: \
idea/c4.c pentium/c4.c ppro/c4.c sparc/c4.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/c4.c > 4c4.c

4c4.o: \
compile 4c4.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4c4.c

4c5.c: \
idea/c5.c pentium/c5.c ppro/c5.c sparc/c5.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/c5.c > 4c5.c

4c5.o: \
compile 4c5.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4c5.c

4d0.c: \
idea/d0.c pentium/d0.c ppro/d0.c sparc/d0.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/d0.c > 4d0.c

4d0.o: \
compile 4d0.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c \
roots/16.c roots/32.c roots/64.c roots/128.c roots/256.c
	./compile 4d0.c

4d1.c: \
idea/d1.c pentium/d1.c ppro/d1.c sparc/d1.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/d1.c > 4d1.c

4d1.o: \
compile 4d1.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c \
roots/512.c
	./compile 4d1.c

4d2.c: \
idea/d2.c pentium/d2.c ppro/d2.c sparc/d2.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/d2.c > 4d2.c

4d2.o: \
compile 4d2.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c \
roots/h1024.c
	./compile 4d2.c

4d3.c: \
idea/d3.c pentium/d3.c ppro/d3.c sparc/d3.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/d3.c > 4d3.c

4d3.o: \
compile 4d3.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c \
roots/h2048.c
	./compile 4d3.c

4d4.c: \
idea/d4.c pentium/d4.c ppro/d4.c sparc/d4.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/d4.c > 4d4.c

4d4.o: \
compile 4d4.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c \
roots/h4096.c
	./compile 4d4.c

4d5.c: \
idea/d5.c pentium/d5.c ppro/d5.c sparc/d5.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/d5.c > 4d5.c

4d5.o: \
compile 4d5.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c \
roots/h8192.c
	./compile 4d5.c

4i.c: \
idea/i.c pentium/i.c ppro/i.c sparc/i.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/i.c > 4i.c

4mc.c: \
idea/mc.c pentium/mc.c ppro/mc.c sparc/mc.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/mc.c > 4mc.c

4mc.o: \
compile 4mc.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4mc.c

4mr.c: \
idea/mr.c pentium/mr.c ppro/mr.c sparc/mr.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/mr.c > 4mr.c

4mr.o: \
compile 4mr.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4mr.c

4r0.c: \
idea/r0.c pentium/r0.c ppro/r0.c sparc/r0.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/r0.c > 4r0.c

4r0.o: \
compile 4r0.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4r0.c

4r1.c: \
idea/r1.c pentium/r1.c ppro/r1.c sparc/r1.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/r1.c > 4r1.c

4r1.o: \
compile 4r1.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4r1.c

4r2.c: \
idea/r2.c pentium/r2.c ppro/r2.c sparc/r2.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/r2.c > 4r2.c

4r2.o: \
compile 4r2.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4r2.c

4r3.c: \
idea/r3.c pentium/r3.c ppro/r3.c sparc/r3.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/r3.c > 4r3.c

4r3.o: \
compile 4r3.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4r3.c

4r4.c: \
idea/r4.c pentium/r4.c ppro/r4.c sparc/r4.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/r4.c > 4r4.c

4r4.o: \
compile 4r4.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4r4.c

4r5.c: \
idea/r5.c pentium/r5.c ppro/r5.c sparc/r5.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/r5.c > 4r5.c

4r5.o: \
compile 4r5.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4r5.c

4sc.c: \
idea/sc.c pentium/sc.c ppro/sc.c sparc/sc.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/sc.c > 4sc.c

4sc.o: \
compile 4sc.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4sc.c

4sr.c: \
idea/sr.c pentium/sr.c ppro/sr.c sparc/sr.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/sr.c > 4sr.c

4sr.o: \
compile 4sr.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4sr.c

4u0.c: \
idea/u0.c pentium/u0.c ppro/u0.c sparc/u0.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/u0.c > 4u0.c

4u0.o: \
compile 4u0.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4u0.c

4u1.c: \
idea/u1.c pentium/u1.c ppro/u1.c sparc/u1.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/u1.c > 4u1.c

4u1.o: \
compile 4u1.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4u1.c

4u2.c: \
idea/u2.c pentium/u2.c ppro/u2.c sparc/u2.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/u2.c > 4u2.c

4u2.o: \
compile 4u2.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4u2.c

4u3.c: \
idea/u3.c pentium/u3.c ppro/u3.c sparc/u3.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/u3.c > 4u3.c

4u3.o: \
compile 4u3.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4u3.c

4u4.c: \
idea/u4.c pentium/u4.c ppro/u4.c sparc/u4.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/u4.c > 4u4.c

4u4.o: \
compile 4u4.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4u4.c

4u5.c: \
idea/u5.c pentium/u5.c ppro/u5.c sparc/u5.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/u5.c > 4u5.c

4u5.o: \
compile 4u5.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4u5.c

4v0.c: \
idea/v0.c pentium/v0.c ppro/v0.c sparc/v0.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/v0.c > 4v0.c

4v0.o: \
compile 4v0.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4v0.c

4v1.c: \
idea/v1.c pentium/v1.c ppro/v1.c sparc/v1.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/v1.c > 4v1.c

4v1.o: \
compile 4v1.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4v1.c

4v2.c: \
idea/v2.c pentium/v2.c ppro/v2.c sparc/v2.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/v2.c > 4v2.c

4v2.o: \
compile 4v2.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4v2.c

4v3.c: \
idea/v3.c pentium/v3.c ppro/v3.c sparc/v3.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/v3.c > 4v3.c

4v3.o: \
compile 4v3.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4v3.c

4v4.c: \
idea/v4.c pentium/v4.c ppro/v4.c sparc/v4.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/v4.c > 4v4.c

4v4.o: \
compile 4v4.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4v4.c

4v5.c: \
idea/v5.c pentium/v5.c ppro/v5.c sparc/v5.c auto_opt
	sed 1s/PRE/pre4.c/ `cat auto_opt`/v5.c > 4v5.c

4v5.o: \
compile 4v5.c pre4.c fftc4.h complex4.h real4.h fftr4.h real4.h 4i.c
	./compile 4v5.c

8c0.c: \
idea/c0.c pentium/c0.c ppro/c0.c sparc/c0.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/c0.c > 8c0.c

8c0.o: \
compile 8c0.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8c0.c

8c1.c: \
idea/c1.c pentium/c1.c ppro/c1.c sparc/c1.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/c1.c > 8c1.c

8c1.o: \
compile 8c1.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8c1.c

8c2.c: \
idea/c2.c pentium/c2.c ppro/c2.c sparc/c2.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/c2.c > 8c2.c

8c2.o: \
compile 8c2.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8c2.c

8c3.c: \
idea/c3.c pentium/c3.c ppro/c3.c sparc/c3.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/c3.c > 8c3.c

8c3.o: \
compile 8c3.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8c3.c

8c4.c: \
idea/c4.c pentium/c4.c ppro/c4.c sparc/c4.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/c4.c > 8c4.c

8c4.o: \
compile 8c4.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8c4.c

8c5.c: \
idea/c5.c pentium/c5.c ppro/c5.c sparc/c5.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/c5.c > 8c5.c

8c5.o: \
compile 8c5.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8c5.c

8d0.c: \
idea/d0.c pentium/d0.c ppro/d0.c sparc/d0.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/d0.c > 8d0.c

8d0.o: \
compile 8d0.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c \
roots/16.c roots/32.c roots/64.c roots/128.c roots/256.c
	./compile 8d0.c

8d1.c: \
idea/d1.c pentium/d1.c ppro/d1.c sparc/d1.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/d1.c > 8d1.c

8d1.o: \
compile 8d1.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c \
roots/512.c
	./compile 8d1.c

8d2.c: \
idea/d2.c pentium/d2.c ppro/d2.c sparc/d2.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/d2.c > 8d2.c

8d2.o: \
compile 8d2.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c \
roots/h1024.c
	./compile 8d2.c

8d3.c: \
idea/d3.c pentium/d3.c ppro/d3.c sparc/d3.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/d3.c > 8d3.c

8d3.o: \
compile 8d3.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c \
roots/h2048.c
	./compile 8d3.c

8d4.c: \
idea/d4.c pentium/d4.c ppro/d4.c sparc/d4.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/d4.c > 8d4.c

8d4.o: \
compile 8d4.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c \
roots/h4096.c
	./compile 8d4.c

8d5.c: \
idea/d5.c pentium/d5.c ppro/d5.c sparc/d5.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/d5.c > 8d5.c

8d5.o: \
compile 8d5.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c \
roots/h8192.c
	./compile 8d5.c

8i.c: \
idea/i.c pentium/i.c ppro/i.c sparc/i.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/i.c > 8i.c

8mc.c: \
idea/mc.c pentium/mc.c ppro/mc.c sparc/mc.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/mc.c > 8mc.c

8mc.o: \
compile 8mc.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8mc.c

8mr.c: \
idea/mr.c pentium/mr.c ppro/mr.c sparc/mr.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/mr.c > 8mr.c

8mr.o: \
compile 8mr.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8mr.c

8r0.c: \
idea/r0.c pentium/r0.c ppro/r0.c sparc/r0.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/r0.c > 8r0.c

8r0.o: \
compile 8r0.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8r0.c

8r1.c: \
idea/r1.c pentium/r1.c ppro/r1.c sparc/r1.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/r1.c > 8r1.c

8r1.o: \
compile 8r1.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8r1.c

8r2.c: \
idea/r2.c pentium/r2.c ppro/r2.c sparc/r2.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/r2.c > 8r2.c

8r2.o: \
compile 8r2.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8r2.c

8r3.c: \
idea/r3.c pentium/r3.c ppro/r3.c sparc/r3.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/r3.c > 8r3.c

8r3.o: \
compile 8r3.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8r3.c

8r4.c: \
idea/r4.c pentium/r4.c ppro/r4.c sparc/r4.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/r4.c > 8r4.c

8r4.o: \
compile 8r4.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8r4.c

8r5.c: \
idea/r5.c pentium/r5.c ppro/r5.c sparc/r5.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/r5.c > 8r5.c

8r5.o: \
compile 8r5.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8r5.c

8sc.c: \
idea/sc.c pentium/sc.c ppro/sc.c sparc/sc.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/sc.c > 8sc.c

8sc.o: \
compile 8sc.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8sc.c

8sr.c: \
idea/sr.c pentium/sr.c ppro/sr.c sparc/sr.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/sr.c > 8sr.c

8sr.o: \
compile 8sr.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8sr.c

8u0.c: \
idea/u0.c pentium/u0.c ppro/u0.c sparc/u0.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/u0.c > 8u0.c

8u0.o: \
compile 8u0.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8u0.c

8u1.c: \
idea/u1.c pentium/u1.c ppro/u1.c sparc/u1.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/u1.c > 8u1.c

8u1.o: \
compile 8u1.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8u1.c

8u2.c: \
idea/u2.c pentium/u2.c ppro/u2.c sparc/u2.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/u2.c > 8u2.c

8u2.o: \
compile 8u2.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8u2.c

8u3.c: \
idea/u3.c pentium/u3.c ppro/u3.c sparc/u3.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/u3.c > 8u3.c

8u3.o: \
compile 8u3.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8u3.c

8u4.c: \
idea/u4.c pentium/u4.c ppro/u4.c sparc/u4.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/u4.c > 8u4.c

8u4.o: \
compile 8u4.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8u4.c

8u5.c: \
idea/u5.c pentium/u5.c ppro/u5.c sparc/u5.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/u5.c > 8u5.c

8u5.o: \
compile 8u5.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8u5.c

8v0.c: \
idea/v0.c pentium/v0.c ppro/v0.c sparc/v0.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/v0.c > 8v0.c

8v0.o: \
compile 8v0.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8v0.c

8v1.c: \
idea/v1.c pentium/v1.c ppro/v1.c sparc/v1.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/v1.c > 8v1.c

8v1.o: \
compile 8v1.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8v1.c

8v2.c: \
idea/v2.c pentium/v2.c ppro/v2.c sparc/v2.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/v2.c > 8v2.c

8v2.o: \
compile 8v2.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8v2.c

8v3.c: \
idea/v3.c pentium/v3.c ppro/v3.c sparc/v3.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/v3.c > 8v3.c

8v3.o: \
compile 8v3.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8v3.c

8v4.c: \
idea/v4.c pentium/v4.c ppro/v4.c sparc/v4.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/v4.c > 8v4.c

8v4.o: \
compile 8v4.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8v4.c

8v5.c: \
idea/v5.c pentium/v5.c ppro/v5.c sparc/v5.c auto_opt
	sed 1s/PRE/pre8.c/ `cat auto_opt`/v5.c > 8v5.c

8v5.o: \
compile 8v5.c pre8.c fftc8.h complex8.h real8.h fftr8.h real8.h 8i.c
	./compile 8v5.c

accuracy: \
load accuracy.o djbfft.a math.lib
	./load accuracy djbfft.a  `cat math.lib`

accuracy.o: \
compile accuracy.c fftc4.h complex4.h real4.h fftc8.h complex8.h \
real8.h fftr4.h real4.h fftr8.h real8.h
	./compile accuracy.c

accuracy2: \
load accuracy2.o djbfft.a math.lib
	./load accuracy2 djbfft.a  `cat math.lib`

accuracy2.o: \
compile accuracy2.c fftc4.h complex4.h real4.h fftc8.h complex8.h \
real8.h fftr4.h real4.h fftr8.h real8.h fftfreq.h
	./compile accuracy2.c

auto-str: \
load auto-str.o substdio.a error.a str.a
	./load auto-str substdio.a error.a str.a 

auto-str.o: \
compile auto-str.c substdio.h readwrite.h exit.h
	./compile auto-str.c

auto_align: \
trycpp.c compilebase
	( ./compilebase trycpp.c \
	-malign-double >/dev/null 2>&1 \
	&& echo -malign-double || exit 0 ) > auto_align
	rm -f trycpp.o

auto_home.c: \
auto-str conf-home
	./auto-str auto_home `head -1 conf-home` > auto_home.c

auto_home.o: \
compile auto_home.c
	./compile auto_home.c

auto_opt: \
print-opt.sh conf-opt systype
	sh print-opt.sh > auto_opt

byte_copy.o: \
compile byte_copy.c byte.h
	./compile byte_copy.c

byte_cr.o: \
compile byte_cr.c byte.h
	./compile byte_cr.c

check: \
it instcheck
	./instcheck

choose: \
warn-auto.sh choose.sh conf-home
	cat warn-auto.sh choose.sh \
	| sed s}HOME}"`head -1 conf-home`"}g \
	> choose
	chmod 755 choose

compile: \
warn-auto.sh conf-cc auto_align
	( cat warn-auto.sh; \
	echo exec \
	"`head -1 conf-cc`" "`cat auto_align`" \
	'-c $${1+"$$@"}' \
	) > compile
	chmod 755 compile

compilebase: \
warn-auto.sh conf-cc
	( cat warn-auto.sh; \
	echo exec \
	"`head -1 conf-cc`" '-c $${1+"$$@"}' \
	) > compilebase
	chmod 755 compilebase

djbfft.a: \
makelib 8sc.o 8u5.o 8u4.o 8u3.o 8u2.o 8u1.o 8u0.o 8mc.o 8c0.o 8c1.o \
8c2.o 8c3.o 8c4.o 8c5.o 8d0.o 8d1.o 8d2.o 8d3.o 8d4.o 8d5.o 8v5.o \
8v4.o 8v3.o 8v2.o 8v1.o 8v0.o 8mr.o 8r0.o 8r1.o 8r2.o 8r3.o 8r4.o \
8r5.o 8sr.o 4sc.o 4u5.o 4u4.o 4u3.o 4u2.o 4u1.o 4u0.o 4mc.o 4c0.o \
4c1.o 4c2.o 4c3.o 4c4.o 4c5.o 4d0.o 4d1.o 4d2.o 4d3.o 4d4.o 4d5.o \
4v5.o 4v4.o 4v3.o 4v2.o 4v1.o 4v0.o 4mr.o 4r0.o 4r1.o 4r2.o 4r3.o \
4r4.o 4r5.o 4sr.o fftfreq.o
	./makelib djbfft.a 8sc.o 8u5.o 8u4.o 8u3.o 8u2.o 8u1.o \
	8u0.o 8mc.o 8c0.o 8c1.o 8c2.o 8c3.o 8c4.o 8c5.o 8d0.o 8d1.o \
	8d2.o 8d3.o 8d4.o 8d5.o 8v5.o 8v4.o 8v3.o 8v2.o 8v1.o 8v0.o \
	8mr.o 8r0.o 8r1.o 8r2.o 8r3.o 8r4.o 8r5.o 8sr.o 4sc.o 4u5.o \
	4u4.o 4u3.o 4u2.o 4u1.o 4u0.o 4mc.o 4c0.o 4c1.o 4c2.o 4c3.o \
	4c4.o 4c5.o 4d0.o 4d1.o 4d2.o 4d3.o 4d4.o 4d5.o 4v5.o 4v4.o \
	4v3.o 4v2.o 4v1.o 4v0.o 4mr.o 4r0.o 4r1.o 4r2.o 4r3.o 4r4.o \
	4r5.o 4sr.o fftfreq.o

error.a: \
makelib error.o error_str.o
	./makelib error.a error.o error_str.o

error.o: \
compile error.c error.h
	./compile error.c

error_str.o: \
compile error_str.c error.h
	./compile error_str.c

fftfreq.o: \
compile fftfreq.c fftfreq.h
	./compile fftfreq.c

hasgethr.h: \
choose compile load trygethr.c hasgethr.h1 hasgethr.h2
	./choose cl trygethr hasgethr.h1 hasgethr.h2 > hasgethr.h

hasrdtsc.h: \
choose compile load tryrdtsc.c hasrdtsc.h1 hasrdtsc.h2
	./choose clr tryrdtsc hasrdtsc.h1 hasrdtsc.h2 > hasrdtsc.h

hier.o: \
compile hier.c auto_home.h
	./compile hier.c

install: \
load install.o hier.o auto_home.o strerr.a substdio.a open.a error.a \
str.a
	./load install hier.o auto_home.o strerr.a substdio.a \
	open.a error.a str.a 

install.o: \
compile install.c substdio.h strerr.h error.h open.h readwrite.h \
exit.h
	./compile install.c

instcheck: \
load instcheck.o hier.o auto_home.o strerr.a substdio.a error.a str.a
	./load instcheck hier.o auto_home.o strerr.a substdio.a \
	error.a str.a 

instcheck.o: \
compile instcheck.c strerr.h error.h readwrite.h exit.h
	./compile instcheck.c

it: \
prog install instcheck

load: \
warn-auto.sh conf-ld
	( cat warn-auto.sh; \
	echo 'main="$$1"; shift'; \
	echo exec "`head -1 conf-ld`" \
	'-o "$$main" "$$main".o $${1+"$$@"}' \
	) > load
	chmod 755 load

makelib: \
warn-auto.sh systype
	( cat warn-auto.sh; \
	echo 'main="$$1"; shift'; \
	echo 'rm -f "$$main"'; \
	echo 'ar cr "$$main" $${1+"$$@"}'; \
	case "`cat systype`" in \
	sunos-5.*) ;; \
	unix_sv*) ;; \
	irix64-*) ;; \
	irix-*) ;; \
	dgux-*) ;; \
	hp-ux-*) ;; \
	sco*) ;; \
	*) echo 'ranlib "$$main"' ;; \
	esac \
	) > makelib
	chmod 755 makelib

open.a: \
makelib open_read.o open_trunc.o
	./makelib open.a open_read.o open_trunc.o

open_read.o: \
compile open_read.c open.h
	./compile open_read.c

open_trunc.o: \
compile open_trunc.c open.h
	./compile open_trunc.c

prog: \
djbfft.a accuracy accuracy2 speed speed.out

setup: \
it install
	./install

speed: \
load speed.o djbfft.a math.lib
	./load speed djbfft.a  `cat math.lib`

speed.o: \
compile speed.c fftr4.h real4.h fftr8.h real8.h fftc4.h complex4.h \
real4.h fftc8.h complex8.h real8.h timing.h hasrdtsc.h hasgethr.h
	./compile speed.c

speed.out: \
speed
	./speed > speed.out

str.a: \
makelib byte_copy.o byte_cr.o str_len.o
	./makelib str.a byte_copy.o byte_cr.o str_len.o

str_len.o: \
compile str_len.c str.h
	./compile str_len.c

strerr.a: \
makelib strerr_sys.o strerr_die.o
	./makelib strerr.a strerr_sys.o strerr_die.o

strerr_die.o: \
compile strerr_die.c substdio.h subfd.h substdio.h exit.h strerr.h
	./compile strerr_die.c

strerr_sys.o: \
compile strerr_sys.c error.h strerr.h
	./compile strerr_sys.c

subfderr.o: \
compile subfderr.c readwrite.h substdio.h subfd.h substdio.h
	./compile subfderr.c

substdi.o: \
compile substdi.c substdio.h byte.h error.h
	./compile substdi.c

substdio.a: \
makelib substdio.o substdi.o substdo.o subfderr.o substdio_copy.o
	./makelib substdio.a substdio.o substdi.o substdo.o \
	subfderr.o substdio_copy.o

substdio.o: \
compile substdio.c substdio.h
	./compile substdio.c

substdio_copy.o: \
compile substdio_copy.c substdio.h
	./compile substdio_copy.c

substdo.o: \
compile substdo.c substdio.h str.h byte.h error.h
	./compile substdo.c

systype: \
find-systype.sh conf-cc conf-ld trycpp.c x86cpuid.c
	( cat warn-auto.sh; \
	echo CC=\'`head -1 conf-cc`\'; \
	echo LD=\'`head -1 conf-ld`\'; \
	cat find-systype.sh; \
	) | sh > systype
