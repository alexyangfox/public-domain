#ifndef TP_FPA2_H
#define TP_FPA2_H

/* The objects in FPA lists are pointers to terms, and the lists
   are ordered (decreasing) by fpa_id field in terms.

   The code was originally written and debugged with integers instead;
   hence, the following, which makes it easy to change back to ints.
*/

#define FTYPE struct term *
#define FDEFAULT NULL

#define FLT(x,y) ((x)->fpa_id <  (y)->fpa_id)
#define FGT(x,y) ((x)->fpa_id >  (y)->fpa_id)
#define FLE(x,y) ((x)->fpa_id <= (y)->fpa_id)
#define FGE(x,y) ((x)->fpa_id >= (y)->fpa_id)
#define FEQ(x,y) ((x) == (y))
#define FNE(x,y) ((x) != (y))

#define FTERM(p) ((p).f == NULL ? NULL : (p).f->d[(p).i])

/* A chunk of an FPA list */

#define FMAX 400  /* maximum number of items per chunk */

typedef struct fnode *Fnode;

struct fnode {
  FTYPE d[FMAX];    /* array for chunk */
  int n;            /* current size of chunk (right justified in array) */
  Fnode next;       /* list of chunks is singly-linked */
};


/* to maintain a position in an FPA list */

struct fposition {
  Fnode f;
  int i;
};

/* function prototypes */

Fnode flist_insert(Fnode f, FTYPE x);
Fnode flist_delete(Fnode f, FTYPE x);
struct fposition first_fpos(Fnode f);
struct fposition next_fpos(struct fposition p);

#endif  /* conditional compilation of whole file */
