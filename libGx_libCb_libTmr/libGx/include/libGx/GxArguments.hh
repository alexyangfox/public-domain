#ifndef GXARGUMENTS_INCLUDED
#define GXARGUMENTS_INCLUDED

#include <string>

/* the command line arguments relevant to a single GxDisplay */
class GxArguments
{
public:
  GxArguments(void);
  GxArguments(const GxArguments &rhs);
  ~GxArguments(void);

  GxArguments& operator=(const GxArguments &rhs);

  std::string displayName; //the display name we are to connect to
  //true if the display should be syncronized
  bool dispSync;

  //if these are non-zero, this is the desired height of the applications main window.
  unsigned mainWinWidth;
  unsigned mainWinHeight;
};

#endif //GXARGUMENTS_INCLUDED
