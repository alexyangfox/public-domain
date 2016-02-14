#ifndef COLORDEFDIALOG_INCLUDED
#define COLORDEFDIALOG_INCLUDED

#include <string>

#include <libCb/CbCallback.hh>

#include <libGx/GxGeomControl.hh>
#include <libGx/GxColumn.hh>
#include <libGx/GxEditWin.hh>
#include <libGx/GxPopupWin.hh>
#include <libGx/GxRow.hh>
#include <libGx/GxButton.hh>
#include <libGx/GxHLine.hh>

class ColorDefDialog : public GxPopupWin
{
public:
  ColorDefDialog(GxTopLevelWin *pOwner);
  virtual ~ColorDefDialog(void);

  bool DoDialog(std::string &rName);

protected:
  void ExitCB(bool tExitStat);

  GxThinColumn mainColumn;

  GxRow buttonRow;
  GxButton ok, cancel;

  GxHLine hLine;

  GxEditWin nameWin;

  bool exitStat;
};

#endif //COLORDEFDIALOG_INCLUDED
