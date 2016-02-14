#include <libGx/GxNumberBox.hh>

GxNumberBox::GxNumberBox(GxRealOwner *pOwner) :
  GxGhost(pOwner), editWin(this), upButton(this), downButton(this),
  numDecimalPlaces(0), value(0), increment(1), minValue(-10), maxValue(10),
  active(true)
{
  inactiveText[0] = '\0';

  editWin.modifyCB.Assign( CbVoidMember<GxNumberBox>
			   (this, &GxNumberBox::EditWinModifyCB) );
  editWin.SetNumVisibleChars(5);

  upButton.SetDirection(GX_UP);
  upButton.cb.Assign( CbVoidMember<GxNumberBox>
		      (this, &GxNumberBox::CountUp) );

  downButton.SetDirection(GX_DOWN);
  downButton.cb.Assign( CbVoidMember<GxNumberBox>
			(this, &GxNumberBox::CountDown) );
  WriteValue();
}

GxNumberBox::~GxNumberBox(void)
{}

void GxNumberBox::Value(int newNum)
{
  if(newNum == value) return;

  value = newNum;
  WriteValue();
}

int GxNumberBox::Value(void)
{
  return value;
}

void GxNumberBox::SetIncrement(unsigned inc)
{
  increment = inc;
}

void GxNumberBox::Minimum(int newMinValue)
{
  minValue = newMinValue;
}

void GxNumberBox::Maximum(int newMaxValue)
{
  maxValue = newMaxValue;
}

void GxNumberBox::NumDecimalPlaces(unsigned tNumDecimalPlaces)
{
  if(tNumDecimalPlaces > 8) return; //?hack?

  numDecimalPlaces = tNumDecimalPlaces;
}

unsigned GxNumberBox::NumDecimalPlaces(void) const
{
  return numDecimalPlaces;
}

void GxNumberBox::SetText(const char *pText)
{
  editWin.SetText(pText);
}

void GxNumberBox::SetInactiveText(const char *pStr)
{
  unsigned junkLen = 0;
  GxSetLabel(inactiveText, GX_DEFAULT_LABEL_LEN, pStr, junkLen);

  if(!active)
    editWin.SetText(inactiveText);
}

void GxNumberBox::Active(bool newState)
{
  active = newState;
  editWin.SetActive(newState);
  if(!active)
    {
      if(inactiveText[0] != '\0')
	editWin.SetText(inactiveText);
    }else
      WriteValue();
}

bool GxNumberBox::Active(void) const
{
  return active;
}

void GxNumberBox::SetNumVisibleChars(UINT newNum)
{
  editWin.SetNumVisibleChars(newNum);
}

UINT GxNumberBox::GetDesiredWidth(void) const
{
  UINT nWidth = editWin.GetDesiredWidth();
  UINT nHeight = editWin.GetDesiredHeight();

  return nWidth + nHeight/2 + 2; //the two pixel gap
}

UINT GxNumberBox::GetDesiredHeight(void) const
{
  return editWin.GetDesiredHeight();  
}

void GxNumberBox::PlaceChildren(void)
{
  //we could implement this by traversing the child list.
  //the first child is the GxEditWin

  //don't forget i'm a ghost
  editWin.Resize(width - height/2 -2,height);
  editWin.Move(x,y);

  //the up button
  upButton.Resize(height/2, height/2);
  upButton.Move(x + width - height/2, y);

  //the down button
  downButton.Resize(height/2, height/2);
  downButton.Move(x + width - height/2, y + height - height/2);
}

void GxNumberBox::EditWinModifyCB(void)
{
  char *pTail;
  const char *pStr = editWin.GetText();
  long tValue = strtol(pStr, &pTail, 10);
  if(pTail == pStr) return; //uknown string. we cannot continue.

  int valueOfOne = 1;
  for(unsigned ii = 0; ii < numDecimalPlaces; ii++)
    valueOfOne*=10;

  if(*pTail != '.')
    { //hack this should only happen if we don't have trailing garbage
      value = tValue*valueOfOne;
      return;
    };

  pStr = ++pTail;
  unsigned decimalVals = strtoul(pStr, &pTail, 10);
  if(pTail == pStr) return;

  while( (int)decimalVals > valueOfOne) //user could have entered 1.99999 with numDecimalPlaces == 2
    decimalVals/=10;

  //I hope this is correct.
  value = tValue*valueOfOne+decimalVals;
  modifyCB();
}

void GxNumberBox::CountUp(void)
{
  if(!active) return;

  value += increment;
  if(value > maxValue)
    value = maxValue;

  WriteValue();
  modifyCB();
}

void GxNumberBox::CountDown(void)
{
  if(!active) return;

  value -= increment;
  if(value < minValue)
    value = minValue;

  WriteValue();
  modifyCB();
}

void GxNumberBox::WriteValue(void)
{
  std::ostringstream numStr;

  int valueOfOne = 1;
  for(unsigned ii = 0; ii < numDecimalPlaces; ii++)
    valueOfOne*=10;

  //may be negative (this is good).
  int onesValue = value/valueOfOne;
  int decimalValue = (value%valueOfOne); //?correct?
  
  //how do we know the locall here? (to switch to comma).
  numStr << onesValue;
  if(numDecimalPlaces)
    {
      numStr << ".";
      numStr.width(numDecimalPlaces);
      numStr.fill('0');
      numStr << decimalValue;
    };

  editWin.SetText( numStr.str().c_str() );
}
