#ifndef GXVOLATILEDATA_INCLUDED
#define GXVOLATILEDATA_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxDisplayInfo.hh>

//this is data used internally by many of the functions which draw decorations
//every thread must have it's own copy of this data
//effectivly this is owned by the GxDisplay
class GxVolatileData
{
public:
  GxVolatileData(void);
  ~GxVolatileData(void);

  void VDAllocAll(GxDisplayInfo &rInfo);
  //we cannot use the distructor because we need a display info.
  void VDFreeAll(GxDisplayInfo &rInfo);

  //gc used in drawing menus. the font is always set to the menu font.
  //every function should leave the color as the default text color.
  GC menuGC;

  //gc used internally by the Draw3dBorder function
  GC borderGC;
  //gc used internally for drawing text in menus, buttons, etc.
  GC textGC;
private:
  bool allocated;
};

#endif //GXVOLATILEDATA_INCLUDED
