#ifndef symbol_h
#define symbol_h

#include "enums.h"
#include "common.h"



/****************************************************/
/*                                                  */
/* SymNode-                                         */
/*                                                  */
/* This is the definition for a symbol table entry. */
/*                                                  */
/****************************************************/


struct extRecs;

struct SymNode 
{
	/*
	** Next points at the previous symbol in the current local
	** symbol table.
	*/

	struct	SymNode		*Next;
	
	
	/*
	** hashList forms a chain of symbols that have colliding
	** hash values when using a hash table (namespaces only)
	*/
	
	union
	{
		struct	SymNode		*hashList;
	} h;
	
	
	/*
	** Name is a pointer to the lower case version of the name.
	** TrueName is a pointer to the upper/lower case version of the name.
	** When searching for a symbol table entry, the search routines use
	** the Name object for comparison.  If found, the search routines
	** compare the key object against TrueName.  If the key value
	** matches Name but does not match TrueName, HLA generates an error
	** because this violates case neutrality.
	**
	**	NameLen is the length of the identifier plus one (to include
	** the zero byte).
	*/
		
	char				*Name;
	char				*TrueName;
	int					NameLen;

	/*
	** The following two entries hold the type information.
	** Type is a pointer to a symbol table entry.  pType is an ordinal
	** value that is useful for quickly and easily checking for predefined 
	** types;  it isn't strictly necessary, but it does make certain
	** operations more convenient.
	**
	** Note: The contents of Type depends upon the pType entry as follows:
	**
	**	If pType is a primitive type (tBoolean..tCset) then Type points
	**	at the corresponding symbol table entry for that type.
	**
	**	If pType is tArray, then Type points at the symbol table entry
	**	of the base type of the array.
	**
	**	If pType is tRecord or tClass (class, record, or union) then Type 
	**	is ignored (it will contain a self reference, i.e., a pointer back
	**	to this symbol table entry).
	**
	**	If pType is anything else, then this program ignores the Type
	**	field.
	*/

	struct	SymNode		*Type;
	enum	PrimType	pType;


	/*
	**	SymClass determines the classification of this symbol.
	**	Typical classifications include cConstant, cType, cVar, cMacro,
	**	cProc, cStatic, etc.
	*/
	
	enum	ClassType	SymClass;

	/*
	**	For parameter objects (SymClass=cParm), the following
	**	field specifies how the parameter is passed.
	*/

	enum	ParmClass	pClass;


	/*
	**	ObjectSize holds the size of this item, in bytes.
	**  For Unions/Records/Classes, MaxObjectSize holds the size
	**		of the largest field in the structure.  For other types,
	**		MaxObjectSize is the same as ObjectSize.
	**	Offset holds the offset of an object (if it is a variable).
	**	If the object is a static variable, StaticName points to
	**	the assembler name for this object.
	*/
   
	unsigned			ObjectSize;
	unsigned			MaxObjectSize;
	int					Offset;
	char				*StaticName;

	/*
	** LexLevel holds the current lex level at the time of the
	** symbol's definition.
	*/

	int					LexLevel;

	/*
	** IsExternal contains true if this symbol is an external symbol.
	** IsReferenced a pointer to an extRecs entry for the external
	** symbol if it has been referenced.  It contains NULL if the
	** symbol has not been referenced.
	*/

	int					IsExternal;
	struct extRecs		*IsReferenced;
	
	/*
	**	If this is an array object, the following entry specifies
	**	the number of elements in the array.
	*/

	int					Arity;
	int					*Dimensions;
	int					NumElements;

	/*
	** If this is a record, union, or class object, then Fields
	** points at the local symbol table containing the field definitions
	** and the ancestor class/record.  FieldCnt contains the number
	** of fields in the class/record.  Base points at the base class/record
	** if such an object exists.
	**
	**	Parent points at the containing type definition if this symbol is
	** a field of a record/class/union. It contains NULL if this is not
	** a record/class/union. Note that if this is  a field definition of
	** a nested record/class/union, then parent only points at the 
	** immediate parent. 
	**
	** If this is an enum type, then Fields points at the list of the
	** constants associated with this enumerated type (note that the list
	** is terminated with the enum type symbol table entry, it is not
	** a NULL terminated list).  If this is an enum type, then FieldCnt
	** contains the number of symbol table entries holding enum constants
	** (that is, the maximum enum value plus one).
	**
	** For tPointer objects, BASE points at the base type that the pointer
	** references.
	*/

	struct	SymNode		*Fields;
	struct	SymNode		*Base;
	struct	SymNode		*Parent;
	int					FieldCnt;


	/*
	** For union constants, the CurField entry keeps track of
	** which union field is the current field.  CurIndex provides
	** an index into the array of field constants for that field.
	*/
	
	struct	SymNode		*CurField;
	int					CurIndex;
	
	
	
	/*
	**	If this is a parameter that is held in a general-purpose
	**  register, the following field holds the enumerated register
	**	number value.  This field contains -1 if there is no
	**	register value in use.
	*/
	
	int					regnum;
	
	/*
	**	If the symbol is a constant, then Value holds the actual
	**	value of the constant (or a pointer to a string if it is a
	**	string value).
	*/


	union
	{
		char						StartOfValues;
		
		struct		SymNode			**hashTable;	// Pts at namespace hash tbls
		
		unsigned	char			bytes[16];
		unsigned	short			words[8];
		unsigned					dwords[4];
		int							intval;
		unsigned					unsval;
		unsigned					boolval;			
		unsigned					charval;
		unsigned					qwordval[2];
		unsigned					lwordval[4];			
		struct		flt80			fltval;			
		char						*strval;
		unsigned	char			csetval[ CSetSizeInBytes ]; 		
		struct		SymNode			*ArrayOfValues;	/* Should really be YYSTYPE */
		struct		SymNode			*FieldValues;	/* Should really be YYSTYPE */
		struct		SymNode			*PtrToValue;
		struct		MacroType		MacroData;
		struct		regexListType	*rx;
		
		struct
		{
					char			*returns;
					char			*use;
			struct	SymNode			*parms;
			struct	SymNode			*Locals;
			struct	SymNode			*Forward;
			struct	SymNode			*BaseClass;
					unsigned		ParmSize;
			enum	CallSeq			cs;
		} proc;
		
		struct
		{
			struct	SymNode			*nextOvld;	// Linked list of ovld entries
			struct	SymNode			*parms;		// Signature for this proc
					int				numParms;	// Signature for this proc
					char			*procName;	// Proc to call if sig matches.
		}ovld;
	}u;
	

};


/*
** The following exists solely to compute the size of the value
** section in the SymNode object.  This allows us to copy on the
** data (at starting offset "StartOfValues") without affecting the
** rest of the SymNode structure.
**
**	Warning: This union must be kept in sync with the structure above.
*/

union ValuesSize
{
		char						StartOfValues;
		
		struct		SymNode			**hashTable;	// Pts at namespace hash tbls
		
		unsigned	char			bytes[16];
		unsigned	short			words[8];
		unsigned					dwords[4];
		int							intval;
		unsigned					unsval;
		unsigned					boolval;			
		unsigned					charval;
		unsigned					qwordval[2];
		unsigned					lwordval[4];			
		struct		flt80			fltval;			
		char						*strval;
		unsigned	char			csetval[ CSetSizeInBytes ]; 		
		struct		SymNode			*ArrayOfValues;	/* Should really be YYSTYPE */
		struct		SymNode			*FieldValues;	/* Should really be YYSTYPE */
		struct		SymNode			*PtrToValue;
		struct		MacroType		MacroData;
		struct		regexListType	*rx;
		
		struct
		{
			char					*returns;
			char					*use;
			struct	SymNode			*parms;
			struct	SymNode			*Locals;
			struct	SymNode			*Forward;
			struct	SymNode			*BaseClass;
					unsigned		ParmSize;
			enum	CallSeq			cs;
		} proc;
		
		struct
		{
			struct	SymNode			*nextOvld;	// Linked list of ovld entries
			struct	SymNode			*parms;		// Signature for this proc
					int				numParms;	// Signature for this proc
					char			*procName;	// Proc to call if sig matches.
		}ovld;
};


/*
** The following is a linked list of external record
** definitions.  Needed here because of the pointer that
** appears in the symbol table record.
*/

struct extRecs
{
	struct	extRecs		*Next;
	struct	SymNode		*theSym;
			char		*Name;
	enum 	PrimType	pType;
			char		IsPublic;
			char		ForceRef;
			char		isVMT;
};


// Fields present in the YYSTYPE UNION in
// the main parser file:

struct adrsYYS
{
	unsigned			Size;
	unsigned			ObjectSize;
	char				*StaticName;
	char				*BaseReg;
	char				*IndexReg;
	unsigned			Scale;
	int					Disp;
	int					regnum;
	struct	SymNode		*Sym;
	struct	SymNode		*Type;
	enum	PrimType	pType;
	enum	ClassType	SymClass;
	enum	ParmClass	pClass;
	char				forcedSize;
	struct	SymNode		*BaseType;
};


#define sizeofAdrsYYS sizeof( struct adrsYYS )
#define YYA (struct adrsYYS *)



struct memYYS
{
	int					base;
	int					index;
	unsigned			scale;
	int					disp;
};

#define YYM (struct memYYS *)


enum regnums
{
	// %00_000..%00_111:
	
	reg_al,
	reg_cl,
	reg_dl,
	reg_bl,
	reg_ah,
	reg_ch,
	reg_dh,
	reg_bh,
	
	// %01_000..%01_111:

	reg_ax,
	reg_cx,
	reg_dx,
	reg_bx,
	reg_sp,
	reg_bp,
	reg_si,
	reg_di,

	// %10_000..%10_111:

	reg_eax,
	reg_ecx,
	reg_edx,
	reg_ebx,
	reg_esp,
	reg_ebp,
	reg_esi,
	reg_edi,
	
	numGPregs
};

#define isReg(r) ((r) < numGPregs)
#define isReg8(r) ((r) <= reg_bh)
#define isReg16(r) ( ((r) >= reg_ax) && ((r) <= reg_di) )
#define isReg816(r) ((r) <= reg_di)
#define isReg32(r) ( ((r) >= reg_eax) && ((r) <= reg_edi) )
#define isReg1632(r) ( ((r) >= reg_ax) && ((r) <= reg_edi) )
#define isLowReg32(r) ( ((r) >= reg_eax) && ((r) <= reg_ebx) )
#define isLowReg8(r) ( (r) <= reg_bh )
#define regCode(reg) ((reg) & 0x7)
#define regSize(reg) (1<<(reg>>3))
#define regDup(reg) (hlastrdup2( regStrs[reg] ))

struct regYYS
{
	unsigned			Size;
	int					IsSigned;
	struct	SymNode		*Type;
	enum	PrimType	pType;
	enum	regnums		encoding;
};

#define sizeofRegYYS sizeof( struct regYYS )
#define YYR (struct regYYS *)


struct operandYYS
{
	enum	opType	operandType;
	char			*text;
	char			*regname;
	union
	{
		struct	regYYS	*reg;
		struct	adrsYYS	*adrs;
		struct	SymNode	v;
	}o;

};

#define sizeofOperandYYS sizeof( struct operandYYS )



struct opnodeYYS
{
	enum	operatorEnum	operator;
	union
	{
		struct	operandYYS		*leftOperand;
		struct	opnodeYYS		*leftSubexpression;
	}l;

	union
	{
		struct	operandYYS		*rightOperand;
		struct	opnodeYYS		*rightSubexpression;
	}r;
};

#define sizeofOpnode sizeof( struct opnodeYYS )






/*********************************************************************/
/*                                                                   */
/* The following are the names of the symbol table entries appearing */
/* at lex level zero.                                                */
/*                                                                   */
/*********************************************************************/

extern struct  SymNode	*SymbolTable;
extern struct  SymNode	*CurrentContext;
extern struct  SymNode	**MainLocals;

extern struct  SymNode boolean_ste;

extern struct  SymNode uns8_ste;
extern struct  SymNode uns16_ste;
extern struct  SymNode uns32_ste;
extern struct  SymNode uns64_ste;
extern struct  SymNode uns128_ste;

extern struct  SymNode int8_ste;
extern struct  SymNode int16_ste;
extern struct  SymNode int32_ste;
extern struct  SymNode int64_ste;
extern struct  SymNode int128_ste;

extern struct  SymNode byte_ste;
extern struct  SymNode word_ste;
extern struct  SymNode dword_ste;
extern struct  SymNode qword_ste;
extern struct  SymNode tbyte_ste;
extern struct  SymNode lword_ste;

extern struct  SymNode real32_ste;
extern struct  SymNode real64_ste;
extern struct  SymNode real80_ste;
extern struct  SymNode real128_ste;

extern struct  SymNode char_ste;
extern struct  SymNode wchar_ste;

extern struct  SymNode cset_ste;
extern struct  SymNode string_ste;
extern struct  SymNode zstring_ste;
extern struct  SymNode wstring_ste;
extern struct  SymNode text_ste;
extern struct  SymNode regex_ste;

extern struct  SymNode false_ste;
extern struct  SymNode true_ste; 
extern struct  SymNode error_ste;
extern struct  SymNode forctrlvar_ste;

extern struct  SymNode undefined_ste;
extern struct  SymNode dummy_ste;
extern struct  SymNode dummyField_ste;
extern struct  SymNode dummyType_ste;
extern struct  SymNode dummyProc_ste;
extern struct  SymNode dummyProc2_ste;

extern struct  SymNode pgmID_ste;
extern struct  SymNode procID_ste;
extern struct  SymNode iterID_ste;
extern struct  SymNode classprocID_ste;
extern struct  SymNode classiterID_ste;
extern struct  SymNode proctype_ste;
extern struct  SymNode thunk_ste;

extern struct  SymNode record_ste;
extern struct  SymNode union_ste;
extern struct  SymNode pointer_ste;
extern struct  SymNode static_ste;
extern struct  SymNode variant_ste;
extern struct  SymNode namespace_ste;
extern struct  SymNode UndefinedType;




/**********************************************************************************/
/*                                                                                */
/* Symbol table functions:                                                        */
/*                                                                                */
/* InsertSym-      Inserts a symbol into the symbol table.                        */
/* SetSym-         Replaces an existing symbol's values.						  */
/* lookup-         Searches through the symbol table to locate                    */
/*                 an entry.                                                      */
/*                                                                                */
/* ClassifyLookup- Like "lookup" but this function handles IDs                    */
/*                 with dots in them (i.e., record/class references).             */
/*                                                                                */
/* DumpSym-        Prints the symbol table.                                       */
/*                                                                                */
/**********************************************************************************/





extern struct SymNode* InsertSym
(
	char				*Name,
	struct	SymNode		*TheType,
	enum	PrimType	pType,
	int					TheClass,
	int					Arity,
	int					*Dimensions,
	int					NumElements,
	union	YYSTYPE		*TheValue,
	unsigned			ObjectSize,
	int					Offset,
	char				*StaticName,
	struct	SymNode		*Base,
	struct	SymNode		*Fields,
	int					FieldCnt
);


extern void SetSym
(

	struct	SymNode		*Name,
	struct	SymNode		*TheType,
	enum	PrimType	pType,
	int					Arity,
	int					*Dimensions,
	int					NumElements,
	union	YYSTYPE		*TheValue,
	unsigned			ObjectSize,
	int					Offset,
	char				*StaticName,
	struct	SymNode		*Base,
	struct	SymNode		*Fields,
	int					FieldCnt,
	struct	SymNode		*CurField,
	int					CurIndex
);

extern struct SymNode *ClrNewSym
(
	char				*Name,
	struct	SymNode		*TheType,
	enum	PrimType	pType,
	int					TheClass,
	int					Arity,
	int					*Dimensions,
	int					NumElements,
	unsigned			ObjectSize,
	int					Offset,
	char				*StaticName,
	struct	SymNode		*Base,
	struct	SymNode		*Fields,
	int					FieldCnt,
	struct	SymNode		*CurField,
	int					CurIndex
);


extern void ClrSym
(
	struct	SymNode		*Name,
	struct	SymNode		*TheType,
	enum	PrimType	pType,
	int					Arity,
	int					*Dimensions,
	int					NumElements,
	unsigned			ObjectSize,
	int					Offset,
	char				*StaticName,
	struct	SymNode		*Base,
	struct	SymNode		*Fields,
	int					FieldCnt,
	struct	SymNode		*CurField,
	int					CurIndex
);





extern struct SymNode *InsertProc
(
	char	*Name,
	char	*StaticName
);


extern struct SymNode* SearchNext
( 
	struct SymNode	*s, 
	char			*lcName, 
	int 			length 
);
extern struct SymNode* SearchHash( struct SymNode *, char *, int );
extern struct SymNode*	lookup( char *, int );
extern struct SymNode*	NSlookup( char *, int, struct SymNode* );
extern struct SymNode*	ClassifyLookup( char *, struct SymNode *table );
extern struct SymNode*	lookupin( char *, struct SymNode *table );
extern struct SymNode*	lookupthis( char *, struct SymNode *table );

extern void				DumpSym( struct SymNode *SymbolTable, int indent );
extern void 			initSymbolTable( void );


static struct SymNode aValue;
#define setval(x)	\
	((struct SymNode*)(memcpy( &aValue, (x), sizeofSymNode )))



extern struct SymNode *GetBaseType( struct SymNode *object );
extern struct SymNode *GetCallType
( 
	struct SymNode 	*Sym, 
	struct SymNode 	*Type, 
	enum PrimType 	pType,
	int				*isPointer 
);
	
extern void BuildAdrs
(
	struct	adrsYYS 	*adrs,
	unsigned			Size,
	unsigned			ObjectSize,
	char				*StaticName,
	char				*BaseReg,
	char				*IndexReg,
	unsigned			Scale,
	int					Disp,
	struct	SymNode		*Sym,
	struct	SymNode		*Type,
	enum	PrimType	pType,
	enum	ClassType	SymClass,
	enum	ParmClass	pClass,
	struct	SymNode		*BaseType
);


/*
** Data structures and functions used to process regular expressions
*/


// NodeType values:

#define nt_StartOfRegex	0

#define nt_Cset			1
#define nt_notCset		2

#define nt_Char			3
#define nt_notChar		4
#define nt_iChar		5
#define nt_notiChar		6

#define nt_String		7
#define nt_notString	8
#define nt_iString		9
#define nt_notiString	10

#define nt_List			11
#define nt_notList		12
#define nt_iList		13
#define nt_notiList		14

#define nt_AnyChar		15
#define nt_Alternative	16
#define nt_Subexpr		17
#define nt_ExtractStr	18
#define nt_EndExtract	19
#define nt_Regex		20
#define nt_Regex2		21

#define nt_EOS			22
#define nt_ws			23

#define nt_reg			24
#define nt_reg8			25
#define nt_reg16		26
#define nt_reg32		27
#define nt_regfpu		28
#define nt_regmmx		29
#define nt_regxmm		30
#define nt_matchid		31
#define nt_matchInt		32
#define nt_matchReal	33
#define nt_matchNum		34
#define	nt_matchStr		35
#define nt_matchWord	36
#define nt_matchiWord	37
#define nt_matchAdrs	38
#define nt_matchExpr	39
#define nt_Balanced		40
#define nt_peekChar		41
#define nt_peekiChar	42
#define nt_peekStr		43
#define nt_peekiStr		44
#define nt_peekCset		45
#define nt_pos			46
#define nt_tab			47
#define nt_at			48
#define nt_match		49
#define nt_match2		50

#define ntstrs_c			\
	{						\
		"nt_StartOfRegex",	\
		"nt_Cset",		   	\
		"nt_notCset",		\
		"nt_Char",			\
		"nt_notChar",		\
		"nt_iChar",			\
		"nt_notiChar",		\
		"nt_String",		\
		"nt_notString",		\
		"nt_iString",		\
		"nt_notiString",	\
		"nt_List",			\
		"nt_notList",		\
		"nt_iList",			\
		"nt_notiList",		\
		"nt_AnyChar",		\
		"nt_Alternative", 	\
		"nt_Subexpr",	   	\
		"nt_ExtractStr",	\
		"nt_EndExtract",  	\
		"nt_Regex",	   		\
		"nt_Regex2",	   	\
		"nt_EOS",			\
		"nt_WS",			\
		"nt_reg",			\
		"nt_reg8",			\
		"nt_reg16",			\
		"nt_reg32",			\
		"nt_regfpu",		\
		"nt_regmmx",		\
		"nt_regxmm",		\
		"nt_matchid",		\
		"nt_matchInt",		\
		"nt_matchReal",		\
		"nt_matchNum",		\
		"nt_matchStr",		\
		"nt_matchWord",		\
		"nt_matchiWord",	\
		"nt_matchAdrs",		\
		"nt_matchExpr",		\
		"nt_matchBalanced",	\
		"nt_peekChar",		\
		"nt_peekiChar",		\
		"nt_peekStr",		\
		"nt_peekiStr",		\
		"nt_peekCset",		\
		"nt_pos",			\
		"nt_tab",			\
		"nt_at",			\
		"nt_match",			\
		"nt_match2"			\
	}


struct regexListType
{
	struct	regexListType	*concat;
	struct	regexListType	*subexpr;
	struct	regexListType	*alt[2];
	char					*extractEnd;
	int						nodeType;
	int						minCnt;
	int						maxCnt;
	int						lazy;
	char					*returns;
	struct	SymNode			*id;
	struct	SymNode			v;
};


struct regexStack
{
	int		rxSP;
	int		size;
	struct	regexListType	*rs[1];	// Allocated dynamically
};


extern void dumpRegex( struct regexListType *regexToPrint );

extern char *matchRegex
( 
	char 					*s, 
	struct regexListType	*rx,
	struct SymNode			*remainder,
	struct SymNode			*matched, 
	struct SymNode			*returnStr 
);

extern char *matchRegex2
( 
	char 					*s, 
	struct regexListType	*rx,
	struct SymNode			*remainder,
	struct SymNode			*matched, 
	struct SymNode			*returnStr 
);

#endif

