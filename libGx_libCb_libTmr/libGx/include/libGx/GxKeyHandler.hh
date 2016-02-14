#ifndef GXKEYHANDLER_INCLUDED
#define GXKEYHANDLER_INCLUDED

#include <libGx/GxInc.hh>
#include <X11/keysym.h>

class GxKeyHandler
{
public:
  virtual ~GxKeyHandler(void);

protected:
  GxKeyHandler(void);

  virtual void HandleKeyEvent(const XKeyEvent &rKEvent);

  //pBuffer is not null terminated
  virtual void HandleKeyString(const char *pBuffer, unsigned long buffLen) = 0;

  virtual void MoveCursorLineStart(void);
  virtual void MoveCursorLineEnd(void);
  virtual void KillTextToLineEnd(void); //from cursor

  virtual void KeyLeft(void);
  virtual void KeyRight(void);
  virtual void KeyUp(void);
  virtual void KeyDown(void);

  //delete the character to the right of the cursor
  virtual void KeyDelete(void);
  //delete the character to the left of the cursor
  virtual void KeyBackspace(void);

  virtual void KeyEnter(void);
};

#endif //GXKEYHANDLER_INCLUDED
