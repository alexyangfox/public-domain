#include "sumlog.h"

//			Long name		Short		Default									Help
//FLAG_OPT(	someflag,		f,													"This is a flag option")

//			Long name		Short		Default		Min			Max				Help
//INT_OPT(	someint,		i,			-1,			INT_MIN,	INT_MAX,		"This is a signed integer option")

//			Long name		Short		Default		Min			Max				Help
//UNS_OPT(	someuns,		-,			0,			0,			UINT_MAX,		"This is an unsigned integer option")

//			Long name		Short		Default		Min			Max				Help
//FLT_OPT(	someflt,		f,			0.0,		-1,			+1,				"This is a floating-point option")

//			Long name		Short		Default									Help
//STR_OPT(	somestr,		s,			0,										"This is a string option")

//			Long name		Short		Default		Values						Help
//ENUM_OPT(	someenum,		e,			1,			"Value1=1|Value2=2",		"This is an enum option\nwith line break")

//			Long name		Short		Default									Help
STR_OPT(	input,			i,			0,										"Input file name (default stdin)")
STR_OPT(	output,			o,			0,										"Output file name (default stdout)")
STR_OPT(	model,			m,			0,										"Graphical model")
STR_OPT(	matrix,			-,			0,										"Substitution Matrix.")
STR_OPT(	subfamfiles,	-,			0,										"Subfamily filename prefix")
STR_OPT(	outliers,		-,			0,										"Write sequences discarded by --prune to this FASTA file")
STR_OPT(	addseqs,		-,			0,										"Add sequences in --input to this SeqDB")
STR_OPT(	optimize,		-,			0,										"Optimize alignment for this sequence")
STR_OPT(	self_aln,		-,			0,										"Find self-alignments in existing alignment")
STR_OPT(	maf,			-,			0,										"Multiple alignment with re-arrangments")
STR_OPT(	labelregex,		-,			0,										"Regular expression for sequence labels.")
STR_OPT(	posregex,		-,			0,										"Regular expression extracting coordinate and optional strand from labels.")
STR_OPT(	tree,			-,			0,										"Guide tree.")
STR_OPT(	drawtree,		-,			0,										"Draw tree.")
STR_OPT(	profile,		-,			0,										"Profile-profile.")
STR_OPT(	blockprefix,	-,			0,										"Save MAF blocks to <blockprefix>.n")

//			Long name		Short												Help
FLAG_OPT(	svnrevision,	-,													"Write subversion revision and exit")
FLAG_OPT(	version,		-,													"Write version and exit")
FLAG_OPT(	inversions_only,-,													"Just report inversions")
FLAG_OPT(	self_only,		-,													"Just report self-alignments (repeats and non-inverted duplications)")
FLAG_OPT(	msa_only,		-,													"Just build multiple alignment (don't look for re-arrangements etc.")
FLAG_OPT(	xlat,			-,													"Translated alignment.")
FLAG_OPT(	frameshift_only,-,													"Just detect frameshifts.")
FLAG_OPT(	prune_only,		-,													"Just prune.")

//			Long name		Short		Default									Help
TOG_OPT(	trace,			-,			false,									"Log DP matrices etc.")
TOG_OPT(	logmodels,		-,			false,									"Log models.")
TOG_OPT(	logparams,		-,			false,									"Log model parameters.")
TOG_OPT(	eaweight,		-,			false,									"Weight dual model posterior by EA")
TOG_OPT(	refinecolprobs,	-,			false,									"Refine by best predicted column accuracy")
TOG_OPT(	refineacc,		-,			false,									"Refine by best predicted letter pair accuracy")
TOG_OPT(	dotplots,		-,			false,									"Log posterior dot plots.")
TOG_OPT(	treeaftercons,	-,			false,									"Compute tree after consistency.")
TOG_OPT(	dualbest,		-,			false,									"Pick best model, not mix")
TOG_OPT(	mixea,			-,			false,									"Mix with weight based on EA")
TOG_OPT(	self,			-,			true,									"Report self-alignments (repeats & duplications)")
TOG_OPT(	seqweights,		-,			true,									"Use sequence weighting")
TOG_OPT(	sumlog,			-,			false,									"Sumlog mixing for dual model")
TOG_OPT(	refinerand2,	-,			true,									"Refine by randomized 2-way splitting")
TOG_OPT(	refinerand2e,	-,			false,									"Refine by randomized 2-way equal splitting")
TOG_OPT(	refinerand3,	-,			false,									"Refine by randomized 3-way splitting")
TOG_OPT(	refinetree,		-,			false,									"Refine by tree-based splitting")
TOG_OPT(	refinerandtree,	-,			true,									"Radomize tree order")
TOG_OPT(	highprobupper,	-,			false,									"Set upper/lower case if prob gt/lt mincolprob")
TOG_OPT(	inversions,		-,			true,									"Report inversions")
TOG_OPT(	lowc,			-,			true,									"Report low complexity regions")
TOG_OPT(	msa,			-,			true,									"Generate multiple alignment")
TOG_OPT(	subfams,		-,			true,									"Generate subfamily alignments")
TOG_OPT(	posteriors,		-,			false,									"Write posterior matrices")
TOG_OPT(	prune,			-,			false,									"Prune sequences with low predicted accuracy")
TOG_OPT(	colprobs,		-,			false,									"Report column probabilities")
TOG_OPT(	clustersize,	-,			true,									"Weight clusters by size")
TOG_OPT(	accweight,		-,			false,									"Sequence weighting by inverse accuracy")
TOG_OPT(	localtree,		-,			false,									"Compute guide tree using local posteriors")
TOG_OPT(	logtree,		-,			false,									"Log guide tree in Newick format.")
TOG_OPT(	treeorder,		-,			true,									"Output alignment in tree order, otherwise input order.")
TOG_OPT(	alllocals,		-,			false,									"Output all local alignments.")
TOG_OPT(	allframes,		-,			false,									"Consider all frame pairs, not just F0 vs. Fn.")
TOG_OPT(	pruneid,		-,			false,									"Prune by id instead of accuracy")
TOG_OPT(	fasttree,		-,			false,									"Use fast distance estimator for guide tree")
TOG_OPT(	treeregex,		-,			false,									"Tree labels are regular expressions.")
TOG_OPT(	self1,			-,			true,									"Do one pass only when self-aligning.")
TOG_OPT(	onepass,		-,			false,									"Do one pass only (no local chaining).")
TOG_OPT(	logfirstpasshits,-,			false,									"Log first pass hits from two-pass chaining.")
TOG_OPT(	realignhits,	-,			true,									"Re-align local hits.")
TOG_OPT(	allblocks,		-,			false,									"Output all alignment blocks, not just root.")
TOG_OPT(	sw,				-,			false,									"Smith-Waterman align all pairs.")
TOG_OPT(	findrepmotifs,	-,			true,									"Find repeat motifs.")
TOG_OPT(	mask,			-,			false,									"Mask by lower-case letters.")

//			Long name		Short		Default		Min			Max				Help
FLT_OPT(	minsparseprob,	-,			0.03,		0.0,		1.0,			"Minimum posterior probability for sparse matrices")
FLT_OPT(	mix,			-,			0.5,		0.0,		1.0,			"Dual = mix*model1 + (1-mix)*model2")
FLT_OPT(	minlocalprob,	-,			0.875,		-1.0,		1.0,			"Min avg posterior prob for local alignment")
FLT_OPT(	minlocalid,		-,			0.0,		0.0,		1.0,			"Min %id for local alignment")
FLT_OPT(	gaplocal,		-,			0.01,		0.0,		1.0,			"Cosmetic gap penalty for local alignment")
FLT_OPT(	mincolprob,		-,			0.8,		0.0,		1.0,			"Minimum probability for reliable column")
FLT_OPT(	maxmaskpct,		-,			20.0,		0.0,		100.0,			"Max pct of sequence mask to include in SeqDB")
FLT_OPT(	prunedev,		-,			1.0,		0.0,		100.0,			"Number of std devs for pruning")
FLT_OPT(	pruneacc,		-,			0.5,		0.0,		1.0,			"Do not prune if predicted accuracy >= pruneacc")
FLT_OPT(	optimize_f,		-,			4.0,		0.0,		100.0,			"Weight for --optimize")
FLT_OPT(	frameskew,		-,			0.2,		0.0,		100.0,			"Frame skew factor")
FLT_OPT(	gbpen,			-,			8.0,		0.0,		1e9,			"Good/bad model switch penalty")

//			Long name		Short		Default		Min			Max				Help
UNS_OPT(	cons,			c,			1,			0,			10,				"Number of consistency transformations")
UNS_OPT(	refine,			r,			10,			0,			UINT_MAX,		"Number of refinement iterations")
UNS_OPT(	smooth,			s,			0,			0,			UINT_MAX,		"Window size for posterior smoothing")
UNS_OPT(	minlocallen,	-,			8,			0,			1000,			"Minimum length for local alignment")
UNS_OPT(	mmband,			-,			8,			1,			100,			"Band radius for multiple local")
UNS_OPT(	maxlocalgap,	-,			8,			1,			100,			"Max gap in local alignment")
UNS_OPT(	maxlocaldi,		-,			4,			1,			100,			"Max length of D/I gap in local alignment")
UNS_OPT(	arrangeov,		-,			4,			1,			100,			"Max overlap of local hits in global chain")
UNS_OPT(	rowlen,			-,			64,			1,			UINT_MAX,		"Max row length of alignments")
UNS_OPT(	maxlabel,		-,			16,			1,			UINT_MAX,		"Max chars in seq label")
UNS_OPT(	minreplen,		-,			8,			1,			UINT_MAX,		"Min length of repeat, shorter repeats are reported but not aligned")
UNS_OPT(	subrefine,		-,			0,			0,			UINT_MAX,		"Refine iterations for subtrees in progressive")
UNS_OPT(	refinemix,		-,			25,			0,			UINT_MAX,		"Refine iterations for subtrees in progressive")
UNS_OPT(	maxseqlen,		-,			10000,		1,			UINT_MAX,		"Max sequence length")
UNS_OPT(	framemin,		-,			32,			1,			UINT_MAX,		"Min aligned AAs for frameshift detection")
UNS_OPT(	maxbubble,		-,			8,			0,			UINT_MAX,		"Min aligned AAs for frameshift detection")
UNS_OPT(	posregexbase,	-,			1,			0,			UINT_MAX,		"Add this to pos in posregex (1=one based, 0=zero based, can be > 1).")

#include "transopts.h"
