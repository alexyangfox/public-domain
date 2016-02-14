#include "ImgData.hh"

#include <math.h>
#include <assert.h>
using namespace std;

ImgData::ImgData(void) :
  modified(false), width(0), height(0)
{}

ImgData::~ImgData(void)
{}

bool ImgData::Modified(void) const
{
  return modified;
}

void ImgData::ClearModified(void)
{
  modified = false;
}

void ImgData::Clear(const PixValue &rValue)
{
  vector<PixValue>::iterator cPlace = data.begin();
  vector<PixValue>::iterator cEnd = data.end();
  while(cPlace != cEnd)
    {
      *cPlace = rValue;
      cPlace++;
    };

  modified = true;
}

void ImgData::ClearSelected(void)
{
  vector<PixValue>::iterator cPlace = data.begin();
  vector<PixValue>::iterator cEnd = data.end();
  while(cPlace != cEnd)
    {
      (*cPlace).selected = false;
      cPlace++;
    };
}

void ImgData::ModifySelected(const PixValue &rValue, PIX_FIELD modifiers)
{
  for(unsigned ii = 0; ii < width; ii++)
    for(unsigned jj = 0; jj < height; jj++)
      {
	const PixValue& rValue = GetValue(ii, jj);
	if(rValue.selected)
	  {
	    SetValue(ii, jj, rValue, modifiers);
	    modified = true;
	  };
      };

}

unsigned ImgData::Width(void) const
{
  return width;
}

unsigned ImgData::Height(void) const
{
  return height;
}

void ImgData::SafeResize(unsigned newWidth, unsigned newHeight, const PixValue &rFillValue)
{
  assert(newWidth && newHeight);

  modified = true;

  //make a local copy of our image
  vector<PixValue> localData(data);
  unsigned localWidth = width;
  unsigned localHeight = height;

  width = newWidth;
  height = newHeight;
  data.resize(newWidth*newHeight);  

  //now copy the elements from the old image that can be saved to the new Image
  unsigned widthToCopy = (newWidth > localWidth) ? localWidth : newWidth;
  unsigned heightToCopy = (newHeight > localHeight) ? localHeight : newHeight;

  for(unsigned x = 0; x < widthToCopy; x++)
    for(unsigned y = 0; y < heightToCopy; y++)
      {
	PixValue &rValue = localData[y*localWidth+x];
	data[y*width+x] = rValue;
      };

  for(unsigned x = widthToCopy; x < newWidth; x++)
    for(unsigned y = 0; y < newHeight; y++)
      data[y*width+x] = rFillValue;

  for(unsigned y = heightToCopy; y < newHeight; y++)
    for(unsigned x = 0; x < newWidth; x++)
      data[y*width+x] = rFillValue;
}

void ImgData::ResizeAndClear(unsigned newWidth, unsigned newHeight, const PixValue &rClearValue)
{
  assert(newWidth && newHeight);

  modified = true;

  width = newWidth;
  height = newHeight;
  data.resize(newWidth*newHeight);  

  Clear(rClearValue);
}

void ImgData::SetValue(unsigned x, unsigned y, const PixValue &rValue, PIX_FIELD modifiers)
{
  modified = true;

  PixValue &rLocalValue = GetValue(x,y);
  if(modifiers & PIX_COLOR)
    rLocalValue.xcolor      = rValue.xcolor;

  if(modifiers & PIX_MASKED)
    rLocalValue.masked = rValue.masked;

  if(modifiers & PIX_SELECTED)
    rLocalValue.selected = rValue.selected;
}

void ImgData::FillArea(unsigned x1, unsigned y1, unsigned fWidth, unsigned fHeight, const PixValue &rValue, PIX_FIELD modifiers)
{
  assert(fWidth && fHeight);
  assert(x1 < width && y1 < height);

  modified = true;

  for(unsigned cY = y1; cY < height && cY < y1+fHeight; cY++)
    for(unsigned cX = x1; cX < width && cX < x1+fWidth; cX++)
      SetValue(cX, cY, rValue, modifiers);
}

void ImgData::DrawLine(unsigned x1, unsigned y1, unsigned x2, unsigned y2, const PixValue &rValue, PIX_FIELD modifiers)
{
  modified = true;

  if( fabs( (float)x2 - (float)x1 ) > fabs( (float)y2 - (float)y1) )
    {
      unsigned startX = x1;
      unsigned endX   = x2;
      unsigned startY = y1;
      unsigned endY   = y2;
      if(x2 < startX)
	{
	  startX = x2;
	  endX = x1;
	  startY = y2;
	  endY = y1;
	};
      float m = ((float)endY - (float)startY)/(float)(endX - startX);
      for(unsigned cX = startX; cX <= endX; cX++)
	{
	  unsigned yVal = startY + (unsigned)rint( m*((float)(cX-startX)) );
	  SetValue(cX, yVal, rValue, modifiers);
	};
    }else
      {
	unsigned startX = x1;
	unsigned endX   = x2;
	unsigned startY = y1;
	unsigned endY   = y2;
	if(y2 < y1)
	  {
	    startX = x2;
	    endX = x1;
	    startY = y2;
	    endY = y1;
	  };
	float m = ((float)endX - (float)startX)/(float)(endY - startY);
	for(unsigned cY = startY; cY <= endY; cY++)
	  {
	    unsigned xVal = startX + (unsigned)rint( m*((float)(cY-startY)) );
	    SetValue(xVal, cY, rValue, modifiers);
	  };
      };
}


void ImgData::MoveBlock(unsigned blockWidth, unsigned blockHeight, const PixCoord &from, const PixCoord &to)
{
  //have to move one pixel at a time
  modified = true;

  int vInc;
  int vOffset = to.y - from.y;
  int vStart, vEnd;
  if(to.y < from.y) //movingUp (remember y=0 is top left corner)
    {
      vStart = from.y;
      vInc = 1;
      vEnd = from.y + blockHeight;
    }else
      {
	vStart = from.y + blockHeight - 1;
	vInc = -1;
	vEnd = from.y -1;
      };

  int hInc;
  int hOffset = to.x - from.x;
  int hStart, hEnd;
  if(to.x > from.x) //movingRight
    {
      hStart = from.x + blockWidth-1;
      hInc = -1;
      hEnd = from.x-1;
    }else
      {
	hStart = from.x;
	hInc = 1;
	hEnd = from.x + blockWidth;
      };
  /* hack. fix this.
  for(int x = hStart; x != hEnd; x += hInc)
    for(int y = vStart; y != vEnd; y += vInc)
      {
	unsigned long pixVal;
	pixVal = XGetPixel(pXImage,x,y);
	XPutPixel(pXImage, x+hOffset,y+vOffset, pixVal);
	//write a white pixel into the place just moved from
	XPutPixel(pXImage, x,y, xData.whitePix);
	if(pXShapeImage)
	  {
	    pixVal = XGetPixel(pXShapeImage, x,y);
	    XPutPixel(pXShapeImage, x+hOffset,y+vOffset, pixVal);
	    XPutPixel(pXImage, x,y, xData.whitePix);
	  };
      };
  */
}

const PixValue & ImgData::GetValue(unsigned tx, unsigned ty) const
{
  assert( tx < width && ty < height);

  return data[ty*width+tx];
}

PixValue& ImgData::GetValue(unsigned tx, unsigned ty)
{
  assert( tx < width && ty < height);

  return data[ty*width+tx];
}
