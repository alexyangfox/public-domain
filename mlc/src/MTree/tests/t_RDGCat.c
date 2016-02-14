// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests methods of RDGCategorizer.
  Doesn't test :
  Enhancements :
  History      : Richard Long                                       9/05/93
                   Initial revision;
      		 Chia-Hsin Li                                       9/18/94
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <ThresholdCat.h>
#include <RDGCat.h>
#include <ConstCat.h>
#include <AttrCat.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_RDGCat.c,v $ $Revision: 1.26 $")

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
  Description : Builds the RootedCatGraph that will be used by the
                  RDGCategorizer.

                   A (on Attr0)
                 /|  \
                / |   \
               C  C     A (on Attr1)
               \  /    / \
                \/    /   \
              C(L0) C(L1) C(?)
		
	      A = AttrCategorizer; C = ConstCategorizer
	      L0 = Label0; L1 = Label1; ? = UNKNOWN_CATEGORY_VAL
  Comments    :
***************************************************************************/
void build_graph(RootedCatGraph& rcg, const InstanceList& bag)
{
  Categorizer* ac = new AttrCategorizer(bag.get_schema(),
					0, "AttrCat on Attr0");
  NodePtr source = rcg.create_node(ac, 0); ASSERT(ac == NULL);
  rcg.set_root(source);

  AugCategory ccAugCat(FIRST_CATEGORY_VAL, "0");
  Categorizer* cc = new ConstCategorizer("CC1", ccAugCat);
  NodePtr target = rcg.create_node(cc, 1);
  ASSERT(cc == NULL); // create_node() gets ownership
  AugCategory* edge;
  Category nextEdgeLabel = FIRST_CATEGORY_VAL;
  edge = make_edge(nextEdgeLabel);
  nextEdgeLabel++;
  rcg.connect(source, target, edge);
  ASSERT(edge == NULL); //connect() gets ownership

  cc = new ConstCategorizer("CC1", ccAugCat);
  target = rcg.create_node(cc, 1);
  ASSERT(cc == NULL); // create_node() gets ownership
  edge = make_edge(nextEdgeLabel);
  nextEdgeLabel++;
  rcg.connect(source, target, edge);
  ASSERT(edge == NULL); //connect() gets ownership
  
  ac = new AttrCategorizer(bag.get_schema(), 1, 
			   "AttrCat on attr 1");
  target = rcg.create_node(ac, 1);
  ASSERT(cc == NULL); // create_node() gets ownership
  edge = make_edge(nextEdgeLabel);
  rcg.connect(source, target, edge);
  ASSERT(edge == NULL); //connect() gets ownership

  nextEdgeLabel = FIRST_CATEGORY_VAL;
  AugCategory getEdge(nextEdgeLabel, MString(nextEdgeLabel -
					     FIRST_CATEGORY_VAL,0));
  nextEdgeLabel++;
  source = rcg.get_child(rcg.get_root(), getEdge);

  cc = new ConstCategorizer("CC2", ccAugCat);
  target = rcg.create_node(cc, 2);
  ASSERT(cc == NULL); // create_node() gets ownership
  edge = make_edge(FIRST_CATEGORY_VAL);
  rcg.connect(source, target, edge);
  ASSERT(edge == NULL); //connect() gets ownership
  
  Category getEdgeLabel = FIRST_CATEGORY_VAL + 1;
  AugCategory getEdge2(getEdgeLabel, MString(getEdgeLabel -
					     FIRST_CATEGORY_VAL,0));
  source = rcg.get_child(rcg.get_root(), getEdge2);
  nextEdgeLabel++;

  //@@ The graph has been changed. The "0" used to be "1"
  edge = new AugCategory(FIRST_CATEGORY_VAL, "0");
  rcg.connect(source, target, edge);
  ASSERT(edge == NULL); //connect() gets ownership

  getEdgeLabel = FIRST_CATEGORY_VAL + 2;
  AugCategory getEdge3(getEdgeLabel, MString(getEdgeLabel -
					     FIRST_CATEGORY_VAL,0));
  source = rcg.get_child(rcg.get_root(), getEdge3);
  AugCategory augCatAlloc1(FIRST_NOMINAL_VAL + 1, "Label1");
								 
  cc = new ConstCategorizer("CC2-1", augCatAlloc1);
  target = rcg.create_node(cc, 2);
  ASSERT(cc == NULL); // create_node() gets ownership
  nextEdgeLabel = FIRST_CATEGORY_VAL;
  edge = make_edge(nextEdgeLabel);
  nextEdgeLabel++;
  rcg.connect(source, target, edge);
  ASSERT(edge == NULL); //connect() gets ownership

  AugCategory UnkownAugCatAlloc(UNKNOWN_CATEGORY_VAL, "unknown");
  cc = new ConstCategorizer("CC2-?", UnkownAugCatAlloc);
  target = rcg.create_node(cc, 2); ASSERT(cc == NULL);
  edge = make_edge(nextEdgeLabel);
  rcg.connect(source, target, edge);
  ASSERT(edge == NULL); //connect() gets ownership
}


/***************************************************************************
  Description : Asserts that the categorizer returns the correct label for
                  each instance in the bag.
  Comments    : The instance bag and the categorizer were designed so that
                  this should always be true.
***************************************************************************/
static void test_categorizer(const RDGCategorizer& rdgCat, 
			     const InstanceList& bag)
{
  for (Pix bagPix = bag.first(); bagPix; bag.next(bagPix)) {
     InstanceRC inst = bag.get_instance(bagPix);
    ASSERT(rdgCat.categorize(inst)
	   == inst.label_info().get_nominal_val(inst.get_label()));
  }
}


main()
{
  cout << "t_RDGCat executing" << endl;

  InstanceList bag("t_RDGCat");
  RootedCatGraph* rcg = new RootedCatGraph;
  build_graph(*rcg, bag);

  RDGCategorizer rdgCat(rcg, "for t_RDGCat", 3);
  ASSERT(rcg == NULL); // categorizer gets ownership
  test_categorizer(rdgCat, bag);
  MLCOStream out1("t_RDGCat.out1");
  rdgCat.display_struct(out1);
  ASSERT(rdgCat.num_nodes() == 7);
  ASSERT(rdgCat.num_leaves() == 3);

   //@@ Remove MString after
   // the ambiguity problem
   // of MLCOStream is
   // solved.

  MLCOStream out2(MString("t_RDGCat.out1.ps"), PrinterStream);
  rdgCat.display_struct(out2);

   // Test for operator==()
/*@@  Instances distribution is not implemented in build_graph.
   ASSERT(rdgCat == rdgCat);
   ASSERT( (*(Categorizer*)&rdgCat) == (*(Categorizer*)&rdgCat));
*/
   AugCategory aca(0,"Aug Cat");
   ConstCategorizer cc("Const Categorizer",aca);
   ASSERT(!(rdgCat == cc));

   AttrCategorizer ac(bag.get_schema().attr_info(2), 2,
                      "Test2 AttrCat");
   ASSERT(!(rdgCat == ac));

   Real value = 4.2;
   ThresholdCategorizer tc(bag.get_schema(), 1, value,"Test1 ThresholdCat");
   ASSERT(!(rdgCat == tc));
  
   return 0; // return success to shell
}   
