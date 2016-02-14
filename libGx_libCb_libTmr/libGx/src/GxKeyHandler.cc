#include <libGx/GxKeyHandler.hh>

using namespace std;

GxKeyHandler::~GxKeyHandler(void)
{}

GxKeyHandler::GxKeyHandler(void)
{}

void GxKeyHandler::HandleKeyEvent(const XKeyEvent &rKEvent)
{
  char lookupBuffer[512];
  KeySym kSym;
  XComposeStatus compose;
  //hack: I'm assuming the cast is safe
  int numChar = XLookupString((XKeyEvent*)&rKEvent, lookupBuffer, 512, &kSym, &compose);

  //make sure it is NULL terminated
  //  lookupBuffer[numChar+1] = '\0';

  if( rKEvent.state & ControlMask )
    {
      if(kSym == XK_A ||  kSym == XK_a )
	{
	  MoveCursorLineStart();
	  return;
	};

      if(kSym == XK_E ||  kSym == XK_e )
	{
	  MoveCursorLineEnd();
	  return;
	};

      if(kSym == XK_K ||  kSym == XK_k )
	{
	  KillTextToLineEnd();
	  return;
	};
    };


  if((kSym == XK_Return) || (kSym == XK_KP_Enter) || (kSym == XK_Linefeed))
    {
      KeyEnter();
      return;
    };

  if(kSym == XK_Delete)
    {
      //remove the character imediatly to the right of the cursor
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxEditWin::HandleKeyEvent Delete" << endl;
#endif //LIBGX_DEBUG_BUILD
      KeyDelete();
      return;
    };

  if(kSym == XK_BackSpace)
    {
      KeyBackspace();
      return;
    };

  if(kSym == XK_Left)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxKeyHandler::HandleKeyEvent move left" << endl;
#endif //LIBGX_DEBUG_BUILD
      KeyLeft();
      return;
    };

  if(kSym == XK_Right)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxKeyHandler::HandleKeyEvent move right" << endl;
#endif //LIBGX_DEBUG_BUILD
      KeyRight();
      return;
    };

  if(kSym == XK_Up)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxKeyHandler::HandleKeyEvent move up" << endl;
#endif //LIBGX_DEBUG_BUILD
      KeyUp();
      return;
    };

  if(kSym == XK_Down)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxKeyHandler::HandleKeyEvent move down" << endl;
#endif //LIBGX_DEBUG_BUILD
      KeyDown();
      return;
    };

  // *************** handle normal characters here *****************
  if( ((kSym >= XK_KP_Space) && (kSym <= XK_KP_9)) ||
      ((kSym >= XK_space) && (kSym <= XK_asciitilde)) )
    {
      HandleKeyString(lookupBuffer, numChar);
      return;
    };

#ifdef LIBGX_DEBUG_BUILD
  //XBell(dInfo.display, 100);
  cout << "Unhandled Keysym in GxKeyHandler::HandleKeyEvent" << endl;
#endif //LIBGX_DEBUG_BUILD
}

void GxKeyHandler::MoveCursorLineStart(void)
{}

void GxKeyHandler::MoveCursorLineEnd(void)
{}

void GxKeyHandler::KillTextToLineEnd(void)
{}

void GxKeyHandler::KeyLeft(void)
{}

void GxKeyHandler::KeyRight(void)
{}

void GxKeyHandler::KeyUp(void)
{}

void GxKeyHandler::KeyDown(void)
{}

void GxKeyHandler::KeyDelete(void)
{}

void GxKeyHandler::KeyBackspace(void)
{}

void GxKeyHandler::KeyEnter(void)
{}
