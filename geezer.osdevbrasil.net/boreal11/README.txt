Borealis (formerly Cosmos): a small open source operating system
        Release 11
Chris Giese     <geezer@execpc.com>
        http://my.execpc.com/~geezer/os/index.htm#borealis
Release date: Nov 4, 2004
This code is public domain (no copyright).
You can do whatever you want with it.

================================================================
Quick start
================================================================
1. You need a PC with a 32-bit CPU (386SX or better), VGA video,
   and at least 2 MB of RAM.

2. You need GCC; any of:
   - GCC for x86 Linux
   - GCC for DOS (DJGPP -- see below)
   - GCC for Windows (MinGW or maybe CygWin -- see below)

3. To create a bootable GRUB floppy disk, DOS and Windows users
   also need John Fine's Partial Copy (PCOPY; PARTCOPY):
        http://my.execpc.com/~geezer/johnfine/index.htm#partcopy

4. Make a bootable GRUB floppy, if you don't already have one.
   Directions for this are given below. Put the GRUB floppy in
   drive A: (DOS or Windows) or mount it on the ./mnt/
   subdirectory (Linux).

5. Copy the top-level makefiles appropriate for your host OS
   and compiler:
    DJGPP:              copy /y djgpp1.mk common1.mk
			copy /y djgpp2.mk common2.mk

    Linux GCC:          cp   -f linux1.mk common1.mk
			cp   -f linux2.mk common2.mk

    MinGW:              copy /y mingw1.mk common1.mk
			copy /y mingw2.mk common2.mk

6. From the top-level directory, type
	make install

   If all goes well, the following files will be created:
    krnl/krnl.x         Kernel executable
    apps/*.r            Loadable kernel modules
    apps/*.x            Application executables                         */
   and copied to the root directory of the floppy disk.

   If the build process does not work, see 'Troubleshooting'
   below.

7. Boot the PC from the bootable GRUB floppy.

8. You can also use MBLOAD to boot the kernel from the FreeDOS
   or MS-DOS command line by typing:
        boot.bat

================================================================
New features vs. Cosmos release 10
================================================================
    - = done,
    . = not done
- New name: Borealis
- Code is now public-domain (no copyright)
- Changed global variable naming convention
- Identical names for syscall dispatchers in libc and syscall
  handlers in kernel. Programs can be built as stand-alone tasks
  or as kernel modules.
- kprintf(), panic(), printf(), and sprintf() now use GCC
  __attribute__((format(printf, ...))) to validate args
- Fixed I/O permission bitmap base in TSS (was 103, now 104)
- Realtime clock code now works with BCD or binary modes, and
  uses interrupt instead of polling to wait for end of update
- Interrupts:
  - sleep_on() now uses assembly-language stack-swap code to
    yield; instead of stupid fake timer interrupt
  - Now enabling interrupts in the kernel during syscall processing
  - Syscall interrupt changed from INT 30h to INT 20h, and IRQ
    0-15 now mapped to INT 28h-37h for future APIC compatability
- Made code build easily with GCC under different host OSes
  (DOS, Windows, Linux):
    - Dummy __main() in libc for MinGW. (Got rid of dummy
      _alloca() by compiling with -mno-stack-arg-probe)
    - Switched to improved OSD linker script
    - Makefiles rewritten for host OS portability
    - NASM code converted to AS
    - ELF, DJGPP COFF, and Win32 PE COFF loaders for executable
      files (tasks) and relocatable files (kernel modules)
- Got rid of:
    - init_keyboard()
    - sleep() syscall (use select() instead)
    . Over-complex paging setup code in KSTART.S
- Enabling CPU caches in CR0 register in kernel startup code
  (big speedup! thanks Rob!)
- More validation of section addresses when loading user tasks
- dump_stack_frames() improved:
    . Probes for valid memory mapping before chaining to next frame
    - Stops when "magic value" detected in EBP or after 10 frames
    - Detects recursive calls
- Allocating kernel objects dynamically, from kernel heap
  instead of fixed-size arrays:
    - Virtual consoles (only 12 VCs can be displayed)
    - Keyboard buffers
    . Threads
    - Memory sections in task_t structure
- Converted to Multiboot:
    - Getting conventional and extended memory sizes via Multiboot
    - Loading tasks/kernel modules via Multiboot
    . Rewrote paging code for compatability with Multiboot
- Changes to paging code:
    - User apps now at 4 meg (0x400000) instead of 1 gig (0x40000000)
    . Paging code now supports variable amount of RAM, not just 16 meg
- UNIX-like device architecture
    - open() syscall; scans internal namespace for device names
    - read(), write(), select(), and close() work with char devices
    - Console character device
    - NULL/ZERO character device
    - Serial character device
    . Realtime clock character device
- Console supports more ANSI escapes: ESC[A ESC[B ESC[C ESC[D
  ESC[s ESC[u ESC[J ESC[K

================================================================
Old features
================================================================
- Runs on 32-bit x86 CPU. Can run on old/slow system,
  e.g. 486SX with 4 meg RAM
. Paging:
  . Page-based address translation, memory allocation, and
    memory protection
  . Demand-allocation and loading of task memory. Still no block
    devices or filesystems; the "load" is actually a memcpy()
  . Discardable kernel memory. Kernel memory (code and data) that
    is used only during kernel initialization is freed after use.
  . Page directory mapped into itself to simplify paging code
- Protection
  - Tasks run at ring 3 in their own address spaces
  - Separate kernel- and user-privilege stacks for each task.
  - Task can not perform I/O
  - Tasks that perform an illegal operation are killed
    (the kernel keeps running)
- Kernel threads with priority levels: lower for idle thread,
  higher for bottom-half interrupt handlers. Preemptive
  multitasking (driven by timer interrupt). Round-robin
  scheduling within same priority level.
- Keyboard driver with bottom-half interrupt handler thread.
  Ctrl+Alt+Del reboots
- Text-mode VGA virtual consoles backed with kernel memory,
  with software scrolling and a subset of ANSI escapes
- Realtime clock driver, time() syscall, unique JDN algorithm
- Tinylib; a small C library (libc)
- Make process creates disassembly and symbol table files
- Simple text-mode apps: clock, text echo, protection test, games
- Unified IRQ/exception/fault/interrupt handler
- Kernel thread to recover memory and resources used by dead tasks

================================================================
DJGPP
================================================================
DJGPP is the GNU C compiler (GCC) modified to run under DOS and
produce 32-bit (DPMI) DOS executables. DJGPP can be run from
MS-DOS or FreeDOS. It also runs in a Windows 95, Windows 98, or
Windows ME DOS box; and it works better than CygWin for these
operating systems. Because of serious bugs in NTVDM, DJGPP does
not work well under Windows NT, Windows 2000, or Windows XP --
use either MinGW or CygWin for these OSes.

To obtain DJGPP, visit the DJGPP home page:
        http://www.delorie.com/djgpp

Either use the "zip picker" to download DJGPP along with 'make'
and 'sed', or chose a mirror site near you and download these
packages:
        v2gnu/gccNNNNb.zip  (C compiler)
        v2gnu/bnuNNNNb.zip  (binutils)
        v2/djdevNNN.zip     (library files for making DOS programs
                            and misc. utilities including REDIR.EXE)
        v2/faqNNNb.zip      (DJGPP FAQ)
        v2gnu/makNNNNb.zip  (Make)
        v2gnu/sedNNb.zip    (sed)
        v2misc/csdpmiNb.zip (DPMI server)

================================================================
MinGW
================================================================
MinGW is the GNU C compiler (GCC) modified to run under Windows
and produce Win32 executables. It works better than DJGPP under
Windows NT, Windows 2000, and Windows XP.

MinGW is not as robust as DJGPP, however. Using it for anything
other than creating Windows applications is fraught with peril.
I have found more bugs in this toolchain than in any other
version of GCC I've used.

MinGW is distributed as a gigantic, self-extracting .EXE file
(an unfortunate choice of distribution method, in my opinion).
This file can be downloaded from the MinGW home page:
        http://mingw.sourceforge.net

'make' and 'sed' are in the separate MSYS package. MSYS contains
two versions of 'make'; one of which works and one of which
crashes. I don't remember which one is which. If you have
trouble with 'make' or 'sed' or any of the other MSYS utilities,
you may want to download these tools instead:
        http://UnxUtils.sourceforge.net/

If all else fails, CygWin may work better than MinGW:
        http://www.cygwin.com

NOTE: Do not send me e-mail about any difficulty you have
building this code with MinGW (because of it's bugs) or CygWin
(because I haven't used CygWin).

================================================================
Making a bootable floppy disk with the GRUB bootloader on it
================================================================
1. You will need
   - Two 1.44 meg floppy disks, one of them formatted with a
     filesystem that GRUB recognizes such as FAT12 or ext2.
     The other floppy (the "unformatted" floppy) may contain
     a filesystem, but it will be destroyed.
   - The GRUB binaries: files 'stage1' and 'stage2'. These
     should be in the subdirectory ./boot/grub
   - A 'menu.lst' configuration file for GRUB; also in the
     ./boot/grub subdirectory.

2. On the formatted floppy disk, create the subdirectory
   /boot/grub, and copy the files stage1, stage2, and
   menu.lst into this subdirectory.

3. Concatenate the binary files stage1 and stage2 into a
   single binary file named 'boot':
	(DOS/Windows):  copy /b stage1 + stage2 boot
	(Linux):        cat stage1 stage2 >boot

4. Write the file 'boot' directly to the unformatted floppy.
   This is a sector-level write that does not use (and will
   destroy) any filesystem present on the disk:
	(DOS/Windows):  partcopy boot 0 168000 -f0
	(Linux):        cat boot >/dev/fd0

   PARTCOPY will complain because 'boot' is much shorter than
   0x168000 bytes, but this is OK.

5. Boot your PC from the unformatted floppy disk.

6. After GRUB has started, eject the unformatted floppy and
   insert the formatted floppy, containing the stage1,
   stage2, and menu.lst files, all in the /boot/grub
   subdirectory. Type:
	setup (fd0)

7. The formatted floppy is now bootable. Do not move, modify,
   delete, or defrag the file /boot/grub/stage2 on this floppy.

================================================================
Troubleshooting
================================================================
If the build process fails early under Linux, it's likely that
the Linux toolchain (GCC or Make or both) is confused by DOS
newlines. You will need to run 'fromdos' or a similar utility
on all text files before building. Check if your unzip program
has a command-line option to convert DOS newlines to UNIX
newlines when it unzips the files. The .zip file contains some
binary files -- avoid running 'fromdos' on these. (The binary
files should be read-only.)

Error message that may appear when you first type 'make':
    makefile:1: ../common1.mk: No such file or directory (ENOENT)
    makefile:25: ../common2.mk: No such file or directory (ENOENT)

You did not copy the appropriate makefiles in the top level
directory, as shown here:

    DJGPP:              copy /y djgpp1.mk common1.mk
			copy /y djgpp2.mk common2.mk

    Linux GCC:          cp   -f linux1.mk common1.mk
			cp   -f linux2.mk common2.mk

    MinGW:              copy /y mingw1.mk common1.mk
			copy /y mingw2.mk common2.mk

Error message that may appear during linking:
    BFD: Dwarf Error: Abbrev offset (3221275176) greater
    than or equal to .debug_abbrev size (7231).

This is a bug in new versions of DJGPP. You can ignore it.

Error message that may appear when you type 'make clean':
    dir /s /b *.d | sed -e s/"^"/"del "/g >delobj.bat
    c:/mingw/bin/sh: dir: command not found

MinGW Make is using 'sh' as the shell to process built-in
commands. The Makefiles were written to use either COMMAND.COM
or CMD.EXE (they were actually tested with WIN95CMD.EXE).
Unfortunately, if MinGW Make finds 'sh' on the path, it will
use 'sh' as the shell, regardless of the value of the SHELL or
COMSPEC environment variables. Workaround: ensure that 'sh'
is not present on the path.

Error message that may appear when you type 'make clean':
    dir /s /b *.d | sed -e s/"^"/"del "/g >delobj.bat
    Bad command or file name

This means that Sed can not be found. If you are using DJGPP,
download Sed from a DJGPP mirror site (it's in the v2gnu
subdirectory). If you are using MinGW, download and install
the MSYS package. If you are using Linux, Sed is in the
GNU fileutils package.

Error message that may appear when you type 'make clean':
    delobj.bat
    make (e=2): The system cannot find the file specified.
    C:\MINGW\BIN\MAKE.EXE: [rclean] Error 2 (ignored)

You can ignore this message. It appears when Win32 tries to run
a zero-length batch file. In the case shown here, it appears
when you type 'make clean' in the ./lib/ subdirectory and that
directory is already 'clean'.

================================================================
Bugs in software tools that you may encounter
================================================================
MinGW: 'make' may attempt to use 'sh' as the shell regardless of
whether or not it's present, or the values of the COMPSEC or
SHELL environment variables. Make sure 'sh' is not present on
the path.

MinGW: Strip will change the file alignment of the executable,
which will make the kernel unbootable. If you are using MinGW,
do not strip the kernel executable.

MinGW: "ld -r -d ..." does not work for the version of MinGW
based on GCC 2.95 (it does not allocate space in the BSS for all
common variables). If the kernel says
        lookup: undefined external symbol 'g_seed'
when it tries to load tetris.r, you are seeing this bug.

MinGW: The following linker command-line:
	ld -Ttext=0x400000 ...
will put section .text at virtual address 0, then put section
.data above 0x400000. To give .text the proper starting address
of 0x400000, you must set the image base as well:
	ld -Ttext=0x400000 --image-base 0 ...

================================================================
Future features
================================================================
- SPEED:
  - Port STRING.H library functions to asm
    (xxx - __inline__ functions containing inline asm trash
     callee-save registers without saving them -- what am
     I doing wrong?)
  - Don't route all interrupts through a common handler
  - Find other areas in the kernel where interrupts can be enabled
- ANSI/VT escapes to reduce effective screen size
- Alternate keyboard drivers for non-US keyboards
- FAT12/16/32, ext2, and ISO-9660 (CD-ROM) filesystems, and VFS
  (everything read-only, at first)
	- Support multiple mount points
	- Read 2nd FAT if error reading 1st FAT
	- Add support for current dir and relative path names
	- Add #defines for later support of UNIX UID and GID
	- Take out #defines for using malloc()
	- Move code that "limit[s] count per file size and pos"
	  from EXT2.C and FAT.C to VFS.C
	- Add preliminary support for FAT32 to FAT.C
- Block devices: RAM disk driver, PC floppy driver, ATA hard
  disk driver, ATAPI CD-ROM driver, disk cache, request queues
	- Got cache code in DTOOLS project, but it needs tweaking
	- (Re)Write ATA driver
        - Write RAM disk driver (simple, I hope...)
- Command-line shell
- Mixed text- and graphics-mode virtual consoles
- Graphics-mode apps (e.g. picture file viewer)
- Serial and PS/2 mouse drivers
- Sound drivers for WSS and SoundBlaster
- Console beep on Ctrl+G, and console scrollback (like Linux)
- Installable interrupt handlers
- Floating point support, floating point emulation?
- PnP/PCI, with arbitration and tracking of resources (IRQ, I/O, DMA, mem)
  and loading of native or compatible drivers based on ID
- DLLs and dynamic linking
- V86 mode monitor
- Apps that use graphics, mouse, and sound (e.g. DOOM)


GUI, IPC, TCP/IP, Ethernet, PPP, PCI DMA IDE, VFAT long filenames
and other filesystems, security, speed, LPT port and limited
printing, user debugger, kernel debugger, ...

