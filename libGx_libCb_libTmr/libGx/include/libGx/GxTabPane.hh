#ifndef GXTABPANE_INCLUDED
#define GXTABPANE_INCLUDED

#include <libGx/GxOwnerWin.hh>

class GxTabManager;

class GxTabPane : public GxOwnerWin //?hack? I'd prefer a ghost
{
public:
  GxTabPane(GxTabManager *pOwner);
  virtual ~GxTabPane(void);

  void Hide(void);
  void SetName(const char *pName);
  const char* GetName(void) const;

protected:
  char tabName[GX_DEFAULT_LABEL_LEN];
};

#endif //GXTABPANE_INCLUDED
