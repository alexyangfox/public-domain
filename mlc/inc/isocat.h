#ifndef __Isomorphic_catgory_h
#define __Isomorphic_catgory_h 1

#include <LEDA/graph.h>
#include <DblLinkList.h>
#include <RootCatGraph.h>
#include <LogOptions.h>
#include <Attribute.h>

void merge_graph(const LogOptions& logOptions, const NominalAttrInfo& nai, 
		 RootedCatGraph& catG, NodePtr n);

void prune_graph(const LogOptions& logOptions, RootedCatGraph& catG);

Real approx_isomorphic(const LogOptions& logOptions,
		       const CatGraph& catG1, NodePtr n1,
		       const CatGraph& catG2, NodePtr n2, Bool &changeLabel);

int num_conflicting_instances(const LogOptions& logOptions,
			      const Categorizer& cat);

Real conflict_ratio(const LogOptions& logOptions, const Categorizer& cat);

int
acyclic_graph_merge_conflict(const LogOptions& logOptions,
			     const CatGraph& catG1, const NodePtr n1,
			     const CatGraph& catG2, const NodePtr n2,
			     Bool& changeLabel);

Bool is_unknown(Categorizer &cat);
#endif

