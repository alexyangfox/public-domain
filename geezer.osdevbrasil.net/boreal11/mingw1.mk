.SUFFIXES:
.SUFFIXES: .d .c .s .asm .o .x .r

# DEFINES
MAKEFILE:= makefile
MAKEDEP	:= $(MAKEFILE)
INCDIR	:= ../inc
USCRIPT	:= user.ld
LIBK	:= ../lib/libk.a
LIBC	:= ../lib/libc.a

# converts / to \ in the pathnames, for the benefit of DOS MOVE
# Must be defined with = instead of :=
# Called indirectly like this: $(call MV, foo/bar, baz/bat)
MV	= move /y $(subst /,\,$(1)) $(subst /,\,$(2))

# Installs file(s) onto floppy disk
define INSTALL
	$(COMSPEC) /c for %f in ($(subst /,\,$(1))) do copy /y %f a:
endef

# How to link relocatable task
# Called like this:	$(call LDRT, tetris, ustart.o tetris.o $(LIBK))
define LDRT
	ld -g -d -r -o $(1).r $(2)
	objdump --source --reloc $(1).r >temp1.tmp
	$(call MV,temp1.tmp,$(1).lst)
	nm -n $(1).r >temp1.tmp
	$(call MV,temp1.tmp,$(1).sym)
	strip --strip-unneeded $(1).r
endef

# How to link executable task
# Called like this:	$(call LDT, tetris, ustart.o tetris.o $(LIBC))
define LDT
	ld -g -T $(USCRIPT) --file-align 4096 --image-base 0 -o $(1).x $(2)
	objdump --source $(1).x >temp1.tmp
	$(call MV,temp1.tmp,$(1).lst)
	nm -n $(1).x >temp1.tmp
	$(call MV,temp1.tmp,$(1).sym)
endef
# MinGW 'strip' changes the file alignment (a bug, dammit!)
#	strip $(1).x

# TARGETS

# IMPLICIT RULES

# DEPENDENCIES

# EXPLICIT RULES
