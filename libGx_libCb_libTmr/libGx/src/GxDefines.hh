#ifndef GXDEFINES_INCLUDED
#define GXDEFINES_INCLUDED

//this file is for library implementation specific defines which cannot
// be seen by client code.
// these values might be able to be read from GxDisplay,
//for clients which want to develop custom widgets which integrate well,
//but I'm not sure.

const UINT GX_SPACE_INC = 8;
//the thickness of the 3d border drawn around objects
const UINT GX_BORDER_WD = 2;
const UINT GX_THIN_BORDER_WD = 1;

//the entire width in pixels of a scrollbar. the slider is GX_SLIDER_WIDTH-2*GX_BORDER_WD
//this is not the originial definition. (not sure if the change is good)
const UINT GX_SLIDER_WIDTH = 17;
//const UINT GX_SCROLLBAR_WIDTH = GX_SLIDER_WIDTH + 2*GX_BORDER_WD;
const UINT GX_DEFAULT_SLIDER_LENGTH = 120; //arbritrary

//the gap between borders of a toolbar
const UINT GX_TOOLBAR_GAP = 4; //the gab between toolbars on a row
const UINT GX_TOOLBAR_ROW_GAP = 2; //the gap between toolbar rows.
//this includes the size of any borders
const UINT GX_TOOLBAR_BUTTON_SIZE = 26;

//maximum length in pixels of a text window line that can be blank
//(used when auto-wraping)
const int TEXT_WIN_MAX_BLANK_LEN = 45;

const UINT GX_PERCENT_BAR_DEFAULT_WIDTH = 150;
const UINT GX_PERCENT_BAR_DEFAULT_HEIGHT = GX_SPACE_INC*3;

//double click time in ms
const int GX_DC_TIME = 350;

#endif //GXDEFINES_INCLUDED
