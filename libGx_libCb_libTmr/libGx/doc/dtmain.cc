#include <iostream>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxMainWin.hh>

#include "MyDialog.hh"
using namespace std;

GxMainWin *pMainWin = 0;

void Dialog(void)
{
  cout << "in dialog" << endl;
  MyDialog mDialog(pMainWin);
  mDialog.DoDialog();
};


int main(int argc, char **argv)
{

  GxMainInterface mainInt("Test2", 0);
  if( !mainInt.Initialize(argc, argv) )
    {
      std::cerr << "failure to open Xdisplay and/or allocate critical resources. exiting..." << std::endl;
      return 1;
    };
  if( !mainInt.OpenAllocateAll() ) return -2;

  //pInt = &mainInt;

  GxMainWin mainW( mainInt.dispVector[0] );
  pMainWin = &mainW;

  GxButton button(&mainW, "Do The Dialog");
  button.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED, GX_V_CENTERED, 2,2,2,2));
  button.cb.Assign( CbVoidPlain(Dialog) );

  mainW.Place();
  mainW.Create();
  mainW.Display();

  mainInt.EventLoop();

  return 0;
};
