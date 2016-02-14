#ifndef GXGEOMCONTROL_INCLUDED
#define GXGEOMCONTROL_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxWinArea.hh>
#include <libGx/GxFraction.hh>

class GxGeomControl
{
public:
  virtual ~GxGeomControl(void);

  //creates a copy of the particular GxGeomControl on the heap and returns
  //the pointer to this object. The recipient is responsible for this
  //objects distruction
  virtual GxGeomControl* Clone(void) const;

  //by default this does not place its owner
  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);

  //hack? obsolete?
  virtual GX_WD_STAT GetWidthStat(void) const;
  virtual GX_HT_STAT GetHeightStat(void) const;

  virtual UINT LBorder(void) const;
  virtual UINT RBorder(void) const;
  virtual UINT TBorder(void) const;
  virtual UINT BBorder(void) const;

protected:
  GxGeomControl(void);
};

//this geom control does not modify either the size or position of its owner
//should be relativly unused unless the user wants to explicitly set the
//object's size and position.  This, when added to a GxWinArea just over-rides
//the default place behavior.
class GxFixed : public GxGeomControl
{
public:
  GxFixed(void);
  virtual ~GxFixed(void);

  virtual GxGeomControl* Clone(void) const;

  //by default this does not place its owner
  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);
};

//only adds borders to the object, keeps traditional placement power. places
//in box, considering its borders
class GxSOnly : public GxGeomControl
{
public:
  GxSOnly(UINT lB, UINT rB, UINT tB, UINT bB);
  virtual ~GxSOnly(void);

  virtual GxGeomControl* Clone(void) const;

  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);

  virtual UINT LBorder(void) const;
  virtual UINT RBorder(void) const;
  virtual UINT TBorder(void) const;
  virtual UINT BBorder(void) const;

protected:
  UINT lBorder;
  UINT rBorder;
  UINT tBorder;
  UINT bBorder;
};

//same as above
class GxSIOnly : public GxGeomControl
{
public:
  GxSIOnly(UINT lB, UINT rB, UINT tB, UINT bB);
  virtual ~GxSIOnly(void);

  virtual GxGeomControl* Clone(void) const;

  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);

  virtual UINT LBorder(void) const;
  virtual UINT RBorder(void) const;
  virtual UINT TBorder(void) const;
  virtual UINT BBorder(void) const;

protected:
  UINT lBorder;
  UINT rBorder;
  UINT tBorder;
  UINT bBorder;
};

//a class good for locating the first major objects on a GxMainWin
//this sizes the object before placing it
class GxBasic : public GxGeomControl
{
public:
  GxBasic(GX_WD_STAT tPWidth, GX_HT_STAT tPHeight,
	  GX_H_PLACEMENT tHPlace, GX_V_PLACEMENT tVPlace,
	  bool tHReset = false, bool tVReset = false);
  virtual ~GxBasic(void);

  virtual GxGeomControl* Clone(void) const;

  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);

//   virtual GX_WD_STAT GetWidthStat(void) const;
//   virtual GX_HT_STAT GetHeightStat(void) const;

protected:
  GX_H_PLACEMENT hPlace;
  GX_V_PLACEMENT vPlace;
  GX_WD_STAT pWidth;
  GX_HT_STAT pHeight;

  bool hReset, vReset;
};

class GxSBasic : public GxBasic
{
public:
  GxSBasic(GX_WD_STAT tPWidth, GX_HT_STAT tPHeight,
	   GX_H_PLACEMENT tHPlace, GX_V_PLACEMENT tVPlace,
	   UINT leftBorder, UINT rightBorder,
	   UINT topBorder, UINT bottomBorder,
	   bool tHReset = false, bool tVReset = false);
  virtual ~GxSBasic(void);

  virtual GxGeomControl* Clone(void) const;

  virtual void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
			  int &tY, int &bY);

  virtual UINT LBorder(void) const;
  virtual UINT RBorder(void) const;
  virtual UINT TBorder(void) const;
  virtual UINT BBorder(void) const;

protected:
  UINT lBorder;
  UINT rBorder;
  UINT tBorder;
  UINT bBorder;
};

//the previous uses it spaces in pixel values, this uses incrents of some pixel
//increment.  this increment at some point may be configurable
class GxSIBasic : public GxBasic
{
public:
  GxSIBasic(void);
  GxSIBasic(GX_WD_STAT tPWidth, GX_HT_STAT tPHeight,
	    GX_H_PLACEMENT tHPlace, GX_V_PLACEMENT tVPlace,
	    UINT leftBorder, UINT rightBorder,
	    UINT topBorder, UINT bottomBorder,
	    bool tHReset = false, bool tVReset = false);
  virtual ~GxSIBasic(void);

  virtual GxGeomControl* Clone(void) const;

  void PlaceOwner(GxWinArea *pObjectToControl, int &lX, int &rX,
		  int &tY, int &bY);

  virtual UINT LBorder(void) const;
  virtual UINT RBorder(void) const;
  virtual UINT TBorder(void) const;
  virtual UINT BBorder(void) const;

protected:
  //we consider the lBorder, etc to be increments of 15 pixels
  UINT lBorder;
  UINT rBorder;
  UINT tBorder;
  UINT bBorder;
};

class GxWDInd
{
protected:
  GxWDInd(void);

public:
  virtual ~GxWDInd(void);

  virtual void SizeWidth(GxWinArea *pObject, int &lX, int &rX);
  virtual GxWDInd * Clone(void) const;
};

class GxWDProp : public GxWDInd
{
public:
  GxWDProp(GxWinArea *pObjectPropTo, const GxFraction &rProp);
  ~GxWDProp(void);

  virtual void SizeWidth(GxWinArea *pObject, int &lX, int &rX);
  virtual GxWDInd * Clone(void) const;

protected:
  GxWinArea *pObj;
  GxFraction prop;
};

class GxWDBasic : public GxWDInd
{
public:
  GxWDBasic(GX_WD_STAT wStat);
  virtual ~GxWDBasic(void);

  virtual void SizeWidth(GxWinArea *pObject, int &lX, int &rX);
  virtual GxWDInd * Clone(void) const;

protected:
  GX_WD_STAT wdStat;
  bool resetH;
};

class GxHTInd
{
public:
  GxHTInd(void);
  virtual ~GxHTInd(void);

  virtual void SizeHeight(GxWinArea *pObject, int &tY, int &bY);
  virtual GxHTInd * Clone(void) const;
};

class GxHTProp : public GxHTInd
{
public:
  GxHTProp(GxWinArea *pObjectPropTo, const GxFraction &rProp);
  ~GxHTProp(void);

  virtual void SizeHeight(GxWinArea *pObject, int &tY, int &bY);
  virtual GxHTInd * Clone(void) const;

protected:
  GxWinArea *pObj;
  GxFraction prop;
};

class GxHTBasic : public GxHTInd
{
public:
  GxHTBasic(GX_HT_STAT tHeightStat);
  virtual ~GxHTBasic(void);

  virtual void SizeHeight(GxWinArea *pObject, int &tY, int &bY);
  virtual GxHTInd * Clone(void) const;

protected:
  GX_HT_STAT htStat;
};


class GxPlaceHInd
{
public:
  GxPlaceHInd(void);
  virtual ~GxPlaceHInd(void);

  virtual void PlaceH(GxWinArea *pObject, int &lX, int &rX);
  virtual GxPlaceHInd * Clone(void) const;
};

class GxHRel : public GxPlaceHInd
{
public:
  GxHRel(GX_ATTACHMENT tObjAttach, GxWinArea *pTRelTo,
	 GX_ATTACHMENT tRelAttach, int Gap = 0);
  virtual ~GxHRel(void);

  virtual void PlaceH(GxWinArea *pObject, int &lX, int &rX);
  virtual GxPlaceHInd * Clone(void) const;

protected:
  GxWinArea *pRelTo;
  GX_ATTACHMENT objAttach, relAttach;
  int gap;
};

class GxHBasic : public GxPlaceHInd
{
public:
  GxHBasic(GX_H_PLACEMENT hPlacement, bool reset = false);
  virtual ~GxHBasic(void);

  virtual void PlaceH(GxWinArea *pObject, int &lX, int &rX);
  virtual GxPlaceHInd * Clone(void) const;

protected:
  GX_H_PLACEMENT hPlace;
  bool resetH;
};

class GxPlaceVInd
{
public:
  GxPlaceVInd(void);
  virtual ~GxPlaceVInd(void);

  virtual void PlaceV(GxWinArea *pObject, int &tY, int &bY);
  virtual GxPlaceVInd * Clone(void) const;
};

class GxVRel : public GxPlaceVInd
{
public:
  GxVRel(GX_ATTACHMENT tObjAttach, GxWinArea *pTRelTo,
	 GX_ATTACHMENT tRelAttach, int Gap = 0);
  virtual ~GxVRel(void);

  virtual void PlaceV(GxWinArea *pObject, int &tY, int &bY);
  virtual GxPlaceVInd * Clone(void) const;

protected:
  GxWinArea *pRelTo;
  GX_ATTACHMENT objAttach, relAttach;
  int gap;
};

class GxVBasic : public GxPlaceVInd
{
public:
  GxVBasic(GX_V_PLACEMENT vPlacement, bool reset = false);
  virtual ~GxVBasic(void);

  virtual void PlaceV(GxWinArea *pObject, int &tY, int &bY);
  virtual GxPlaceVInd * Clone(void) const;

protected:
  GX_V_PLACEMENT vPlace;
  bool resetV;
};

//sizes width first, height next, then hPlace then vPlace
class GxInd : public GxGeomControl
{
public:
  GxInd(const GxWDInd &rWdInd, const GxHTInd &rHtInd,
	const GxPlaceHInd &rPlaceHInd, const GxPlaceVInd &rPlaceVInd);
  virtual ~GxInd(void);

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

#endif //GXGEOMCONTROL_INCLUDED
