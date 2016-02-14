#define MAX_LITS 10

/* Types of step in the proof object */

#define P_RULE_UNDEFINED     0
#define P_RULE_INPUT         1
#define P_RULE_EQ_AXIOM      2
#define P_RULE_INSTANTIATE   3
#define P_RULE_PROPOSITIONAL 4
#define P_RULE_RESOLVE       5
#define P_RULE_PARAMOD       6
#define P_RULE_FLIP          7

struct proof_object_node {
    int id;
    int rule;
    int parent1, parent2;
    struct ilist *position1, *position2;
    BOOLEAN backward_subst;
    struct term *subst[2*MAX_VARS];
    struct clause *c;
    int old_id;           /* id of original clause */
    struct ilist *map;  /* position of literals in original clause */
    struct proof_object_node *next;
    };

struct proof_object {
    int steps;
    struct proof_object_node *first;
    struct proof_object_node *last;
    };
