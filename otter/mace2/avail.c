#include "Mace2.h"  /* for MACE_Parms[], MACE_abend(). */

#define TP_ALLOC_SIZE 5000000   /* Size of blocks allocated by malloc */

static char *Alloc_block;     /* location returned by most recent malloc */
static char *Alloc_pos;       /* current position in block */

static int Malloc_calls;  /* number of calls to malloc */
static int Free_calls;    /* number of calls to free */

/* Maintain a list of pointers to the malloced chunks so they can be freed. */

static Gen_ptr_ptr Chunks;

/*************
 *
 *    add_chunk -- to the front of Chunks
 *
 *************/

static void add_chunk(char *p)
{
  Gen_ptr_ptr new = malloc(sizeof(struct gen_ptr));
  new->u.v = p;
  new->next = Chunks;
  Chunks = new;
}  /* add_chunk */

/*************
 *
 *    free_chunks
 *
 *************/

static void free_chunks(Gen_ptr_ptr p)
{
  if (p != NULL) {
    free_chunks(p->next);  /* free the tail */
    free(p->u.v);          /* free the chunk */
    Free_calls++;
    free(p);               /* free this node */
  }
}  /* free_chunks */

/*************
 *
 *    reinit_mem
 *
 *************/

void reinit_mem(void)
{
  free_chunks(Chunks);  /* free previously malloced memory */
  Chunks = NULL;
  Alloc_block = NULL;
  Alloc_pos = NULL;
}  /* reinit_mem */

/*************
 *
 *    void *MACE_tp_alloc(n)
 *
 *    Allocate n contiguous bytes, aligned on pointer boundry.
 *
 *************/

void *MACE_tp_alloc(size_t n)
{
  char *return_block;
  int scale;
    
  /* if n is not a multiple of sizeof(void *), then round up so that it is */
    
  scale = sizeof(void *);
  if (n % scale != 0)
    n = n + (scale - (n % scale));
    
  if (!Alloc_block || Alloc_block + TP_ALLOC_SIZE - Alloc_pos < n) {
    /* try to malloc a new block */
    if (n > TP_ALLOC_SIZE)
      MACE_abend("in MACE_tp_alloc, request too big.");
    else if (MACE_Parms[MACE_MAX_MEM].val != -1 &&
	     ((Malloc_calls - Free_calls)+1)*(TP_ALLOC_SIZE/1024.0) >
	     MACE_Parms[MACE_MAX_MEM].val) {

      exit_with_message(MACE_MAX_MEM_EXIT, 1);
    }
    else {
      Alloc_pos = Alloc_block = (char *) malloc((size_t) TP_ALLOC_SIZE);
      if (!Alloc_pos)
	MACE_abend("in MACE_tp_alloc, operating system cannot supply any more memory.");
      Malloc_calls++;
      add_chunk(Alloc_block);
    }
  }
  return_block = Alloc_pos;
  Alloc_pos += n;
  return(return_block);
}  /* MACE_tp_alloc */

/*************
 *
 *    int MACE_total_mem() -- How many K are currently allocated?
 *
 *************/

int MACE_total_mem(void)
{
  return( (int) ((Malloc_calls-Free_calls) * (TP_ALLOC_SIZE / 1024.)));
}  /* MACE_total_mem */


