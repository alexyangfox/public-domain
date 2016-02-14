#ifndef GXCURSOR_INCLUDED
#define GXCURSOR_INCLUDED

#include <libGx/GxDisplayInfo.hh>
#include <libGx/GxVolatileData.hh>
#include <libGx/GxInc.hh>

class GxCursor
{
public:
  GxCursor(const GxDisplayInfo &rTDInfo, GxVolatileData &rTVData);
  virtual ~GxCursor(void);

  //if !draw this erases
  void Draw(bool draw);

  void SetPos(int tCurXPix, int tTextY);

  int XPos(void) const;
  int TextBaselinePos(void) const;

  Window xWin;

protected:
  const GxDisplayInfo &rDInfo;
  GxVolatileData &rVData;

  int curXPix;
  int textY; //the baseline of the line of text the cursor is on.
};


#endif //GXCURSOR_INCLUDED
