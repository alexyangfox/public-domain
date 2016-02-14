// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : SplitInfoCache serves as a generic cache for saving
                   any type of split information, such as information gain
                   when a given attribute is split on a specific value.
                   The cache simply speeds up the operation of calling
                   build_info_value(attrNum, val) every time.
  Comments     : 
  Enhancements :
  Complexity   :
  History      : Yeogirl Yun, Ronny Kohavi                      2/28/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <SplitInfoCache.h>

/*****************************************************************************
  Description : Destructor
  Comments    :
*****************************************************************************/
CacheInfo::~CacheInfo()
{
   delete sic;
}


/*****************************************************************************
  Description : Constructor. Copies bag and sets infoValue = -REAL_MAX,
                  and initializes infoMatrix array with NULLs.
  Comments    :
*****************************************************************************/
SplitInfoCache::SplitInfoCache(CtrInstanceBag*& aBag)
   : bag(aBag), infoMatrix(0, aBag->get_schema().num_attr())
{
   if (aBag == NULL)
      err << "SplitInfoCache::SplitInfoCache: NULL bag" << fatal_error;

   aBag = NULL;
   numInstances = bag->num_instances();
}

SplitInfoCache::SplitInfoCache(const SplitInfoCache& sic, CtorDummy)
   : bag(new CtrInstanceBag(sic.get_bag(), ctorDummy)),
     infoMatrix(0, sic.get_bag().get_schema().num_attr()),
     numInstances(sic.numInstances)
{
   set_log_options(sic.get_log_options());
}

/***************************************************************************
  Description : OK.  Check that the dummy cache as infoValue of -REAL_MAX
  Comments    :
***************************************************************************/

void SplitInfoCache::OK(int /* level */) const
{
   // empty body for now.
}

/***************************************************************************
  Description : Destructor.  Delete infoMatrix skipping the dummy entries.
  Comments    :
***************************************************************************/

SplitInfoCache::~SplitInfoCache()
{
   delete bag;
   DBG(OK());
}


/*****************************************************************************
  Description : Returns info value at given attribute number and value.
                Remember that attribute number and value together defines
		  one sub bag of the current bag.
  Comments    : Logically const, but physically builds the array entry
*****************************************************************************/
Real SplitInfoCache::info_value(int attrNum, NominalVal val) const
{
   if (infoMatrix[attrNum] == NULL) {
      LOG(2, "building attribute num " << attrNum << endl);
      // Cast constness away because we're logically const
      ((SplitInfoCache*)this)->build_info_values(attrNum);
   }

   ASSERT(infoMatrix[attrNum] != NULL);

   return (*infoMatrix[attrNum])[val].infoValue;
}




/*****************************************************************************
  Description : Deletes small caches.
  Comments    :
*****************************************************************************/
void SplitInfoCache::delete_small_caches(int minInstances)
{
   LOG(2, "deleting small caches... " << endl);
   for (int i = infoMatrix.low(); i <= infoMatrix.high(); i++) {
      if (infoMatrix[i]) 
	 for (int j = infoMatrix[i]->low(); j <= infoMatrix[i]->high(); j++) {
	    CacheInfo& ci = (*infoMatrix[i])[j];
	    if (ci.sic) { // there is a cache here
	       if (ci.sic->num_instances() > minInstances)
		  ci.sic->delete_small_caches(minInstances);
	       else {
		  LOG(2, "deleting one row : " << endl);
		  delete infoMatrix[i];
		  infoMatrix[i] = NULL;
		  break;
	       }
	    }
	 }
   }
}
	 
	 
/***************************************************************************
  Description : Return the number of instances in child bag
  Comments    :
***************************************************************************/

int SplitInfoCache::num_instances(int attrNum, NominalVal val) const
{
   const NominalAttrInfo& nai = get_bag().get_schema().attr_info(attrNum).
      cast_to_nominal();

   if (val < UNKNOWN_NOMINAL_VAL || val > nai.num_values()) 
      err << "SplitInfoCache::num_instances: illegal attribute value : "
	 << val << fatal_error;
   if (attrNum < 0 || attrNum >= get_bag().get_schema().num_attr())
      err << "SplitInfoCache::num_instances: illegal attribute num : "
	 << attrNum << fatal_error;

   const CacheInfo& ci = (*infoMatrix[attrNum])[val];

   if (ci.numInstances == INT_MIN)
      err << "SplitInfoCache::numInstances on non-built attribute "
	  << attrNum << fatal_error;

   return ci.numInstances;
}





