#include <libGx/GxDisplay.hh>

#include <libGx/GxCentralColor.hh>

using namespace std;

GxCentralColor::GxCentralColor(void) :
  foundColor(false)
{}

/*
GxCentralColor::GxCentralColor(const char *pColorName) :
  foundColor(false)
{
  GxColorHolder *pDefaultColor = new GxNamedColorHolder(pColorName);
  colorList.append(pDefaultColor);
}

GxCentralColor::GxCentralColor(UINT redC, UINT greenC, UINT blueC) :
  foundColor(false)
{} //hack; unimplemented
*/

GxCentralColor::~GxCentralColor(void)
{
  ClearColorList();
}

GxCentralColor* GxCentralColor::Clone(void) const
{
  GxCentralColor *pClonedColor = new GxCentralColor;

  std::list<GxColorHolder*>::const_iterator cPlace = colorList.begin();
  std::list<GxColorHolder*>::const_iterator cEnd = colorList.end();
  while(cPlace != cEnd)
    {
      //we are manipulating the list in pClonedColor
      pClonedColor->colorList.push_back( (*cPlace)->Clone() );
      cPlace++;
    };

  return pClonedColor;
}

void GxCentralColor::ClearColorList(void)
{
  while( !colorList.empty() )
    {
      GxColorHolder *pCHolder = colorList.front();
      delete pCHolder;
      pCHolder = 0;
      colorList.pop_front();
    };
}

void GxCentralColor::AddColor(const char *pAltName)
{
  colorList.push_back( new GxNamedColorHolder(pAltName) );
}

bool GxCentralColor::Allocate(GxErrorSink &rErrorSink, GxDisplay &rDisplay, Colormap tCMap)
{
  std::list<GxColorHolder*>::iterator cPlace = colorList.begin();
  std::list<GxColorHolder*>::iterator cEnd = colorList.end();
  while(cPlace != cEnd)
    {
      GxColorHolder *pCHolder = *cPlace;
      if( pCHolder->AllocColor(rErrorSink, rDisplay, tCMap, myColor) )
	{
	  foundColor = true;
	  return true;
	};
      cPlace++;
    };

  foundColor = false;
  return false;
}

const XColor& GxCentralColor::GetColor(void)
{
  return myColor;
}

GxCentralColor::GxColorHolder::GxColorHolder(void)
{}

GxCentralColor::GxColorHolder::~GxColorHolder(void)
{}

GxCentralColor::GxNamedColorHolder::GxNamedColorHolder(const char *pName) :
  GxCentralColor::GxColorHolder()
{
  unsigned junkLen = 0;
  GxSetLabel(colorName, GX_DEFAULT_LABEL_LEN, pName, junkLen);
}

GxCentralColor::GxNamedColorHolder::~GxNamedColorHolder(void)
{}

GxCentralColor::GxColorHolder *GxCentralColor::GxNamedColorHolder::Clone(void) const
{
  return new GxCentralColor::GxNamedColorHolder(colorName);
}

bool GxCentralColor::GxNamedColorHolder::AllocColor(GxErrorSink &rErrorSink, GxDisplay &rDisplay,
						    Colormap tCMap, XColor &rColor)
{
  if( !XParseColor(rDisplay.XDisp(), tCMap, colorName, &rColor) )
    {
      rErrorSink.SinkError( string("Unable to find ") + colorName + " in color database on display: " + rDisplay.GetDispName() );
      return false;
    };
  if( !XAllocColor(rDisplay.XDisp(), tCMap, &rColor) )
    {
      rErrorSink.SinkError( string("Unable to allocate ") + colorName + " on display: " + rDisplay.GetDispName() );
      return false;
    };

  return true;
}
