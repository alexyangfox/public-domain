#ifndef GXINC_INCLUDED
#define GXINC_INCLUDED

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <stdlib.h>
#ifdef LIBGX_DEBUG_BUILD
#include <iostream>
#endif //LIBGX_DEBUG_BUILD
#include <string>

typedef unsigned int UINT;
typedef unsigned long int ULINT;
typedef long int LINT;

const unsigned GX_SHORT_LABEL_LEN = 128;
const unsigned GX_DEFAULT_LABEL_LEN = 256;
const unsigned GX_LONG_LABEL_LEN = 512;

//this is implemented in GxWinArea.cc
//a pretty bad palce to implement this this. this will _always_ leave pDest null terminated.
//destLen is the _total_ space in pDest including the space for a null character.
//rNewLength is set to the number of characers in pDest, not including the null termination. 
void GxSetLabel(char *pDest, unsigned destLen, const char *pContents,
		unsigned &rNewLength);

//I want the handlerID to be an opaque type
typedef void* GxEventHandlerID;
const GxEventHandlerID NULL_EVENT_HANDLER_ID = 0;

enum GX_H_STACKING{GX_STACK_LEFT, GX_STACK_RIGHT, GX_STACK_H_CEN};
enum GX_V_STACKING{GX_STACK_TOP, GX_STACK_BOTTOM, GX_STACK_V_CEN};

enum GX_ATTACHMENT{UPPER_LEFT, UPPER_RIGHT, LOWER_LEFT, LOWER_RIGHT,
		   MIDDLE_LEFT, MIDDLE_TOP, MIDDLE_RIGHT, MIDDLE_BOTTOM};

enum GX_DIRECTION{GX_UP, GX_DOWN, GX_LEFT, GX_RIGHT};
enum GX_H_PLACEMENT{GX_H_FIXED, GX_H_CENTERED, GX_FLOW_LEFT, GX_FLOW_RIGHT};
enum GX_V_PLACEMENT{GX_V_FIXED, GX_V_CENTERED, GX_FLOW_UP, GX_FLOW_DOWN};
//GX_WD_STAT == width status
//GX_WD_INT == width set internally
enum GX_WD_STAT{GX_WD_FIXED, GX_WD_FILL, GX_WD_INT};
//GX_HT_STAT == height status
//GX_H_INT == height set internally
enum GX_HT_STAT{GX_HT_FIXED, GX_HT_FILL, GX_HT_INT};

enum GX_STATUS{GX_OK, GX_CANCELED, GX_YES, GX_NO};

#endif //GXINC_INCLUDED





