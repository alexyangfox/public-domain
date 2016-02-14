
#include <libCb/CbCallback.hh>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxInc.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxEditWin.hh>
#include <libGx/GxButton.hh>
#include <libGx/GxStateButton.hh>
#include <libGx/GxScrollBar.hh>
#include <libGx/GxSlider.hh>
#include <libGx/GxGeomControl.hh>
#include <libGx/GxFilledArrowButton.hh>

GxMainInterface *pMainInt = NULL;

void Quit(void)
{
  pMainInt->EndEventLoop();
}

int main(int argc, char ** argv)
{
  GxMainInterface mainInt("test4");
  pMainInt = &mainInt;

  assert( mainInt.Initialize(argc, argv) );
  assert( mainInt.OpenAllocateAll() );

  GxMainWin *pMain = new GxMainWin( mainInt.dispVector[0] );
  pMain->Resize(575, 250);
  pMain->SetGeomControl( GxBasic(GX_WD_FIXED, GX_HT_FIXED, GX_H_FIXED,
				 GX_V_FIXED) );

  GxScrollBar *pScrollBar = new GxVScrollBar(pMain);
  pScrollBar->Move(10,10);
  pScrollBar->Height(230);
  pScrollBar->SetGeomControl( GxFixed() );

  GxSlider *pSlider = new GxVSlider(pMain);
  pSlider->Move(35,10);
  pSlider->Height(230);
  pSlider->SetGeomControl( GxFixed() );

  pSlider = new GxVSlider(pMain);
  pSlider->Move(60,10);
  pSlider->Height(230);
  pSlider->Active(false);
  pSlider->SetGeomControl( GxFixed() );

  pScrollBar = new GxHScrollBar(pMain);
  pScrollBar->Move(85, 100);
  pScrollBar->Width(230);
  pScrollBar->SetGeomControl( GxFixed() );

  pSlider = new GxHSlider(pMain);
  pSlider->Move(85, 125);
  pSlider->Width(230);
  pSlider->SetGeomControl( GxFixed() );

  pSlider = new GxHSlider(pMain);
  pSlider->Move(85, 150);
  pSlider->Width(230);
  pSlider->Active(false);
  pSlider->SetGeomControl( GxFixed() );

  GxEditWin *pEWin = new GxEditWin(pMain);
  pEWin->Width(230);
  pEWin->Move(330,65);
  pEWin->SetGeomControl( GxFixed() );

  pEWin = new GxEditWin(pMain);
  pEWin->Width(230);
  pEWin->Move(330,135);
  pEWin->SetGeomControl( GxFixed() );

  GxStateButton *pSButton = new GxStateButton(pMain);
  pSButton->Resize(75,20);
  pSButton->Move(100,220);
  pSButton->SetGeomControl( GxFixed() );

  GxButton *pButton = new GxButton(pMain);
  pButton->SetLabel("Quit");
  pButton->Resize(75,20);
  pButton->Move(200,220);
  pButton->cb.Assign( CbVoidPlain(Quit) );
  pButton->SetGeomControl( GxFixed() );

  GxFilledArrowButton *pFAButton = new GxFilledArrowButton(pMain);
  pFAButton->SetDirection(GX_UP);
  pFAButton->Resize(20,20);
  pFAButton->Move(400,220);
  pFAButton->SetGeomControl( GxFixed() );

  pFAButton = new GxFilledArrowButton(pMain);
  pFAButton->SetDirection(GX_DOWN);
  pFAButton->Resize(20,20);
  pFAButton->Move(420,220);
  pFAButton->SetGeomControl( GxFixed() );

  pFAButton = new GxFilledArrowButton(pMain);
  pFAButton->SetDirection(GX_LEFT);
  pFAButton->Resize(20,20);
  pFAButton->Move(440,220);
  pFAButton->SetGeomControl( GxFixed() );

  pFAButton = new GxFilledArrowButton(pMain);
  pFAButton->SetDirection(GX_RIGHT);
  pFAButton->Resize(20,20);
  pFAButton->Move(460,220);
  pFAButton->SetGeomControl( GxFixed() );

  pMain->Place();
  pMain->Create();
  pMain->Display();

  mainInt.EventLoop();

  return 0;
};
