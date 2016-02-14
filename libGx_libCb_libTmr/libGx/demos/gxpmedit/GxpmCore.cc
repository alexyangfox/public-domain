#include <iostream>
#include <assert.h>
#include <string.h>

#include "GxpmCore.hh"

ProgOptions::ProgOptions(void) :
  saveColorsAsNames(0), pixelScale(DEFAULT_WORK_WIN_SCALE)
{}

ProgOptions::~ProgOptions(void)
{}

const ProgOptions& ProgOptions::operator=(const ProgOptions &rhs)
{
  rgbFileName = rhs.rgbFileName;
  saveColorsAsNames = rhs.saveColorsAsNames;
  pixelScale = rhs.pixelScale;

  return *this;
}

// *********************** start GxpmCore ***************************

GxpmCore::GxpmCore(appXData &rxData) :
  fileNamed(false), xData(rxData)
{
  PixValue pixValue(rxData.whitePix);
  imgData.ResizeAndClear(DEFAULT_PIX_WIDTH, DEFAULT_PIX_HEIGHT, pixValue);

  fileName[0] = '\0';
  pixName[0] = '\0';

  memset( &xpmAttrib, 0, sizeof(XpmAttributes) );

  //these members cannot be modified.
  xpmAttrib.valuemask = XpmVisual | XpmColormap | XpmDepth;
  xpmAttrib.visual = xData.pVis;
  xpmAttrib.colormap = xData.cmap;
  xpmAttrib.depth = xData.xdepth;

  CreateNewXPM();
}

GxpmCore::~GxpmCore(void)
{}

bool GxpmCore::UnSavedData(void)
{
  return imgData.Modified();
}

bool GxpmCore::PixmapFileNamed(void)
{
  return fileNamed;
}

const char *GxpmCore::GetPixName(void)
{
  return pixName;
}

const char *GxpmCore::GetFileName(void)
{
  return fileName;
}

const ProgOptions& GxpmCore::GetOptions(void) const
{
  return progOptions;
}

void GxpmCore::SetOptions(const ProgOptions &rOptions)
{
  progOptions = rOptions;
}

ImgData & GxpmCore::GetImgData(void)
{
  return imgData;
}

UINT GxpmCore::GetPixWidth(void)
{
  return imgData.Width();
}

UINT GxpmCore::GetPixHeight(void)
{
  return imgData.Height();
}

void GxpmCore::CreateNewXPM(void)
{
  fileName[0] = '\0'; //nuke the file name;

  PixValue pixValue(xData.whitePix);
  imgData.ResizeAndClear(DEFAULT_PIX_WIDTH, DEFAULT_PIX_HEIGHT, pixValue);
  imgData.ClearModified();

  /*
  CreateNewXImages(pixWidth, pixHeight, &pXImage, &pXShapeImage);

  //now clear the image and the shape mask
  for(UINT ii = 0; ii < pixWidth; ii++)
    for(UINT jj = 0; jj < pixHeight; jj++)
      {
	XPutPixel(pXImage, ii,jj, xData.whitePix);
	XPutPixel(pXShapeImage, ii,jj, xData.whitePix);
      };
  */
}

bool GxpmCore::ReadFile(const std::string &rFileName)
{
  if( rFileName.empty() ) return false;

  XpmImage image;
  XpmInfo info;
  memset(&image, 0, sizeof(XpmImage) );
  memset(&info, 0, sizeof(XpmInfo) );

  //hack; don't like cast
  int result = XpmReadFileToXpmImage((char*)rFileName.c_str(), &image, &info);
  if(result != XpmSuccess) return false;

  if( image.width > MAX_PIX_WIDTH ||
      image.height > MAX_PIX_HEIGHT ) return false;

  XImage *pXImage = NULL;
  XImage *pXShapeImage = NULL;
  //CreateNewXImages(image.width, image.height, &pXImage, &pXShapeImage);

  result = XpmCreateImageFromXpmImage(xData.pDisplay, &image, &pXImage,
				      &pXShapeImage, &xpmAttrib);

  if(result != XpmSuccess)
    {
      if(pXImage)
	{
	  XDestroyImage(pXImage);
	  pXImage = NULL;
	};

      if(pXShapeImage)
	{
	  XDestroyImage(pXShapeImage);
	  pXShapeImage = NULL;
	};

      return false;
    };

  //clear everything.
  strncpy(fileName, rFileName.c_str(), MAX_FILE_NAME_LEN-2);
  fileName[MAX_FILE_NAME_LEN-1] = '\0'; //guarentee termination
  fileNamed = true;

  PixValue pixValue(xData.whitePix);
  imgData.ResizeAndClear(image.width, image.height, pixValue);

  //fill in our colors
  for(unsigned ii = 0; ii < imgData.Width(); ii++)
    for(unsigned jj = 0; jj < imgData.Height(); jj++)
      {
	pixValue.xcolor = XGetPixel(pXImage, ii, jj);
	imgData.SetValue(ii, jj, pixValue, PIX_COLOR);
      };

  if(pXShapeImage) //mark our masked pixels
    {
      PixValue maskPixValue;
      maskPixValue.masked = true;
      for(unsigned ii = 0; ii < imgData.Width(); ii++)
	for(unsigned jj = 0; jj < imgData.Height(); jj++)
	  {
	    Pixel xColor = XGetPixel(pXShapeImage, ii, jj);
	    if(!xColor)
	      imgData.SetValue(ii, jj, maskPixValue, PIX_MASKED);
	  };
    };

  XpmFreeXpmImage(&image);

  if(pXImage)
    {
      XDestroyImage(pXImage);
      pXImage = NULL;
    };

  if(pXShapeImage)
    {
      XDestroyImage(pXShapeImage);
      pXShapeImage = NULL;
    };

  imgData.ClearModified();
  return true;
}

bool GxpmCore::WriteFile(void)
{
  if(!fileNamed) return false;

  //create x images from our imgData
  XImage *pNewImage;
  XImage *pNewShape;
  CreateNewXImages(imgData.Width(), imgData.Height(), &pNewImage, &pNewShape);
  if(!pNewImage || !pNewShape)
    {
      if(pNewImage)
	XDestroyImage(pNewImage);
      if(pNewShape)
	XDestroyImage(pNewShape);

      return false;
    };

  for(unsigned ii = 0; ii < imgData.Width(); ii++)
    for(unsigned jj = 0; jj < imgData.Height(); jj++)
      {
	const PixValue &rValue = ((const ImgData&)imgData).GetValue(ii, jj);
	XPutPixel(pNewImage, ii, jj, rValue.xcolor);
	if(rValue.masked)
	  XPutPixel(pNewShape, ii,jj, 0);
	else
	  XPutPixel(pNewShape, ii,jj, 1);
      };

  XpmImage image;
  memset(&image, 0, sizeof(XpmImage) );

  if(progOptions.saveColorsAsNames)
    {
      std::cout << "saving colors as names" << std::endl;
      std::cout <<  progOptions.rgbFileName.c_str() << std::endl;
      xpmAttrib.rgb_fname = (char*)progOptions.rgbFileName.c_str();
      xpmAttrib.valuemask |= XpmRgbFilename;
    }else
      {
	xpmAttrib.rgb_fname = NULL;
	xpmAttrib.valuemask &= ~XpmRgbFilename;
      };

  int result = XpmCreateXpmImageFromImage(xData.pDisplay, pNewImage,
					  pNewShape, &image, &xpmAttrib);

  xpmAttrib.rgb_fname = NULL;
  xpmAttrib.valuemask &= ~XpmRgbFilename;

  if(pNewImage)
    {
      XDestroyImage(pNewImage);
      pNewImage = 0;
    };

  if(pNewShape)
    {
      XDestroyImage(pNewShape);
      pNewShape = 0;
    };

  if(result != XpmSuccess) return false;

  XpmInfo info;
  memset(&info, 0, sizeof(XpmInfo) );
  //fill in the info.

  //?result?
  XpmWriteFileFromXpmImage(fileName, &image, &info);

  XpmFreeXpmImage(&image);

  imgData.ClearModified();
  return true;
}

void GxpmCore::SetFilename(const std::string &rFileName)
{
  if( rFileName.size() == 0 || rFileName.size() >= MAX_FILE_NAME_LEN)
    return;

  //this plus the above check guarentees termination
  strncpy(fileName, rFileName.c_str(), MAX_FILE_NAME_LEN);
  fileNamed = true;
}

bool GxpmCore::SizePixmap(UINT newWidth, UINT newHeight)
{
  if( (newWidth < MIN_PIX_WIDTH) || (newWidth > MAX_PIX_WIDTH) ||
      (newHeight < MIN_PIX_HEIGHT) || (newHeight > MAX_PIX_HEIGHT) )
    return false;

  //?success?
  if( (newWidth == imgData.Width()) && (newHeight == imgData.Height()) ) return true;

  PixValue fillValue(xData.whitePix);
  imgData.SafeResize(newWidth, newHeight, fillValue);

  return true;
}

void GxpmCore::CreateNewXImages(UINT newWidth, UINT newHeight,
				XImage **pImage, XImage **pShapeImage) const
{
  assert( newWidth && newHeight );
  assert( pImage && pShapeImage );

  int image_bp; //image bit pad
  int image_bpl; //image bytes per line
  char *pXImageData;
  switch(xData.xdepth)
    {
    case 24:
      image_bp = 32;
      image_bpl = newWidth*4;
      pXImageData = new char[(newWidth*4)*newHeight];
      break;
    case 16:
      image_bp = 16;
      image_bpl = newWidth*2;
      pXImageData = new char[(newWidth*2)*newHeight];
      break;
    case 8:
      image_bp = 8;
      image_bpl = newWidth;
      pXImageData = new char[newWidth*newHeight];
      break;
    default:
      std::cerr << "fatal. unknown X depth of: " << xData.xdepth << std::endl;
      assert(false);
    }

  *pImage = XCreateImage(xData.pDisplay, xData.pVis, xData.xdepth,
			 ZPixmap, 0, pXImageData, newWidth, newHeight,
			 image_bp, image_bpl);

  if(!pImage) return; //no point continuing


  //shape bit pad
  unsigned numChars = (((newWidth-1)/8+1)*newHeight);
  char* pShapeData = new char[numChars];

  for(unsigned ii = 0; ii < numChars; ii++)
    pShapeData[ii] = 0xFF; //every bit set.

  *pShapeImage = XCreateImage(xData.pDisplay, xData.pVis, 1, ZPixmap,
			      0, pShapeData, newWidth, newHeight, 8, 0);
  (*pShapeImage)->byte_order = MSBFirst;
  (*pShapeImage)->bitmap_bit_order = MSBFirst;

  /*
  std::cout << "\n\nin GxpmCore::CreateNewXImages" << std::endl;

  std::cout << "width: "              << (*pShapeImage)->width << std::endl;
  std::cout << "height: "             << (*pShapeImage)->height << std::endl;
  std::cout << "xoffset: "            << (*pShapeImage)->xoffset << std::endl;
  std::cout << "format: "             << (*pShapeImage)->format << std::endl;

  std::cout << "data: "               << (*pShapeImage)->data << std::endl;
  std::cout << "byte_order: "         << (*pShapeImage)->byte_order << std::endl;
  std::cout << "bitmap_unit: "        << (*pShapeImage)->bitmap_unit << std::endl;
  std::cout << "bitmap_bit_order: "   << (*pShapeImage)->bitmap_bit_order << std::endl;

  std::cout << "bitmap_pad: "         << (*pShapeImage)->bitmap_pad << std::endl;
  std::cout << "depth: "              << (*pShapeImage)->bitmap_pad << std::endl;
  std::cout << "bytes_per_line: "     << (*pShapeImage)->bytes_per_line << std::endl;
  std::cout << "bits_per_pixel: "     << (*pShapeImage)->bits_per_pixel << std::endl;
  */
}
/*
void GxpmCore::CreateAndClearXShapeImage(void)
{
  assert(pixWidth && pixHeight);
  assert( !pXShapeImage );

  unsigned numChars = (((pixWidth-1)/8+1)*pixHeight);
  char* pShapeData = new char[numChars];

  for(unsigned ii = 0; ii < numChars; ii++)
    pShapeData[ii] = 0xFF; //every bit set.

  pXShapeImage = XCreateImage(xData.pDisplay, xData.pVis, 1, ZPixmap,
			      0, pShapeData, pixWidth, pixHeight, 8, 0);
  pXShapeImage->byte_order = MSBFirst;
  pXShapeImage->bitmap_bit_order = MSBFirst;
}
*/
