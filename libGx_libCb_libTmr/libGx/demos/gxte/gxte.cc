#include <libGx/GxMainInterface.hh>

#include "GxteGui.hh"

using namespace std;

GxMainInterface *pMainInt = 0;

int main(int argc, char **argv)
{
  GxMainInterface mainInt("gxte");
  pMainInt = &mainInt;

  if( !mainInt.Initialize(argc, argv) ) return -1;
  if( !mainInt.OpenAllocateAll() ) return -2;

  GxteGui gui( mainInt.dispVector[0] );

  gui.Place();
  gui.Create();
  gui.Display();

  mainInt.EventLoop();

  return 0;
};

