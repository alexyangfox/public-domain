#ifndef TP_AVAIL_H
#define TP_AVAIL_H

/* function prototypes from avail.c */

void reinit_mem(void);

void *MACE_tp_alloc(size_t n);

int MACE_total_mem(void);

#endif  /* ! TP_AVAIL_H */
