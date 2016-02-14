#include <libGx/GxCoreWinMap.hh>

#include <libGx/GxCoreWin.hh>


GxCoreWinMap::GxMapNode::GxMapNode(void)
{
  for(UINT i = 0; i < GX_MAP_NODE_SIZE; i++)
    {
      WinMatrix[i] = NULL;
    };
  pNextNode = NULL;
}

GxCoreWinMap::GxMapNode::~GxMapNode(void)
{}

// ******************** start GxMapWin *********************

GxCoreWinMap::GxCoreWinMap(void)
{
  pFirstNode = new GxMapNode;
}

GxCoreWinMap::~GxCoreWinMap(void)
{
  //traversal of the list for every node to delete is a bit ugly.
  //fortunatly this is not done often, and the list is not long.
  if(pFirstNode->pNextNode == 0)
    {
      delete pFirstNode;
      pFirstNode = 0;
    }else
      {
	while(pFirstNode)
	  {
	    GxMapNode *pPrevNode = pFirstNode;
	    GxMapNode *pCNode = pFirstNode->pNextNode;
	    while(pCNode->pNextNode)
	      {
		pPrevNode = pCNode;
		pCNode = pCNode->pNextNode;
	      };
	    
	    delete pCNode;
	    pCNode = 0;
	    pPrevNode->pNextNode = 0;
	    
	    if(pPrevNode == pFirstNode)
	      {
		delete pFirstNode;
		pFirstNode = 0;
	      };
	  }
      };
}

void GxCoreWinMap::ManageWin(GxCoreWin *pNewWin, UINT winId)
{
  GxMapNode *pCurrentNode = pFirstNode;
  GxMapNode *pPrevNode = NULL;

  UINT index = winId%GX_MAP_NODE_SIZE;

  while(pCurrentNode)
    {
      //see if we can fit the new GxCoreWin into the currentNode
      if( !pCurrentNode->WinMatrix[index] )
	{
	  //the space is empty so go ahead and add the GxCoreWin and return
	  pCurrentNode->WinMatrix[index] = pNewWin;
	  return;
	};
      //if we are here we have to calculate a step size and try elsewhere
      //in the WinMatrix of the current node
      UINT step = ((winId%(GX_MAP_NODE_SIZE - 2)) + 1);
      //if i started with 0 we would retry the same space we tried first
      for(UINT i = 1; i < (GX_NUM_NODE_RETRIES+1); i++)
	{
	  //we will try GX_NUM_NODE_RETRIES to find an empty space in the
	  //current Node's WinMatrix before moveing on to the next node
	  UINT newIndex = ((index + (i*step))%GX_MAP_NODE_SIZE);
	  if( !pCurrentNode->WinMatrix[newIndex] )
	    {
	      pCurrentNode->WinMatrix[newIndex] = pNewWin;
	      return;
	    };
	};

      pPrevNode = pCurrentNode;
      pCurrentNode = pCurrentNode->pNextNode;
    };

  //if were here we must add a new node; note-pPrevNode must be valid
  pPrevNode->pNextNode = new GxMapNode;
  pPrevNode->pNextNode->WinMatrix[index] = pNewWin;
}

GxCoreWin *GxCoreWinMap::GetWin(UINT winId)
{
  GxMapNode *pCurrentNode = pFirstNode;

  UINT index = winId%GX_MAP_NODE_SIZE;

  while(pCurrentNode)
    {
      //see if our first guess is correct
      if( pCurrentNode->WinMatrix[index] )
	{
	  //the pointer is valid (we hope) so see if this is really the
	  //correct window
	  if( (pCurrentNode->WinMatrix[index])->GetWindow() == winId )
	    return pCurrentNode->WinMatrix[index];
	};
      //if we are here we have to calculate a step size and try elsewhere
      //in the WinMatrix of the current node
      UINT step = ((winId%(GX_MAP_NODE_SIZE - 2)) + 1);
      //if i started with 0 we would retry the same space we tried first
      for(UINT i = 1; i < (GX_NUM_NODE_RETRIES+1); i++)
	{
	  //we will try GX_NUM_NODE_RETRIES to find the correct window in the
	  //current Node's WinMatrix before moveing on to the next node
	  UINT newIndex = ((index + (i*step))%GX_MAP_NODE_SIZE);
	  if( pCurrentNode->WinMatrix[newIndex] )
	    {
	      //the pointer is valid (we hope) so see if this is really the
	      //correct window
	      if( (pCurrentNode->WinMatrix[newIndex])->GetWindow() == winId )
		return pCurrentNode->WinMatrix[newIndex];
	    };
	};

      pCurrentNode = pCurrentNode->pNextNode;
    };


  //the window was not in our data
  //if were here we do not have a GxCoreWin of that id, so
  //we will return a NULL*
  return (GxCoreWin*)NULL;
}

void GxCoreWinMap::RemoveWin(UINT winId)
{
  //when we find the correct window, just set its pointer to NULL as this
  //funcion was called from the window's distructor anyway

  GxMapNode *pCurrentNode = pFirstNode;

  UINT index = winId%GX_MAP_NODE_SIZE;

  while(pCurrentNode)
    {
      //see if our first guess is correct
      if( pCurrentNode->WinMatrix[index] )
	{
	  //the pointer is valid (we hope) so see if this is really the
	  //correct window
	  if( (pCurrentNode->WinMatrix[index])->GetWindow() == winId )
	    {
	      pCurrentNode->WinMatrix[index] = NULL;
	      return;
	    };
	};
      //if we are here we have to calculate a step size and try elsewhere
      //in the WinMatrix of the current node
      UINT step = ((winId%(GX_MAP_NODE_SIZE - 2)) + 1);
      //if i started with 0 we would retry the same space we tried first
      for(UINT i = 1; i < (GX_NUM_NODE_RETRIES+1); i++)
	{
	  //we will try GX_NUM_NODE_RETRIES to find the correct window in the
	  //current Node's WinMatrix before moveing on to the next node
	  UINT newIndex = ((index + (i*step))%GX_MAP_NODE_SIZE);
	  if( pCurrentNode->WinMatrix[newIndex] )
	    {
	      //the pointer is valid (we hope) so see if this is really the
	      //correct window
	      if( (pCurrentNode->WinMatrix[newIndex])->GetWindow() == winId )
		{
		  pCurrentNode->WinMatrix[newIndex] = NULL;
		  return;
		};
	    };
	};

      pCurrentNode = pCurrentNode->pNextNode;
    };
}

