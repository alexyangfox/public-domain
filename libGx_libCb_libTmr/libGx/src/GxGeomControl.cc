#include <libGx/GxGeomControl.hh>

#include "GxDefines.hh"

GxGeomControl::GxGeomControl(void)
{}

GxGeomControl::~GxGeomControl(void)
{}

GxGeomControl* GxGeomControl::Clone(void) const
{
  return new GxGeomControl;
}

void GxGeomControl::PlaceOwner(GxWinArea *, int &, int &, int &, int &)
{}

GX_WD_STAT GxGeomControl::GetWidthStat(void) const
{
  return GX_WD_FIXED;
}

GX_HT_STAT GxGeomControl::GetHeightStat(void) const
{
  return GX_HT_FIXED;
}

UINT GxGeomControl::LBorder(void) const
{
  return 0;
}

UINT GxGeomControl::RBorder(void) const
{
  return 0;
}

UINT GxGeomControl::TBorder(void) const
{
  return 0;
}

UINT GxGeomControl::BBorder(void) const
{
  return 0;
}

// ****************** start GxFixed ***************************

GxFixed::GxFixed(void) :
  GxGeomControl()
{}

GxFixed::~GxFixed(void)
{}

GxGeomControl* GxFixed::Clone(void) const
{
  return (GxGeomControl*) new GxFixed;
}

void GxFixed::PlaceOwner(GxWinArea *, int &, int &, int &, int &)
{}

// ****************** start GxBasic ***************************

GxBasic::GxBasic(GX_WD_STAT tPWidth, GX_HT_STAT tPHeight,
		 GX_H_PLACEMENT tHPlace, GX_V_PLACEMENT tVPlace,
		 bool tHReset, bool tVReset) :
  hPlace(tHPlace), vPlace(tVPlace), pWidth(tPWidth), pHeight(tPHeight),
  hReset(tHReset), vReset(tVReset)
{}

GxBasic::~GxBasic(void)
{}

GxGeomControl* GxBasic::Clone(void) const
{
  return (GxGeomControl*) new GxBasic(pWidth, pHeight, hPlace, vPlace,
				      hReset, vReset);
}

void GxBasic::PlaceOwner(GxWinArea *pObject, int &lX, int &rX,
			 int &tY, int &bY)
{
  switch(pWidth)
    {
    case GX_WD_FILL:
      pObject->Width(rX - lX);
      break;

    case GX_WD_INT:
      pObject->Width(pObject->GetDesiredWidth());
      break;

    default: //GX_WD_FIXED
      break;
    };

  switch(pHeight)
    {
    case GX_HT_FILL:
      pObject->Height(bY - tY);
      break;

    case GX_HT_INT:
      pObject->Height( pObject->GetDesiredHeight() );
      break;

    default: //GX_HT_FIXED
      break;
    };

  UINT oWidth, oHeight;
  pObject->GetSize(oWidth, oHeight);

  switch(hPlace)
    {
    case GX_FLOW_LEFT:
      pObject->X(lX);
      break;

    case GX_FLOW_RIGHT:
      pObject->X(rX-oWidth);
      break;

    case GX_H_CENTERED:
      pObject->X(lX + ((rX - lX - oWidth)/2));
      break;

    default: // GX_H_FIXED
      break;//do nothing
    };

  switch(vPlace)
    {
    case GX_FLOW_UP:
      pObject->Y(tY);
      break;

    case GX_FLOW_DOWN:
      pObject->Y(bY-oHeight);
      break;

    case GX_V_CENTERED:
      pObject->Y(tY + ((bY - tY - oHeight)/2));
      break;

    default: //GX_V_FIXED
      break;//do nothing
    };

  if(vPlace == GX_V_CENTERED || pHeight == GX_HT_FILL || hReset == true)
    {
      if(hPlace == GX_FLOW_LEFT)
	{
	  lX += oWidth;
	};
      if(hPlace == GX_FLOW_RIGHT)
	{
	  rX -= oWidth;
	};
    };

  if(hPlace == GX_H_CENTERED || pWidth == GX_WD_FILL || vReset == true)
    {
      if(vPlace == GX_FLOW_UP)
	{
	  tY += oHeight;
	};
      if(vPlace == GX_FLOW_DOWN)
	{
	  bY -= oHeight;
	};
    };
}

// ****************** start GxSBasic ***************************

GxSBasic::GxSBasic(GX_WD_STAT tPWidth, GX_HT_STAT tPHeight,
		   GX_H_PLACEMENT tHPlace, GX_V_PLACEMENT tVPlace,
		   UINT leftBorder, UINT rightBorder,
		   UINT topBorder, UINT bottomBorder,
		   bool tHReset, bool tVReset) :
  GxBasic(tPWidth, tPHeight, tHPlace, tVPlace, tHReset, tVReset),
  lBorder(leftBorder), rBorder(rightBorder),
  tBorder(topBorder), bBorder(bottomBorder)
{}

GxSBasic::~GxSBasic(void)
{}

GxGeomControl* GxSBasic::Clone(void) const
{
  return (GxGeomControl*) new GxSBasic(pWidth, pHeight, hPlace, vPlace,
				       lBorder, rBorder, tBorder, bBorder,
				       hReset, vReset);
}

void GxSBasic::PlaceOwner(GxWinArea *pObject, int &lX, int &rX, int &tY, int &bY)
{
  switch(pWidth)
    {
    case GX_WD_FILL:
      pObject->Width(rX - lX - lBorder - rBorder);
      break;

    case GX_WD_INT:
      pObject->Width(pObject->GetDesiredWidth());
      break;

    default: //GX_WD_FIXED
      break;
    };

  switch(pHeight)
    {
    case GX_HT_FILL:
      pObject->Height(bY - tY - tBorder - bBorder);
      break;

    case GX_HT_INT:
      pObject->Height( pObject->GetDesiredHeight() );
      break;

    default: //GX_HT_FIXED
      break;
    };

  UINT oWidth, oHeight;
  pObject->GetSize(oWidth, oHeight);

  switch(hPlace)
    {
    case GX_FLOW_LEFT:
      pObject->X(lX + lBorder);
      break;

    case GX_FLOW_RIGHT:
      pObject->X(rX - oWidth - rBorder);
      break;

    case GX_H_CENTERED: //ignores the borders
      pObject->X( lX + ((rX - lX - oWidth)/2));
      break;

    default: // GX_H_FIXED
      break;//do nothing
    };

  switch(vPlace)
    {
    case GX_FLOW_UP:
      pObject->Y(tY + tBorder);
      break;

    case GX_FLOW_DOWN:
      pObject->Y(bY - oHeight - bBorder);
      break;

    case GX_V_CENTERED: //ignores the borders
      pObject->Y(tY + ((bY - tY - oHeight)/2));
      break;

    default: //GX_V_FIXED
      break;//do nothing
    };


  if(vPlace == GX_V_CENTERED || pHeight == GX_HT_FILL || hReset == true)
    {
      if(hPlace == GX_FLOW_RIGHT)
	{
	  rX -= (oWidth + lBorder + rBorder);
	};

      if(hPlace == GX_FLOW_LEFT)
	{
	  lX += oWidth + lBorder + rBorder;
	};
    };

  if(hPlace == GX_H_CENTERED || pWidth == GX_WD_FILL || vReset == true)
    {
      if(vPlace == GX_FLOW_UP)
	{
	  tY += oHeight + tBorder + bBorder;
	};

      if(vPlace == GX_FLOW_DOWN)
	{
	  bY -= (oHeight + tBorder + bBorder);
	};
    };
}

UINT GxSBasic::LBorder(void) const
{
  return lBorder;
}

UINT GxSBasic::RBorder(void) const
{
  return rBorder;
}

UINT GxSBasic::TBorder(void) const
{
  return tBorder;
}

UINT GxSBasic::BBorder(void) const
{
  return bBorder;
}

// ****************** start GxSIBasic ***************************

GxSIBasic::GxSIBasic(GX_WD_STAT tPWidth, GX_HT_STAT tPHeight,
		     GX_H_PLACEMENT tHPlace, GX_V_PLACEMENT tVPlace,
		     UINT leftBorder, UINT rightBorder,
		     UINT topBorder, UINT bottomBorder,
		     bool tHReset, bool tVReset) :
  GxBasic(tPWidth, tPHeight, tHPlace, tVPlace, tHReset, tVReset),
  lBorder(leftBorder), rBorder(rightBorder), tBorder(topBorder),
  bBorder(bottomBorder)
{}

GxSIBasic::~GxSIBasic(void)
{}

GxGeomControl* GxSIBasic::Clone(void) const
{
  return (GxGeomControl*) new GxSIBasic(pWidth, pHeight, hPlace, vPlace,
					lBorder, rBorder, tBorder, bBorder,
					hReset, vReset);
}

void GxSIBasic::PlaceOwner(GxWinArea *pObject, int &lX, int &rX,
			   int &tY, int &bY)
{
  switch(pWidth)
    {
    case GX_WD_FILL:
      pObject->Width(rX - lX - lBorder*GX_SPACE_INC - rBorder*GX_SPACE_INC);
      break;

    case GX_WD_INT:
      pObject->Width(pObject->GetDesiredWidth());
      break;

    default: //GX_WD_FIXED
      break;
    };

  switch(pHeight)
    {
    case GX_HT_FILL:
      pObject->Height(bY - tY - tBorder*GX_SPACE_INC - bBorder*GX_SPACE_INC);
      break;

    case GX_HT_INT:
      pObject->Height( pObject->GetDesiredHeight() );
      break;

    default: //GX_HT_FIXED
      break;
    };

  UINT oWidth, oHeight;
  pObject->GetSize(oWidth, oHeight);

  switch(hPlace)
    {
    case GX_FLOW_LEFT:
      pObject->X(lX + lBorder*GX_SPACE_INC);
      break;

    case GX_FLOW_RIGHT:
      pObject->X(rX - oWidth - rBorder*GX_SPACE_INC);
      break;

    case GX_H_CENTERED: //ignores the borders
      pObject->X( lX + ((rX - lX - oWidth)/2));
      break;

    default: // GX_H_FIXED
      break;//do nothing
    };

  switch(vPlace)
    {
    case GX_FLOW_UP:
      pObject->Y(tY + tBorder*GX_SPACE_INC);
      break;

    case GX_FLOW_DOWN:
      pObject->Y(bY - oHeight - bBorder*GX_SPACE_INC);
      break;

    case GX_V_CENTERED: //ignores the borders
      pObject->Y(tY + ((bY - tY - oHeight)/2));
      break;

    default: //GX_V_FIXED
      break;//do nothing
    };


  if(vPlace == GX_V_CENTERED || pHeight == GX_HT_FILL || hReset == true)
    {
      if(hPlace == GX_FLOW_RIGHT)
	{
	  rX -= (oWidth + lBorder*GX_SPACE_INC + rBorder*GX_SPACE_INC);
	};

      if(hPlace == GX_FLOW_LEFT)
	{
	  lX += oWidth + lBorder*GX_SPACE_INC + rBorder*GX_SPACE_INC;
	};
    };

  if(hPlace == GX_H_CENTERED || pWidth == GX_WD_FILL || vReset == true)
    {
      if(vPlace == GX_FLOW_UP)
	{
	  tY += oHeight + tBorder*GX_SPACE_INC + bBorder*GX_SPACE_INC;
	};

      if(vPlace == GX_FLOW_DOWN)
	{
	  bY -= (oHeight + tBorder*GX_SPACE_INC + bBorder*GX_SPACE_INC);
	};
    };
}

UINT GxSIBasic::LBorder(void) const
{
  return lBorder*GX_SPACE_INC;
}

UINT GxSIBasic::RBorder(void) const
{
  return rBorder*GX_SPACE_INC;
}

UINT GxSIBasic::TBorder(void) const
{
  return tBorder*GX_SPACE_INC;
}

UINT GxSIBasic::BBorder(void) const
{
  return bBorder*GX_SPACE_INC;
}

// ****************** start GxSOnly ***************************

GxSOnly::GxSOnly(UINT lB, UINT rB, UINT tB, UINT bB) :
  GxGeomControl(), lBorder(lB), rBorder(rB), tBorder(tB), bBorder(bB)
{}

GxSOnly::~GxSOnly(void)
{}

GxGeomControl* GxSOnly::Clone(void) const
{
  return (GxGeomControl*) new GxSOnly(lBorder, rBorder, tBorder, bBorder);
}

void GxSOnly::PlaceOwner(GxWinArea *pObject, int &lX, int &rX,
			  int &tY, int &bY)
{
  pObject->Resize(rX -lX - lBorder - rBorder,
		  bY - tY - tBorder - bBorder);
  pObject->Move(lX + lBorder,
		tY + bBorder);
}

UINT GxSOnly::LBorder(void) const
{
  return lBorder;
}

UINT GxSOnly::RBorder(void) const
{
  return rBorder;
}

UINT GxSOnly::TBorder(void) const
{
  return tBorder;
}

UINT GxSOnly::BBorder(void) const
{
  return bBorder;
}

// ****************** start GxSOnly ***************************

GxSIOnly::GxSIOnly(UINT lB, UINT rB, UINT tB, UINT bB) :
  GxGeomControl(), lBorder(lB), rBorder(rB), tBorder(tB), bBorder(bB)
{}

GxSIOnly::~GxSIOnly(void)
{}

GxGeomControl* GxSIOnly::Clone(void) const
{
  return (GxGeomControl*) new GxSIOnly(lBorder, rBorder, tBorder, bBorder);
}

void GxSIOnly::PlaceOwner(GxWinArea *pObject, int &lX, int &rX,
			  int &tY, int &bY)
{
  pObject->Resize(rX -lX - lBorder*GX_SPACE_INC - rBorder*GX_SPACE_INC,
		  bY - tY - tBorder*GX_SPACE_INC - bBorder*GX_SPACE_INC);
  pObject->Move(lX + lBorder*GX_SPACE_INC,
		tY + tBorder*GX_SPACE_INC);
}

UINT GxSIOnly::LBorder(void) const
{
  return lBorder*GX_SPACE_INC;
}

UINT GxSIOnly::RBorder(void) const
{
  return rBorder*GX_SPACE_INC;
}

UINT GxSIOnly::TBorder(void) const
{
  return tBorder*GX_SPACE_INC;
}

UINT GxSIOnly::BBorder(void) const
{
  return bBorder*GX_SPACE_INC;
}

//********************** start independant stuff *******************

// ****************** start GxWDInd ***************************

GxWDInd::GxWDInd(void)
{}

GxWDInd::~GxWDInd(void)
{}

void GxWDInd::SizeWidth(GxWinArea *, int &, int &)
{}

GxWDInd * GxWDInd::Clone(void) const
{
  return new GxWDInd;
}

GxWDProp::GxWDProp(GxWinArea *pObjectPropTo, const GxFraction &rProp) :
  GxWDInd(), pObj(pObjectPropTo), prop(rProp)
{}

GxWDProp::~GxWDProp(void)
{}

void GxWDProp::SizeWidth(GxWinArea *pObject, int &, int &)
{
  pObject->Width( prop.Convert( pObj->Width() ) );
}

GxWDInd * GxWDProp::Clone(void) const
{
  return (GxWDInd*) new GxWDProp(pObj, prop);
}

GxWDBasic::GxWDBasic(GX_WD_STAT wStat) :
  GxWDInd(), wdStat(wStat)
{}

GxWDBasic::~GxWDBasic(void)
{}

void GxWDBasic::SizeWidth(GxWinArea *pObject, int &lX, int &rX)
{
  switch(wdStat)
    {
    case GX_WD_FILL:
      pObject->Width(rX - lX);
      break;
    case GX_WD_INT:
      pObject->Width( pObject->GetDesiredWidth() );
      break;
    default: //GX_WD_FIXED
      break;
      //do nothing
    };
}

GxWDInd * GxWDBasic::Clone(void) const
{
  return (GxWDInd*) new GxWDBasic(wdStat);
}

GxHTInd::GxHTInd(void)
{}

GxHTInd::~GxHTInd(void)
{}

void GxHTInd::SizeHeight(GxWinArea *, int &, int &)
{}

GxHTInd * GxHTInd::Clone(void) const
{
  return new GxHTInd;
}

GxHTProp::GxHTProp(GxWinArea *pObjectPropTo, const GxFraction &rProp) :
  GxHTInd(), pObj(pObjectPropTo), prop(rProp)
{}

GxHTProp::~GxHTProp(void)
{}

void GxHTProp::SizeHeight(GxWinArea *pObject, int &, int &)
{
  pObject->Height( prop.Convert( pObj->Height() ) );
}

GxHTInd * GxHTProp::Clone(void) const
{
  return (GxHTInd*) new GxHTProp(pObj, prop);
}

GxHTBasic::GxHTBasic(GX_HT_STAT tHeightStat) :
  GxHTInd(), htStat(tHeightStat)
{}

GxHTBasic::~GxHTBasic(void)
{}

void GxHTBasic::SizeHeight(GxWinArea *pObject, int &tY, int &bY)
{
  switch(htStat)
    {
    case GX_HT_FILL:
      pObject->Height(bY - tY);
      break;
      
    case GX_HT_INT:
      pObject->Height(pObject->GetDesiredHeight());
      break;

    default: //GX_HT_FIXED
      //do nothing
      break;
    };
}

GxHTInd * GxHTBasic::Clone(void) const
{
  return (GxHTInd*) new GxHTBasic(htStat);
}


GxPlaceHInd::GxPlaceHInd(void)
{}

GxPlaceHInd::~GxPlaceHInd(void)
{}

void GxPlaceHInd::PlaceH(GxWinArea *, int &, int &)
{}

GxPlaceHInd * GxPlaceHInd::Clone(void) const
{
  return  new GxPlaceHInd;
}

GxHRel::GxHRel(GX_ATTACHMENT tObjAttach, GxWinArea *pTRelTo,
	       GX_ATTACHMENT tRelAttach, int Gap) :
  GxPlaceHInd(), pRelTo(pTRelTo), objAttach(tObjAttach), relAttach(tRelAttach),
  gap(Gap)
{}

GxHRel::~GxHRel(void)
{}

void GxHRel::PlaceH(GxWinArea *pObject, int &lX, int &rX)
{
  /*
  GxWinArea *pRelTo;
  GX_ATTACHMENT objAttach, relAttach;
  int gap;
  */
}

GxPlaceHInd * GxHRel::Clone(void) const
{
  return (GxPlaceHInd*) new GxHRel(objAttach, pRelTo, relAttach, gap);
}

GxHBasic::GxHBasic(GX_H_PLACEMENT hPlacement, bool reset) :
  GxPlaceHInd(), hPlace(hPlacement), resetH(reset)
{}

GxHBasic::~GxHBasic(void)
{}

void GxHBasic::PlaceH(GxWinArea *pObject, int &lX, int &rX)
{
  switch(hPlace)
    {
    case GX_FLOW_LEFT:
      pObject->X(lX);
      if(resetH)
	{
	  lX += pObject->Width();
	};
      break;

    case GX_FLOW_RIGHT:
      pObject->X(rX - pObject->Width());
      if(resetH)
	{
	  rX -= pObject->Width();
	};
      break;

    case GX_H_CENTERED:
      pObject->X(lX + ((rX-lX)/2) - (pObject->Width()/2));
      break;

    default: // GX_H_FIXED
      break;//do nothing
    };
}

GxPlaceHInd * GxHBasic::Clone(void) const
{
  return (GxPlaceHInd*) new GxHBasic(hPlace, resetH);
}

GxPlaceVInd::GxPlaceVInd(void)
{}

GxPlaceVInd::~GxPlaceVInd(void)
{}

void GxPlaceVInd::PlaceV(GxWinArea *, int &, int &)
{}

GxPlaceVInd * GxPlaceVInd::Clone(void) const
{
  return (GxPlaceVInd*) new GxPlaceVInd;
}

GxVRel::GxVRel(GX_ATTACHMENT tObjAttach, GxWinArea *pTRelTo,
	       GX_ATTACHMENT tRelAttach, int Gap) :
  GxPlaceVInd(), pRelTo(pTRelTo), objAttach(tObjAttach),
  relAttach(tRelAttach), gap(Gap)
{}

GxVRel::~GxVRel(void)
{}

void GxVRel::PlaceV(GxWinArea *pObject, int &tY, int &bY)
{

}

GxPlaceVInd * GxVRel::Clone(void) const
{
  return (GxPlaceVInd*) new GxVRel(objAttach, pRelTo, relAttach, gap);
}

GxVBasic::GxVBasic(GX_V_PLACEMENT vPlacement, bool reset) :
  GxPlaceVInd(), vPlace(vPlacement), resetV(reset)
{}

GxVBasic::~GxVBasic(void)
{}

void GxVBasic::PlaceV(GxWinArea *pObject, int &tY, int &bY)
{
  switch(vPlace)
    {
    case GX_FLOW_UP:
      pObject->Y(tY);
      if(resetV)
	{
	  tY += pObject->Height();
	};
      break;

    case GX_FLOW_DOWN:
      pObject->Y(bY - pObject->Height());
      if(resetV)
	{
	  bY -= pObject->Height();
	};
      break;

    case GX_V_CENTERED: //ignores the borders
      pObject->Y(tY + (bY - tY)/2 - (pObject->Height()/2));
      break;

    default: //GX_V_FIXED
      break;//do nothing
    };
}

GxPlaceVInd * GxVBasic::Clone(void) const
{
  return (GxPlaceVInd*) new GxVBasic(vPlace, resetV);
}

// ****************** start GxInd ***************************

//sizes width first, height next, then hPlace then vPlace
GxInd::GxInd(const GxWDInd &rWdInd, const GxHTInd &rHtInd,
	     const GxPlaceHInd &rPlaceHInd, const GxPlaceVInd &rPlaceVInd) :
  GxGeomControl(), pWdInd(rWdInd.Clone()), pHtInd(rHtInd.Clone()),
  pPlaceHInd(rPlaceHInd.Clone()), pPlaceVInd(rPlaceVInd.Clone())
{}

GxInd::~GxInd(void)
{}

GxGeomControl* GxInd::Clone(void) const
{
  return (GxGeomControl*) new GxInd(*pWdInd, *pHtInd, *pPlaceHInd,
				    *pPlaceVInd);
}

void GxInd::PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
		       int &tY, int &bY)
{
  pWdInd->SizeWidth(pObjectToControl, lX, rX);
  pHtInd->SizeHeight(pObjectToControl, tY, bY);
  pPlaceHInd->PlaceH(pObjectToControl, lX, rX);
  pPlaceVInd->PlaceV(pObjectToControl, tY, bY);
}

/********
class GxSInd : public GxGeomControl
{
public:
  GxSInd(const GxWDInd &rWDInd, const GxHTInd &rHTInd,
	 const GxPlaceHInd &rPlaceHInd, const GxPlaceVInd &rPlaceVInd);
  virtual ~GxSInd(void);

  virtual GxGeomControl* Clone(void) const;

  //by default this does not place its owner
  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);

protected:
  GxWDInd *pWdInd;
  GxHTInd *pHtInd;
  GxPlaceHInd *pPlaceHInd;
  GxPlaceVInd *pPlaceVInd;
};

class GxSIInd : public GxGeomControl
{
public:
  GxSIInd(const GxWDInd &rWDInd, const GxHTInd &rHTInd,
	  const GxPlaceHInd &rPlaceHInd, const GxPlaceVInd &rPlaceVInd);
  virtual ~GxSIInd(void);

  virtual GxGeomControl* Clone(void) const;

  //by default this does not place its owner
  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);

protected:
  GxWDInd *pWdInd;
  GxHTInd *pHtInd;
  GxPlaceHInd *pPlaceHInd;
  GxPlaceVInd *pPlaceVInd;
};
*/

