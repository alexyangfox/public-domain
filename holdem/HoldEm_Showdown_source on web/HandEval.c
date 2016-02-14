/* HandEval.c           by Steve Brecher    version 26Jun10.0 */

#include "HandEval.h"

Hand_T emptyHand = {0};

#define CARD_WIDTH (4)
#define BOT_CARDS_MASK(n)   ((1<<(n*CARD_WIDTH))-1)
#define CARD_SHIFT(rank,n)  ((rank)<<((n)*CARD_WIDTH))
#define SHIFT4(rank)        (CARD_SHIFT((rank),4))
#define SHIFT3(rank)        (CARD_SHIFT((rank),3))
#define SHIFT2(rank)        (CARD_SHIFT((rank),2))
#define SHIFT1(rank)        (CARD_SHIFT((rank),1))
#define TOP_CARD(mask)      (SHIFT4(hiRank[(mask)]))
#define SECOND_CARD(mask)   (SHIFT3(hiRank[(mask)]))
#define THIRD_CARD(mask)    (SHIFT2(hiRank[(mask)]))
#define MASK(rank)          (1<<(rank))

#define Value(a)        ((Eval_T)(a) << RESULT_VALUE_SHIFT)

#if d_flop_game
#define with(kickers) | (kickers)
#else
#define with(kickers)
#endif

/* Pointers to heap blocks for which subscript is bit mask of card ranks
   in hand: */
typedef int	Mask_T;

#define kArraySize (0x1FC0 + 1)	/* all combos of up to 7 of LS 13 bits on */

static int      *straightValue; /* Value(STRAIGHT) | (straight's high card rank-2 (2..12) << CARD5_SHIFT); 0 if no straight */
static int	    *nbrOfRanks;    /* count of bits set */
static int      *hiRank;        /* 4-bit card rank of highest bit set, right justified */
static int      *hiUpTo5Ranks;  /* 4-bit card ranks of highest (up to) 5 bits set, right-justified */

static int      *lo3_8OBRanksMask;	/* bits other than lowest 3 8-or-better reset;
								   0 if not at least 3 8-or-better bits set */
static Eval_T   *loMaskOrNo8Low; /* low-order 5 of the low-order 8 bits set, or constant
									indicating no 8-or-better low */

#if d_asian /* ranks 2-6 removed from deck; 5-card stud */
#define d_wheel 0x000011E0 /* AT987 */
#else /* { */
#define d_wheel 0x0000100F /* A5432 */

/* x is the ranks mask for a suit; N is the number of cards available */
/* These macros assume availablility of scratchpad variables: Mask_T i,j */
#define mFlushOrStraightFlush(x,N)                          \
	if ((j += nbrOfRanks[x]) > (N-5)) {                     \
		if (nbrOfRanks[x] >= 5)                             \
			if (!(i = straightValue[x]))					\
				return Value(FLUSH) | hiUpTo5Ranks[x];		\
			else                                            \
				return (Value(STRAIGHT_FLUSH) - Value(STRAIGHT)) + i; }
/* This macro uses the variables: Mask_T ranks, c, d, h, s */
#define mStraightAndOrFlush(N)                              \
	j = 0;                                                  \
    mFlushOrStraightFlush(c,N) else                         \
    mFlushOrStraightFlush(d,N) else                         \
    mFlushOrStraightFlush(h,N) else                         \
	    /* total cards in other suits <= N-5; spade flush:*/ \
        if (!(i = straightValue[s]))						\
            return Value(FLUSH) | hiUpTo5Ranks[s];			\
        else                                                \
            return (Value(STRAIGHT_FLUSH) - Value(STRAIGHT)) + i; \
    if ((i = straightValue[ranks]) != 0)					\
        return i;

#define CLUBS (hand.bySuit.clubs)
#define DIAMONDS (hand.bySuit.diamonds)
#define HEARTS (hand.bySuit.hearts)
#define SPADES (hand.bySuit.spades)

d_prefix Eval_T Hand_7_Eval(Hand_T hand)
{
    Mask_T  i, j, ranks,
			c = CLUBS, d = DIAMONDS, h = HEARTS, s = SPADES;

    switch (nbrOfRanks[ranks = c | d | h | s]) {

        case 2: /* quads with trips kicker */
                i = c & d & h & s;  /* bit for quads */
                return Value(FOUR_OF_A_KIND) | TOP_CARD(i) with(SECOND_CARD(i ^ ranks));

        case 3: /* trips and pair (full house) with non-playing pair,
                   or two trips (full house) with non-playing singleton,
                   or quads with pair and singleton */
                /* bits of singleton, if any, and trips, if any: */
                if (nbrOfRanks[i = c ^ d ^ h ^ s] == 3) {
                    /* two trips (full house) with non-playing singleton */
                    if (nbrOfRanks[i = c & d] != 2)
                        if (nbrOfRanks[i = c & h] != 2)
                            if (nbrOfRanks[i = c & s] != 2)
                                if (nbrOfRanks[i = d & h] != 2)
                                    if (nbrOfRanks[i = d & s] != 2)
                                        i = h & s;  /* bits of the trips */
                    return Value(FULL_HOUSE) | SHIFT3(hiUpTo5Ranks[i]); }
                 if ((j = c & d & h & s) != 0)   /* bit for quads */
                    /* quads with pair and singleton */
                    return Value(FOUR_OF_A_KIND) | TOP_CARD(j) with(SECOND_CARD(ranks ^ j));
                /* trips and pair (full house) with non-playing pair */
                return Value(FULL_HOUSE) | TOP_CARD(i) with(SECOND_CARD(ranks ^ i));

        case 4: /* three pair and singleton,
                   or trips and pair (full house) and two non-playing
                    singletons,
                   or quads with singleton kicker and two non-playing
                    singletons */
                i = c ^ d ^ h ^ s; /* the bit(s) of the trips, if any,
                                    and singleton(s) */
                if (nbrOfRanks[i] == 1) {   
                    /* three pair and singleton */
                    j = hiUpTo5Ranks[ranks ^ i]; /* ranks of the 3 pairs */
                    c = j & BOT_CARDS_MASK(1); /* rank of the worst pair */
                    return Value(TWO_PAIR) | SHIFT2(j ^ c) | THIRD_CARD(i | MASK(c)); }
                 if (!(j = c & d & h & s)) {
                    /* trips and pair (full house) and two non-playing
                        singletons */
                    i ^= ranks;     /* bit for the pair */
                    if (!(j = (c & d) & (~i)))
                        j = (h & s) & (~i); /* bit for the trips */
                    return Value(FULL_HOUSE) | TOP_CARD(j) with(SECOND_CARD(i)); }
                /* quads with singleton kicker and two
                    non-playing singletons */
                return Value(FOUR_OF_A_KIND) | TOP_CARD(j) with(SECOND_CARD(i));

        case 5: /* flush and/or straight,
                   or two pair and three singletons,
                   or trips and four singletons */
                mStraightAndOrFlush(7)
                i = c ^ d ^ h ^ s; /* the bits of the trips, if any,
                                      and singletons */
                if (nbrOfRanks[i] != 5) {
                    /* two pair and three singletons */
                    j = i ^ ranks;  /* the two bits of the pairs */
                    return Value(TWO_PAIR) | SHIFT3(hiUpTo5Ranks[j]) | THIRD_CARD(i); }
                /* trips and four singletons */
                if (!(j = c & d))
                    j = h & s;
                return Value(THREE_OF_A_KIND) | TOP_CARD(j)
                    with(hiUpTo5Ranks[i ^ j] & (~BOT_CARDS_MASK(2)));

        case 6: /* flush and/or straight,
                   or one pair and three kickers and two nonplaying singletons */
                mStraightAndOrFlush(7)
                i = c ^ d ^ h ^ s; /* the bits of the five singletons */
                return Value(PAIR) | TOP_CARD(ranks ^ i) | ((hiUpTo5Ranks[i] >> CARD_WIDTH) & (~BOT_CARDS_MASK(1)));

        case 7: /* flush and/or straight or no pair */
                mStraightAndOrFlush(7)
                return /* Value(NO_PAIR) | */ hiUpTo5Ranks[ranks];

        } /* end switch */

    return 0; /* never reached, but avoids compiler warning */
}

// each of the following handXlo extracts the appropriate 13-bit field from hand and
// rotates it left to position the ace in the least significant bit
#define CLUBS_LO ( ((CLUBS & 0xFFF) << 1) + (CLUBS >> 12) )
#define DIAMONDS_LO ( ((DIAMONDS & 0xFFF) << 1) + (DIAMONDS >> 12) )
#define HEARTS_LO ( ((HEARTS & 0xFFF) << 1) + (HEARTS >> 12) )
#define SPADES_LO ( ((SPADES & 0xFFF) << 1) + (SPADES >> 12) )

d_prefix Eval_T Hand_Razz_Eval(Hand_T hand)
{
    Mask_T  i, j, ranks,
			c = CLUBS_LO,
			d = DIAMONDS_LO,
			h = HEARTS_LO,
			s = SPADES_LO;

	switch (nbrOfRanks[ranks = c | d | h | s]) {

        case 2: /* AAAABBB -- full house */
                i = c & d & h & s;  /* bit for quads */
				j = i ^ ranks;		/* bit for trips */
				// it can't matter in comparison of results from a 52-card deck,
				// but we return the correct value per relative ranks
				if (i < j)
					return Value(FULL_HOUSE) | TOP_CARD(i) with(SECOND_CARD(j));
				return Value(FULL_HOUSE) | TOP_CARD(j) with(SECOND_CARD(i));

        case 3: /*	AAABBBC -- two pair,
                    AAAABBC -- two pair,
 					AAABBCC -- two pair w/ kicker = highest rank.
               /* bits of singleton, if any, and trips, if any: */
                if (nbrOfRanks[i = c ^ d ^ h ^ s] == 3) {
                    /* odd number of each rank: AAABBBC -- two pair */
                    if (nbrOfRanks[i = c & d] != 2)
                        if (nbrOfRanks[i = c & h] != 2)
                            if (nbrOfRanks[i = c & s] != 2)
                                if (nbrOfRanks[i = d & h] != 2)
                                    if (nbrOfRanks[i = d & s] != 2)
                                        i = h & s;  /* bits of the trips */
                    return Value(TWO_PAIR) | SHIFT3(hiUpTo5Ranks[i])
							| THIRD_CARD(ranks ^ i); }
                if ((j = c & d & h & s) != 0) {   /* bit for quads */
                    /* AAAABBC -- two pair */
					j = ranks ^ i;				/* bits of pairs */
                    return Value(TWO_PAIR) | SHIFT3(hiUpTo5Ranks[j])
							| THIRD_CARD(i);
                }
                /* AAABBCC -- two pair w/ kicker = highest rank */
                i = hiUpTo5Ranks[ranks]; /* 00KPP 20Jun10.0 */
                j = i & (~(BOT_CARDS_MASK(2))); /* 00K00 */
                return Value(TWO_PAIR) | SHIFT3(i ^ j) | j;

        case 4: /*	AABBCCD -- one pair (lowest of A, B, C),
					AAABBCD -- one pair (A or B),
					AAAABCD -- one pair (A)
				 */
                i = c ^ d ^ h ^ s; /* the bit(s) of the trips, if any,
                                    and singleton(s) */
                if (nbrOfRanks[i] == 1) {   /* 26Jun10.0: */
					/* AABBCCD -- one pair, C with ABD; D's bit is in i */
					j = ranks ^ i;	/* ABC bits */
					ranks = hiUpTo5Ranks[j] & 0x0000F;	/* C rank */
					i |= j ^ (1 << (ranks));	/* ABD bits */
					return Value(PAIR) | SHIFT4(ranks) | SHIFT1(hiUpTo5Ranks[i]); }
                if (!(j = c & d & h & s)) {
                    /* AAABBCD -- one pair (A or B) */
                    i ^= ranks;     /* bit for B */
                    if (!(j = (c & d) & (~i)))
                        j = (h & s) & (~i); /* bit for A */
					if (i < j)
						return Value(PAIR) | TOP_CARD(i) | SHIFT1(hiUpTo5Ranks[ranks ^ i]);
                    return Value(PAIR) | TOP_CARD(j) | SHIFT1(hiUpTo5Ranks[ranks ^ j]); }
                /* AAAABCD -- one pair (A) */
                return Value(PAIR) | TOP_CARD(j) | SHIFT1(hiUpTo5Ranks[i]);	// 20Jun10.0

        case 5: return /* Value(NO_PAIR) | */ hiUpTo5Ranks[ranks];

        case 6: ranks ^= MASK(hiRank[ranks]);
                return /* Value(NO_PAIR) | */ hiUpTo5Ranks[ranks];

        case 7:	ranks ^= MASK(hiRank[ranks]);
                ranks ^= MASK(hiRank[ranks]);
                return /* Value(NO_PAIR) | */ hiUpTo5Ranks[ranks];

        } /* end switch */

    return 0; /* never reached, but avoids compiler warning */
}

d_prefix Eval_T Hand_6_Eval(Hand_T hand)
{
    Mask_T  i, j, ranks,
			c = CLUBS, d = DIAMONDS, h = HEARTS, s = SPADES;

    switch (nbrOfRanks[ranks = c | d | h | s]) {

        case 2: /* quads with pair kicker,
				   or two trips (full house) */
				/* bits of trips, if any: */
                if (nbrOfRanks[i = c ^ d ^ h ^ s])
                    /* two trips (full house) */
					return Value(FULL_HOUSE) | SHIFT3(hiUpTo5Ranks[i]);
				/* quads with pair kicker */
                i = c & d & h & s;  /* bit for quads */
                return Value(FOUR_OF_A_KIND) | TOP_CARD(i)
							with(SECOND_CARD(i ^ ranks));

		case 3:	/* quads with singleton kicker and non-playing
				    singleton,
				   or full house with non-playing singleton,
				   or two pair with non-playing pair */
				if (!(c ^ d ^ h ^ s))
					/* no trips or singletons:  three pair */
                    return Value(TWO_PAIR) | SHIFT2(hiUpTo5Ranks[ranks]);
				if ((i = c & d & h & s) == 0) {
					/* full house with singleton */
					if (!(i = c & d & h))
						if (!(i = c & d & s))
							if (!(i = c & h & s))
								i = d & h & s; /* bit of trips */
					j = c ^ d ^ h ^ s; /* the bits of the trips
										  and singleton */
					return Value(FULL_HOUSE) | TOP_CARD(i)
							with(SECOND_CARD(j ^ ranks)); }
				/* quads with kicker and singleton */
				return Value(FOUR_OF_A_KIND) | TOP_CARD(i)
						with(SECOND_CARD(i ^ ranks));

		case 4:	/* trips and three singletons,
				   or two pair and two singletons */
				if ((i = c ^ d ^ h ^ s) != ranks) {
					/* two pair and two singletons; i has the singleton bits */
                    return Value(TWO_PAIR) | SHIFT3(hiUpTo5Ranks[ranks ^ i])
                            | THIRD_CARD(i); }
				/* trips and three singletons */
				if (!(i = c & d))
					i = h & s; /* bit of trips */
				return Value(THREE_OF_A_KIND) | TOP_CARD(i)
						with(SHIFT1(hiUpTo5Ranks[ranks ^ i] & (~BOT_CARDS_MASK(1))));	/* 08May10.1 */

		case 5:	/* flush and/or straight,
				   or one pair and three kickers and
				    one non-playing singleton */
				mStraightAndOrFlush(6)
                i = c ^ d ^ h ^ s; /* the bits of the four singletons */
                return Value(PAIR) | TOP_CARD(ranks ^ i) | (hiUpTo5Ranks[i] & (~BOT_CARDS_MASK(1)));

		case 6:	/* flush and/or straight or no pair */
                mStraightAndOrFlush(6)
                return /* Value(NO_PAIR) | */ hiUpTo5Ranks[ranks];

        } /* end switch */

    return 0; /* never reached, but avoids compiler warning */
}
#endif  /* d_asian } */

#if d_asian
d_prefix Eval_T Asian_5_Eval(Hand_T hand)
#else
d_prefix Eval_T Hand_5_Eval(Hand_T hand)
#endif
{
    Mask_T  i, j, ranks,
			c = CLUBS, d = DIAMONDS, h = HEARTS, s = SPADES;

	switch (nbrOfRanks[ranks = c | d | h | s]) {

        case 2: /* quads or full house */
                i = c & d;				/* any two suits */
                if (!(i & h & s)) {     /* no bit common to all suits */
                    i = c ^ d ^ h ^ s;  /* trips bit */
                    return Value(FULL_HOUSE) | TOP_CARD(i) with(SECOND_CARD(i ^ ranks)); }
                else
                    /* the quads bit must be present in each suit mask,
                       but the kicker bit in no more than one; so we need
                       only AND any two suit masks to get the quad bit: */
                    return Value(FOUR_OF_A_KIND) | TOP_CARD(i) with(SECOND_CARD(i ^ ranks));

        case 3: /* trips and two kickers,
                   or two pair and kicker */
                if ((i = c ^ d ^ h ^ s) == ranks) {
                    /* trips and two kickers */
                    if ((i = c & d) == 0)
                    	if ((i = c & h) == 0)
                    		i = d & h;
                    return Value(THREE_OF_A_KIND) | TOP_CARD(i) with(SHIFT2(hiUpTo5Ranks[i ^ ranks])); }	// 20Jun10.0
                /* two pair and kicker; i has kicker bit */
                j = i ^ ranks;      /* j has pairs bits */
                return Value(TWO_PAIR) | SHIFT3(hiUpTo5Ranks[j]) | THIRD_CARD(i);

        case 4: /* pair and three kickers */
                i = c ^ d ^ h ^ s; /* kicker bits */
				return Value(PAIR) | TOP_CARD(ranks ^ i) | SHIFT1(hiUpTo5Ranks[i]);	/* 08May10.0 */

        case 5: /* flush and/or straight, or no pair */
				if (!(i = straightValue[ranks]))
					i = hiUpTo5Ranks[i];
				if (c != 0) {			/* if any clubs... */
					if (c != ranks)		/*   if no club flush... */
						return i; }		/*      return straight or no pair value */
				else
					if (d != 0) {
						if (d != ranks)
							return i; }
					else
						if (h != 0) {
							if (h != ranks)
								return i; }
					/*	else s == ranks: spade flush */
				/* There is a flush */
				if (i < Value(STRAIGHT))
					/* no straight */
					return Value(FLUSH) | i;
				else
					return (Value(STRAIGHT_FLUSH) - Value(STRAIGHT)) + i;
	}

    return 0; /* never reached, but avoids compiler warning */
}

d_prefix Eval_T Hand_2_to_7_Eval(Hand_T hand)
{

#define WHEEL_EVAL			0x04030000
#define WHEEL_FLUSH_EVAL	0x08030000
#define NO_PAIR_ACE_HIGH	0x000C3210

	Eval_T result = Hand_5_Eval(hand);
	if (result == WHEEL_EVAL)
		return NO_PAIR_ACE_HIGH;
	if (result == WHEEL_FLUSH_EVAL)
		return Value(FLUSH) | NO_PAIR_ACE_HIGH;
	return result;
}

#if !d_asian

d_prefix Eval_T Hand_5_Ato5Lo_Eval(Hand_T hand)
{
    Mask_T  i, j, ranks,
			c = CLUBS_LO,
			d = DIAMONDS_LO,
			h = HEARTS_LO,
			s = SPADES_LO;

	switch (nbrOfRanks[ranks = c | d | h | s]) {

        case 2: /* quads or full house */
                i = c & d;				/* any two suits */
                if (!(i & h & s)) {     /* no bit common to all suits */
                    i = c ^ d ^ h ^ s;  /* trips bit */
                    return Value(FULL_HOUSE) | TOP_CARD(i) with(SECOND_CARD(i ^ ranks)); }
                else
                    /* the quads bit must be present in each suit mask,
                       but the kicker bit in no more than one; so we need
                       only AND any two suit masks to get the quad bit: */
                    return Value(FOUR_OF_A_KIND) | TOP_CARD(i) with(SECOND_CARD(i ^ ranks));

        case 3: /* trips and two kickers,
                   or two pair and kicker */
                if ((i = c ^ d ^ h ^ s) == ranks) {
                    /* trips and two kickers */
                    if ((i = c & d) == 0)
                    	if ((i = c & h) == 0)
                    		i = d & h;
                    return Value(THREE_OF_A_KIND) | TOP_CARD(i) with(SHIFT2(hiUpTo5Ranks[i ^ ranks])); } // 20Jun10.0
                /* two pair and kicker; i has kicker bit */
                j = i ^ ranks;      /* j has pairs bits */
                return Value(TWO_PAIR) | SHIFT3(hiUpTo5Ranks[j]) | THIRD_CARD(i);

        case 4: /* pair and three kickers */
                i = c ^ d ^ h ^ s; /* kicker bits */
                return Value(PAIR) | TOP_CARD(ranks ^ i) | SHIFT1(hiUpTo5Ranks[i]);

        case 5: /* no pair */
				return hiUpTo5Ranks[ranks];
	}

    return 0; /* never reached, but avoids compiler warning */
}
#endif

d_prefix int Hi_Card_Rank(short mask)
{
    return hiRank[mask];
}

d_prefix int Number_Of_Ranks(short mask)
{
    return nbrOfRanks[mask];
}

/*
 * Sets rank and suit of the first (usually only) card in hand
 * looking in order of clubs, diamonds, hearts, spades and from
 * low to high rank within each. If hand is empty *rank and
 * *suit are unchanged.
 */
d_prefix void Decode(Hand_T hand, int *rank, int *suit)
{
    int m, r, s;

    s = (int)kClubs;
    if (!(m = CLUBS)) {
        s = (int)kDiamonds;
        if (!(m = DIAMONDS)) {
            s = (int)kHearts;
            if (!(m = HEARTS)) {
                s = (int)kSpades;
                m = SPADES;
            }
        }
    }
    for (r = 2; r <= 14; ++r, m >>= 1)
        if (m & 1) {
            *rank = r;
            *suit = s;
            break;
        }
    return;
}

#if !d_asian
d_prefix Eval_T Hand_8Low_Eval(Hand_T hand) {

	int result = loMaskOrNo8Low[CLUBS_LO | DIAMONDS_LO | HEARTS_LO | SPADES_LO];
    if (result != NO_8_LOW)
        return hiUpTo5Ranks[result];
    return result;
}

d_prefix Eval_T Ranks_8Low_Eval(int ranks) {

	int result = loMaskOrNo8Low[ranks];
    if (result != NO_8_LOW)
        return hiUpTo5Ranks[result];
    return result;
}

d_prefix Eval_T Omaha8_Low_Eval(int twoHolesMask, int boardMask) {

    int result = loMaskOrNo8Low[lo3_8OBRanksMask[boardMask & ~twoHolesMask] | twoHolesMask];
    if (result != NO_8_LOW)
        return hiUpTo5Ranks[result];
    return result;
}

#endif

/************ Initialization ***********************/

static void SetStraight(int ts)
/* must call with ts from A..T to 5..A in that order */
{
    int es, i, j;

    for (i = 0x1000; i > 0; i >>= 1)
        for (j = 0x1000; j > 0; j >>= 1) {
            es = ts | i | j;	/* 5 straight bits plus up to two other bits */
			if (!straightValue[es])
				if (ts == d_wheel)
#if d_asian
                    straightValue[es] = Value(STRAIGHT) + SHIFT4(10-2);
#else
					straightValue[es] = Value(STRAIGHT) + SHIFT4(5-2); 
#endif
				else
					straightValue[es] = Value(STRAIGHT) + TOP_CARD(ts);
		}
}

#define kAce            (12)

#if d_asian
d_prefix int Init_Asian_Eval(void)
#else
d_prefix int Init_Hand_Eval(void)
#endif
{
  static	int initted = 0;

    int     mask, bitCount, ranks;
    Mask_T  shiftReg, i;
	Mask_T	value;

	if (initted)
		return 1;

#define mAllocate(type, array)                                  \
    array = (type *)(d_ram_alloc(kArraySize, sizeof(*array)));  \
    if (!array) return 0;

    mAllocate(Mask_T, straightValue);
    mAllocate(int, nbrOfRanks);
    mAllocate(Mask_T, hiRank);
    mAllocate(Mask_T, hiUpTo5Ranks);
	mAllocate(Mask_T, lo3_8OBRanksMask);
	mAllocate(Eval_T, loMaskOrNo8Low);
    loMaskOrNo8Low[0] = NO_8_LOW; /* other elements set in following loop */

    for (mask = 1; mask < kArraySize; ++mask) {
        bitCount = ranks = 0;
        shiftReg = mask;
        for (i = kAce; i >= 0; --i, shiftReg <<= 1)
            if (shiftReg & 0x1000) {
                if (++bitCount <= 5) {
                    ranks <<= CARD_WIDTH;
                    ranks += i;
                    if (bitCount == 1)
                        hiRank[mask] = i;
                }
            }
        hiUpTo5Ranks[mask] = ranks;
        nbrOfRanks[mask] = bitCount;

		loMaskOrNo8Low[mask] = NO_8_LOW;
        bitCount = value = 0;
		shiftReg = mask;
		/* For the purpose of this loop, Ace is low; it's in the LS bit */
        for (i = 0; i < 8; ++i, shiftReg >>= 1)
	        if ((shiftReg & 1) != 0) {
		        value |= (1 << i); /* undo previous shifts, copy bit */
		        if (++bitCount == 3)
			        lo3_8OBRanksMask[mask] = value;
                if (bitCount == 5) {
                    loMaskOrNo8Low[mask] = value;
					break;
				}
	        }
        }

    for (mask = 0x1F00/*A..T*/; mask >= 0x001F/*6..2*/; mask >>= 1)
        SetStraight(mask);
    SetStraight(d_wheel);       /* A,5..2 (A,T..7 for Asian stud) */

	initted = 1;

  return 1;
}
