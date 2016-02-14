/*
 *  demod.c -- Demodulation (rewriting) routines.
 *
 */

#include "header.h"

/*************
 *
 *    struct term *contract_lin(term, demods, context, demod_id_p)
 *
 *        Attempt to rewrite the top level of `term', using a
 *    sequential search of `demods'.  If success, term is freed; if fail,
 *    NULL is returned.
 *
 *************/

static struct term *contract_lin(struct term *t,
				 int *demods,
				 struct context *c,
				 int *demod_id_p)
{
  struct term *atom, *contractum, *t2, *t3, *beta;
  struct rel *alpha_rel;
  struct trail *tr;
  struct clause *p;
  struct list *d;
  int mult_flag, dummy, ok;

  tr = NULL;
  d = (struct list *) demods;
  if (d == NULL)
    return(NULL);
  p = d->first_cl;
  contractum = NULL;
  while (p && contractum == NULL) {
    atom = ith_literal(p,1)->atom;
    alpha_rel = (atom->varnum == CONDITIONAL_DEMOD ?
		 atom->farg->narg->argval->farg :
		 atom->farg);
    tr = NULL;
    if (match(alpha_rel->argval, c, t, &tr)) {

      if (atom->varnum == CONDITIONAL_DEMOD) {
	/* rewrite instantiated condition */
	t2 = apply_demod(atom->farg->argval, c, &dummy);
	un_share_special(t2);
	t3 = convenient_demod(t2);
	ok = is_symbol(t3, "$T", 0);
	zap_term(t3);
      }
      else
	ok = 1;

      if (ok) {
	beta = alpha_rel->narg->argval;
	mult_flag = 0;
	contractum = apply_demod(beta, c, &mult_flag);
	if (mult_flag)
	  c->multiplier++;

	/* varnum == LEX_DEP_DEMOD means it's lex-dependent */
	if (atom->varnum != LEX_DEP_DEMOD)
	  ok = 1;
	else if (Flags[LRPO].val)
	  ok = lrpo_greater(t, contractum);
	else
	  ok = lex_check(contractum, t) == LESS_THAN;

	if (ok) {
	  zap_term_special(t);
	  *demod_id_p = p->id;
	}
	else {
	  zap_term_special(contractum);
	  contractum = NULL;
	}
      }
      clear_subst_1(tr);
    }
    p = p->next_cl;
  }

  return(contractum);  /* may be NULL */

}  /* contract_lin */

/*************
 *
 *    dollar_out_non_list(t) - Process $OUT(t).
 *
 *************/

static void dollar_out_non_list(struct term *t)
{
  int i;

  if (t->sym_num == Chr_sym_num && str_int(sn_to_str(t->farg->argval->sym_num), &i))
    printf("%c", i);
  else
    print_term(stdout, t);
}  /* dollar_out_non_list */

/*************
 *
 *    dollar_out(t) - Process $OUT(t) or $OUT([t1,...,tn]).
 *
 *************/

static void dollar_out(struct term *t)
{
  struct term *t1;

  if (proper_list(t)) {
    printf("\n");
    for (t1 = t; t1->sym_num != Nil_sym_num; t1 = t1->farg->narg->argval) {
      if (t != t1)
	printf(" ");
      dollar_out_non_list(t1->farg->argval);
    }
    printf(".\n\n");
  }
  else
    dollar_out_non_list(t);
}  /* dollar_out */

/*************
 *
 *    struct term *dollar_contract(t) - evaluate $EQ, $SUM, ...
 *
 *    If t is evaluated, it is deallocated and the result is returned.
 *    If t cannot be evaluated, it is left alone and NULL is returned.
 *
 *    Here are the current built-ins.  (int is actually long, and
 *    float is actually double.  Recall that floats are always
 *    surrouded by double quotes.)
 *
 *       int x int -> int             $SUM, $PROD, $DIFF, $DIV, $MOD
 *       int x int -> bool            $EQ, $NE, $LT, $LE, $GT, $GE
 *       float x float -> float       $FSUM, $FPROD, $FDIFF, $FDIV
 *       float x float -> bool        $FEQ, $FNE, $FLT, $FLE, $FGT, $FGE
 *       term x term -> bool          $ID, $LNE, $LLT, $LLE, $LGT, $LGE,
 *                                    $OCCURS, $VOCCURS, VFREE
 *       bool x bool -> bool          $AND, $OR
 *       bool -> bool                 $TRUE, $NOT
 *       -> bool                      $T, $F
 *       term -> bool                 $ATOMIC, $INT, $BITS, $VAR
 *       -> int                       $NEXT_CL_NUM
 *       bool x term x term -> term   $IF
 *       term -> bool                 $GROUND
 *       bits x bits -> bits          $BIT_AND, $BIT_OR, $BIT_XOR
 *       bits x int -> bits           $SHIFT_LEFT, $SHIFT_RIGHT
 *       bits -> bits                 $BIT_NOT
 *       bits -> int                  $BITS_OT_INT
 *       int -> bits                  $INT_TO_BITS
 *
 *************/

static struct term *dollar_contract(struct term *t)
{
  static int unique_num = 0;
  long i1, i2;
  long i3 = 0;
  unsigned long u1, u2;
  unsigned long u3 = 0;
  int b1, op_code, op_type, s1t, s1f, s2t, s2f;
  int b3 = 0;
  double d1, d2;
  double d3 = 0.0;
  char *s1, *s2, str[MAX_NAME];
  struct term *t1, *ta, *tb;

  op_code = sn_to_ec(t->sym_num);  /* get eval code */

  if (op_code < 1)
    return(NULL);
  else if (op_code <= MAX_USER_EVALUABLE)
    return(evaluate_user_function(t, op_code));
  else {
    switch(op_code) {
    case SUM_SYM:
    case PROD_SYM:
    case DIFF_SYM:
    case DIV_SYM:
    case MOD_SYM:  op_type = 1; break; /* int x int -> int */
    case EQ_SYM:
    case NE_SYM:
    case LT_SYM:
    case LE_SYM:
    case GT_SYM:
    case GE_SYM:   op_type = 2; break; /* int x int -> bool */
    case AND_SYM:
    case OR_SYM:   op_type = 3; break; /* bool x bool -> bool */
    case TRUE_SYM:
    case NOT_SYM:  op_type = 4; break; /* bool -> bool */
    case IF_SYM:   op_type = 5; break; /* bool x term x term -> term */
    case LLT_SYM:
    case LLE_SYM:
    case LGT_SYM:
    case LGE_SYM:
    case LNE_SYM:
    case ID_SYM:   op_type = 6; break;     /* term x term -> bool (lex) */
    case NEXT_CL_NUM_SYM:  op_type = 7; break; /* -> int */
    case ATOMIC_SYM:
    case BITS_SYM:
    case INT_SYM:
    case GROUND_SYM:
    case VAR_SYM:  op_type = 8; break;         /* term -> bool */
    case T_SYM: return(NULL);
    case F_SYM: return(NULL);
    case OUT_SYM:  op_type = 9; break;  /* term -> same_term_with_output */
    case BIT_NOT_SYM:  op_type = 10; break; /* bits -> bits */
    case FSUM_SYM:
    case FPROD_SYM:
    case FDIFF_SYM:
    case FDIV_SYM: op_type = 11; break;     /* float x float -> float */
    case FEQ_SYM:
    case FNE_SYM:
    case FLT_SYM:
    case FLE_SYM:
    case FGT_SYM:
    case FGE_SYM:   op_type = 12; break; /* float x float -> bool */
    case BIT_AND_SYM:
    case BIT_OR_SYM:
    case BIT_XOR_SYM: op_type = 13; break; /* bits x bits -> bits */
    case SHIFT_RIGHT_SYM:
    case SHIFT_LEFT_SYM: op_type = 14; break; /* bits x int -> bits */
    case INT_TO_BITS_SYM: op_type = 15; break; /* int -> bits */
    case BITS_TO_INT_SYM: op_type = 16; break; /* bits -> int */
    case OCCURS_SYM:
    case VOCCURS_SYM:
    case VFREE_SYM:
    case RENAME_SYM:  op_type = 17; break;  /* term x term -> bool (misc) */
    case UNIQUE_NUM_SYM:  op_type = 18; break;     /* -> int */

    default: printf("ERROR, dollar_contract, bad op_code: %d.\n", op_code); return(NULL);
    }
	
    switch (op_type) {
    case 1:  /* int x int -> int */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      if (ta->type != NAME || tb->type != NAME)
	return(NULL);
	
      s1 = sn_to_str(ta->sym_num);
      s2 = sn_to_str(tb->sym_num);
      if (str_long(s1, &i1) == 0 || str_long(s2, &i2) == 0)
	return(NULL);
	
      if ((op_code == DIV_SYM || op_code == MOD_SYM) && i2 == 0) {
	print_term_nl(stdout, t);
	abend("integer divide by 0.");
      }
      switch (op_code) {
      case SUM_SYM:   i3 = i1 + i2; break;
      case PROD_SYM:  i3 = i1 * i2; break;
      case DIFF_SYM:  i3 = i1 - i2; break;
      case DIV_SYM:   i3 = i1 / i2; break;
      case MOD_SYM:   i3 = i1 % i2; break;
      }
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      long_str(i3, str);
      t1->sym_num = str_to_sn(str, 0);
      return(t1);
    case 2:  /* int x int -> bool */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      if (ta->type != NAME || tb->type != NAME)
	return(NULL);
	
      s1 = sn_to_str(ta->sym_num);
      s2 = sn_to_str(tb->sym_num);
      if (str_long(s1, &i1) == 0 || str_long(s2, &i2) == 0)
	return(NULL);
      switch (op_code) {
      case EQ_SYM:    b3 = i1 == i2; break;
      case NE_SYM:    b3 = i1 != i2; break;
      case LT_SYM:    b3 = i1 <  i2; break;
      case LE_SYM:    b3 = i1 <= i2; break;
      case GT_SYM:    b3 = i1 >  i2; break;
      case GE_SYM:    b3 = i1 >= i2; break;
      }
      t->occ.lit = NULL; /* in case t is a literal */
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      t1->sym_num = str_to_sn(b3 ? "$T" : "$F", 0);
      return(t1);
    case 3:  /* bool x bool -> bool */
      s1 = sn_to_str(t->farg->argval->sym_num);
      s2 = sn_to_str(t->farg->narg->argval->sym_num);
      s1t = str_ident(s1,"$T");
      s1f = str_ident(s1,"$F");
      s2t = str_ident(s2,"$T");
      s2f = str_ident(s2,"$F");
      if ((s1t == 0 && s1f == 0) || (s2t == 0 && s2f == 0))
	return(NULL);
      switch (op_code) {
      case AND_SYM:   b3 = s1t && s2t; break;
      case OR_SYM:    b3 = s1t || s2t; break;
      }
      t->occ.lit = NULL; /* in case t is a literal */
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      t1->sym_num = str_to_sn(b3 ? "$T" : "$F", 0);
      return(t1);
    case 4:  /* bool -> bool  $NOT(x), $TRUE(x) */
      s1 = sn_to_str(t->farg->argval->sym_num);
      s1t = str_ident(s1,"$T");
      s1f = str_ident(s1,"$F");
      if (s1t == 0 && s1f == 0)
	return(NULL);
      t->occ.lit = NULL; /* in case t is a literal */
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      switch (op_code) {
      case NOT_SYM:  t1->sym_num = str_to_sn(s1t ? "$F" : "$T", 0);
	break;
      case TRUE_SYM: t1->sym_num = str_to_sn(s1t ? "$T" : "$F", 0);
	break;
      }
      return(t1);
    case 5:  /* bool x term x term -> term   $IF(x,y,z) */
      s1 = sn_to_str(t->farg->argval->sym_num);
      s1t = str_ident(s1,"$T");
      s1f = str_ident(s1,"$F");
      if (s1t == 0 && s1f == 0)
	return(NULL);
      if (s1t)
	t1 = t->farg->narg->argval;
      else
	t1 = t->farg->narg->narg->argval;
      t1->fpa_id++;  /* one more pointer to t1 */
      zap_term_special(t);  /* one less pointer to t */
      return(t1);
    case 6:  /* term x term -> bool (lexical comparisons) */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      b1 = lex_check(ta, tb);
      switch (op_code) {
      case ID_SYM:  b3 = (b1 == SAME_AS); break;
      case LNE_SYM: b3 = (b1 != SAME_AS); break;
      case LLT_SYM: b3 = (b1 == LESS_THAN); break;
      case LLE_SYM: b3 = (b1 == LESS_THAN || b1 == SAME_AS); break;
      case LGT_SYM: b3 = (b1 == GREATER_THAN); break;
      case LGE_SYM: b3 = (b1 == GREATER_THAN || b1 == SAME_AS); break;
      }
      t->occ.lit = NULL; /* in case t is a literal */
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      t1->sym_num = str_to_sn(b3 ? "$T" : "$F", 0);
      return(t1);
    case 7:  /* -> int */
      int_str(next_cl_num(), str);
      t->sym_num = str_to_sn(str, 0);
      return(t);
    case 8:  /* term -> bool (metalogical properties) */
      ta = t->farg->argval;
      switch (op_code) {
      case ATOMIC_SYM: b3 = ta->type == NAME; break;
      case INT_SYM:
	b3 = ( ta->type == NAME &&
	       str_long(sn_to_str(ta->sym_num), &i1));
	break;
      case BITS_SYM:
	b3 = ( ta->type == NAME &&
	       bits_ulong(sn_to_str(ta->sym_num), &u1));
	break;
      case VAR_SYM: b3 = ta->type == VARIABLE; break;
      case GROUND_SYM: b3 = ground(ta); break;
      }
	
      t->occ.lit = NULL; /* in case t is a literal */
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      t1->sym_num = str_to_sn(b3 ? "$T" : "$F", 0);
      return(t1);
    case 9:  /* term -> same_term_with_output */
      dollar_out(t->farg->argval);
      return(NULL);
    case 10:  /* bits -> bits */
      s1 = sn_to_str(t->farg->argval->sym_num);
      if (bits_ulong(s1, &u1) == 0)
	return(NULL);
      switch (op_code) {
      case BIT_NOT_SYM: u3 = ~u1;
	break;
      }
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      ulong_bits(u3, str);
      t1->sym_num = str_to_sn(str, 0);
      return(t1);
    case 11:  /* float x float -> float */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      if (ta->type != NAME || tb->type != NAME)
	return(NULL);
	
      s1 = sn_to_str(ta->sym_num);
      s2 = sn_to_str(tb->sym_num);
      if (str_double(s1, &d1) == 0 || str_double(s2, &d2) == 0)
	return(NULL);
	
      if (op_code == FDIV_SYM && d2 == 0) {
	print_term_nl(stdout, t);
	abend("float divide by 0."); 
      }
      switch (op_code) {
      case FSUM_SYM:   d3 = d1 + d2; break;
      case FPROD_SYM:  d3 = d1 * d2; break;
      case FDIFF_SYM:  d3 = d1 - d2; break;
      case FDIV_SYM:   d3 = d1 / d2; break;
      }
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      double_str(d3, str);
      t1->sym_num = str_to_sn(str, 0);
      return(t1);
    case 12:  /* float x float -> bool */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      if (ta->type != NAME || tb->type != NAME)
	return(NULL);
      s1 = sn_to_str(ta->sym_num);
      s2 = sn_to_str(tb->sym_num);
      if (str_double(s1, &d1) == 0 || str_double(s2, &d2) == 0)
	return(NULL);
      switch (op_code) {
      case FEQ_SYM:    b3 = d1 == d2; break;
      case FNE_SYM:    b3 = d1 != d2; break;
      case FLT_SYM:    b3 = d1 <  d2; break;
      case FLE_SYM:    b3 = d1 <= d2; break;
      case FGT_SYM:    b3 = d1 >  d2; break;
      case FGE_SYM:    b3 = d1 >= d2; break;
      }
      t->occ.lit = NULL; /* in case t is a literal */
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      t1->sym_num = str_to_sn(b3 ? "$T" : "$F", 0);
      return(t1);
    case 13:  /* bits x bits -> bits */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      if (ta->type != NAME || tb->type != NAME)
	return(NULL);
	
      s1 = sn_to_str(ta->sym_num);
      s2 = sn_to_str(tb->sym_num);
      if (bits_ulong(s1, &u1) == 0 || bits_ulong(s2, &u2) == 0)
	return(NULL);
	
      switch (op_code) {
      case BIT_AND_SYM:      u3 = u1 & u2; break;
      case BIT_OR_SYM:       u3 = u1 | u2; break;
      case BIT_XOR_SYM:      u3 = u1 ^ u2; break;
      }
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      ulong_bits(u3, str);
      t1->sym_num = str_to_sn(str, 0);
      return(t1);
    case 14:  /* bits x int -> bits */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      if (ta->type != NAME || tb->type != NAME)
	return(NULL);
	
      s1 = sn_to_str(ta->sym_num);
      s2 = sn_to_str(tb->sym_num);
      if (bits_ulong(s1, &u1) == 0 || str_long(s2, &i2) == 0)
	return(NULL);
	
      switch (op_code) {
      case SHIFT_RIGHT_SYM:  u3 = u1 >> i2; break;
      case SHIFT_LEFT_SYM:   u3 = u1 << i2; break;
      }
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      ulong_bits(u3, str);
      t1->sym_num = str_to_sn(str, 0);
      return(t1);
    case 15:  /* int -> bits */
      s1 = sn_to_str(t->farg->argval->sym_num);
      if (str_long(s1, &i1) == 0)
	return(NULL);
      switch (op_code) {
      case INT_TO_BITS_SYM: u3 = i1;
	break;
      }
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      ulong_bits(u3, str);
      t1->sym_num = str_to_sn(str, 0);
      return(t1);
    case 16:  /* bits -> int */
      s1 = sn_to_str(t->farg->argval->sym_num);
      if (bits_ulong(s1, &u1) == 0)
	return(NULL);
      switch (op_code) {
      case BITS_TO_INT_SYM: i3 = u1;
	break;
      }
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      long_str(i3, str);
      t1->sym_num = str_to_sn(str, 0);
      return(t1);
    case 17:  /* term x term -> bool (misc) */
      ta = t->farg->argval;
      tb = t->farg->narg->argval;
      switch (op_code) {
      case RENAME_SYM:  b3 = same_structure(ta, tb); break;
      case OCCURS_SYM:  b3 = occurs_in(ta, tb); break;
      case VOCCURS_SYM: b3 = ta->type == VARIABLE && occurs_in(ta, tb); break;
      case VFREE_SYM:   b3 = ta->type == VARIABLE && !occurs_in(ta, tb); break;
      default: b3 = 0;
      }
      t->occ.lit = NULL; /* in case t is a literal */
      zap_term_special(t);
      t1 = get_term();
      t1->type = NAME;
      t1->sym_num = str_to_sn(b3 ? "$T" : "$F", 0);
      return(t1);
    case 18:  /* -> int */
      unique_num++;
      int_str(unique_num, str);
      t->sym_num = str_to_sn(str, 0);
      return(t);
    }
    printf("ERROR, dollar_contract, bad op_type: %d.\n", op_type);
    return(NULL);
  }
}  /* dollar_contract */

/*************
 *
 *
 *
 *************/

static struct term *replace_special(struct term *t,
				    struct term *target,
				    struct term *replacement)
{
  struct rel *r;

  if (term_ident(t, target)) {
    zap_term_special(t);
    replacement->fpa_id++;
    return(replacement);
  }
  else if (t->type == COMPLEX) {
    for (r = t->farg; r; r = r->narg)
      r->argval = replace_special(r->argval, target, replacement);
    return(t);
  }
  else
    return(t);
}  /* replace_special */

/*************
 *
 *    struct term *demod(term, contract_proc, demods, count, context, histp)
 *
 *    Demodulate a term.
 *
 *        The demodulated term is returned, and the given term
 *    becomes garbage, so a good way to invoke is `t = demod(t, demods, ...'.
 *    A context must be allocated before the call--the same one is used
 *    for all subterms--this saves allocating and deallocating at
 *    each subterm.  `count' is pointer to the maximum number of
 *    rewrites that will be applied.  `contract_proc' is a pointer to the
 *    routine that looks for demodulators and does the rewriting.
 *    The type of `demods' depends on `contract_proc'.
 *
 *************/

static struct term *demod(struct term *t,
			  struct term *(*contract_proc)(struct term *t,
							int *demods,
							struct context *c,
							int *demod_id_p),
			  int *demods,
			  long int *count,
			  struct context *c,
			  struct ilist **histp)
{
  struct rel *r;
  struct term *t1;
  struct ilist *ip;
  int demod_id;

  if (t->type == VARIABLE || TP_BIT(t->bits, SCRATCH_BIT) || *count <= 0)
    /* don't try to demodulate if a var or if already fully demodulated */
    return(t);   
  else if (t->type == COMPLEX) {

    /* if $IF, evaulate right now! */

    if (Internal_flags[DOLLAR_PRESENT] && sn_to_ec(t->sym_num) == IF_SYM) {
      /* first reduce condition */
      t->farg->argval = demod(t->farg->argval, contract_proc, demods, count, c, histp);
      /* now evaluate $IF */
      t1 = dollar_contract(t);
      if (t1 != NULL) {
	(*count)--;
	return(demod(t1, contract_proc, demods, count, c, histp));
      }
    }
#if 0
    else if (Internal_flags[DOLLAR_PRESENT] && sn_to_ec(t->sym_num) == COMMON_EXPRESSION_SYM) {
      t->farg->narg->argval = demod(t->farg->narg->argval, contract_proc, demods, count, c, histp);
      t->farg->narg->narg->argval = replace_special(t->farg->narg->narg->argval, t->farg->argval, t->farg->narg->argval);
      t1 = t->farg->narg->narg->argval;
      t1->fpa_id++;
      zap_term_special(t);
      return(demod(t1, contract_proc, demods, count, c, histp));
    }
#endif

    /* Fully demodulate subterms.  Do not demod subterms of $RENAME(_,_). */

    if (!Internal_flags[DOLLAR_PRESENT] || sn_to_ec(t->sym_num) != RENAME_SYM) {
      r = t->farg;
      while (r && *count > 0) {
	r->argval = demod(r->argval, contract_proc, demods, count, c, histp);
	r = r->narg;
      }
    }
  }

  if (*count > 0) {
    int debug = Flags[VERY_VERBOSE].val &&
      (Parms[DEMOD_LIMIT].val - *count) >= Parms[VERBOSE_DEMOD_SKIP].val;

    if (debug) {
      fprintf(stdout, "   demod term: ");
      print_term_nl(stdout, t);
      fflush(stdout);
    }

    t1 = (*contract_proc)(t, demods, c, &demod_id);
    if (t1 != NULL) {
      if (debug) {
	fprintf(stdout, "   --> result: ");
	print_term(stdout, t1);
	fprintf(stdout, "   demod<%d>\n", demod_id);
	fflush(stdout);
      }
      (*count)--;
      if (*histp != NULL) {
	ip = get_ilist();
	ip->i = demod_id;
	(*histp)->next = ip;
	*histp = ip;
      }
      t = demod(t1, contract_proc, demods, count, c, histp);
    }
    else if (Internal_flags[DOLLAR_PRESENT]) {
      t1 = dollar_contract(t);
      if (t1 != NULL) {
	(*count)--;
	t = t1;
      }
    }
    SET_BIT(t->bits, SCRATCH_BIT);
  }
  return(t);
}  /* demod */

/*************
 *
 *    struct term *left_most_one_step(t, contract_proc, demods, c, histp)
 *
 *************/

static struct term *left_most_one_step(struct term *t,
			       struct term *(*contract_proc)(struct term *t,
							     int *demods,
							     struct context *c,
							     int *demod_id_p),
				       int *demods,
				       struct context *c,
				       struct ilist **histp)
{
  struct term *t1;
  struct ilist *ip;
  struct rel *r;
  int demod_id;

  if (t->type == VARIABLE || TP_BIT(t->bits, SCRATCH_BIT))
    return(NULL);
  else {
    t1 = (*contract_proc)(t, demods, c, &demod_id);
    if (t1 != NULL) {
      if (*histp != NULL) {
	ip = get_ilist();
	ip->i = demod_id;
	(*histp)->next = ip;
	*histp = ip;
      }
    }
    else {
      if (Internal_flags[DOLLAR_PRESENT]) {
	t1 = dollar_contract(t);
      }
    }
	
    if (t1 != NULL)
      return(t1);
    else {
      r = t->farg;
      while (r != NULL) {
	t1 = left_most_one_step(r->argval, contract_proc,
				demods, c, histp);
	if (t1 != NULL) {
	  r->argval = t1;
	  return(t);
	}
	SET_BIT(r->argval->bits, SCRATCH_BIT);
	r = r->narg;
      }
      return(NULL);
    }
  }
}  /* left_most_one_step */

/*************
 *
 *    struct term *demod_out_in(term, contract_proc, demods, count, subst, histp)
 *
 *************/

static struct term *demod_out_in(struct term *t,
				 struct term *(*contract_proc)(struct term *t,
							       int *demods,
							       struct context *c,
							       int *demod_id_p),
				 int *demods,
				 long int *count,
				 struct context *c,
				 struct ilist **histp)
{
  struct term *t1;

  t1 = left_most_one_step(t, contract_proc, demods, c, histp);
  while (t1 != NULL) {
    (*count)--;
    if (*count <= 0)
      return(t1);
    else {
      t = t1;
      t1 = left_most_one_step(t, contract_proc, demods, c, histp);
    }
  }
  return(t);
}  /* demod_out_in */

/*************
 *
 *    un_share_special(term)
 *
 *        Given a term in which some of the subterms (not the term
 *    itself) may be referenced more than once (fpa_id > 0),
 *    transform it into a term in which all of the subterms
 *    are referenced exactly once (a normal nonintegrated term)
 *    by copying the appropriate subterms.
 *    Also clear the SCRATCH bit in all subterms visited.
 *
 *************/

void un_share_special(struct term *t)
{
  struct rel *r;

  CLEAR_BIT(t->bits, SCRATCH_BIT);
  if (t->type != COMPLEX)
    return;
  else {
    r = t->farg;
    while (r != NULL) {
      if (r->argval->fpa_id != 0) {
	r->argval->fpa_id--;
	r->argval = copy_term(r->argval);
      }
      else
	un_share_special(r->argval);
      r = r->narg;
    }
  }
}  /* un_share_special */

/*************
 *
 *    convenient_demod(t)
 *
 *************/

struct term *convenient_demod(struct term *t)
{
  struct term *t1;
  struct context *c;
  struct ilist *hist;
  long limit;

  limit = Parms[DEMOD_LIMIT].val;

  limit = (limit == -1 ? MAX_LONG_INT : limit);

  hist = NULL;  /* so that history will not be kept */

  c = get_context();

  if (Flags[DEMOD_OUT_IN].val) {
    if (Flags[DEMOD_LINEAR].val)
      t1 = demod_out_in(t, contract_lin, (int *) Demodulators, &limit, c,&hist);
    else
      t1 = demod_out_in(t, contract_imd, (int *) Demod_imd, &limit, c, &hist);
  }
  else {
    if (Flags[DEMOD_LINEAR].val)
      t1 = demod(t, contract_lin, (int *) Demodulators, &limit, c, &hist);
    else
      t1 = demod(t, contract_imd, (int *) Demod_imd, &limit, c, &hist);
  }

  free_context(c);
  un_share_special(t1);
  return(t1);

}  /* convenient_demod */

/*************
 *
 *    zap_term_special(term)  --  Special term deletion.
 *
 *        Deletion of nonintegrated term in which the term and
 *    some of its subterms might be referenced more than once.
 *    term->fpa_id is a count of the number of extra references.
 *    If we get to a term with more than one reference, then
 *    decrement the number of references; else recurse on the
 *    subterms and free the node.
 *
 *************/

void zap_term_special(struct term *t)
{
  struct rel *r1, *r2;

  if (t->occ.rel != NULL) {
    printf("WARNING, zap_term_special called with contained term: ");
    print_term(stdout, t);
    printf("\n");
  }
  else if (t->fpa_id != 0)
    t->fpa_id--;
  else {
    if (t->type == COMPLEX) { /* complex term */
      r1 = t->farg;
      while (r1 != NULL) {
	zap_term_special(r1->argval);
	r2 = r1;
	r1 = r1->narg;
	free_rel(r2);
      }
    }
    free_term(t);
  }
}  /* zap_term_special */

/*************
 *
 *    struct term *apply_demod(t, context, mult_flag_ptr)
 *
 *    Special purpose apply -- for demodulation.
 *
 *         If t is a variable instantiated to some term t2,
 *    then increment the reference count of t2, and return t2;
 *    else create a new node and recurse on any subterms.
 *
 *    mult_flag_ptr is a pointer to flag for incrementing multiplier
 *    for demodulators that have a variable on the right that doesn't
 *    occur on the left:  If an uninstantiated variable is encountered,
 *    then the flag is set;  when finished applying to beta, if the
 *    flag is set, then the multiplier is incremented.
 *    (It may be the case that demods of this type are not allowed.)
 *
 *************/

struct term *apply_demod(struct term *t,
			 struct context *c,
			 int *pf)
{
  struct term *t2;
  struct rel *r1, *r2, *r3;

  if (t->type == VARIABLE && c->terms[t->varnum] != NULL) {  /* bound var */
    t2 = c->terms[t->varnum];
    t2->fpa_id++;  /* count of extra references to a term */
    return(t2);
  }

  if (t->type == VARIABLE) {  /* unboud variable */
    t2 = get_term();
    t2->type = VARIABLE;
    t2->varnum = c->multiplier * MAX_VARS + t->varnum;
    *pf = 1;  /* when finished applying to beta, increment multiplier */
    return(t2);
  }
  else if (t->type == NAME) {  /* name */
    t2 = get_term();
    t2->type = NAME;
    t2->sym_num = t->sym_num;
    return(t2);
  }
  else {  /* complex term */
    t2 = get_term();
    t2->type = COMPLEX;
    t2->sym_num = t->sym_num;
    r3 = NULL;
    r1 = t->farg;
    while (r1 != NULL ) {
      r2 = get_rel();
      if (r3 == NULL)
	t2->farg = r2;
      else
	r3->narg = r2;
      r2->argval = apply_demod(r1->argval, c, pf);
      r3 = r2;
      r1 = r1->narg;
    }
    return(t2);
  }
}  /* apply_demod */

/*************
 *
 *    demod_cl(c) -- demodulate a clause
 *
 *************/

void demod_cl(struct clause *c)
{
  struct literal *lit;
  struct term *atom;
  int linear, out_in, hist;
  long limit_save, limit;
  struct context *subst;
  struct ilist *ip_save, *ip_send, *ip_d;
  int *d;
  static int limit_warning_issued;

  linear = Flags[DEMOD_LINEAR].val;
  limit = Parms[DEMOD_LIMIT].val;
  out_in = Flags[DEMOD_OUT_IN].val;
  limit_save = limit = (limit == -1 ? MAX_LONG_INT : limit);
  hist = Flags[DEMOD_HISTORY].val;
  if (c->parents == NULL)
    ip_d = ip_save = get_ilist();
  else {
    ip_save = c->parents;
    while (ip_save->next != NULL)
      ip_save = ip_save->next;
    ip_d = NULL;
  }
  /* ip_save saves position to insert "d" if any demodulation occurs */
  ip_send = (hist ? ip_save : NULL);
  subst = get_context();
  subst->multiplier = 1;
  if (linear)
    d = (int *) Demodulators;
  else
    d = (int *) Demod_imd;

  lit = c->first_lit;
  while (lit != NULL && limit > 0) {
    atom = lit->atom;
    atom->occ.lit = NULL;  /* reset at end of loop */
    if (out_in) {
      if (linear)
	atom = demod_out_in(atom, contract_lin, d, &limit, subst, &ip_send);
      else
	atom = demod_out_in(atom, contract_imd, d, &limit, subst, &ip_send);
    }
    else {
      if (linear)
	atom = demod(atom, contract_lin, d, &limit, subst, &ip_send);
      else
	atom = demod(atom, contract_imd, d, &limit, subst, &ip_send);
    }
    if (atom->type == VARIABLE) {
      abend("demod_cl, an atom has been rewritten to a variable ---\n"
	    "perhaps you are using a symbol as both a function symbol\n"
	    "and as a relation symbol.");
    }
    un_share_special(atom);
    if (atom->varnum == TERM) {  /* if the atom itself was changed */
      lit->atom = atom;
      mark_literal(lit);
    }
    atom->occ.lit = lit;
    lit = lit->next_lit;
  }

  if (subst->multiplier != 1) {
    /* new variables were introduced */
    if (renumber_vars(c) == 0) {
      print_clause(stdout, c);
      abend("demod_cl, demodulation introduced too many variables.");
    }
  }

  if (limit <= 0) {
    Stats[DEMOD_LIMITS]++;
    if (!limit_warning_issued) {
      printf("WARNING, demod_limit (given clause %ld): ", Stats[CL_GIVEN]);
      print_clause(stdout, c);
      fprintf(stderr, "\n%cWARNING, demod_limit stopped the demodulation of a clause.\nThis warning will not be repeated.\n\n", Bell);
      limit_warning_issued = 1;
    }
    else
      printf("WARNING, demod_limit.\n");
  }
  /* if some demodulation occured, insert DEMOD_RULE into parent list */
  if (limit_save > limit) {
    if (ip_d != NULL) {
      c->parents = ip_d;
    }
    else {
      ip_d = get_ilist();
      ip_d->next = ip_save->next;
      ip_save->next = ip_d;
    }
    ip_d->i = DEMOD_RULE;
  }
  else if (ip_d != NULL)
    free_ilist(ip_d);
  Stats[REWRITES] += (limit_save - limit);
  free_context(subst);
}  /* demod_cl */

/*************
 *
 *    back_demod(d, c, input, lst) - back demodulate with d
 *
 *    But don't back demodulate d or c
 *
 *************/

void back_demod(struct clause *d,
		struct clause *c,
		int input,
		struct list *lst)
{
  struct term *atom;
  struct term_ptr *tp, *tp2;
  struct clause_ptr *cp, *cp2;
  struct ilist *ip0, *ip1;
  struct clause *c1, *c2;
  int ok;

  atom = ith_literal(d,1)->atom;

  if (Flags[INDEX_FOR_BACK_DEMOD].val)
    tp = all_instances_fpa(atom, Fpa_back_demod);
  else
    tp = all_instances(atom);

  cp = NULL;
  while (tp) {
    all_cont_cl(tp->term, &cp);
    tp2 = tp;
    tp = tp->next;
    free_term_ptr(tp2);
  }
  while (cp) {
    c1 = cp->c;
    ok = 0;
    if (c1 == d || c1 == c)
      ok = 0;  /* don't back demodulate yourself */
    else if (c1->container == Usable) {
      ok = 1;
      CLOCK_START(UN_INDEX_TIME);
      un_index_lits_all(c1);
      un_index_lits_clash(c1);
      CLOCK_STOP(UN_INDEX_TIME);
    }
    else if (c1->container == Sos) {
      ok = 1;
      CLOCK_START(UN_INDEX_TIME);
      un_index_lits_all(c1);
      CLOCK_STOP(UN_INDEX_TIME);
    }
    else if (c1->container == Demodulators) {
      if (ith_literal(c1,1)->atom->varnum == CONDITIONAL_DEMOD)
	ok = 0;
      else {
	if (Flags[DEMOD_LINEAR].val == 0) {
	  CLOCK_START(UN_INDEX_TIME);
	  imd_delete(c1, Demod_imd);
	  CLOCK_STOP(UN_INDEX_TIME);
	}
	if (c1->parents != NULL && c1->parents->i == NEW_DEMOD_RULE) {
	  /* just delete it.  this works because list is decreasing; */
	  /* dynamic demodulator has id greater than parent. */
	  rem_from_list(c1);
	  hide_clause(c1);
	  ok = 0;
	}
	else {
	  ith_literal(c1,1)->atom->varnum = POS_EQ; /* in case lex-dep */
	  ok = 1;
	}
      }
    }
    if (ok) {
      Stats[CL_BACK_DEMOD]++;
      if (Flags[PRINT_BACK_DEMOD].val || input)
	printf("    >> back demodulating %d with %d.\n", c1->id, d->id);
      rem_from_list(c1);
      c2 = cl_copy(c1);
      ip0 = get_ilist(); ip0->i = BACK_DEMOD_RULE;
      ip1 = get_ilist(); ip1->i = c1->id;
      c2->parents = ip0; ip0->next = ip1;
      hide_clause(c1);
      CLOCK_STOP(BACK_DEMOD_TIME);
      CLOCK_STOP(POST_PROC_TIME);
      pre_process(c2, 0,lst);
      CLOCK_START(POST_PROC_TIME);
      CLOCK_START(BACK_DEMOD_TIME);
    }
    cp2 = cp;
    cp = cp->next;
    free_clause_ptr(cp2);
  }
}  /* back_demod */

/*************
 *
 *    int lit_t_f_reduce(c) -- evaluate evaluable literals
 *
 *    delete any false literals, $F or -$T.
 *
 *    return:  0 -- clause evaluated successfully.
 *             1 -- clause evaluated successfully to TRUE (clause/lits
 *                  not deallocated).
 *
 *************/

int lit_t_f_reduce(struct clause *c)
{
  struct literal *lit, *prev_lit, *next_lit;
  int atom_true, atom_false;
  char *s;
  int changed = 0;
  int eval_to_true = 0;

  lit = c->first_lit;
  prev_lit = NULL;
  while (lit != NULL && !eval_to_true) {
    next_lit = lit->next_lit;
    if (lit->atom->varnum == EVALUABLE) {
      s = sn_to_str(lit->atom->sym_num);
      atom_true = str_ident(s, "$T");
      atom_false = str_ident(s, "$F");
      if (atom_true || atom_false) {
	changed = 1;
	if ((atom_true && lit->sign ) ||
	    (atom_false && lit->sign == 0 ))
	  eval_to_true = 1;
	else if ((atom_false && lit->sign) ||
		 (atom_true && lit->sign == 0 )) {
	  /* lit is false, so delete it */
	  if (prev_lit == NULL) {
	    c->first_lit = next_lit;
	    lit->atom->occ.lit = NULL;
	    zap_term(lit->atom);
	    free_literal(lit);
	    lit = NULL;
	  }
	  else {
	    prev_lit->next_lit = next_lit;
	    lit = prev_lit;
	  }
	}
      }
    }
    prev_lit = lit;
    lit = next_lit;
  }
  if (changed) {
    /* append "PROPOSITIONAL_RULE" to list of parents */
    struct ilist *p1, *p2, *p3;
    for (p1 = NULL, p2 = c->parents; p2; p1 = p2, p2 = p2->next);
    p3 = get_ilist();
    p3->i = PROPOSITIONAL_RULE;
    if (p1)
      p1->next = p3;
    else
      c->parents = p3;
  }
  return(eval_to_true);
}  /* lit_t_f_reduce */

/*************
 *
 *    int check_input_demod(c)
 *
 *    Check if it is a valid demodulator, possibly flipping and
 *    making lex_dependent.
 *
 *************/

int check_input_demod(struct clause *c)
{
  struct term *atom, *alpha, *beta;

  if (!unit_clause(c))
    return(0);
  else if (num_literals_including_answers(c) != 1)
    return(0);
  else {
    /* exactly one literal, which is non-answer */
    atom = ith_literal(c,1)->atom;
    if (atom->varnum == CONDITIONAL_DEMOD) {
      alpha = atom->farg->narg->argval->farg->argval;
      beta =  atom->farg->narg->argval->farg->narg->argval;
      return(!term_ident(alpha, beta));
    }
    else if (atom->varnum != POS_EQ)
      return(0);
    else {
      alpha = atom->farg->argval;
      beta = atom->farg->narg->argval;
      if (term_ident(alpha, beta))
	return(0);
      else {
	/* it can be a demodulator */
	if (!Flags[LRPO].val) {
	  if (term_ident_x_vars(alpha, beta)) {
	    printf("lex dependent demodulator: ");
	    print_clause(stdout, c);
	    atom->varnum = LEX_DEP_DEMOD;
	  }
	}
	else {
	  if (lrpo_greater(alpha, beta))
	    ;  /* do nothing */
	  else if (lrpo_greater(beta, alpha)) {
	    /* flip args */
	    printf("Flipping following input demodulator due to lrpo ordering: ");
	    print_clause(stdout, c);
	    atom->farg->argval = beta;
	    atom->farg->narg->argval = alpha;
	  }
	  else {
	    printf("LRPO dependent demodulator: ");
	    print_clause(stdout, c);
	    atom->varnum = LEX_DEP_DEMOD;
	  }
	}
#if 0
	if (!var_subset(atom->farg->narg->argval,atom->farg->argval)) {
	  printf("beta has a variable not in alpha: ");
	  return(0);
	}
	else
#endif
	  return(1);
      }
    }
  }
}  /* check_input_demod */

/*************
 *
 *   dynamic_demodulator(c)
 *
 *   return  0: don't make it a demodulator
 *           1: regular demodulator
 *           2: lex- or lrpo- dependent demodulator
 *
 *************/

int dynamic_demodulator(struct clause *c)
{
  struct literal *l;
  struct term *alpha, *beta;
  int wt_left, wt_right;

  l = ith_literal(c, 1);

  alpha = l->atom->farg->argval;
  beta  = l->atom->farg->narg->argval;

  if (TP_BIT(l->atom->bits, ORIENTED_EQ_BIT)) {
    if (Flags[LRPO].val)
      return(1);
    else if (var_subset(beta, alpha)) {
      if (Flags[DYNAMIC_DEMOD_ALL].val)
	return(1);
      else {
	wt_left  =  weight(alpha, Weight_terms_index);
	wt_right =  weight(beta, Weight_terms_index);
	if (wt_right <= Parms[DYNAMIC_DEMOD_RHS].val &&
	    wt_left - wt_right >= Parms[DYNAMIC_DEMOD_DEPTH].val)
	  return(1);
      }
    }
  }

  if (!Flags[DYNAMIC_DEMOD_LEX_DEP].val)
    return(0);

  else if (Flags[LRPO].val) {
    if (var_subset(beta, alpha) && !term_ident(alpha, beta))
      return(2);
    else
      return(0);
  }

  else {
    if (!Flags[DYNAMIC_DEMOD_ALL].val)
      return(0);
    else if (term_ident_x_vars(alpha, beta) && !term_ident(alpha, beta))
      return(2);
    else
      return(0);
  }
}  /* dynamic_demodulator */

/*************
 *
 *    new_demod(c, demod_flag)
 *
 *    Make an equality unit into a demodulator.
 *    It has already been checked (in order_equalities) if
 *    alpha > beta.  (Don't flip or back demodulate.)
 *
 *    If demod_flag == 2, make it lex-dependent.
 *
 *    If back_demod is set, set SCRATCH_BIT in the atom so that
 *    post_process knows to back demodulate.
 *
 *    Return the new demodulator.
 *
 *************/

struct clause *new_demod(struct clause *c,
			 int demod_flag)
{
  struct clause *d;
  struct ilist *ip0, *ip1;

  Stats[NEW_DEMODS]++;
  d = cl_copy(c);
  ip0 = get_ilist(); ip0->i = NEW_DEMOD_RULE;
  ip1 = get_ilist(); ip1->i = c->id;
  d->parents = ip0; ip0->next = ip1;
  cl_integrate(d);
  if (demod_flag == 2)
    ith_literal(d,1)->atom->varnum = LEX_DEP_DEMOD;
  if (Flags[BACK_DEMOD].val) {
    struct term *atom;
    atom = ith_literal(c,1)->atom;
    SET_BIT(atom->bits, SCRATCH_BIT);
  }
  if (Flags[DEMOD_LINEAR].val == 0) {
    if (Demod_imd == NULL)
      Demod_imd = get_imd_tree();
    imd_insert(d, Demod_imd);
  }

  append_cl(Demodulators, d);
  return(d);

}  /* new_demod */

