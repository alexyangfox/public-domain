#ifndef GXIMAGEOBJECT_INCLUDE
#define GXIMAGEOBJECT_INCLUDE

#include <libGx/GxInc.hh>
#include <libGx/GxDisplayInfo.hh>
#include <libGx/GxVolatileData.hh>

//major hack; image size is hardcoded

//hack; image masks are unimplemented
//hackish? we never make a local copy of the bitmap data. we take a pointer
//to it but don't actually use it untill we are created.  This means that
//the data must be ?static? (global in scope, not local to a function)
//not making or requiring the reserving of the memory for a copy is certainly
//faster and under most cases is valid

// ***********************
// DeleteImage must be called to actually remove the image from the server
// ***********************

class GxImageObject
{
public:
  GxImageObject(void);
  virtual ~GxImageObject(void);

  //to keep the namespace clean
  UINT ImageWidth(void) const;
  UINT ImageHeight(void) const;

  //why can't we have const's here
  // **** the actual image is not changed untill Create() is called *****
  //used for included Bitmap images
  void SetImage(char *pImage);
  //used for included Pixmap images
  void SetImage(char **pImage);

  bool ImageCreated(void);

  // the GxImageObject assumes control of the dImage and will delete it from
  // the X Server.  The Pixmaps must be of the correct depth & visual.
  // this function makes no checks for validity.
  // If you have an image without a shape mask, set dImage to None;
  void SetImage(Pixmap dImage, Pixmap dMask, UINT iWidth, UINT iHeight);

  //we need a valid window for the visual information
  void CreateImage(const GxDisplayInfo &rInfo, Window iWin);
  void DeleteImage(const GxDisplayInfo &rInfo);

  //it is perfictally ok to redraw the same GxImageObject over multiple windows
  //other than the one it was created with as long as the visual is the same
  //if it is not the same, just call CreateImage again with the new window

  //this automatically handles the presance of a imageMask. if not for
  //this, would be too much of a convience function for libGx
  void DrawImage(const GxDisplayInfo &rInfo, const GxVolatileData &rData,
		 Window iWin, int x, int y) const;

protected:
  Pixmap image;
  Pixmap imageMask; //may or may not be used is set to None if not used
  char *pBitmapData;
  char **pPixmapData;

  UINT imageWidth, imageHeight;
};

#endif //GXIMAGEOBJECT_INCLUDE
