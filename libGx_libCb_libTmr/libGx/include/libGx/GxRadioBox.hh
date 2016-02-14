#ifndef GXRADIOBOX_INCLUDED
#define GXRADIOBOX_INCLUDED

#include <libGx/GxOwnerWin.hh>
#include <libGx/GxToggleButton.hh>

class GxRadioBox;

class GxRadioButton : public GxToggleButton
{
public:
  GxRadioButton(GxRadioBox *pOwner, const char *pLabel = NULL);
  virtual ~GxRadioButton(void);

  virtual void State(bool newState);
  virtual void CutOff(void);
protected:
  virtual void DoAction(void);
};

class GxRadioBox : public GxOwnerWin
{
public:
  GxRadioBox(GxRealOwner *pOwner);
  virtual ~GxRadioBox(void);

  //if true (the default) only one item may be selected at a time
  void SetRadioBehavior(bool newStat);
  void SetNum(UINT numberPer); //number buttons per row or column
  //if formating into columns numberPer is number of items per column or the
  //number per row. if formating into rows numberPer is the number of items
  //per row or the number of columns
  void SetFormating(bool format); // TRUE is rows FALSE is columns

  void SetActive(bool nActive); //a utility funcions. calls SetActive on all children

  //GxOwner overload; like the GxOwner behaviour, removes the object,
  // does not delete it. (this calls GxOwner::RemoveChild internally)
  //this just removes the object from pToggleList too.
  virtual void RemoveChild(GxWinArea *pChild);

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void PlaceChildren(void);

  //called by children's constructor
  virtual void AddToggleButton(GxRadioButton *pNewButton);
  //called by a child being activated; called before the child calls its cb
  virtual void ChildActivated(GxRadioButton *pActivatedChild);

protected:
  bool radio;
  std::list<GxRadioButton*> radioList;

  UINT numPer; //row or column (rows only now)
  bool order; // TRUE is rows FALSE is columns
};

#endif //GXRADIOBOX_INCLUDED
