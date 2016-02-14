#include <libGx/GxImageObject.hh>

GxImageObject::GxImageObject(void) :
  image(None), imageMask(None), pBitmapData(NULL), pPixmapData(NULL),
  imageWidth(0), imageHeight(0)
{}

GxImageObject::~GxImageObject(void)
{}

UINT GxImageObject::ImageWidth(void) const
{
  return imageWidth;
}

UINT GxImageObject::ImageHeight(void) const
{
  return imageHeight;
}

void GxImageObject::SetImage(char *pImage)
{
  pPixmapData = NULL;
  pBitmapData = pImage;
}

void GxImageObject::SetImage(char **pImage)
{
  pBitmapData = NULL;
  pPixmapData = pImage;
}

void GxImageObject::SetImage(Pixmap dImage, Pixmap dMask, UINT iWidth,
			     UINT iHeight)
{
  image = dImage;
  imageMask = dMask;
  imageWidth = iWidth;
  imageHeight = iHeight;
}

bool GxImageObject::ImageCreated(void)
{
  //hack; sould take into account whether or not the image data
  //has been updated or not even if the image exists (might not be up-to-date)
  if(image == None)
    return false;
  else
    return true;
}

void GxImageObject::CreateImage(const GxDisplayInfo &dInfo, Window iWin)
{
  if(image != None)
    {
      XFreePixmap(dInfo.display, image);
      image = None;
    };

  if(imageMask != None)
    {
      XFreePixmap(dInfo.display, image);
      imageMask = None;
    };

  if(pPixmapData)
    {
      //create the pixmap
      XpmAttributes xpmAttrib;
      xpmAttrib.valuemask = 0;
      XpmCreatePixmapFromData(dInfo.display, iWin, pPixmapData, &image,
			      &imageMask, &xpmAttrib);
      imageWidth = xpmAttrib.width;
      imageHeight = xpmAttrib.height;

      return;
    };

  if(pBitmapData)
    {
      //hack, hardcoded size;
      image = XCreatePixmapFromBitmapData(dInfo.display, iWin, pBitmapData,
					  16, 16, dInfo.blackPix,
					  dInfo.whitePix, dInfo.cVisualInfo.depth);
      imageWidth = 16;
      imageHeight = 16;
      return;
    };
}

void GxImageObject::DeleteImage(const GxDisplayInfo &rInfo)
{
  if(image != None)
    {
      XFreePixmap(rInfo.display, image);
      image = None;
    };

  if(imageMask != None)
    {
      XFreePixmap(rInfo.display, imageMask);
      imageMask = None;
    };
}

void GxImageObject::DrawImage(const GxDisplayInfo &dInfo,
			      const GxVolatileData &vData,
			      Window iWin, int x, int y) const
{
  //hack; does not handle mask
  if(image != None)
    {
      XGCValues gcValues;
      if(imageMask != None)
	{
	  gcValues.clip_x_origin = x;
	  gcValues.clip_y_origin = y;
	  gcValues.clip_mask = imageMask;
	  XChangeGC(dInfo.display, vData.borderGC, (GCClipXOrigin | GCClipYOrigin | GCClipMask), &gcValues);
	};

      XCopyArea(dInfo.display, image, iWin, vData.borderGC, 0,0,
		imageWidth,imageHeight, x,y);

      if(imageMask != None)
	{
	  gcValues.clip_x_origin = 0;
	  gcValues.clip_y_origin = 0;
	  gcValues.clip_mask = None; 
	  XChangeGC(dInfo.display, vData.borderGC, (GCClipXOrigin | GCClipYOrigin | GCClipMask), &gcValues);
	};
    };
}

