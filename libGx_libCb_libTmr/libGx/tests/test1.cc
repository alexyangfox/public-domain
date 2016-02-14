//test 1 of libGx
#include <iostream>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxMainWin.hh>

int main(int argc, char **argv)
{
  GxMainInterface mainInt("Test1");
  if( !mainInt.Initialize(argc, argv) ) return -1;
  if( !mainInt.OpenAllocateAll() ) return -2;

  GxMainWin main( mainInt.dispVector[0] );
  main.Resize(250,250);

  main.Place();
  main.Create();
  main.Display();

  /*
  XSetCommand(disp.XDisp(), main.GetWindow(), argv, argc);

  int numArgs;
  char **pArgs;
  if( XGetCommand(disp.XDisp(), main.GetWindow(), &pArgs, &numArgs) )
    {
      std::cout << "got args" << std::endl;
      for(int i = 0; i < numArgs; i++)
	{
	  std::cout << pArgs[i] << std::endl;
	};
      XFreeStringList(pArgs);
    }else
      std::cout << "failed to get args" << std::endl;
  */

  mainInt.EventLoop(); //this currently never ends

  //everything will be distructed when they go out of scope
  return 0;
};

