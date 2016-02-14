// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : TableCasInd constructs an array of n TableCategorizers 
                   where n is less than the number of attributes.  All
		   categorizers except the last are table-categorizers
		   with no majority.  The last one is a table-majority.
                 Each categorizer has one attribute less than the previous
                   one.  We thus try the table with the most attributes
		   and remove them one by one until we get a hit or
		   fall to the last table which has a majority fall-back.
  Comments     :
  Complexity   :
  Assumptions  :
  Enhancements :
  History      : Ronny Kohavi                                    7/13/95
                   Small code fixups
                 Yeogirl Yun                                     7/8/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <TableCasInd.h>
#include <TableInducer.h>
#include <ProjectInd.h>
#include <BoolArray.h>
#include <CtrBag.h>
#include <AugCategory.h>
#include <CascadeCat.h>

/*****************************************************************************
  Description : Integrity constraint.
                attrNums must have all and distinct n attribute numbers.
		cat must have n+1 categorizers.
  Comments    :
*****************************************************************************/
void TableCasInd::OK(int /* level */) const
{
   if (TS == NULL || !ao.order_set())
      return;

   const Array<int>& attrNums = ao.get_order();
   Array<Bool> boolArray(0, TS->num_attr(), FALSE);
   for (int i = attrNums.low(); i <= attrNums.high(); i++) {
      if (attrNums[i] >= TS->num_attr() || attrNums[i] < 0)
	 err << "TableCasInd::OK: attribute number " << attrNums[i]
	     << " negative or too big" << fatal_error;
      if (boolArray[attrNums[i]] == TRUE)
	 err << "TableCasInd::OK: duplicate attribute " << attrNums[i]
             << fatal_error;
      boolArray[attrNums[i]] = TRUE;
   }

   ASSERT(cat == NULL || TS->num_attr() == 0 ||
          cat->num_categorizers() <= TS->num_attr());
}



/*****************************************************************************
  Description : Assign the given data to the training Set.
                Also updates AttrOrder data member.
  Comments    :
*****************************************************************************/
InstanceBag* TableCasInd::assign_bag(InstanceBag*& newTS)
{
   if (!ao.order_set())
      ao.init();
   delete cat; cat = NULL;
   return BaseInducer::assign_bag(newTS);
}


/*****************************************************************************
  Description : Constructor/Destructor.
  Comments    :
*****************************************************************************/
TableCasInd::TableCasInd(const MString& dscr, const Array<int>& aAttrNums)
   : CtrInducer(dscr),
     cat(NULL)
{
   OK();
   ao.set_order(aAttrNums);
}

TableCasInd::~TableCasInd()
{
   delete cat;
}


/*****************************************************************************
  Description : Constructor with MString.
  Comments    :
*****************************************************************************/
TableCasInd::TableCasInd(const MString& dscr)
   : CtrInducer(dscr),
     cat(NULL)
{
   OK();
}

/*****************************************************************************
  Description : Copy constructor.
  Comments    :
*****************************************************************************/
TableCasInd::TableCasInd(const TableCasInd& source, CtorDummy)
   : CtrInducer(source, ctorDummy),
     cat(new CascadeCat(*source.cat, ctorDummy)),
     ao(source.ao, ctorDummy)
{
   OK();
}


/******************************************************************************
  Description  : Return TRUE iff the class has a valid categorizer.
  Comments     :
******************************************************************************/
Bool TableCasInd::was_trained(Bool fatal_on_false) const
{
   if( fatal_on_false && cat == NULL)
      err << "TableCasInd::was_trained: No categorizer, "
	     "Call train() to create categorizer" << fatal_error;
   return (cat != NULL);
}


/*****************************************************************************
  Description : Train. Constructs n TableCategorizers or one constCat if 
                  no attributes are given in ao.
  Comments    :
*****************************************************************************/
void TableCasInd::train()
{
   has_data();

   OK();
   const Array<int>& attrNums = ao.compute_order(get_log_options(),
						 *TS);   
   PtrArray<Categorizer *>* catList = 
      new PtrArray<Categorizer *>(0, max(1, attrNums.size()));
   if (attrNums.size() == 0) {
      Category catValue = TS->majority_category();
      Categorizer* constCat =
	 new ConstCategorizer("Const Categorizer for table",
             AugCategory(catValue,
			 TS->get_schema().category_to_label_string(catValue)));
      catList->index(0) = constCat;
   } else {
      BoolArray maskArray(0, TS->num_attr(), FALSE);

      ProjectInd projInd("TableCasInd");
      // majority on unknown first time around
      TableInducer* tableInd = new TableInducer("TableInducer", TRUE);
      // to avoid compiler warning on next stmt
      BaseInducer *baseInd = tableInd;
      projInd.set_wrapped_inducer(baseInd); // note we lose ownership now
      CtrInstanceBag* bag = &release_data()->cast_to_ctr_instance_bag();
      ASSERT(projInd.assign_data(bag) == NULL);

      for (int i = 0; i < attrNums.size(); i++) {
	 ASSERT(maskArray[attrNums.index(i)] == FALSE);
	 maskArray[attrNums.index(i)] = TRUE;
	 BoolArray* tmpMask = new BoolArray(maskArray, ctorDummy);
	 projInd.set_project_mask(tmpMask); // give ownership
	 projInd.train();
	 Categorizer* c = projInd.get_categorizer().copy();
	 IFLOG(3, c->display_struct(get_log_stream(), defaultDisplayPref)); 
	 catList->index(attrNums.size() - i - 1) = c; // give ownership
	 // next time it has no majority on unknown
	 tableInd->set_majority_on_unknown(FALSE);
      }
      bag = &projInd.release_data()->cast_to_ctr_instance_bag();
      ASSERT(assign_data(bag) == NULL);
   }

   LOG(2, "Number of categorizers : " << catList->size() << endl); 
   delete cat;
   cat = new CascadeCat("Cascade Categorizer", catList);
   cat->set_log_options(get_log_options());
   OK();
}

      

/*****************************************************************************
  Description : Return categorizer.
  Comments    :
*****************************************************************************/
const Categorizer& TableCasInd::get_categorizer() const
{
   return *cat;
}


/*****************************************************************************
  Description : Returns a copy of this inducer.
  Comments    :
*****************************************************************************/
Inducer* TableCasInd::copy() const
{
   Inducer *ind = new TableCasInd(*this, ctorDummy);
   return ind;
}


/*****************************************************************************
  Description : Set user options.
  Comments    :
*****************************************************************************/
void TableCasInd::set_user_options(const MString& prefix)
{
   get_attr_order_info().set_user_options(prefix);
}




