#ifndef GXMISCDIALOGS_INCLUDED
#define GXMISCDIALOGS_INCLUDED

#include <libCb/CbCallback.hh>

#include <libGx/GxPopupWin.hh>
#include <libGx/GxButton.hh>
#include <libGx/GxTextWin.hh>
#include <libGx/GxHLine.hh>
#include <libGx/GxRow.hh>
#include <libGx/GxGeomControl.hh>
#include <libGx/GxPercentBar.hh>

// displays a long message in a text win, with an ok button which dismisses
// the dialog
class GxMessageDialog : public GxPopupWin
{
public:
  GxMessageDialog(GxTopLevelWin *pOwner);
  virtual ~GxMessageDialog(void);

  void SetMessage(const char *pText);

  //if using external event loops use this rather than DoDialog
  CbOneFO<GX_STATUS> resultCB;

  //this only returns GX_OK
  GX_STATUS DoDialog(void);
protected:
  GxButton okButton;
  GxHLine hLine;
  GxTextWin textWin;

  void ButtonCB(void);
};

//a dialog like the above but with a cancel button too.
class GxConfirmDialog : public GxPopupWin
{
public:
  GxConfirmDialog(GxTopLevelWin *pOwner);
  virtual ~GxConfirmDialog(void);

  void SetMessage(const char *pText);

  //if using external event loops use this rather than DoDialog
  CbOneFO<GX_STATUS> resultCB;

  GX_STATUS DoDialog(void);
protected:
  GxRow buttonRow;
  GxButton okB, cancelB;

  GxHLine hLine;
  GxTextWin textWin;

  void ButtonCB(GX_STATUS bPressed);
  GxTextWin *pTWin;
  GX_STATUS rStat;
};


//displays a short message with yes/no buttons below
class GxYesNoDialog : public GxPopupWin
{
public:
  GxYesNoDialog(GxTopLevelWin *pOwner);
  virtual ~GxYesNoDialog(void);

  void SetMessage(const char *pMessage);

  //returns GX_YES or GX_NO
  GX_STATUS DoDialog(void);
protected:
  GxRow buttonRow;
  GxButton yesB, noB;

  GxHLine buttonLine;

  GxTextWin textWin;

  void ButtonCB(GX_STATUS tStat);
  GX_STATUS retStat;
};

//displays a short message with yes/no/cancel buttons below
class GxYesNoCancelDialog : public GxPopupWin
{
public:
  GxYesNoCancelDialog(GxTopLevelWin *pOwner);
  virtual ~GxYesNoCancelDialog(void);

  void SetMessage(const char *pMessage);

  //returns GX_YES, GX_NO, or GX_CANCELED
  GX_STATUS DoDialog(void);
protected:
  GxRow buttonRow;
  GxButton yesB, noB, cancelB;

  GxHLine buttonLine;

  GxTextWin textWin;
  
  void ButtonCB(GX_STATUS tStat);
  GX_STATUS retStat;
};

//a dialog with a percent bar and a cancel button
//this dialog is intrinsicly driven by an external process and therefore
//does not have a DoDialog function.
class GxStatusDialog : public GxPopupWin
{
public:
  GxStatusDialog(GxTopLevelWin *pOwner);
  virtual ~GxStatusDialog(void);

  //pass in an integer from 0 to 100. invalid values are forced to 100.
  void SetPercent(UINT percent);

  bool cancelAction; //the cancelButton sets this true
protected:
  void CancelCB(void);

  GxButton cancelButton;
  GxPercentBar percentBar;
};

#endif //GXMISCDIALOGS_INCLUDED
