#include <assert.h>

#include <libCb/CbCallback.hh>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxGeomControl.hh>
#include <libGx/GxToolBarButton.hh>
#include <libGx/GxToolBar.hh>
#include <libGx/GxToolBarManager.hh>
#include <libGx/GxToolBarDock.hh>
#include <libGx/GxMenuBar.hh>
#include <libGx/GxMenu.hh>
#include <libGx/GxMenuItems.hh>

GxMainInterface *pMainInt = NULL;

void Exit(void)
{
  pMainInt->EndEventLoop();
}

GxToolBarManager *pTBManager = 0;
GxToolBar *pBarOne = 0, *pBarTwo = 0, *pBarThree = 0, *pBarFour = 0;

void Tool1CB(bool show)
{
  pTBManager->DisplayToolBar(show, pBarOne);
}

void Tool2CB(bool show)
{
  pTBManager->DisplayToolBar(show, pBarTwo);
}

void Tool3CB(bool show)
{
  pTBManager->DisplayToolBar(show, pBarThree);
}

void Tool4CB(bool show)
{
  pTBManager->DisplayToolBar(show, pBarFour);
}

#include "xpm1.xpm"
#include "xpm2.xpm"
#include "xpm3.xpm"
#include "xpm4.xpm"

int main(int argc, char **argv)
{
  GxMainInterface mainInt("Test6");
  pMainInt = &mainInt;

  if( !mainInt.Initialize(argc, argv) )
    {
      std::cerr << "failure to open Xdisplay and/or allocate critical resources. exiting..." << std::endl;
      return 1;
    };
  if( !mainInt.OpenAllocateAll() ) return -2;

  GxToolBarManager tbManager( mainInt.dispVector[0] );
  pTBManager = &tbManager;

  GxToolBar barOne(&tbManager), barTwo(&tbManager), barThree(&tbManager),
    barFour(&tbManager);
  GxToolBarButton bOne(&barOne, xpm1_xpm, "b one");
  GxToolBarButton bTwo(&barOne, xpm1_xpm, "b two");
  GxToolBarButton bThree(&barOne, xpm1_xpm, "b three");
  GxToolBarButton bFour(&barOne, xpm1_xpm, "b four");
  GxToolBarButton bFive(&barOne, xpm1_xpm, "b five");

  GxToolBarButton bSix(&barTwo, xpm2_xpm);
  GxToolBarButton bSeven(&barTwo, xpm2_xpm);
  GxToolBarButton bEight(&barTwo, xpm2_xpm);
  GxToolBarButton bNine(&barTwo, xpm2_xpm);
  GxToolBarButton bTen(&barTwo, xpm2_xpm);

  GxToolBarButton bEleven(&barThree, xpm3_xpm);
  GxToolBarButton bTwelve(&barThree, xpm3_xpm);
  GxToolBarButton bThirteen(&barThree, xpm3_xpm);

  GxToolBarButton bFourteen(&barFour, xpm4_xpm);
  GxToolBarButton bFiveteen(&barFour, xpm4_xpm);

  pBarOne = &barOne;
  pBarTwo = &barTwo;
  pBarThree = &barThree;
  pBarFour = &barFour;

  barOne.SetDesDock(1);
  barOne.SetDesDockPlace(1,1);

  barTwo.SetDesDock(1);
  barTwo.SetDesDockPlace(1,2);

  GxMainWin mainW( mainInt.dispVector[0] );
  mainW.Resize(250,250);
  mainW.SetGeomControl( GxBasic(GX_WD_FIXED, GX_HT_FIXED, GX_H_FIXED,
			       GX_V_FIXED) );

  GxMenuBar mBar(&mainW);
  GxMenu file(&mBar, "File");
  GxMenuOption exit(&file, "Exit");
  exit.cb.Assign( CbVoidPlain(Exit) );

  GxMenu tools(&mBar, "Tools");
  GxMenuCheckOption tool1(&tools, "tool1");
  tool1.cb.Assign( CbOnePlain<bool>( Tool1CB ) );
  GxMenuCheckOption tool2(&tools, "tool2");
  tool2.cb.Assign( CbOnePlain<bool>( Tool2CB ) );
  GxMenuCheckOption tool3(&tools, "tool3");
  tool3.cb.Assign( CbOnePlain<bool>( Tool3CB ) );
  GxMenuCheckOption tool4(&tools, "tool4");
  tool4.cb.Assign( CbOnePlain<bool>( Tool4CB ) );

  //connect the events from the toolbar DisplayFunction
  barOne.displayChangeCB.Assign( CbOneMember<bool, GxMenuCheckOption>(&tool1, &GxMenuCheckOption::Checked) );
  barTwo.displayChangeCB.Assign( CbOneMember<bool, GxMenuCheckOption>(&tool2, &GxMenuCheckOption::Checked) );
  barThree.displayChangeCB.Assign( CbOneMember<bool, GxMenuCheckOption>(&tool3, &GxMenuCheckOption::Checked) );
  barFour.displayChangeCB.Assign( CbOneMember<bool, GxMenuCheckOption>(&tool4, &GxMenuCheckOption::Checked) );

  GxToolBarDock leftDock(&mainW, tbManager, 1), rightDock(&mainW, tbManager, 2),
    topDock(&mainW, tbManager, 3), bottomDock(&mainW, tbManager, 4);

  tbManager.OrganizeAndPlaceToolBars();

  leftDock.SetVertical(true);
  leftDock.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_FILL, GX_FLOW_LEFT,
				   GX_FLOW_UP) );

  rightDock.SetVertical(true);
  rightDock.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_FILL, GX_FLOW_RIGHT,
				   GX_FLOW_UP) );

  topDock.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
				 GX_FLOW_UP) );

  bottomDock.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
				    GX_FLOW_DOWN) );

  mainW.Place();
  mainW.Create();
  //so the toolbars act as a group
  tbManager.SetGroupWindow( mainW.GetClosestXWin() );
  mainW.Display();

  mainInt.EventLoop();

  return 0;
};
