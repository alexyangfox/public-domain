
                     -----------------------
                     ATADRVR README.TXT FILE
                     -----------------------

ATADRVR.ZIP contains the ATA/ATAPI low level driver C source
code, two example C programs to demonstrate how to use the driver
and a User Guide document (ATADRVR.HTM).

This code, created by Hale Landis, has been placed into the
public domain by Hale Landis.  There is no copyright and there
are no restrictions on the use of this ATA Low Level I/O Driver
code.  It is distributed to help other programmers understand how
the ATA device interface works and it is distributed without any
warranty.  Use this code at your own risk.

If you have used a previous version of ATADRVR, read the
HISTORY.TXT file first.

If you are a new user of ATADRVR, read ATADRVR.HTM and review the
EXAMPLE1.C and EXAMPLE2.C source code files.

Compile with one of the Borland C or C++ compilers.  See the make
files EXAMPLE1.MAK and EXAMPLE2.MAK.

And please read all the User Guide files!


FILE LIST
---------

ATADRVR.HTM  - ATADRVR User Guide (READ THIS!)
ATAIO.H      - ATADRVR's external Header file
ATAIOINT.C   - legacy interrupt mode handler
ATAIOISA.C   - ISA SW and MW DMA
ATAIOPCI.C   - PCI bus DMA (MW and UltraDMA)
ATAIOPIO.C   - lowest level ATA register in/out functions
ATAIOREG.C   - S/W reset, non-data and PIO data transfer
ATAIOSUB.C   - common ATADRVR subroutines
ATAIOTMR.C   - timer subroutines
ATAIOTRC.C   - register access and command history tracing
DMA.HTM      - the DMA User Guide
EXAMPLE1.C   - example program for ATA (hard disk) commands
EXAMPLE1.MAK - make file
EXAMPLE2.C   - example program for ATAPI (CD/DVD/etc) commands
EXAMPLE2.MAK - make file
HISTORY.TXT  - change log
INITPCC.EXE  - program to setup PCMCIA PC Card ATA modes
INITPCC.HTM  - User Guide for INITPCC
PCC.HTM      - PC Card ATA User Guide
PIO.HTM      - PIO User Guide
README.TXT   - this file
SATA.HTM     - Serial ATA (SATA) User Guide


QUESTIONS OR PROBLEMS?
----------------------

Send your question(s) or problem description(s) to:

   info@ata-atapi.com

Web site:

   http://www.ata-atapi.com

/end/
