#ifndef GXPMCORE_INCLUDED
#define GXPMCORE_INCLUDED

#include <X11/Xlib.h>
#include <string>

#include <libGx/GxInc.hh>

#include "PixValue.hh"
#include "ImgData.hh"

//a simple class which contains basic information needed by
//various xpm functions. (these are put into the XpmAttributes structure);
class appXData
{
public:
  appXData(){};
  ~appXData(){};

  Display *pDisplay;
  ULINT whitePix;
  Visual *pVis;
  Colormap cmap;
  unsigned int xdepth;
};

class ProgOptions
{
public:
  ProgOptions(void);
  ~ProgOptions(void);

  const ProgOptions& operator=(const ProgOptions &rhs);

  std::string rgbFileName;
  bool saveColorsAsNames;

  unsigned pixelScale;
};

const unsigned DEFAULT_PIX_WIDTH = 20;
const unsigned DEFAULT_PIX_HEIGHT = 20;
const unsigned DEFAULT_WORK_WIN_SCALE = 10;
const UINT MIN_PIX_WIDTH = 1;
const UINT MIN_PIX_HEIGHT = 1;
const UINT MAX_PIX_WIDTH = 100;
const UINT MAX_PIX_HEIGHT = 100;
const unsigned MAX_FILE_NAME_LEN = 1023;

//after this is constructed; we MUST have valid data. perhaps we could add
//a fuction to check it which could be called after main.
class GxpmCore
{
public:
  GxpmCore(appXData &rxData);
  ~GxpmCore(void);

  //bool EditingXPM(void); //we are always editing a xpm
  bool UnSavedData(void);
  bool PixmapFileNamed(void); //so we know if we can saveas
  const char *GetPixName(void);
  const char *GetFileName(void);

  const ProgOptions& GetOptions(void) const;
  void SetOptions(const ProgOptions &rOptions);

  //the GxpmCore should remain the holder of the ImgData, the gui
  //just requests a handle when it needs it.
  ImgData & GetImgData(void);

  UINT GetPixWidth(void);
  UINT GetPixHeight(void);

  void CreateNewXPM(void);
  
  //returns true if the file was sucessfully read
  bool ReadFile(const std::string &rFileName);
  //updates the XImages from the contents of the image and info structures
  //bool UpdateXImages(void);
  //updates image and info from the pXImage and pXShapeImage Information
  //bool UpdateXpmImage(void);
  //returns true if the file was sucessfully written.The former function
  //must be called first to update the informstion in image and info first.
  //obviously the file will not be written if the filename has not been set
  bool WriteFile(void);
  void SetFilename(const std::string &rFileName);

  bool SizePixmap(UINT newWidth, UINT newHeight);

private:
  //we use this to store our width and height
  ImgData imgData;

  //frees all storage related to any images and may
  //reset some image parameters
  //void DeleteXImages(void);
  void CreateNewXImages(UINT newWidth, UINT newHeight,
			XImage **pImage, XImage **pShapeImage) const;
  //void CreateAndClearXShapeImage(void);

  bool fileNamed;
  char fileName[MAX_FILE_NAME_LEN+1];
  //?hack? is this a duplication of data within info?
  char pixName[512];

  XpmAttributes xpmAttrib;

  ProgOptions progOptions;

  appXData &xData;
};

#endif //GXPMCORE_INCLUDED
