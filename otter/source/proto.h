/* proto.h made 
Thursday, July 17, 10:19:37 AM 2003 (CDT)
*/

/* main.c */

void print_banner(int argc,
		  char **argv);

/* av.c */

int **tp_alloc(int n);
struct term *get_term(void);
void free_term(struct term *p);
struct rel *get_rel(void);
void free_rel(struct rel *p);
struct sym_ent *get_sym_ent(void);
void free_sym_ent(struct sym_ent *p);
struct term_ptr *get_term_ptr(void);
void free_term_ptr(struct term_ptr *p);
struct formula_ptr_2 *get_formula_ptr_2(void);
void free_formula_ptr_2(struct formula_ptr_2 *p);
struct fpa_tree *get_fpa_tree(void);
void free_fpa_tree(struct fpa_tree *p);
struct fpa_head *get_fpa_head(void);
void free_fpa_head(struct fpa_head *p);
struct context *get_context(void);
void free_context(struct context *p);
struct trail *get_trail(void);
void free_trail(struct trail *p);
struct imd_tree *get_imd_tree(void);
void free_imd_tree(struct imd_tree *p);
struct imd_pos *get_imd_pos(void);
void free_imd_pos(struct imd_pos *p);
struct is_tree *get_is_tree(void);
void free_is_tree(struct is_tree *p);
struct is_pos *get_is_pos(void);
void free_is_pos(struct is_pos *p);
struct fsub_pos *get_fsub_pos(void);
void free_fsub_pos(struct fsub_pos *p);
struct literal *get_literal(void);
void free_literal(struct literal *p);
struct clause *get_clause(void);
void free_clause(struct clause *p);
struct list *get_list(void);
void free_list(struct list *p);
struct clash_nd *get_clash_nd(void);
void free_clash_nd(struct clash_nd *p);
struct clause_ptr *get_clause_ptr(void);
void free_clause_ptr(struct clause_ptr *p);
struct ci_ptr *get_ci_ptr(void);
void free_ci_ptr(struct ci_ptr *p);
struct ilist *get_ilist(void);
void free_ilist(struct ilist *p);
struct ans_lit_node *get_ans_lit_node(void);
void free_ans_lit_node(struct ans_lit_node *p);
struct formula_box *get_formula_box(void);
void free_formula_box(struct formula_box *p);
struct formula *get_formula(void);
void free_formula(struct formula *p);
struct formula_ptr *get_formula_ptr(void);
void free_formula_ptr(struct formula_ptr *p);
struct cl_attribute *get_cl_attribute(void);
void free_cl_attribute(struct cl_attribute *p);
struct link_node *get_link_node(void);
void free_link_node(struct link_node *p);
void free_imd_pos_list(struct imd_pos *p);
void free_is_pos_list(struct is_pos *p);
struct glist *get_glist(void);
void free_glist(struct glist *p);
struct g2list *get_g2list(void);
void free_g2list(struct g2list *p);
struct fnode *get_fnode(void);
void free_fnode(struct fnode *p);
void print_mem(FILE *fp);
void print_mem_brief(FILE *fp);
int total_mem(void);
int total_mem_calls(void);
void print_linked_ur_mem_stats(void);

/* io.c */

int str_double(char *s,
	       double *dp);
void double_str(double d,
		char *s);
int str_int(char *s,
	    int *np);
void int_str(int i,
	     char *s);
int str_long(char *s,
	     long int *np);
int bits_ulong(char *s,
	       long unsigned int *np);
void long_str(long int i,
	      char *s);
void ulong_bits(long unsigned int i,
		char *s);
void cat_str(char *s1,
	     char *s2,
	     char *s3);
int str_ident(char *s,
	      char *t);
void reverse(char *s);
struct sym_ent *insert_sym(char *s,
			   int arity);
int str_to_sn(char *str,
	      int arity);
void print_syms(FILE *fp);
void p_syms(void);
char *sn_to_str(int sym_num);
int sn_to_arity(int sym_num);
struct sym_ent *sn_to_node(int sym_num);
int sn_to_ec(int sym_num);
struct sym_ent *sym_tab_member(char *str,
			       int arity);
int in_sym_tab(char *s);
void free_sym_tab(void);
int is_symbol(struct term *t,
	      char *str,
	      int arity);
void mark_as_skolem(int sym_num);
int is_skolem(int sym_num);
int initial_str(char *s,
		char *t);
int set_vars(struct term *t);
int set_vars_term(struct term *t,
		  char **varnames);
int var_name(char *s);
struct term_ptr *read_list(FILE *fp,
			   int *ep,
			   int integrate);
void print_list(FILE *fp,
		struct term_ptr *p);
void bird_print(FILE *fp,
		struct term *t);
void write_term(FILE *fp,
		struct term *t,
		int n,
		int *prev);
void display_term(FILE *fp,
		  struct term *t);
void print_term(FILE *fp,
		struct term *t);
void p_term(struct term *t);
void d_term(struct term *t);
void print_term_nl(FILE *fp,
		   struct term *t);
int print_term_length(struct term *t);
void  pretty_print_term(FILE *fp,
			struct term *t,
			int indents);
void print_variable(FILE *fp,
		    struct term *t);
void built_in_symbols(void);
int declare_op(int prec,
	       int type,
	       char *str);
void init_special_ops(void);
int process_op_command(struct term *t);
void skip_white(char *buf,
		int *p);
int name_sym(char *s);
void print_error(FILE *fp,
		 char *buf,
		 int pos);
struct term *str_to_term(char *buf,
			 int *p,
			 int in_list);
int read_buf(FILE *fp,
	     char *buf);
struct term *term_fixup(struct term *t);
struct term *term_fixup_2(struct term *t);
struct term *read_term(FILE *fp,
		       int *rcp);
void merge_sort(void **a,
		void **w,
		int start,
		int end,
		int (*comp_proc)(void *v1, void *v2));
int compare_for_auto_lex_order(void *d1,
			       void *d2);
void auto_lex_order(void);

/* share.c */

struct term *integrate_term(struct term *t);
void disintegrate_term(struct term *t);
void set_up_pointers(struct term *t);
void zap_term(struct term *t);
void print_term_tab(FILE *fp);
void p_term_tab(void);
void test_terms(FILE *fp);
struct term_ptr *all_instances(struct term *atom);
struct term_ptr *all_instances_fpa(struct term *atom, struct fpa_index *fpa);
void bd_kludge_insert(struct term *t);
void bd_kludge_delete(struct term *t);

/* fpa.c */

struct fpa_index *alloc_fpa_index(void);
void term_fpa_rec(int insert,
		  struct term *t,
		  struct term *super_term,
		  struct fpa_index *index,
		  int *path,
		  int j,
		  int bound);
void fpa_insert(struct term *t,
		int level,
		struct fpa_index *index);
void fpa_delete(struct term *t,
		int level,
		struct fpa_index *index);
struct fpa_tree *build_tree(struct term *t,
			    int u_type,
			    int bound,
			    struct fpa_index *index);
struct term *next_term(struct fpa_tree *n,
		       int max);
struct fpa_tree *build_for_all(struct fpa_index *index);
void zap_prop_tree(struct fpa_tree *n);
void print_fpa_tab(FILE *fp,
		   struct fpa_index *index);
void p_fpa_tab(struct fpa_index *index);
void print_prop_tree(FILE *fp,
		     struct fpa_tree *n,
		     int level);
void p_prop_tree(struct fpa_tree *n);
void print_path(FILE *fp,
		int *path);
void p_path(int *path);
int new_sym_num(void);

/* clocks.c */

void clock_init(void);
long clock_val(int c);
void clock_reset(int c);
char *get_time(void);
long system_time(void);
long run_time(void);
long wall_seconds(void);

/* unify.c */

int occur_check(int vn,
		struct context *vc,
		struct term *t,
		struct context *c);
int unify(struct term *t1,
	  struct context *c1,
	  struct term *t2,
	  struct context *c2,
	  struct trail **trp);
int unify_no_occur_check(struct term *t1,
			 struct context *c1,
			 struct term *t2,
			 struct context *c2,
			 struct trail **trp);
int match(struct term *t1,
	  struct context *c1,
	  struct term *t2,
	  struct trail **trp);
struct term *apply(struct term *t,
		   struct context *c);
int term_ident(struct term *t1,
	       struct term *t2);
void clear_subst_2(struct trail *t1,
		   struct trail *t2);
void clear_subst_1(struct trail *t1);
void print_subst(FILE *fp,
		 struct context *c);
void p_subst(struct context *c);
void print_trail(FILE *fp,
		 struct trail *t);

/* demod.c */

void un_share_special(struct term *t);
struct term *convenient_demod(struct term *t);
void zap_term_special(struct term *t);
struct term *apply_demod(struct term *t,
			 struct context *c,
			 int *pf);
void demod_cl(struct clause *c);
void back_demod(struct clause *d,
		struct clause *c,
		int input,
		struct list *lst);
int lit_t_f_reduce(struct clause *c);
int check_input_demod(struct clause *c);
int dynamic_demodulator(struct clause *c);
struct clause *new_demod(struct clause *c,
			 int demod_flag);

/* weight.c */

struct term_ptr *read_wt_list(FILE *fp,
			      int *ep);
int noncomplexifying(struct context *c);
int overbeek_match(struct term *t);
int weight(struct term *t,
	   struct is_tree *wt_index);
int wt_match(struct term *t,
	     struct term *template,
	     int *wtp,
	     struct is_tree *wt_index);
void set_wt_list(struct term_ptr *wt_list,
		 struct is_tree *wt_index,
		 int *ep);
void weight_index_delete(struct is_tree *wt_index);
int lex_order(struct term *t1,
	      struct term *t2);
int lex_order_vars(struct term *t1,
		   struct term *t2);
int lex_check(struct term *t1,
	      struct term *t2);
int var_subset(struct term *t1,
	       struct term *t2);
void order_equalities(struct clause *c);
int term_ident_x_vars(struct term *t1,
		      struct term *t2);

/* imd.c */

void imd_insert(struct clause *demod,
		struct imd_tree *imd);
void imd_delete(struct clause *demod,
		struct imd_tree *root_imd);
struct term *contract_imd(struct term *t_in,
			  int *demods,
			  struct context *subst,
			  int *demod_id_p);
void print_imd_tree(FILE *fp,
		    struct imd_tree *imd,
		    int level);
void p_imd_tree(struct imd_tree *imd);

/* is.c */

void is_insert(struct term *t,
	       struct is_tree *root_is);
void is_delete(struct term *t,
	       struct is_tree *root_is);
struct term_ptr *is_retrieve(struct term *t,
			     struct context *subst,
			     struct is_tree *is,
			     struct is_pos **is_pos);
struct term *fs_retrieve(struct term *t,
			 struct context *subst,
			 struct is_tree *is,
			 struct fsub_pos **fs_pos);
void canc_fs_pos(struct fsub_pos *pos,
		 struct context *subst);
void print_is_tree(FILE *fp,
		   struct is_tree *is);
void p_is_tree(struct is_tree *is);

/* clause.c */

void reset_clause_counter(void);
int next_cl_num(void);
void assign_cl_id(struct clause *c);
void hot_cl_integrate(struct clause *c);
void cl_integrate(struct clause *c);
void cl_del_int(struct clause *c);
void cl_del_non(struct clause *c);
void cl_int_chk(struct clause *c);
struct term *clause_to_term(struct clause *c);
struct clause *term_to_clause(struct term *t);
struct clause *read_sequent_clause(FILE *fp,
				   int *rcp);
struct clause *read_clause(FILE *fp,
			   int *rcp);
struct list *read_cl_list(FILE *fp,
			  int *ep);
int set_vars_cl(struct clause *cl);
void print_sequent_clause(FILE *fp,
			  struct clause *c);
void print_justification(FILE *fp,
		       struct ilist *just);
void print_clause_bare(FILE *fp,
		  struct clause *cl);
void print_clause(FILE *fp,
		  struct clause *cl);
void print_clause_without_just(FILE *fp,
		  struct clause *cl);
void p_clause(struct clause *cl);
void print_cl_list(FILE *fp,
		   struct list *lst);
void cl_merge(struct clause *c);
int tautology(struct clause *c);
int prf_weight(struct clause *c);
int proof_length(struct clause *c);
int subsume(struct clause *c,
	    struct clause *d);
int map_rest(struct clause *c,
	     struct clause *d,
	     struct context *s,
	     struct trail **trp);
int anc_subsume(struct clause *c,
		struct clause *d);
struct clause *for_sub_prop(struct clause *d);
struct clause *forward_subsume(struct clause *d);
struct clause_ptr *back_subsume(struct clause *c);
struct clause_ptr *unit_conflict(struct clause *c);
int propositional_clause(struct clause *c);
int xx_resolvable(struct clause *c);
int pos_clause(struct clause *c);
int answer_lit(struct literal *lit);
int pos_eq_lit(struct literal *lit);
int neg_eq_lit(struct literal *lit);
int eq_lit(struct literal *lit);
int neg_clause(struct clause *c);
int num_literals(struct clause *c);
int num_answers(struct clause *c);
int num_literals_including_answers(struct clause *c);
int literal_number(struct literal *lit);
int unit_clause(struct clause *c);
int horn_clause(struct clause *c);
int equality_clause(struct clause *c);
int symmetry_clause(struct clause *c);
struct literal *ith_literal(struct clause *c,
			    int n);
void append_cl(struct list *l,
	       struct clause *c);
void prepend_cl(struct list *l,
		struct clause *c);
void insert_before_cl(struct clause *c,
		      struct clause *c_new);
void insert_after_cl(struct clause *c,
		     struct clause *c_new);
void rem_from_list(struct clause *c);
void insert_clause(struct clause *c,
		   struct clause_ptr **cpp);
int max_literal_weight(struct clause *c,
		       struct is_tree *wt_index);
int weight_cl(struct clause *c,
	      struct is_tree *wt_index);
void hide_clause(struct clause *c);
struct clause *proof_last_hidden_empty(void);
void del_hidden_clauses(void);
struct clause *cl_copy(struct clause *c);
int clause_ident(struct clause *c1,
		 struct clause *c2);
void remove_var_syms(struct term *t);
void cl_insert_tab(struct clause *c);
void cl_delete_tab(struct clause *c);
struct clause *cl_find(int id);
int lit_compare(struct literal *l1,
		struct literal *l2);
int ordered_sub_clause(struct clause *c1,
		       struct clause *c2);
int sub_clause(struct clause *c1,
	       struct clause *c2);
int sort_lits(struct clause *c);
void all_cont_cl(struct term *t,
		 struct clause_ptr **cpp);
void zap_cl_list(struct list *lst);
int is_eq(int sym_num);
void mark_literal(struct literal *lit);
int get_ancestors(struct clause *c,
		  struct clause_ptr **cpp,
		  struct ilist **ipp);
struct ilist *clauses_to_ids(struct clause_ptr *p);
void free_clause_ptr_list(struct clause_ptr *p);
struct ilist *get_ancestors2(struct clause *c);
struct ilist *just_to_supporters(struct ilist *ip);
int renumber_vars_term(struct term *t);
int renumber_vars(struct clause *c);
int renum_vars_term(struct term *t,
		    int *varnums);
void clear_var_names(struct term *t);
void cl_clear_vars(struct clause *c);
int distinct_vars(struct clause *c);
struct clause *find_first_cl(struct list *l);
struct clause *find_last_cl(struct list *l);
struct clause *find_random_cl(struct list *l);
struct clause_ptr *get_clauses_of_wt_range(struct clause *c,
					   int min, int max);
int clause_ptr_list_size(struct clause_ptr *p);
struct clause *nth_clause(struct clause_ptr *p, int n);
void zap_clause_ptr_list(struct clause_ptr *p);
struct clause *find_random_lightest_cl(struct list *l);
struct clause *find_mid_lightest_cl(struct list *l);
struct clause *find_lightest_cl(struct list *l);
struct clause *find_lightest_geo_child(struct list *l);
struct clause *find_interactive_cl(void);
struct clause *find_given_clause(void);
struct clause *extract_given_clause(void);
int unit_del(struct clause *c);
void back_unit_deletion(struct clause *c,
			int input);

/* options.c */

void init_options(void);
void print_options(FILE *fp);
void p_options(void);
void auto_change_flag(FILE *fp,
		      int index,
		      int val);
void dependent_flags(FILE *fp,
		     int index);
void auto_change_parm(FILE *fp,
		      int index,
		      int val);
void dependent_parms(FILE *fp,
		     int index);
int change_flag(FILE *fp,
		struct term *t,
		int set);
int change_parm(FILE *fp,
		struct term *t);
void check_options(void);

/* resolve.c */

int maximal_lit(struct literal *l1);
void hyper_res(struct clause *giv_cl);
void neg_hyper_res(struct clause *giv_cl);
void ur_res(struct clause *giv_cl);
int one_unary_answer(struct clause *c);
struct term *build_term(int sn,
			struct term *arg1,
			struct term *arg2,
			struct term *arg3);
void combine_answers(struct clause *res,
		     struct term *a1,
		     struct context *s1,
		     struct term *a2,
		     struct context *s2);
struct clause *build_bin_res(struct term *a1,
			     struct context *s1,
			     struct term *a2,
			     struct context *s2);
struct clause *apply_clause(struct clause *c,
			    struct context *s);
void bin_res(struct clause *giv_cl);
struct clause *first_or_next_factor(struct clause *c,
				    struct literal **l1p,
				    struct literal **l2p);
void all_factors(struct clause *c,
		 struct list *lst);
int factor_simplify(struct clause *c);

/* index.c */

void index_lits_all(struct clause *c);
void un_index_lits_all(struct clause *c);
void index_lits_clash(struct clause *c);
void un_index_lits_clash(struct clause *c);

/* paramod.c */

void para_from(struct clause *giv_cl);
void para_into(struct clause *giv_cl);

/* formula.c */

void print_formula(FILE *fp,
		   struct formula *f);
void p_formula(struct formula *f);
struct term *formula_to_term(struct formula *f);
struct formula *term_to_formula(struct term *t);
struct formula *read_formula(FILE *fp,
			     int *rcp);
struct formula_ptr *read_formula_list(FILE *fp,
				      int *ep);
void print_formula_list(FILE *fp,
			struct formula_ptr *p);
struct formula *copy_formula(struct formula *f);
void zap_formula(struct formula *f);
struct formula *negate_formula(struct formula *f);
struct formula *nnf(struct formula *f);
struct formula *skolemize(struct formula *f);
struct formula *anti_skolemize(struct formula *f);
void subst_free_formula(struct term *var,
			struct formula *f,
			struct term *sk);
void gen_sk_sym(struct term *t);
int skolem_symbol(int sn);
int contains_skolem_symbol(struct term *t);
int new_var_name(void);
int new_functor_name(int arity);
void unique_all(struct formula *f);
struct formula *zap_quant(struct formula *f);
void flatten_top(struct formula *f);
struct formula *cnf(struct formula *f);
struct formula *dnf(struct formula *f);
void rename_syms_formula(struct formula *f,
			 struct formula *fr);
void subst_sn_term(int old_sn,
		   struct term *t,
		   int new_sn,
		   int type);
void subst_sn_formula(int old_sn,
		      struct formula *f,
		      int new_sn,
		      int type);
int gen_subsume_prop(struct formula *c,
		     struct formula *d);
struct formula *subsume_conj(struct formula *c);
struct formula *subsume_disj(struct formula *c);
int formula_ident(struct formula *f,
		  struct formula *g);
void conflict_tautology(struct formula *f);
void ts_and_fs(struct formula *f);
struct list *clausify(struct formula *f);
struct list *clausify_formula_list(struct formula_ptr *fp);
struct formula *negation_inward(struct formula *f);
struct formula *expand_imp(struct formula *f);
struct formula *iff_to_conj(struct formula *f);
struct formula *iff_to_disj(struct formula *f);
struct formula *nnf_cnf(struct formula *f);
struct formula *nnf_dnf(struct formula *f);
struct formula *nnf_skolemize(struct formula *f);
struct formula *clausify_formed(struct formula *f);
void rms_conflict_tautology(struct formula *f);
struct formula *rms_subsume_conj(struct formula *c);
struct formula *rms_subsume_disj(struct formula *c);
int free_occurrence(struct term *v,
		    struct formula *f);
struct formula *rms_distribute_quants(struct formula *f_quant);
struct formula *rms_push_free(struct formula *f);
struct formula *rms_quantifiers(struct formula *f);
struct formula *rms(struct formula *f);
struct formula *renumber_unique(struct formula *f,
				int *vnum_p);
int gen_subsume_rec(struct formula *c,
		    struct context *cs,
		    struct formula *d,
		    struct context *ds,
		    struct trail **tr_p);
int gen_subsume(struct formula *c,
		struct formula *d);
int gen_conflict(struct formula *c,
		 struct formula *d);
int gen_tautology(struct formula *c,
		  struct formula *d);
struct formula *rms_cnf(struct formula *f);
struct formula *rms_dnf(struct formula *f);
struct formula *distribute_quantifier(struct formula *f);

/* process.c */

void post_proc_all(struct clause *lst_pos,
		   int input,
		   struct list *lst);
void infer_and_process(struct clause *giv_cl);
int proc_gen(struct clause *c,
	     int input);
void pre_process(struct clause *c,
		 int input,
		 struct list *lst);

/* misc.c */

void init(void);
void abend(char *str);
void read_a_file(FILE *in_fp,
		 FILE *out_fp);
void sos_argument(char *buf);
void read_all_input(int argc,
		    char **argv);
void set_lex_vals(struct term *t);
void set_lrpo_status(struct term *t,
		     int val);
void set_special_unary(struct term *t);
void set_skolem(struct term *t);
void free_all_mem(void);
void output_stats(FILE *fp,
		  int level);
void print_stats(FILE *fp);
void print_stats_brief(FILE *fp);
void p_stats(void);
void print_times(FILE *fp);
void print_times_brief(FILE *fp);
void p_times(void);
void append_lists(struct list *l1,
		  struct list *l2);
struct term *copy_term(struct term *t);
int biggest_var(struct term *t);
int biggest_var_clause(struct clause *c);
int ground_clause(struct clause *c);
void zap_list(struct term_ptr *p);
int occurs_in(struct term *t1,
	      struct term *t2);
int occurrences(struct term *s,
		struct term *t);
int sn_occur(int sn,
	     struct term *t);
int is_atom(struct term *t);
int ident_nested_skolems(struct clause *c);
int ground(struct term *t);
void cleanup(void);
int check_stop(void);
void report(void);
void control_memory(void);
void print_proof(FILE *fp,
		 struct clause *c);
struct clause *check_for_proof(struct clause *c);
int proper_list(struct term *t);
void move_clauses(int (*clause_proc)(struct clause *c),
		  struct list *source,
		  struct list *destination);
void automatic_1_settings(void);
int sos_has_pos_nonground(void);
void automatic_2_settings(void);
void log_for_x_show(FILE *fp);
int same_structure(struct term *t1,
		   struct term *t2);
void zap_variable_names(struct term *t);
int commuted_terms(struct term *t1, struct term *t2);
int symbol_count(struct term *t);
int commutativity_consequence(struct clause *c);

/* lrpo.c */

int lrpo(struct term *t1,
	 struct term *t2);
int lrpo_greater(struct term *t1,
		 struct term *t2);
void order_equalities_lrpo(struct clause *c);

/* linkur.c */

void linked_ur_res(struct clause *giv_cl);
int process_linked_tags(struct clause *cp);

/* linkhyp.c */

void linked_hyper_res(struct clause *giv_cl);

/* foreign.c */

long foo(long int l,
	 double d,
	 char *s);
long user_test_long(long int l,
		    double d,
		    int b,
		    char *s,
		    struct term *t);
double user_test_double(long int l,
			double d,
			int b,
			char *s,
			struct term *t);
int user_test_bool(long int l,
		   double d,
		   int b,
		   char *s,
		   struct term *t);
char *user_test_string(long int l,
		       double d,
		       int b,
		       char *s,
		       struct term *t);
struct term *user_test_term(long int l,
			    double d,
			    int b,
			    char *s,
			    struct term *t);
void declare_user_functions(void);
int get_args_for_user_function(struct term *t,
			       int op_code,
			       long int *long_args,
			       double *double_args,
			       int *bool_args,
			       char **string_args,
			       struct term **term_args);
struct term *long_to_term(long int i);
struct term *double_to_term(double d);
struct term *bool_to_term(int i);
struct term *string_to_term(char *s);
struct term *evaluate_user_function(struct term *t,
				    int op_code);

/* geometry.c */

int geo_rewrite(struct clause *c);
void geometry_rule_unif(struct clause *giv_cl);
int child_of_geometry(struct clause *c);
void gl_demod(struct clause *c,
	      struct list *lst);

/* hot.c */

void init_hot(void);
int heat_is_on(void);
void switch_to_hot_index(void);
void switch_to_ordinary_index(void);
void hot_index_clause(struct clause *c);
void hot_dynamic(struct clause *c);
void hot_mark_clash_cl(struct clause *c,
		       int mark);
void hot_inference(struct clause *new_cl);

/* nonport.c */

void non_portable_init(int argc,
		       char **argv);
void sig_handler(int condition);
char *username(void);
char *hostname(void);
void interact(void);
void foreach_sos(void);
FILE *init_log_for_x_show(void);
int my_process_id(void);

/* check.c */

struct gen_node *get_gen_node(void);
struct proof_object *get_proof_object(void);
struct proof_object_node *get_proof_object_node(void);
int trivial_subst(struct context *c);
struct proof_object_node *connect_new_node(struct proof_object *new_proof);
void print_term_s(FILE *fp,
		  struct term *t);
void p_term_s(struct term *t);
void print_clause_s(FILE *fp,
		    struct clause *c);
void p_clause_s(struct clause *c);
void print_clause_s2(FILE *fp,
		     struct clause *c);
void p_clause_s2(struct clause *c);
void print_proof_object_node(FILE *fp,
			     struct proof_object_node *pn);
void p_proof_object_node(struct proof_object_node *pn);
void print_proof_object(FILE *fp,
			struct proof_object *po);
void p_proof_object(struct proof_object *po);
struct clause *cl_copy_delete_literal(struct clause *c,
				      int n);
int variant(struct term *t1,
	    struct context *c1,
	    struct term *t2,
	    struct context *c2,
	    struct trail **trp,
	    int flip);
struct ilist *match_clauses(struct clause *c1,
			      struct clause *c2);
struct clause *cl_append(struct clause *c1,
			 struct clause *c2);
struct clause *identity_resolve(struct clause *c1,
				int i1,
				struct clause *c2,
				int i2);
void renumber_vars_subst(struct clause *c,
			 struct term **terms);
int finish_translating(struct clause *c,
		       struct ilist *rest_of_history,
		       struct proof_object_node *current,
		       struct proof_object *new_proof);
int ipx(struct ilist *ip,
	int n);
struct proof_object_node *find_match2(struct clause *c,
				      struct proof_object *obj,
				      struct term **vars);
int contains_answer_literal(struct clause *c);
int contains_rule(struct clause *c,
		  int rule);
struct ilist *trans_2_pos(int id,
			    struct ilist *pos);
void type_2_trans(struct proof_object *po);
int glist_subsume(struct clause *c, struct glist *g);
void p_proof_object_as_hints(struct proof_object *po);
struct literal *remove_answer_literals(struct literal *lit);
void build_proof_object(struct clause *c);
void init_proof_object_environment(void);

/* hints.c */

void compile_hints(void);
void print_hint_clause(FILE *fp,
		       struct clause *c);
void p_hint_clause(struct clause *c);
void print_hints_cl_list(FILE *fp,
			 struct list *lst);
void p_hints_cl_list(struct list *lst);
void adjust_weight_with_hints(struct clause *c);
int hint_keep_test(struct clause *c);

/* hints2.c */

struct clause *find_hint2(struct clause *c);
void print_hint2_clause(FILE *fp,
		       struct clause *c);
void print_hints2_cl_list(FILE *fp,
			 struct list *lst);
void hint2_integrate(struct clause *h);
void compile_hints2(void);
void adjust_weight_with_hints2(struct clause *c);
int hint2_keep_test(struct clause *c);
void back_demod_hints(struct clause *d);
void zap_hints2(void);

/* attrib.c */

void init_attributes(void);
int get_attribute_index(char *s);
int attribute_type(int name);
struct cl_attribute *get_attribute(struct clause *c,
				   int name);
void set_attribute(struct clause *c,
		   int name,
		   void *val_ptr);
void delete_attributes(struct clause *c);
struct cl_attribute *term_to_attributes(struct term *t);
void print_attributes(FILE *fp,
		      struct cl_attribute *a);

/* case.c */

int splitting(void);
int max_split_depth(void);
int splitable_literal(struct clause *c,
		      struct literal *l);
int splitable_clause(struct clause *c);
struct clause *compare_splitable_clauses(struct clause *c,
					 struct clause *d);
void print_case(FILE *fp);
void p_case(void);
void print_case_n(FILE *fp,
		  int n);
void p_case_n(int n);
void p_assumption_depths(char assumptions[]);
struct ilist *current_case(void);
void add_subcase(int i);
int case_depth(void);
struct clause *find_clause_to_split(void);
struct term *find_atom_to_split(void);
int prover_forks(int n,
		 int *ip,
		 char assumptions[]);
int split_clause(struct clause *giv_cl);
int split_atom(void);
void possible_split(void);
void always_split(void);
void possible_given_split(struct clause *c);
void assumps_to_parent(struct clause *e);
void exit_with_possible_model(void);

/* lisp.c */


/* ivy.c */

int special_is_symbol(struct term *t, char *str, int arity);
void trans_logic_symbols(struct term *t);
struct proof_object *parse_initial_proof_object(FILE *fp);
struct list *init_proof_object(FILE *fin,
			       FILE *fout);
struct proof_object *retrieve_initial_proof_object(void);

/* pickdiff.c */

struct ilist *cldiff(struct clause *c, struct clause *d);
void zap_ci_ptr_list(struct ci_ptr *p);
struct clause *find_pickdiff_cl(struct list *sos, struct list *usable);

/* overbeek.c */

void overbeek_insert(struct term *t);
int overbeek_weight(struct term *t, int *ip);
void print_overbeek_world(void);
void check_overbeek_world(void);

/* multijust.c */

struct glist *keep_only(struct glist *a, int i);
struct glist *remove_supersets(struct glist *a, struct ilist *s);
struct glist *remove_all_supersets(struct glist *a, struct glist *b);
struct g2list *g2_remove_supersets(struct g2list *g2, struct ilist *p);
int input_clause(int id);
int first_just_input_only(struct g2list *p);
int all_supporters_less_than(struct clause *c, struct ilist *supporters);
int derived_from_itself(struct clause *c, struct ilist *supporters);
int proof_not_longer_than(struct clause *c, struct ilist *p);
void possibly_append_parents(struct clause *c, struct ilist *ip);
int map_demod(struct ilist *p, int i);
void set_jset_size(int n);
int *get_jset(void);
int *copy_jset(int *a);
int jset_member(int *s, int n);
void add_to_jset(int *s, int n);
void remove_from_jset(int *s, int n);
void print_set(int *s);
void print_set_b_to_a(int *s);
int subset_or_input(struct ilist *a, struct ilist *b);
struct ilist *jset_to_ilist(int *s);
int *ilist_to_jset(struct ilist *p);
void multi_just_process(struct clause *c);
int multi_justifications();

/* lists.c */

void free_ilist_list(struct ilist *p);
struct ilist *ilist_tack_on(struct ilist *a, int i);
struct ilist *iset_add(int i, struct ilist *p);
struct ilist *iset_remove(int i, struct ilist *p);
int ilist_member(int i, struct ilist *p);
int iset_subset(struct ilist *a, struct ilist *b);
struct ilist *iset_subtract(struct ilist *a, struct ilist *b);
struct ilist *iset_sort(struct ilist *a);
struct ilist *idempot_ip(struct ilist *a);
struct ilist *reverse_ip(struct ilist *ip1, struct ilist *ip2);
struct ilist *ilist_append(struct ilist *a, struct ilist *b);
struct ilist *copy_ilist(struct ilist *p);
int ilist_length(struct ilist *a);
struct ilist *copy_ilist_segment(struct ilist *p, int n);
void print_ilist(FILE *fp,
		    struct ilist *ip);
void p_ilist(struct ilist *ip);
int glist_length(struct glist *a);
struct glist *copy_glist(struct glist *p);
struct glist *glist_append(struct glist *a, struct glist *b);
struct glist *glist_prepend(void *p, struct glist *a);
struct glist *glist_tack_on(struct glist *a, void *v);
void free_glist_list(struct glist *p);
int g2list_length(struct g2list *a);
int member_is_subset(struct glist *a, struct ilist *s);
struct glist *copy_glist_of_ilists(struct glist *a);
void free_glist_of_ilists(struct glist *p);
