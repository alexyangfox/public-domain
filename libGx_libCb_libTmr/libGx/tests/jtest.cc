#include <iostream.h>
#include <stdlib.h>
#include <g++/bool.h>
#include <X11/Xlib.h>

Window myWin;
Display *display;
int screenNum;

void GrabEventLoop(void);

int main(void)
{
  char *pDispName = getenv("DISPLAY");
  display = XOpenDisplay(pDispName);
  if(!display)
    {
      cerr << "Display evironmental variable not/incorrecctly set" << endl;
      exit(1);
    };

  screenNum = DefaultScreen(display);

  myWin = XCreateSimpleWindow(display, RootWindow(display, screenNum),
			      0,0, 100,100, 0,
			      BlackPixel(display, screenNum),
			      WhitePixel(display, screenNum) );
  XSelectInput(display, myWin, ButtonPressMask);
  XMapWindow(display, myWin);

  XEvent event;
  while(1)
    {
      XNextEvent(display, &event);
      if(event.type == ButtonPress)
	GrabEventLoop();
    };

  return 0;
};

void GrabEventLoop(void)
{
  cout << "Grabing pointer" << endl;

  //uncoment this and coment out below to demonstrate
  /*
  XGrabPointer(display, myWin, TRUE, ButtonReleaseMask | ExposureMask,
	       GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
  */

  //*
  XGrabPointer(display, myWin, TRUE, ButtonReleaseMask,
	       GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
  //*/

  XEvent event;
  while(1)
    {
      XNextEvent(display, &event);

      if(event.type == ButtonRelease)
	{
	  cout << "Natural Ungrab" << endl;
	  XUngrabPointer(display, CurrentTime);
	  break;
	};
    };
}
