#ifndef GXCOREWINMAP_INCLUDED
#define GXCOREWINMAP_INCLUDED

#include <libGx/GxInc.hh>

class GxCoreWin;
//note: this is my first attempt at creating a hash table so this
//class most assuredly needs professional help.
//If you want to volunter; please do so.

//this implementation uses double hashing

//note both of these must be prime
const UINT GX_MAP_NODE_SIZE = 103;
const UINT GX_OTHER_HASH = GX_MAP_NODE_SIZE - 2;

//the number of times we search a GxMapNode before moving on to the next
//both in adding a node and deleting a node
const UINT GX_NUM_NODE_RETRIES = 3;

class GxCoreWinMap
{
public:
  GxCoreWinMap(void);
  ~GxCoreWinMap(void);

  void ManageWin(GxCoreWin *pNewWin, UINT winId);
  //if the GxCoreWin winId does not exist in this map (very possible)
  //we return a null pointer
  GxCoreWin *GetWin(UINT winId);
  void RemoveWin(UINT winId);
private:
  class GxMapNode
  {
  private:
    friend class GxCoreWinMap;

    GxMapNode(void);
    ~GxMapNode(void);

    GxCoreWin* WinMatrix[GX_MAP_NODE_SIZE];
    
    GxMapNode *pNextNode;
  };

  GxMapNode *pFirstNode;
};

#endif //GXCOREWINMAP_INCLUDED
