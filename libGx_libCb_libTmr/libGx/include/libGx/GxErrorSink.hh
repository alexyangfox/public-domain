#ifndef GXERRORSINK_INCLUDED
#define GXERRORSINK_INCLUDED

#include <string>

/* implemented by GxMainInterface */

class GxErrorSink
{
public:
  GxErrorSink(void);
  virtual ~GxErrorSink(void);

  virtual void SinkError(const std::string &rError) = 0;
};

#endif //GXERRORSINK_INCLUDED
