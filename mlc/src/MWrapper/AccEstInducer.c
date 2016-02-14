// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper inducer for accuracy estimation
                 The "accuracy" returned by train-and-test is the accuracy
		   estimate.
  Assumptions  :
  Comments     : The test list is only used for statistical purposes
                   (i.e. computing real accuracy for comparison and bias)
  Complexity   : Depends on the accuracy estimation method and inducer
                   chosen.
  Enhancements :
  History      : Dan Sommerfield                                  12/13/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <AccEstInducer.h>
#include <CtrInstList.h>
#include <env_inducer.h>
#include <GetOption.h>


RCSID("MLC++, $RCSfile: AccEstInducer.c,v $ $Revision: 1.4 $")


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
AccEstInducer::AccEstInducer(const MString& prefix,
			     const MString& description, BaseInducer *ind)
   : BaseInducer(description),
     baseInducer(ind)
{   
   init(prefix + "ACC_");
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
AccEstInducer::~AccEstInducer()
{
}


/***************************************************************************
  Description : Initialize arguments to their default values.
  Comments    :
***************************************************************************/

void AccEstInducer::init(const MString& prefix)
{
   // lower the inducer's log level.
   if(!baseInducer)
      baseInducer = env_inducer(prefix);
   baseInducer->set_log_level(max(0, get_log_level() - 4));
   accEst.set_user_options(prefix);
   LOG(1, *this);
}

                                                                            
/***************************************************************************
  Description : Display info
  Comments    :
***************************************************************************/

void AccEstInducer::display(MLCOStream& stream) const
{
   accEst.display_settings(stream);
}


DEF_DISPLAY(AccEstInducer);


/***************************************************************************
  Description : Use the accuracy estimator to get estimated accuracy
  Comments    :
***************************************************************************/
Real AccEstInducer::train_and_test(InstanceBag* trainingSet,
				   const InstanceBag& testList)
{
   const InstanceList& trainList = trainingSet->cast_to_instance_list();
   const InstanceList& testListList = testList.cast_to_instance_list();
   Real acc = accEst.estimate_accuracy(*baseInducer, trainList,
				       testListList);
   return acc;
}

// maybe much faster for inducers that do not need counters
Real AccEstInducer::train_and_test_files(const MString& fileStem,
					 const MString& namesExtension,
					 const MString& dataExtension,
					 const MString& testExtension)
#ifdef INSTLIST_ONLY
{ BaseInducer::train_and_test_files(fileStem, namesExtension,
				    dataExtension, testExtension);}
#else
{
   CtrInstanceList trainList("", fileStem + namesExtension,
			    fileStem + dataExtension);
   CtrInstanceList testList ("", fileStem + namesExtension,
			      fileStem + testExtension);
   Real acc = train_and_test(&trainList, testList);
   return acc;
}
#endif
