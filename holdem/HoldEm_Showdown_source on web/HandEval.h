/* HandEval.h           by Steve Brecher */

/*
 *	26Jun10.0	Fix handRazzEval return for full house for AAAABBB rank pattern (make the
 *					lower rank the trips rank and the other the pair rank rather than vice-versa).
 *				Fix kicker ordering for handRazzEval one pair for AABBCCD rank pattern.
 *	23Jun10.0	Fix initialization of lo3_8OBRanksMask and loMaskOrNo8Low:
 *					remove rotation of mask which is unnecessary and was being done incorrectly.
 *				Fix results of Hand_2_to_7_Eval for wheel and wheel flush.
 *  20Jun10.1	Fix result of Hand_2_to_7_Eval for wheel flush.
 *	20Jun10.0	Fix a two pair result and a pair result in Hand_Razz_Eval.
 *				Fix a trips result in Hand_5_Eval.
 *				Fix a trips result in Hand_5_Ato5Lo_Eval.
 *				New function: Hand_2_to_7_Eval (Kansas City Lowball).
 *	08May10.1	Fix trips result in Hand_6_Eval.
 *	08May10.0	Fix one pair result in Hand_5_Eval.
 *  23Aug08.0	If 64-bit ints, use uint64_t from stdint.h.
 *  07Jan07.0   HandEval parameter and result the same as pokersource Eval_N.
 *              Number of lookup tables reduced.
 *              Portability improved.
 *  24Nov06.0   HandEval parameter is four 13-bit fields in low order 52 bits.
 *              Rank masks 13 low-order bits instead of 14 bits -- kArraySize reduced by half.
 *              HandEval result for straight/straight flush specifies high rank, not 5-bit mask.
 *	08Nov06.0	__int64 (MS-specific) instead of int[4] for Hand_X_Eval argument
 *	14Sep02.0	Minor code edits for clarity.
 *				Add Omaha8_Low_Eval function.
 *  10Jul02.0	Fix SetStraight (broken in unreleased 15May02.0).
 *	15May02.0	Use straightValue (formerly theStraight) nonzero in place of
 *					former Boolean straight[].
 *				Two pair eval result always includes kicker regardless
 *					of d_flop_game.
 *				Change type of nbrOfRanks[] and nbrOfRanksGE5[] to int, hoping
 *					for faster access than for char.
 *				Remove first (ranks summary) element from hand array.
 *	26Apr02.0	Finer decision granularity in mStraightAndOrFlush macro
 *					and likewise in straight/flush logic in Hand_5_Eval.
 *	14Apr02.0	Add Hand_6_Eval function.
 *				Change name of d_lang macro to d_prefix.
 *  04Aug96.0   AND only two suit masks to get quads bit in HandEval.
 *  14May96.0   Eliminate compiler warnings
 *  23May95.0   fix case of three pair and singleton to take kicker from higher
 *                  of 3rd-ranking pair or singleton instead of always from
 *                  3rd-ranking pair
 *              introduce Boolean nbrOfRanksGE5 array to help testing for flush;
 #              use bits 1 to 13 instead of 2 to 14 in masks;
 *              make lookup table elements longs instead of shorts;
 *              misc. minor optimizations, some due to Cliff Matthews et al;
 *              remove "unsigned" from Mask_T -- big win in code gen;
 *              remove total pots from output and re-format output;
 *              InitTables:  remove knowledge of types from calls to calloc;
 */

#ifndef d_HandEval_h    /* { */
#define d_HandEval_h
#pragma once

/* true for Asian stud: */
#ifndef d_asian
#define d_asian 0
#endif

/*
 *  If d_flop_game is nonzero then evaluation result includes cards for
 *  kickers with quads or trips, and non-playing pair with full house.
 *  This costs a little execution time.  I.e., set to zero for draw
 *  poker, stud poker, Chinese Poker; set to nonzero for hold 'em, Omaha,
 *  and other common-card games.
 */
#ifndef d_flop_game
#define d_flop_game 1
#endif

/* Comment out one of the following #defines */
/* If appropriate change the following typedef as necessary */
#define HAVE_INT64 1
//#define HAVE_INT64 0

#if HAVE_INT64
#include <stdint.h>
typedef uint64_t uint64;
#endif

typedef unsigned int uint32; /* used in Hand_T definition */

/* Function which allocates zero-filled bytes on the heap: */
/* optional for Macintosh: */
/* #define d_ram_alloc(quantity, size) NewPtrClear((quantity) * (size)) */
/* Portable ANSI C: */
#include <stdlib.h>
#define d_ram_alloc(quantity, size) calloc(quantity, size)

/* function prefix */
#ifdef d_is_DLL
/* We are making a DLL for Windows... */
#define d_prefix __declspec(dllexport)
#else
#define d_prefix
/* For making a library to be used with Macintosh Pascal: */
/* #define d_prefix pascal */
#endif

/*
	 * A hand is 0 or more cards represented in four 16-bit masks, one
	 * mask per suit, in a 64-bit integer. In each mask, bit 0 set (0x0001)
     * for a deuce, ..., bit 12 set (0x1000) for an ace. Each mask denotes
     * the ranks present in one of the suits.
 */

#define Encode(rank, suit) (((uint64)1) << (16*suit + rank - 2))
#define IndexToMask(index) (((uint64)1) << (16*(index/13) + index%13))

enum {kClubs, kDiamonds, kHearts, kSpades, kNbrOfSuits}; /* ordering within Hand_T */
/* The ordering is significant only to the Decode untility function */

enum {NO_PAIR, PAIR, TWO_PAIR, THREE_OF_A_KIND,
        STRAIGHT, FLUSH, FULL_HOUSE, FOUR_OF_A_KIND, STRAIGHT_FLUSH};

typedef unsigned long Eval_T; /*  Evaluation result	*/
	/*
	 * Different functions are called for high and for lowball evaluation.
	 *
	 * For high evaluation if results R1 > R2, hand 1 beats hand 2;
	 * for lowball evaluation if results R1 > R2, hand 2 beats hand 1.
	 *
	 * Evaluation result in 32 bits = 0x0V0RRRRR where V, R are
     * hex digits or "nybbles" (half-bytes).
	 * 
	 * V nybble = value code (NO_PAIR..STRAIGHT_FLUSH)
     * The R nybbles are the significant ranks (0..12), where 0 is the Ace
     * in a lowball result (King is 12, 0xC), and otherwise 0 is the deuce
     * (Ace is 0xC).  The Rs may be considered to consist of Ps for ranks
     * which determine the primary value of the hand, and Ks for kickers
     * where applicable.  Ordering is left-to-right:  first the Ps, then
     * any Ks, then padding with 0s.  Because 0 is a valid rank, to
     * deconstruct a result you must know how many ranks are significant,
     * which is a function of the value code and whether high or lowball.
     * E.g. (high where not indicated):
     *  Royal flush: 0x080C0000
     *  Four of a kind, Queens, with a 5 kicker:  0x070A3000
     *  Threes full of eights:  0x06016000
     *  Straight to the five (wheel): 0x04030000 (high)
     *  Straight to the five (wheel): 0x04040000 (lowball)
     *  One pair, deuces (0x0), with A65: 0x0100C430 (high)
     *  One pair, deuces (0x1), with 65A: 0x01015400 (lowball)
     *  No pair, KJT85: 0x000B9863
     *  Razz, wheel:  0x00043210
     *
     * For the eight-or-better lowball ..._Eval functions, the result is
     * either as above or the constant NO_8_LOW.  NO_8_LOW > any other
     * ..._Eval function result.
	 */
#define RESULT_VALUE_SHIFT  24

/* eval result meaning no 8-or-better low: */
#define NO_8_LOW	0x0FFFFFFF

typedef union {
    // assume little-endian (e.g., Intel) for now
    struct {
        unsigned short clubs;
        unsigned short diamonds;
        unsigned short hearts;
        unsigned short spades;
   } bySuit;
#if HAVE_INT64
    uint64 as64Bits;
#else
    struct {
        uint32 cd;
        uint32 hs;
    } as2x32Bits;
#endif
} Hand_T;

#ifdef __cplusplus
extern "C" {
#endif

/************
 ************ AN Init_... FUNCTION MUST BE CALLED BEFORE
 ************ CALLING ANY OTHER FUNCTION! (except Decode)
 ************/
#if d_asian
d_prefix int Init_Asian_Eval(void);   /* returns false if heap allocation
                                            fails */
d_prefix Eval_T Asian_5_Eval(Hand_T hand);
#else
d_prefix int Init_Hand_Eval(void);    /* returns false if heap allocation
                                            fails */
d_prefix Eval_T Hand_7_Eval(Hand_T hand);
d_prefix Eval_T Hand_6_Eval(Hand_T hand);
d_prefix Eval_T Hand_5_Eval(Hand_T hand);
d_prefix Eval_T Hand_5_Ato5Lo_Eval(Hand_T hand);
d_prefix Eval_T Hand_Razz_Eval(Hand_T hand);
d_prefix Eval_T Hand_2_to_7_Eval(Hand_T hand);

d_prefix Eval_T Hand_8Low_Eval(Hand_T hand);
d_prefix Eval_T Ranks_8Low_Eval(int ranks);
d_prefix Eval_T Omaha8_Low_Eval(int twoHolesMask, int boardMask);

#endif

/* Utilities */
/* Init_..._Eval must have been previously called! */
/* mask argument must not exceed 0x1FC0 */
d_prefix int Hi_Card_Rank(short mask);    /* rank of highest bit set */
d_prefix int Number_Of_Ranks(short mask);    /* number of bits set in mask */

/*
 * Sets rank and suit of the first (usually only) card in hand
 * looking in order of clubs, diamonds, hearts, spades and from
 * low to high rank within each. If hand is empty *rank and
 * *suit are unchanged.
 */
d_prefix void Decode(Hand_T hand, int *rank, int *suit);

extern Hand_T emptyHand;

#ifdef __cplusplus
}
#endif

#define Hand_T_Rank_Suit_Table(tableName)  \
    Hand_T tableName##[13][4] = {              \
        {{   1,0,0,0}, {0,   1,0,0}, {0,0,   1,0}, {0,0,0,   1}}, \
        {{   2,0,0,0}, {0,   2,0,0}, {0,0,   2,0}, {0,0,0,   2}}, \
        {{   4,0,0,0}, {0,   4,0,0}, {0,0,   4,0}, {0,0,0,   4}}, \
        {{   8,0,0,0}, {0,   8,0,0}, {0,0,   8,0}, {0,0,0,   8}}, \
        {{  16,0,0,0}, {0,  16,0,0}, {0,0,  16,0}, {0,0,0,  16}}, \
        {{  32,0,0,0}, {0,  32,0,0}, {0,0,  32,0}, {0,0,0,  32}}, \
        {{  64,0,0,0}, {0,  64,0,0}, {0,0,  64,0}, {0,0,0,  64}}, \
        {{ 128,0,0,0}, {0, 128,0,0}, {0,0, 128,0}, {0,0,0, 128}}, \
        {{ 256,0,0,0}, {0, 256,0,0}, {0,0, 256,0}, {0,0,0, 256}}, \
        {{ 512,0,0,0}, {0, 512,0,0}, {0,0, 512,0}, {0,0,0, 512}}, \
        {{1024,0,0,0}, {0,1024,0,0}, {0,0,1024,0}, {0,0,0,1024}}, \
        {{2048,0,0,0}, {0,2048,0,0}, {0,0,2048,0}, {0,0,0,2048}}, \
        {{4096,0,0,0}, {0,4096,0,0}, {0,0,4096,0}, {0,0,0,4096}}  \
   }

#define Hand_T_Deck_Index_Table(tableName) \
    Hand_T tableName##[52] = {                   \
        {1,0,0,0}, {2,0,0,0}, {4,0,0,0}, {8,0,0,0}, {16,0,0,0}, {32,0,0,0}, {64,0,0,0}, \
        {128,0,0,0}, {256,0,0,0}, {512,0,0,0}, {1024,0,0,0}, {2048,0,0,0}, {4096,0,0,0}, \
        {0,1,0,0}, {0,2,0,0}, {0,4,0,0}, {0,8,0,0}, {0,16,0,0}, {0,32,0,0}, {0,64,0,0}, \
        {0,128,0,0}, {0,256,0,0}, {0,512,0,0}, {0,1024,0,0}, {0,2048,0,0}, {0,4096,0,0}, \
        {0,0,1,0}, {0,0,2,0}, {0,0,4,0}, {0,0,8,0}, {0,0,16,0}, {0,0,32,0}, {0,0,64,0}, \
        {0,0,128,0}, {0,0,256,0}, {0,0,512,0}, {0,0,1024,0}, {0,0,2048,0}, {0,0,4096,0}, \
        {0,0,0,1}, {0,0,0,2}, {0,0,0,4}, {0,0,0,8}, {0,0,0,16}, {0,0,0,32}, {0,0,0,64}, \
        {0,0,0,128}, {0,0,0,256}, {0,0,0,512}, {0,0,0,1024}, {0,0,0,2048}, {0,0,0,4096} \
    }

#if HAVE_INT64
#define AddHandTo(toThisHand, addThisHand) \
    do { (toThisHand).as64Bits |= (addThisHand).as64Bits; } while (0)
#define CombineHands(dest, source1, source2) \
    do { (dest).as64Bits = (source1).as64Bits | (source2).as64Bits; } while (0)
#define ZeroHand(hand) \
    do { (hand).as64Bits = 0; } while (0)
#else
#define AddHandTo(addThisHand, toThisHand) \
    do { \
        (toThisHand).as2x32Bits.cd |= (addThisHand).as2x32Bits.cd; \
        (toThisHand).as2x32Bits.hs |= (addThisHand).as2x32Bits.hs; \
    while (0)
#define CombineHands(dest, source1, source2) \
    do { \
        (dest).as2x32Bits.cd = (source1).as2x32Bits.cd | (source2).as2x32Bits.cd; \
        (dest).as2x32Bits.hs = (source1).as2x32Bits.hs | (source2).as2x32Bits.hs; \
    while (0)
#define ZeroHand(hand) \
    do { \
        (hand).as2x32Bits.cd = 0; \
        (hand).as2x32Bits.hs = 0; \
    } while (0)
#endif

#endif  /* } */
