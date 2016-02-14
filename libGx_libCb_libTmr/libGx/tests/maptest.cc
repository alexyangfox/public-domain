//this program tests out the implementation of the GxWinMap class
#include <iostream>

#include <libGx/GxInc.hh>
#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxCoreWin.hh>
#include <libGx/GxArguments.hh>
#include <libGx/GxCoreWinMap.hh>

//a hack to test the win maps
//just be sure not to create these windows or other nonsence
class testWin : public GxCoreWin
{
public:
  testWin(GxOwner *pOwn, UINT winID) :
    GxCoreWin(pOwn) 
  {
    xWin = winID;
  };
  ~testWin(void){};

};

int main(void)
{
  GxMainInterface mainInt("maptest");
  GxArguments junk;

  GxDisplay *pDisplay = new GxDisplay(mainInt, junk);
  GxCoreWinMap winMap;

  for(UINT i = 1; i < 251; i++)
    {
      winMap.ManageWin((new testWin(pDisplay,i)), i);
    };

  for(UINT i = 1; i < 251; i++)
    {
      //just be very sure not to de-reference these "pointers" !!!
      GxCoreWin *pWin = winMap.GetWin(i);
      if(!pWin)
	std::cout << "lookup returned null. i = " << i << std::endl;
    };

//   winMap.ManageWin((new testWin(pDisplay,139)), 139);
//   GxCoreWin *pWin = winMap.GetWin(139);
//   if(!pWin)
//     cout << "139 lookup returned null" << endl;

  return 0;
};
