#ifndef MYDIALOG_INCLUDED
#define MYDIALOG_INCLUDED

#include <libGx/GxPopupWin.hh>
#include <libGx/GxButton.hh>
#include <libGx/GxRow.hh>
#include <libGx/GxHLine.hh>
#include <libGx/GxGeomControl.hh>
#include <libGx/GxLabel.hh>
#include <libGx/GxTextWin.hh>

class MyDialog : public GxPopupWin
{
public:
     MyDialog(GxTopLevelWin *pOwner);
     virtual ~MyDialog(void);

     bool DoDialog(void);

protected:
     void Ok(void);
     void Cancel(void);
     bool retStatus;

     /* the order of construction matters here, the placement algorithm depends on it */
     GxRow buttonRow; //the ok/cancel button row

     GxButton ok, cancel; //could be constructed any point after the button row, but makes sense here 

     GxHLine hLine;

     GxLabel textLabel;
     GxTextWin textWin;
};

#endif //MYDIALOG_INCLUDED
