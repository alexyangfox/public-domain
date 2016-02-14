#include "MyDialog.hh"

MyDialog::MyDialog(GxTopLevelWin *pOwner) :
    GxPopupWin(pOwner),
    buttonRow(this), ok(&buttonRow,"Ok"), cancel(&buttonRow, "Cancel"),
    hLine(this),
    textLabel(this, "Text is here:"),
    textWin(this)
{
 //set a default size for the top level window (it will be resize-able by the window manager)
 width = 500;
 height = 400;

 //adding this just means the top window will not try to shrink down to the size of its children
 //i.e. it will respect the above values (but again the Window Manager is free to override)
 SetGeomControl( GxFixed() );

 //set the widths and heights of the button row's children to be identical.
 buttonRow.SetWidthIdentical(true);
 buttonRow.SetHeightIdentical(true);

 //the hline widget has a little border on its top and bottom, so we
 //don't need to add one to the button row.
 buttonRow.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_DOWN, 1,1,0,1) );

 //the hline has only one vertical size, so it can have GX_HT_FIXED (or GX_HT_INT)
 hLine.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT, GX_FLOW_DOWN, 1,1,0,0) );

 //let the text label flow to the top left of the window
 //the final false, true forces the remaining parent area to not be changed in X, but to be reset in Y to include
 //the height of this widget. If the desired width specifier would be set to GX_WD_FILL instead of GX_WD_INT, the
 //final true false could be excluded because the placement action that would occur would
 //be the same as what we are forcing to happen

 textLabel.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP, 1,1,1,1, false, true) );

 //the text window does not need a geom control as it will fill the available area left in the window.
 //textWin.SetGeomControl();

 //we can set some callbacks
 ok.cb.Assign( CbVoidMember<MyDialog>(this, &MyDialog::Ok) );
 cancel.cb.Assign( CbVoidMember<MyDialog>(this, &MyDialog::Cancel) );

 Place(); //compute sizes and positions of x11 windows
 Create(); //create all x11 windows (and necessary gc, colors, etc)
};


MyDialog::~MyDialog(void)
{}

bool MyDialog::DoDialog(void)
{
 Display(); //recursively map the dialog's window tree

 EventLoop();
 return retStatus;
}

void MyDialog::Ok(void)
{
 retStatus = true;
 processEvents = false;
}

void MyDialog::Cancel(void)
{
 retStatus = false;
 processEvents = false;
};


