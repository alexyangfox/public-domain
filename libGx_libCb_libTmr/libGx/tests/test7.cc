#include <libGx/GxMainInterface.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxGeomControl.hh>
#include <libGx/GxColumn.hh>
#include <libGx/GxEditWin.hh>

int main(int argc, char **argv)
{
  GxMainInterface mainInt("Test7");

  if( !mainInt.Initialize(argc, argv) ) return -1;
  if( !mainInt.OpenAllocateAll() ) return -2;

  //first window here
  GxMainWin mainOne( mainInt.dispVector[0] );

  GxColumn cOne(&mainOne);
  cOne.SetAdditionalGap(2);
  cOne.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT,
				 GX_FLOW_UP, 1,1,1,1) );
  GxEditWin eOneOne(&cOne);
  GxEditWin eOneTwo(&cOne);

  mainOne.AddFocusObject(&eOneOne);
  mainOne.AddFocusObject(&eOneTwo);

  //second window starts here

  GxMainWin mainTwo(mainInt.dispVector[0] );
  mainTwo.Resize(250, 250);

  GxColumn cTwo(&mainTwo);
  cTwo.SetAdditionalGap(2);
  cTwo.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT,
				 GX_FLOW_UP, 1,1,1,1) );
  GxEditWin eTwoOne(&cTwo);
  GxEditWin eTwoTwo(&cTwo);

  mainTwo.AddFocusObject(&eTwoOne);
  mainTwo.AddFocusObject(&eTwoTwo);

  mainOne.Place();
  mainOne.Create();
  mainOne.Display();

  mainTwo.Place();
  mainTwo.Create();
  mainTwo.Display();

  std::cout << mainOne.GetWindow() << " , " << mainTwo.GetWindow() << std::endl;
 
  mainInt.EventLoop();
  return 0;
};
