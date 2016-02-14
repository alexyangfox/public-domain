// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the methods of DTCategorizer.
  Doesn't test :
  Enhancements :
  History      : Richard Long                                       9/05/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <DTCategorizer.h>
#include <ConstCat.h>
#include <AttrCat.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_DTCategorizer.c,v $ $Revision: 1.21 $")

/***************************************************************************
  Description : Allocates a new AugCategory with the given value and
                  the description matching the value.
  Comments    :
***************************************************************************/
AugCategory* make_edge(const Category cat)
{
   return new AugCategory(cat, MString(cat - FIRST_CATEGORY_VAL,0));
}


/***************************************************************************
  Description : Builds the DecisionTree that will be used by the
                  DTCategorizer.

                   A (on Attr0)
                 / |  \
                /  |   \
            C(L0) C(L1) A (on Attr1)
                       / \
                      /   \
                     C(L2) C(?)
		
	      A = AttrCategorizer; C = ConstCategorizer
	      L0 = Label0; L1 = Label1; L2 = Label2; 
	      ? = UNKNOWN_CATEGORY_VAL
  Comments    :
***************************************************************************/
void build_tree(DecisionTree& dt, const InstanceList& bag)
{
  Categorizer* ac = new AttrCategorizer(bag.get_schema(), 0,
					"AttrCat on attr 0");
  NodePtr source = dt.create_node(ac, 0);
  ASSERT(ac == NULL);
  dt.set_root(source);


  AugCategory cc1AugAlloc(FIRST_NOMINAL_VAL, "Label0");
  Categorizer* cc = new ConstCategorizer("Const Cat returning Label0", 
					 cc1AugAlloc);
  NodePtr target = dt.create_node(cc, 1);
  ASSERT(cc == NULL);
  Category nextEdgeLabel = FIRST_CATEGORY_VAL;
  AugCategory* edge = make_edge(nextEdgeLabel);
  nextEdgeLabel++;
  dt.connect(source, target, edge);
  ASSERT(edge == NULL); // connect() gets ownership
  
  AugCategory cc2AugAlloc(FIRST_NOMINAL_VAL + 1, "Label1");
  cc = new ConstCategorizer("Const Cat returning Label1", cc2AugAlloc);
  target = dt.create_node(cc, 1);
  ASSERT(cc == NULL);
  edge = make_edge(nextEdgeLabel);
  nextEdgeLabel++;
  dt.connect(source, target, edge);
  ASSERT(edge == NULL); // connect() gets ownership

  ac = new AttrCategorizer(bag.get_schema(), 1, 
			   "AttrCat on attr 1");
  target = dt.create_node(ac, 1);
  ASSERT(ac == NULL);
  edge = make_edge(nextEdgeLabel);
  dt.connect(source, target, edge);
  ASSERT(edge == NULL); // connect() gets ownership

  source = target;
  nextEdgeLabel = FIRST_CATEGORY_VAL;
  AugCategory cc3AugAlloc(FIRST_NOMINAL_VAL + 2, "Label2");
  cc = new ConstCategorizer("Const Cat returning Label2", cc3AugAlloc);
  target = dt.create_node(cc, 2);
  ASSERT(cc == NULL);
  nextEdgeLabel = FIRST_CATEGORY_VAL;
  edge = make_edge(nextEdgeLabel);
  nextEdgeLabel++;
  dt.connect(source, target, edge);
  ASSERT(edge == NULL); // connect() gets ownership

  AugCategory cc4AugAlloc(UNKNOWN_CATEGORY_VAL, "Unknown Label");
  cc = new ConstCategorizer("Const Cat returning Unknown Label", cc4AugAlloc);
  target = dt.create_node(cc, 2);
  ASSERT(cc == NULL);
  edge = make_edge(nextEdgeLabel);
  dt.connect(source, target, edge);
  ASSERT(edge == NULL); // connect() gets ownership
}


/***************************************************************************
  Description : Asserts that the categorizer returns the correct label for
                  each instance in the bag.
  Comments    : The instance bag and the categorizer were designed so that
                  this should always be true.
***************************************************************************/
static void test_categorizer(const DTCategorizer& dtCat, 
			     const InstanceList& bag)
{
  for (Pix bagPix = bag.first(); bagPix; bag.next(bagPix)) {
     InstanceRC inst = bag.get_instance(bagPix);
    ASSERT(dtCat.categorize(inst) ==
	   inst.label_info().get_nominal_val(inst.get_label()));
  }
}


main()
{
  cout << "t_DTCategorizer executing" << endl;

  InstanceList bag("t_DTCategorizer");
  DecisionTree *dt = new DecisionTree;
  build_tree(*dt, bag);
  DTCategorizer dtCat(dt, "for t_DTCategorizer", 3);
  ASSERT(dt == NULL); //categorizer got ownership
  test_categorizer(dtCat, bag);
  MLCOStream out1("t_DTCategorizer.out1");
  dtCat.display_struct(out1);
  return 0; // return success to shell
}   
