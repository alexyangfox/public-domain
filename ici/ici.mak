# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=icicmd - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to icicmd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ici - Win32 Release" && "$(CFG)" != "ici - Win32 Debug" &&\
 "$(CFG)" != "icicmd - Win32 Release" && "$(CFG)" != "icicmd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ici.mak" CFG="icicmd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ici - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ici - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "icicmd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "icicmd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "icicmd - Win32 Debug"

!IF  "$(CFG)" == "ici - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\ici.lib"

CLEAN : 
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\arith.obj"
	-@erase "$(INTDIR)\array.obj"
	-@erase "$(INTDIR)\buf.obj"
	-@erase "$(INTDIR)\call.obj"
	-@erase "$(INTDIR)\catch.obj"
	-@erase "$(INTDIR)\cfunc.obj"
	-@erase "$(INTDIR)\clib.obj"
	-@erase "$(INTDIR)\clib2.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\conf.obj"
	-@erase "$(INTDIR)\control.obj"
	-@erase "$(INTDIR)\exec.obj"
	-@erase "$(INTDIR)\exerror.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\float.obj"
	-@erase "$(INTDIR)\forall.obj"
	-@erase "$(INTDIR)\func.obj"
	-@erase "$(INTDIR)\icimain.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\int.obj"
	-@erase "$(INTDIR)\lex.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mark.obj"
	-@erase "$(INTDIR)\mem.obj"
	-@erase "$(INTDIR)\mkvar.obj"
	-@erase "$(INTDIR)\nptrs.obj"
	-@erase "$(INTDIR)\null.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\op.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\pc.obj"
	-@erase "$(INTDIR)\ptr.obj"
	-@erase "$(INTDIR)\regexp.obj"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\sfile.obj"
	-@erase "$(INTDIR)\skt.obj"
	-@erase "$(INTDIR)\smash.obj"
	-@erase "$(INTDIR)\src.obj"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\struct.obj"
	-@erase "$(INTDIR)\syserr.obj"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\unary.obj"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\wrap.obj"
	-@erase "$(OUTDIR)\ici.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D CONFIG_FILE=\"confdos.h\" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /Zi /O2 /I "." /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWS" /D CONFIG_FILE=\"confdos.h\" /Fp"$(INTDIR)/ici.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ici.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/ici.lib" 
LIB32_OBJS= \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\arith.obj" \
	"$(INTDIR)\array.obj" \
	"$(INTDIR)\buf.obj" \
	"$(INTDIR)\call.obj" \
	"$(INTDIR)\catch.obj" \
	"$(INTDIR)\cfunc.obj" \
	"$(INTDIR)\clib.obj" \
	"$(INTDIR)\clib2.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\conf.obj" \
	"$(INTDIR)\control.obj" \
	"$(INTDIR)\exec.obj" \
	"$(INTDIR)\exerror.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\float.obj" \
	"$(INTDIR)\forall.obj" \
	"$(INTDIR)\func.obj" \
	"$(INTDIR)\icimain.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\int.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mark.obj" \
	"$(INTDIR)\mem.obj" \
	"$(INTDIR)\mkvar.obj" \
	"$(INTDIR)\nptrs.obj" \
	"$(INTDIR)\null.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\op.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\pc.obj" \
	"$(INTDIR)\ptr.obj" \
	"$(INTDIR)\regexp.obj" \
	"$(INTDIR)\set.obj" \
	"$(INTDIR)\sfile.obj" \
	"$(INTDIR)\skt.obj" \
	"$(INTDIR)\smash.obj" \
	"$(INTDIR)\src.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\struct.obj" \
	"$(INTDIR)\syserr.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\unary.obj" \
	"$(INTDIR)\wrap.obj"

"$(OUTDIR)\ici.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ici - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\ici.lib"

CLEAN : 
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\arith.obj"
	-@erase "$(INTDIR)\array.obj"
	-@erase "$(INTDIR)\buf.obj"
	-@erase "$(INTDIR)\call.obj"
	-@erase "$(INTDIR)\catch.obj"
	-@erase "$(INTDIR)\cfunc.obj"
	-@erase "$(INTDIR)\clib.obj"
	-@erase "$(INTDIR)\clib2.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\conf.obj"
	-@erase "$(INTDIR)\control.obj"
	-@erase "$(INTDIR)\exec.obj"
	-@erase "$(INTDIR)\exerror.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\float.obj"
	-@erase "$(INTDIR)\forall.obj"
	-@erase "$(INTDIR)\func.obj"
	-@erase "$(INTDIR)\icimain.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\int.obj"
	-@erase "$(INTDIR)\lex.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mark.obj"
	-@erase "$(INTDIR)\mem.obj"
	-@erase "$(INTDIR)\mkvar.obj"
	-@erase "$(INTDIR)\nptrs.obj"
	-@erase "$(INTDIR)\null.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\op.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\pc.obj"
	-@erase "$(INTDIR)\ptr.obj"
	-@erase "$(INTDIR)\regexp.obj"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\sfile.obj"
	-@erase "$(INTDIR)\skt.obj"
	-@erase "$(INTDIR)\smash.obj"
	-@erase "$(INTDIR)\src.obj"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\struct.obj"
	-@erase "$(INTDIR)\syserr.obj"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\unary.obj"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\wrap.obj"
	-@erase "$(OUTDIR)\ici.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /GX /Zi /Od /I "." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D CONFIG_FILE=\"confdos.h\" /YX /c
CPP_PROJ=/nologo /MTd /W3 /GX /Zi /Od /I "." /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D CONFIG_FILE=\"confdos.h\" /Fp"$(INTDIR)/ici.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ici.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/ici.lib" 
LIB32_OBJS= \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\arith.obj" \
	"$(INTDIR)\array.obj" \
	"$(INTDIR)\buf.obj" \
	"$(INTDIR)\call.obj" \
	"$(INTDIR)\catch.obj" \
	"$(INTDIR)\cfunc.obj" \
	"$(INTDIR)\clib.obj" \
	"$(INTDIR)\clib2.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\conf.obj" \
	"$(INTDIR)\control.obj" \
	"$(INTDIR)\exec.obj" \
	"$(INTDIR)\exerror.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\float.obj" \
	"$(INTDIR)\forall.obj" \
	"$(INTDIR)\func.obj" \
	"$(INTDIR)\icimain.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\int.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mark.obj" \
	"$(INTDIR)\mem.obj" \
	"$(INTDIR)\mkvar.obj" \
	"$(INTDIR)\nptrs.obj" \
	"$(INTDIR)\null.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\op.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\pc.obj" \
	"$(INTDIR)\ptr.obj" \
	"$(INTDIR)\regexp.obj" \
	"$(INTDIR)\set.obj" \
	"$(INTDIR)\sfile.obj" \
	"$(INTDIR)\skt.obj" \
	"$(INTDIR)\smash.obj" \
	"$(INTDIR)\src.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\struct.obj" \
	"$(INTDIR)\syserr.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\unary.obj" \
	"$(INTDIR)\wrap.obj"

"$(OUTDIR)\ici.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "icicmd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "icicmd\Release"
# PROP BASE Intermediate_Dir "icicmd\Release"
# PROP BASE Target_Dir "icicmd"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "icicmd\Release"
# PROP Intermediate_Dir "icicmd\Release"
# PROP Target_Dir "icicmd"
OUTDIR=.\icicmd\Release
INTDIR=.\icicmd\Release

ALL : "$(OUTDIR)\icicmd.exe"

CLEAN : 
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\arith.obj"
	-@erase "$(INTDIR)\array.obj"
	-@erase "$(INTDIR)\buf.obj"
	-@erase "$(INTDIR)\call.obj"
	-@erase "$(INTDIR)\catch.obj"
	-@erase "$(INTDIR)\cfunc.obj"
	-@erase "$(INTDIR)\clib.obj"
	-@erase "$(INTDIR)\clib2.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\conf.obj"
	-@erase "$(INTDIR)\control.obj"
	-@erase "$(INTDIR)\exec.obj"
	-@erase "$(INTDIR)\exerror.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\findpath.obj"
	-@erase "$(INTDIR)\float.obj"
	-@erase "$(INTDIR)\forall.obj"
	-@erase "$(INTDIR)\func.obj"
	-@erase "$(INTDIR)\icimain.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\int.obj"
	-@erase "$(INTDIR)\lex.obj"
	-@erase "$(INTDIR)\load.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mark.obj"
	-@erase "$(INTDIR)\mem.obj"
	-@erase "$(INTDIR)\mkvar.obj"
	-@erase "$(INTDIR)\nptrs.obj"
	-@erase "$(INTDIR)\null.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\op.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\pc.obj"
	-@erase "$(INTDIR)\ptr.obj"
	-@erase "$(INTDIR)\regexp.obj"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\sfile.obj"
	-@erase "$(INTDIR)\skt.obj"
	-@erase "$(INTDIR)\smash.obj"
	-@erase "$(INTDIR)\src.obj"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\struct.obj"
	-@erase "$(INTDIR)\syscall.obj"
	-@erase "$(INTDIR)\syserr.obj"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\unary.obj"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\wrap.obj"
	-@erase "$(OUTDIR)\icicmd.exe"
	-@erase "$(OUTDIR)\icicmd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /YX /D CONFIG_FILE=\"conf-w32.h\" /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O2 /I "." /D "NDEBUG" /D "WIN32" /D\
 "_CONSOLE" /Fp"$(INTDIR)/icicmd.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /D\
 CONFIG_FILE=\"conf-w32.h\" /c 
CPP_OBJS=.\icicmd\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/icicmd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/icicmd.pdb" /debug /machine:I386 /def:".\ici.def"\
 /out:"$(OUTDIR)/icicmd.exe" 
DEF_FILE= \
	".\ici.def"
LINK32_OBJS= \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\arith.obj" \
	"$(INTDIR)\array.obj" \
	"$(INTDIR)\buf.obj" \
	"$(INTDIR)\call.obj" \
	"$(INTDIR)\catch.obj" \
	"$(INTDIR)\cfunc.obj" \
	"$(INTDIR)\clib.obj" \
	"$(INTDIR)\clib2.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\conf.obj" \
	"$(INTDIR)\control.obj" \
	"$(INTDIR)\exec.obj" \
	"$(INTDIR)\exerror.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\findpath.obj" \
	"$(INTDIR)\float.obj" \
	"$(INTDIR)\forall.obj" \
	"$(INTDIR)\func.obj" \
	"$(INTDIR)\icimain.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\int.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\load.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mark.obj" \
	"$(INTDIR)\mem.obj" \
	"$(INTDIR)\mkvar.obj" \
	"$(INTDIR)\nptrs.obj" \
	"$(INTDIR)\null.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\op.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\pc.obj" \
	"$(INTDIR)\ptr.obj" \
	"$(INTDIR)\regexp.obj" \
	"$(INTDIR)\set.obj" \
	"$(INTDIR)\sfile.obj" \
	"$(INTDIR)\skt.obj" \
	"$(INTDIR)\smash.obj" \
	"$(INTDIR)\src.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\struct.obj" \
	"$(INTDIR)\syscall.obj" \
	"$(INTDIR)\syserr.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\unary.obj" \
	"$(INTDIR)\wrap.obj"

"$(OUTDIR)\icicmd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "icicmd\Debug"
# PROP BASE Intermediate_Dir "icicmd\Debug"
# PROP BASE Target_Dir "icicmd"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "icicmd\Debug"
# PROP Intermediate_Dir "icicmd\Debug"
# PROP Target_Dir "icicmd"
OUTDIR=.\icicmd\Debug
INTDIR=.\icicmd\Debug

ALL : "$(OUTDIR)\icicmd.exe"

CLEAN : 
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\arith.obj"
	-@erase "$(INTDIR)\array.obj"
	-@erase "$(INTDIR)\buf.obj"
	-@erase "$(INTDIR)\call.obj"
	-@erase "$(INTDIR)\catch.obj"
	-@erase "$(INTDIR)\cfunc.obj"
	-@erase "$(INTDIR)\clib.obj"
	-@erase "$(INTDIR)\clib2.obj"
	-@erase "$(INTDIR)\compile.obj"
	-@erase "$(INTDIR)\conf.obj"
	-@erase "$(INTDIR)\control.obj"
	-@erase "$(INTDIR)\exec.obj"
	-@erase "$(INTDIR)\exerror.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\findpath.obj"
	-@erase "$(INTDIR)\float.obj"
	-@erase "$(INTDIR)\forall.obj"
	-@erase "$(INTDIR)\func.obj"
	-@erase "$(INTDIR)\icimain.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\int.obj"
	-@erase "$(INTDIR)\lex.obj"
	-@erase "$(INTDIR)\load.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mark.obj"
	-@erase "$(INTDIR)\mem.obj"
	-@erase "$(INTDIR)\mkvar.obj"
	-@erase "$(INTDIR)\nptrs.obj"
	-@erase "$(INTDIR)\null.obj"
	-@erase "$(INTDIR)\object.obj"
	-@erase "$(INTDIR)\op.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\pc.obj"
	-@erase "$(INTDIR)\ptr.obj"
	-@erase "$(INTDIR)\regexp.obj"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\sfile.obj"
	-@erase "$(INTDIR)\skt.obj"
	-@erase "$(INTDIR)\smash.obj"
	-@erase "$(INTDIR)\src.obj"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\struct.obj"
	-@erase "$(INTDIR)\syscall.obj"
	-@erase "$(INTDIR)\syserr.obj"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\unary.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\wrap.obj"
	-@erase "$(OUTDIR)\icicmd.exe"
	-@erase "$(OUTDIR)\icicmd.ilk"
	-@erase "$(OUTDIR)\icicmd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /YX /D CONFIG_FILE=\"conf-w32.h\" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /D "_DEBUG" /D "WIN32" /D\
 "_CONSOLE" /Fp"$(INTDIR)/icicmd.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /D\
 CONFIG_FILE=\"conf-w32.h\" /c 
CPP_OBJS=.\icicmd\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/icicmd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/icicmd.pdb" /debug /machine:I386 /def:".\ici.def"\
 /out:"$(OUTDIR)/icicmd.exe" 
DEF_FILE= \
	".\ici.def"
LINK32_OBJS= \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\arith.obj" \
	"$(INTDIR)\array.obj" \
	"$(INTDIR)\buf.obj" \
	"$(INTDIR)\call.obj" \
	"$(INTDIR)\catch.obj" \
	"$(INTDIR)\cfunc.obj" \
	"$(INTDIR)\clib.obj" \
	"$(INTDIR)\clib2.obj" \
	"$(INTDIR)\compile.obj" \
	"$(INTDIR)\conf.obj" \
	"$(INTDIR)\control.obj" \
	"$(INTDIR)\exec.obj" \
	"$(INTDIR)\exerror.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\findpath.obj" \
	"$(INTDIR)\float.obj" \
	"$(INTDIR)\forall.obj" \
	"$(INTDIR)\func.obj" \
	"$(INTDIR)\icimain.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\int.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\load.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mark.obj" \
	"$(INTDIR)\mem.obj" \
	"$(INTDIR)\mkvar.obj" \
	"$(INTDIR)\nptrs.obj" \
	"$(INTDIR)\null.obj" \
	"$(INTDIR)\object.obj" \
	"$(INTDIR)\op.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\pc.obj" \
	"$(INTDIR)\ptr.obj" \
	"$(INTDIR)\regexp.obj" \
	"$(INTDIR)\set.obj" \
	"$(INTDIR)\sfile.obj" \
	"$(INTDIR)\skt.obj" \
	"$(INTDIR)\smash.obj" \
	"$(INTDIR)\src.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\struct.obj" \
	"$(INTDIR)\syscall.obj" \
	"$(INTDIR)\syserr.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\unary.obj" \
	"$(INTDIR)\wrap.obj"

"$(OUTDIR)\icicmd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "ici - Win32 Release"
# Name "ici - Win32 Debug"

!IF  "$(CFG)" == "ici - Win32 Release"

!ELSEIF  "$(CFG)" == "ici - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\alloc.c
DEP_CPP_ALLOC=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\alloc.obj" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\arith.c
DEP_CPP_ARITH=\
	".\alloc.h"\
	".\array.h"\
	".\binop.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\arith.obj" : $(SOURCE) $(DEP_CPP_ARITH) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\array.c
DEP_CPP_ARRAY=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\ptr.h"\
	".\struct.h"\
	

"$(INTDIR)\array.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\buf.c
DEP_CPP_BUF_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\buf.obj" : $(SOURCE) $(DEP_CPP_BUF_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\call.c
DEP_CPP_CALL_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\call.obj" : $(SOURCE) $(DEP_CPP_CALL_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\catch.c
DEP_CPP_CATCH=\
	".\alloc.h"\
	".\array.h"\
	".\catch.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\catch.obj" : $(SOURCE) $(DEP_CPP_CATCH) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cfunc.c
DEP_CPP_CFUNC=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\mem.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\cfunc.obj" : $(SOURCE) $(DEP_CPP_CFUNC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\clib.c
DEP_CPP_CLIB_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\skt.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\clib.obj" : $(SOURCE) $(DEP_CPP_CLIB_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\clib2.c
DEP_CPP_CLIB2=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\clib2.obj" : $(SOURCE) $(DEP_CPP_CLIB2) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\compile.c
DEP_CPP_COMPI=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\conf.c
DEP_CPP_CONF_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\conf.obj" : $(SOURCE) $(DEP_CPP_CONF_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\control.c
DEP_CPP_CONTR=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\forall.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\struct.h"\
	

"$(INTDIR)\control.obj" : $(SOURCE) $(DEP_CPP_CONTR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\exec.c
DEP_CPP_EXEC_=\
	".\alloc.h"\
	".\array.h"\
	".\binop.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\float.h"\
	".\forall.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\src.h"\
	".\str.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\exec.obj" : $(SOURCE) $(DEP_CPP_EXEC_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\exerror.c
DEP_CPP_EXERR=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\exerror.obj" : $(SOURCE) $(DEP_CPP_EXERR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\file.c
DEP_CPP_FILE_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\file.obj" : $(SOURCE) $(DEP_CPP_FILE_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\float.c
DEP_CPP_FLOAT=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\float.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\forall.c
DEP_CPP_FORAL=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\forall.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\forall.obj" : $(SOURCE) $(DEP_CPP_FORAL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\func.c
DEP_CPP_FUNC_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\mark.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\func.obj" : $(SOURCE) $(DEP_CPP_FUNC_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\icimain.c
DEP_CPP_ICIMA=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	".\wrap.h"\
	

"$(INTDIR)\icimain.obj" : $(SOURCE) $(DEP_CPP_ICIMA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\init.c
DEP_CPP_INIT_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\int.c
DEP_CPP_INT_C=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\int.obj" : $(SOURCE) $(DEP_CPP_INT_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\lex.c
DEP_CPP_LEX_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\src.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\lex.obj" : $(SOURCE) $(DEP_CPP_LEX_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\mark.c
DEP_CPP_MARK_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\mark.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\mark.obj" : $(SOURCE) $(DEP_CPP_MARK_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\mem.c
DEP_CPP_MEM_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\mem.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\mkvar.c
DEP_CPP_MKVAR=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\mkvar.obj" : $(SOURCE) $(DEP_CPP_MKVAR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\nptrs.c
DEP_CPP_NPTRS=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\nptrs.obj" : $(SOURCE) $(DEP_CPP_NPTRS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\null.c
DEP_CPP_NULL_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\null.obj" : $(SOURCE) $(DEP_CPP_NULL_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\object.c
DEP_CPP_OBJEC=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\op.c
DEP_CPP_OP_C38=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\op.obj" : $(SOURCE) $(DEP_CPP_OP_C38) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\parse.c
DEP_CPP_PARSE=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\parse.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\pc.c
DEP_CPP_PC_C3c=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\pc.h"\
	".\struct.h"\
	

"$(INTDIR)\pc.obj" : $(SOURCE) $(DEP_CPP_PC_C3c) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ptr.c
DEP_CPP_PTR_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\ptr.h"\
	".\struct.h"\
	

"$(INTDIR)\ptr.obj" : $(SOURCE) $(DEP_CPP_PTR_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\regexp.c
DEP_CPP_REGEX=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\re.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\regexp.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\set.c
DEP_CPP_SET_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\set.h"\
	".\struct.h"\
	

"$(INTDIR)\set.obj" : $(SOURCE) $(DEP_CPP_SET_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sfile.c
DEP_CPP_SFILE=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\sfile.obj" : $(SOURCE) $(DEP_CPP_SFILE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\skt.c
DEP_CPP_SKT_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\set.h"\
	".\skt.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\skt.obj" : $(SOURCE) $(DEP_CPP_SKT_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\smash.c
DEP_CPP_SMASH=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\smash.obj" : $(SOURCE) $(DEP_CPP_SMASH) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\src.c
DEP_CPP_SRC_C=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\src.h"\
	".\struct.h"\
	

"$(INTDIR)\src.obj" : $(SOURCE) $(DEP_CPP_SRC_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\string.c
DEP_CPP_STRIN=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\string.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\strtol.c
DEP_CPP_STRTO=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\strtol.obj" : $(SOURCE) $(DEP_CPP_STRTO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\struct.c
DEP_CPP_STRUC=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\struct.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\syserr.c
DEP_CPP_SYSER=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\syserr.obj" : $(SOURCE) $(DEP_CPP_SYSER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\trace.c
DEP_CPP_TRACE=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\trace.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\unary.c
DEP_CPP_UNARY=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\unary.obj" : $(SOURCE) $(DEP_CPP_UNARY) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\wrap.c
DEP_CPP_WRAP_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	".\wrap.h"\
	

"$(INTDIR)\wrap.obj" : $(SOURCE) $(DEP_CPP_WRAP_) "$(INTDIR)"


# End Source File
# End Target
################################################################################
# Begin Target

# Name "icicmd - Win32 Release"
# Name "icicmd - Win32 Debug"

!IF  "$(CFG)" == "icicmd - Win32 Release"

!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\alloc.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_ALLOC=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\alloc.obj" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_ALLOC=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\trace.h"\
	

"$(INTDIR)\alloc.obj" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\arith.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_ARITH=\
	".\alloc.h"\
	".\array.h"\
	".\binop.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\arith.obj" : $(SOURCE) $(DEP_CPP_ARITH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_ARITH=\
	".\alloc.h"\
	".\binop.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\arith.obj" : $(SOURCE) $(DEP_CPP_ARITH) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\array.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_ARRAY=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\ptr.h"\
	".\struct.h"\
	

"$(INTDIR)\array.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_ARRAY=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\primes.h"\
	".\ptr.h"\
	

"$(INTDIR)\array.obj" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\buf.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_BUF_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\buf.obj" : $(SOURCE) $(DEP_CPP_BUF_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_BUF_C=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	

"$(INTDIR)\buf.obj" : $(SOURCE) $(DEP_CPP_BUF_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\call.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_CALL_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\call.obj" : $(SOURCE) $(DEP_CPP_CALL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_CALL_=\
	".\alloc.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\str.h"\
	

"$(INTDIR)\call.obj" : $(SOURCE) $(DEP_CPP_CALL_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\catch.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_CATCH=\
	".\alloc.h"\
	".\array.h"\
	".\catch.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\catch.obj" : $(SOURCE) $(DEP_CPP_CATCH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_CATCH=\
	".\alloc.h"\
	".\catch.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	

"$(INTDIR)\catch.obj" : $(SOURCE) $(DEP_CPP_CATCH) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cfunc.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_CFUNC=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\mem.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\cfunc.obj" : $(SOURCE) $(DEP_CPP_CFUNC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_CFUNC=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\mem.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\cfunc.obj" : $(SOURCE) $(DEP_CPP_CFUNC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\clib.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_CLIB_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\skt.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\clib.obj" : $(SOURCE) $(DEP_CPP_CLIB_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_CLIB_=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\skt.h"\
	".\str.h"\
	

"$(INTDIR)\clib.obj" : $(SOURCE) $(DEP_CPP_CLIB_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\clib2.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_CLIB2=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\clib2.obj" : $(SOURCE) $(DEP_CPP_CLIB2) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_CLIB2=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	

"$(INTDIR)\clib2.obj" : $(SOURCE) $(DEP_CPP_CLIB2) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\compile.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_COMPI=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_COMPI=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	

"$(INTDIR)\compile.obj" : $(SOURCE) $(DEP_CPP_COMPI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\conf.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_CONF_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\conf.obj" : $(SOURCE) $(DEP_CPP_CONF_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_CONF_=\
	".\alloc.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	

"$(INTDIR)\conf.obj" : $(SOURCE) $(DEP_CPP_CONF_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\control.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_CONTR=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\forall.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\struct.h"\
	

"$(INTDIR)\control.obj" : $(SOURCE) $(DEP_CPP_CONTR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_CONTR=\
	".\alloc.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\forall.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\pc.h"\
	".\struct.h"\
	

"$(INTDIR)\control.obj" : $(SOURCE) $(DEP_CPP_CONTR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\exec.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_EXEC_=\
	".\alloc.h"\
	".\array.h"\
	".\binop.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\float.h"\
	".\forall.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\src.h"\
	".\str.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\exec.obj" : $(SOURCE) $(DEP_CPP_EXEC_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_EXEC_=\
	".\alloc.h"\
	".\binop.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\float.h"\
	".\forall.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\re.h"\
	".\set.h"\
	".\src.h"\
	".\str.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\exec.obj" : $(SOURCE) $(DEP_CPP_EXEC_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\exerror.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_EXERR=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\exerror.obj" : $(SOURCE) $(DEP_CPP_EXERR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_EXERR=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\str.h"\
	

"$(INTDIR)\exerror.obj" : $(SOURCE) $(DEP_CPP_EXERR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\file.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_FILE_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\file.obj" : $(SOURCE) $(DEP_CPP_FILE_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_FILE_=\
	".\alloc.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\primes.h"\
	

"$(INTDIR)\file.obj" : $(SOURCE) $(DEP_CPP_FILE_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\float.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_FLOAT=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\float.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_FLOAT=\
	".\alloc.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\primes.h"\
	

"$(INTDIR)\float.obj" : $(SOURCE) $(DEP_CPP_FLOAT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\forall.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_FORAL=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\forall.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\forall.obj" : $(SOURCE) $(DEP_CPP_FORAL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_FORAL=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\forall.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\forall.obj" : $(SOURCE) $(DEP_CPP_FORAL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\func.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_FUNC_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\mark.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\func.obj" : $(SOURCE) $(DEP_CPP_FUNC_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_FUNC_=\
	".\alloc.h"\
	".\buf.h"\
	".\catch.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\mark.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\func.obj" : $(SOURCE) $(DEP_CPP_FUNC_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\icimain.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_ICIMA=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	".\wrap.h"\
	

"$(INTDIR)\icimain.obj" : $(SOURCE) $(DEP_CPP_ICIMA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_ICIMA=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	".\wrap.h"\
	

"$(INTDIR)\icimain.obj" : $(SOURCE) $(DEP_CPP_ICIMA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\init.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_INIT_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_INIT_=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\struct.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\int.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_INT_C=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\int.obj" : $(SOURCE) $(DEP_CPP_INT_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_INT_C=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\primes.h"\
	

"$(INTDIR)\int.obj" : $(SOURCE) $(DEP_CPP_INT_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lex.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_LEX_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\src.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\lex.obj" : $(SOURCE) $(DEP_CPP_LEX_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_LEX_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\src.h"\
	".\trace.h"\
	

"$(INTDIR)\lex.obj" : $(SOURCE) $(DEP_CPP_LEX_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\mark.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_MARK_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\mark.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\mark.obj" : $(SOURCE) $(DEP_CPP_MARK_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_MARK_=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\mark.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	

"$(INTDIR)\mark.obj" : $(SOURCE) $(DEP_CPP_MARK_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mem.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_MEM_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\mem.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_MEM_C=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\mem.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\primes.h"\
	

"$(INTDIR)\mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mkvar.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_MKVAR=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\mkvar.obj" : $(SOURCE) $(DEP_CPP_MKVAR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_MKVAR=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\struct.h"\
	

"$(INTDIR)\mkvar.obj" : $(SOURCE) $(DEP_CPP_MKVAR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nptrs.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_NPTRS=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\nptrs.obj" : $(SOURCE) $(DEP_CPP_NPTRS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_NPTRS=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	

"$(INTDIR)\nptrs.obj" : $(SOURCE) $(DEP_CPP_NPTRS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\null.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_NULL_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\null.obj" : $(SOURCE) $(DEP_CPP_NULL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_NULL_=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	

"$(INTDIR)\null.obj" : $(SOURCE) $(DEP_CPP_NULL_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\object.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_OBJEC=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_OBJEC=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\primes.h"\
	".\str.h"\
	

"$(INTDIR)\object.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\op.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_OP_C38=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\struct.h"\
	

"$(INTDIR)\op.obj" : $(SOURCE) $(DEP_CPP_OP_C38) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_OP_C38=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\primes.h"\
	

"$(INTDIR)\op.obj" : $(SOURCE) $(DEP_CPP_OP_C38) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\parse.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_PARSE=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\parse.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_PARSE=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\parse.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pc.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_PC_C3c=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\pc.h"\
	".\struct.h"\
	

"$(INTDIR)\pc.obj" : $(SOURCE) $(DEP_CPP_PC_C3c) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_PC_C3c=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\pc.h"\
	

"$(INTDIR)\pc.obj" : $(SOURCE) $(DEP_CPP_PC_C3c) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ptr.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_PTR_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\ptr.h"\
	".\struct.h"\
	

"$(INTDIR)\ptr.obj" : $(SOURCE) $(DEP_CPP_PTR_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_PTR_C=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\primes.h"\
	".\ptr.h"\
	".\struct.h"\
	

"$(INTDIR)\ptr.obj" : $(SOURCE) $(DEP_CPP_PTR_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\regexp.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_REGEX=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\re.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\regexp.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_REGEX=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\primes.h"\
	".\re.h"\
	".\str.h"\
	

"$(INTDIR)\regexp.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\set.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_SET_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\set.h"\
	".\struct.h"\
	

"$(INTDIR)\set.obj" : $(SOURCE) $(DEP_CPP_SET_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_SET_C=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\primes.h"\
	".\set.h"\
	

"$(INTDIR)\set.obj" : $(SOURCE) $(DEP_CPP_SET_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sfile.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_SFILE=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\sfile.obj" : $(SOURCE) $(DEP_CPP_SFILE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_SFILE=\
	".\alloc.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	

"$(INTDIR)\sfile.obj" : $(SOURCE) $(DEP_CPP_SFILE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\skt.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_SKT_C=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\primes.h"\
	".\set.h"\
	".\skt.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\skt.obj" : $(SOURCE) $(DEP_CPP_SKT_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_SKT_C=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\primes.h"\
	".\set.h"\
	".\skt.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\skt.obj" : $(SOURCE) $(DEP_CPP_SKT_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\smash.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_SMASH=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\smash.obj" : $(SOURCE) $(DEP_CPP_SMASH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_SMASH=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	

"$(INTDIR)\smash.obj" : $(SOURCE) $(DEP_CPP_SMASH) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\src.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_SRC_C=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\src.h"\
	".\struct.h"\
	

"$(INTDIR)\src.obj" : $(SOURCE) $(DEP_CPP_SRC_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_SRC_C=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\src.h"\
	

"$(INTDIR)\src.obj" : $(SOURCE) $(DEP_CPP_SRC_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\string.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_STRIN=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\primes.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\string.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_STRIN=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\primes.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\string.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\strtol.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_STRTO=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\strtol.obj" : $(SOURCE) $(DEP_CPP_STRTO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_STRTO=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	

"$(INTDIR)\strtol.obj" : $(SOURCE) $(DEP_CPP_STRTO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\struct.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_STRUC=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\struct.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_STRUC=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\pc.h"\
	".\primes.h"\
	".\ptr.h"\
	".\str.h"\
	".\struct.h"\
	

"$(INTDIR)\struct.obj" : $(SOURCE) $(DEP_CPP_STRUC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\syserr.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_SYSER=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\syserr.obj" : $(SOURCE) $(DEP_CPP_SYSER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_SYSER=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	

"$(INTDIR)\syserr.obj" : $(SOURCE) $(DEP_CPP_SYSER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\trace.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_TRACE=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\trace.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_TRACE=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\file.h"\
	".\float.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\re.h"\
	".\set.h"\
	".\str.h"\
	".\struct.h"\
	".\trace.h"\
	

"$(INTDIR)\trace.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\unary.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_UNARY=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\unary.obj" : $(SOURCE) $(DEP_CPP_UNARY) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_UNARY=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\float.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	

"$(INTDIR)\unary.obj" : $(SOURCE) $(DEP_CPP_UNARY) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\wrap.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_WRAP_=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	".\wrap.h"\
	

"$(INTDIR)\wrap.obj" : $(SOURCE) $(DEP_CPP_WRAP_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_WRAP_=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\wrap.h"\
	

"$(INTDIR)\wrap.obj" : $(SOURCE) $(DEP_CPP_WRAP_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\load.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_LOAD_=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\load.obj" : $(SOURCE) $(DEP_CPP_LOAD_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_LOAD_=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\load.obj" : $(SOURCE) $(DEP_CPP_LOAD_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\findpath.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_FINDP=\
	".\alloc.h"\
	".\array.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\parse.h"\
	".\struct.h"\
	

"$(INTDIR)\findpath.obj" : $(SOURCE) $(DEP_CPP_FINDP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_FINDP=\
	".\alloc.h"\
	".\exec.h"\
	".\fwd.h"\
	".\multi.h"\
	

"$(INTDIR)\findpath.obj" : $(SOURCE) $(DEP_CPP_FINDP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ici.def

!IF  "$(CFG)" == "icicmd - Win32 Release"

!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\syscall.c

!IF  "$(CFG)" == "icicmd - Win32 Release"

DEP_CPP_SYSCA=\
	".\alloc.h"\
	".\array.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\parse.h"\
	".\re.h"\
	".\str.h"\
	".\struct.h"\
	{$(INCLUDE)}"\sys\Stat.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\syscall.obj" : $(SOURCE) $(DEP_CPP_SYSCA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "icicmd - Win32 Debug"

DEP_CPP_SYSCA=\
	".\alloc.h"\
	".\buf.h"\
	".\exec.h"\
	".\file.h"\
	".\func.h"\
	".\fwd.h"\
	".\int.h"\
	".\multi.h"\
	".\null.h"\
	".\object.h"\
	".\op.h"\
	".\re.h"\
	".\str.h"\
	{$(INCLUDE)}"\sys\Stat.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\syscall.obj" : $(SOURCE) $(DEP_CPP_SYSCA) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
