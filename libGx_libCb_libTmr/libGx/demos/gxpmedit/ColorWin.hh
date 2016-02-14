#ifndef COLORWIN_INCLUDED
#define COLORWIN_INCLUDED

#include <iostream>
#include <string>
#include <vector>

#include <libCb/CbCallback.hh>
#include <libGx/GxWin.hh>
#include <libGx/GxRealOwner.hh>

#include "PixValue.hh"

const int cellSize = 20; //the size in pixels of each color win
const UINT NUM_COLORS = 48;
const UINT NUM_DEFAULT_COLORS = 22;
const unsigned COLOR_COLS = 6;
const unsigned COLOR_ROWS = 4;

class ColorDef
{
public:
  ColorDef(void);
  ColorDef(const ColorDef &rhs);
  ColorDef(Pixel tXColor);
  ~ColorDef(void);

  bool defined;
  bool transparent; //only one color def should have this set.
  unsigned long xcolor;
  std::string colorName;
};

class BoxFiller
{
public:
  BoxFiller(const GxDisplayInfo &rDInfo, GxVolatileData &rVData, Drawable tXWin);
  ~BoxFiller(void);

  void Fill(const ColorDef &rColor, int x, int y, unsigned size);

protected:
  const GxDisplayInfo &dInfo;
  GxVolatileData &vData;
  Drawable xWin;
};


class ColorWin : public GxWin
{
public:
  ColorWin(GxRealOwner *pOwner, unsigned tStartColor = 0, unsigned tNumColors = NUM_DEFAULT_COLORS);
  virtual ~ColorWin(void);

  const ColorDef& GetDefaultColor(void) const;
  CbOneFO<const ColorDef&> colorChangeCB;
  CbOneFO<ColorDef&> colorDefineCB;

  CbVoidFO colorEnterCB;
  CbVoidFO colorLeaveCB;

  void Refresh(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;
  virtual void Resize(UINT nWidth, UINT nHeight);
  virtual void Create(void);

  virtual void HandleEvent(const XEvent &rEvent);
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);

  void CreatePixmap(void);
  void FillPixmap(void);
  int GetColorPlace(int xPlace, int yPlace);
  void DrawInternal(void);

  static const char* default_color_names[]; //the first n colors are default colors
  static std::vector<ColorDef> allColors; //the allocated colors
public:
  static void AllocDefaultColors(const GxDisplayInfo &rDInfo);
  static void FreeColors(const GxDisplayInfo &rDInfo); //not implemented (we would be exiting)
protected:

  unsigned startColor;
  unsigned numColors;

  Pixmap colorMap;
  PixValue currentColor;
  UINT pixWidth, pixHeight;
  int xPix, yPix;
};

class ColorBox : public GxWin
{
public:
  ColorBox(GxRealOwner *pOwner);
  virtual ~ColorBox(void);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  void SetCurrentColor(const ColorDef &rColor);
  const ColorDef &GetCurrentColor(void) const;
  void GetCurrentColor(PixValue &rValue) const;

  virtual void HandleEvent(const XEvent &rEvent);
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
  void DrawInternal(void);

  ColorDef currentColor;
};

#endif //COLORWIN_INCLUDED
