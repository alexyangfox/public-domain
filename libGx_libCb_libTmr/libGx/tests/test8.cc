#include <iostream>
#include <assert.h>

#include <libCb/CbCallback.hh>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxInc.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxTextWin.hh>
#include <libGx/GxButton.hh>
#include <libGx/GxGeomControl.hh>
#include <libGx/GxVDivider.hh>

using namespace std;

GxMainInterface *pMainInt = NULL;
GxMainWin *pMainWin = NULL;
GxTextWin *pTextWin = NULL;

void Quit(void)
{
  pMainInt->EndEventLoop();
}

void ResizeCB(int pos)
{
  int width = pTextWin->Width();
  width += pos;
  if( width > 350 )
    width = 350;
  else
    if( width < 100 )
      width = 100;

  pTextWin->SetDesiredWidth(width);
  pMainWin->PlaceChildren();

  cout << "pos: " << pos << "  new width: " << width << endl;
}

int main(int argc, char ** argv)
{
  GxMainInterface mainInt("test4");

  assert( mainInt.Initialize(argc, argv) );
  assert( mainInt.OpenAllocateAll() );

  GxMainWin mainWin(mainInt.dispVector[0] );
  mainWin.Resize(575, 250);
  mainWin.SetGeomControl( GxBasic(GX_WD_FIXED, GX_HT_FIXED, GX_H_FIXED, GX_V_FIXED) );

  GxButton quitButton(&mainWin, "Quit");
  quitButton.cb.Assign( CbVoidPlain(Quit) );
  quitButton.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED, GX_FLOW_DOWN,
				       1,1,1,1, false, true) );

  GxTextWin tw1( &mainWin);
  tw1.Width(150);
  tw1.Editable(true);

  GxVDivider divider( &mainWin );
  GxTextWin tw2( &mainWin);
  tw2.Editable(true);

  tw1.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );
  divider.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );
  tw2.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );

  divider.cb.Assign( CbOnePlain<int>(ResizeCB) );

  pMainInt = &mainInt;
  pMainWin = &mainWin;
  pTextWin = &tw1;

  mainWin.Place();
  mainWin.Create();
  mainWin.Display();

  mainInt.EventLoop();

  return 0;
};
