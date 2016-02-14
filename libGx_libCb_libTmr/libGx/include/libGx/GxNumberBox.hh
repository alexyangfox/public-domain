#ifndef GXNUMBERBOX_INCLUDED
#define GXNUMBERBOX_INCLUDED

#include <sstream>

#include <libGx/GxInc.hh>
#include <libGx/GxGhost.hh>
#include <libGx/GxEditWin.hh>
#include <libGx/GxFilledArrowButton.hh>

class GxNumberBox : public GxGhost
{
public:
  GxNumberBox(GxRealOwner *pOwner);
  virtual ~GxNumberBox(void);

  void Value(int newNum);
  int Value(void);

  void SetIncrement(unsigned inc); //one by default

  void Minimum(int newMinValue); //-10 by default
  void Maximum(int newMaxValue); //10 by default

  void NumDecimalPlaces(unsigned tNumDecimalPlaces); //zero by default
  unsigned NumDecimalPlaces(void) const;

  //this does not change our value but it does set the displayed string
  //this is forgotten on an up or down
  void SetText(const char *pText);

  //if no inactive text, it this should just display value
  void SetInactiveText(const char *pStr);

  void Active(bool newState);
  bool Active(void) const;

  //should we make the edit win public?
  void SetNumVisibleChars(UINT newNum);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;
  void PlaceChildren(void);

  //write the current value to the editWin. made public so if the user changes
  //the configuration (but not value) the UI can be updated
  void WriteValue(void);

  CbVoidFO modifyCB;

protected:
  GxEditWin editWin;
  GxFilledArrowButton upButton, downButton;

  char inactiveText[GX_DEFAULT_LABEL_LEN];

  unsigned numDecimalPlaces;

  //this is scaled to 1 by dividing by numDecimalPlaces*10
  int value;

  unsigned increment;

  int minValue;
  int maxValue;

  bool active;

  void EditWinModifyCB(void);

  void CountUp(void);
  void CountDown(void);
};

#endif //GXNUMBERBOX_INCLUDED
