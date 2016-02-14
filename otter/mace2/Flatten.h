#ifndef TP_FLATTEN_H
#define TP_FLATTEN_H

int kludgey_e_subsume(struct clause *c,
		      struct clause *d);
void check_for_bad_things(struct clause *c);
int int_term(struct term *t);
int domain_element(struct term *t);
void process_negative_equalities(struct clause *c);
void flatten_clause(struct clause *d);
struct list *flatten_clauses(struct list *l);
void check_transformed_clause(struct clause *c,
			      struct clause *orig);

#endif  /* ! TP_FLATTEN_H */
