/*
 *  formed.c -- FormEd (Formula Editor) X Windows program.
 *
 */

#define IN_MAIN
#include "formed.h"
#include "../header.h"

/*************
 *
 *    main(argc, argv)
 *
 *************/

main(argc, argv)
int argc;
char *argv[];
{
    
    init();
    
    Flags[CHECK_ARITY].val = 0;
    Parms[MAX_MEM].val = 4000;
    Crnt_formula = NULL;
    Crnt_transform = NULL;
    Display_setup = 0;
    Have_message = 0;
    strcpy(Crnt_file,"\0");
    
    /* process any command line arguments */
    proc_command_line(argc, argv);
    
    if(Load_file)
	load_formula_list(Crnt_file);  /* note - errors caught inside routine */
    
    /* if had a load error, just display screen, user can load via button */
    
    /* set up initial display & event handlers */
    setup_display(argc, argv);
    
    /* if have a file loaded */
    
    if(Crnt_formula) {
	Top_formula = Crnt_formula;
	
	Crnt_transform = Crnt_formula;
	
	/* display the first formula */
	display_formula(Crnt_transform);
	}
    
    /* start events - callbacks will handle from now on*/
    XtAppMainLoop(App_con);
    
}  /* main */

/*************************
*  
*     void proc_command_line(argc,argv)
*
*****************************/

void proc_command_line(argc, argv)
int argc;
char *argv[];
{
    int opt_count,c;
    int opt_index = 0;
    char *option;
    
    /* get & process any command line arguments */
    
    opt_count = argc;
    Fore_set = 0;
    Back_set = 0;
    Load_file = 0;
    
    while(--opt_count > 0 && (strncmp("-",argv[++opt_index],1)==0)) {
	option = strpbrk(argv[opt_index], "fbl");
	if (option) {
	    c = *option;
	    switch(c) {
	      case 'f':
		Fore_set = 1;
		strcpy( User_fore,argv[++opt_index]);
		opt_count--;
		break;
	      case 'b':
		Back_set = 1;
		strcpy( User_back,argv[++opt_index]);
		opt_count--;
		break;
	      case 'l':
		Load_file = 1;
		strcpy( Crnt_file,argv[++opt_index]);
		opt_count--;
		break;
	      default:
		printf("\nError: Illegal option.");
		printf("\nUsage:  xform2 [-f color] [-b color] [-l filename]\n\n");
		exit(0);
		break;
		}
	    }
	else {
	    printf("\nError: Illegal option.");
	    printf("\nUsage:  xform2 [-f color] [-b color] [-l filename]\n\n");
	    exit(0);
	    }
	}
    
}   /* end proc_command_line */

/*************
 *
 *   str_print_variable(str, ip, variable)
 *
 *************/

static void str_print_variable(str, ip, t)
char *str;
int *ip;
struct term *t;
{
    int i;
    char *s2, s3[100];

    if (t->sym_num != 0) {
        s2 = sn_to_str(t->sym_num);
	strcpy( str += *ip,s2);
	*ip += strlen(s2);
        }
    else if (Flags[PROLOG_STYLE_VARIABLES].val) {
	str[(*ip)++] = (t->varnum % 26) + 'A';
	i = t->varnum / 26;
	if (i > 0) {
	    sprintf(s3, "%d", i);
	    strcpy( str+*ip,s3);
	    *ip += strlen(s3);
	    }
	}
    else {
	if (t->varnum <= 2)
	    str[(*ip)++] = t->varnum + 'x';
	else if (t->varnum <= 5)
	    str[(*ip)++] = t->varnum + 'r';
	else {
	    str[(*ip)++] = 'v';
	    sprintf(s3, "%d", t->varnum);
	    strcpy( str+*ip,s3);
	    *ip += strlen(s3);
	    }
	}
}  /* str_print_variable */

/*************
 *
 *    str_print_term(string, ip, term)  --  Print a term to a file.
 *
 *        Variables 0-5 are printed as x,y,z,u,v,w, and equalities
 *    and negated equalities are printed in infix.
 *
 *************/

static void str_print_term(str, ip, t)
char *str;
int *ip;
struct term *t;
{
    struct rel *r;
    struct term *t1;
    char *s2;
    
    if (t == NULL) {
        strcpy( str+*ip,"(nil)");
	*ip += 5;
	}
    else if (t->type == NAME) {           /* name */
	if (t->sym_num == Nil_sym_num) {
	    strcpy( str+*ip,"[]");
	    *ip += 2;
	    }
	else {
	    s2 = sn_to_str(t->sym_num);
	    strcpy( str+*ip,s2);
	    *ip += strlen(s2);
	    }
	}
    else if (t->type == VARIABLE)               /* variable */
	str_print_variable(str, ip, t);
    else {  /* complex */
	if (t->sym_num == Cons_sym_num) {   /* list notation */
	    str[(*ip)++] = '[';
	    str_print_term(str, ip, t->farg->argval);
	    if (proper_list(t) == 0) {
		str[(*ip)++] = '|';
		str_print_term(str, ip, t->farg->narg->argval);
		str[(*ip)++] = ']';
		}
	    else {
		t1 = t->farg->narg->argval;
		while (t1->sym_num != Nil_sym_num) {
		    str[(*ip)++] = ',';
		    str_print_term(str, ip, t1->farg->argval);
		    t1 = t1->farg->narg->argval;
		    }
		str[(*ip)++] = ']';
		}
	    }   /* list notation */
	else if (is_symbol(t, "=", 2) && t->varnum != TERM) {
	    /* (t1 = t2) or (t1 != t2) */
	    str[(*ip)++] = '(';
	    str_print_term(str, ip, t->farg->argval);
	    if (t->occ.lit != NULL && t->occ.lit->sign == 0) {
		strcpy( str+*ip," != ");
		*ip += 4;
		}
	    else {
		strcpy( str+*ip," = ");
		*ip += 3;
		}
	    str_print_term(str, ip, t->farg->narg->argval);
	    str[(*ip)++] = ')';
	    }
	else {
	    s2 = sn_to_str(t->sym_num);
	    strcpy( str+*ip,s2);
	    *ip += strlen(s2);
	    str[(*ip)++] = '(';
	    r = t->farg;
	    while(r != NULL) {
		str_print_term(str, ip, r->argval);
		r = r->narg;
		if(r != NULL)
		    str[(*ip)++] = ',';
		}
	    str[(*ip)++] = ')';
	    }
	}
}  /* str_print_term */

/*************
 *
 *    int sprint_term(s, t) -- return length of s.
 *
 *************/

static int sprint_term(s, t)
char *s;
struct term *t;
{
    int i;

    i = 0;
    str_print_term(s, &i, t);
    s[i] = '\0';
    return(i);

}  /* sprint_term */

/*************
 *
 *    static void str_print_formula(str, ip, f)
 *
 *    Print a formula to a string and count the length of the string.
 *
 *************/

static void str_print_formula(str, ip, f)
char *str;
int *ip;
struct formula *f;
{

    char op[MAX_NAME];
    struct formula *f1;
    
    if (f == NULL) {
        strcpy( str+*ip,"(nil)");
        *ip += 5;
        }
    else if (f->type == ATOM_FORM) {
	str_print_term(str, ip, f->t);
	}
    else if (f->type == NOT_FORM) {
	strcpy( str+*ip,"-");
	*ip += 1;
	str_print_formula(str, ip, f->first_child);
	}
    else if (f->type == AND_FORM && f->first_child == NULL) {
	strcpy( str+*ip,"TRUE");
	*ip += 4;
	}
    else if (f->type == OR_FORM && f->first_child == NULL) {
	strcpy( str+*ip,"FALSE");
	*ip += 5;
	}
    else if (f->type == QUANT_FORM) {
	strcpy( str+*ip,"(");
	*ip += 1;
	if (f->quant_type == ALL_QUANT) {
	    strcpy( str+*ip,"all ");
	    *ip += 4;
	    }
	else {
	    strcpy( str+*ip,"exists ");
	    *ip += 7;
	    }
	str_print_term(str, ip, f->t);
	strcpy( str+*ip," ");
	*ip += 1;
	str_print_formula(str, ip, f->first_child);
	strcpy( str+*ip,")");
	*ip += 1;
	}
    else {
	if (f->type == AND_FORM)
	    strcpy( op," & ");
	else if (f->type == OR_FORM)
	    strcpy( op," | ");
	else if (f->type == IMP_FORM)
	    strcpy( op," -> ");
	else if (f->type == IFF_FORM)
	    strcpy( op," <-> ");
	else
	    op[0] = '\0';
	
	strcpy( str+*ip,"(");
	*ip += 1;
	for (f1 = f->first_child; f1; f1 = f1->next) {
	    str_print_formula(str, ip, f1);
	    if (f1->next) {
		strcpy( str+*ip,op);
		*ip += strlen(op);
		}
	    }
	strcpy( str+*ip,")");
	*ip += 1;
	}

}  /* str_print_formula */

/*************
 *
 *    int sprint_formula(s, f) -- return length of s.
 *
 *************/

int sprint_formula(s, f)
char *s;
struct formula *f;
{
    int i;
    i = 0;
    str_print_formula(s, &i, f);
    s[i] = '\0';
    return(i);
}  /* sprint_formula */

/*************************
*  
*     void select_area(w, client_data, event)
*
*****************************/

void select_area(w, client_data, event)
Widget w;
caddr_t client_data;
XEvent *event;
{
    struct formula_box *prev_area;
    
    XButtonEvent *button_prssd;   /* button pressed */
    
    button_prssd = (XButtonEvent *) event;
    
    kill_message();
	
    /* only find & highlight box if have a formula displayed */
    
    if(Crnt_transform != NULL) {
	
	prev_area = Sel_area;
	
	Sel_area = find_point(B, button_prssd->x, button_prssd->y,
			      B->x_off, B->y_off);
	
	if (Highlighted) {
	    
	    /* clear previous selection by simply redrawing it*/
	    
	    XSetForeground(Dpy, Gc, Background);
	    XFillRectangle(Dpy, Win, Gc, prev_area->abs_x_loc, prev_area->abs_y_loc,
			   prev_area->length, prev_area->height);
	    XSetForeground(Dpy, Gc, Foreground);
	    draw_formula_box(prev_area, prev_area->abs_x_loc, prev_area->abs_y_loc);
	    }
	
	/* highlight area if not same selection, and inside formula box */
	
	if(Sel_area == NULL || (Highlighted && prev_area == Sel_area)) {
	    Highlighted = 0;
	    Sel_area = B;
	    }
	else {
	    draw_formula_box_inverted(Sel_area, Sel_area->abs_x_loc, Sel_area->abs_y_loc);
	    Highlighted = 1;
	    
	    }
	}
}    /* select_area */

/*************
 *
 *    draw_formula_box_inverted(b, x, y)
 *
 *************/

void draw_formula_box_inverted(b, x, y)
struct formula_box *b;
int x, y;
{   
    
    /*  couldn't make this work.
	XSetFillStyle(Dpy, Win, FillTiled);
	XSetTile(Dpy, Win, Pattern);
	*/    
    
    XFillRectangle(Dpy, Win, Gc, x, y, b->length, b->height);
    
    XSetForeground(Dpy, Gc, Background);
    XSetBackground(Dpy, Gc, Foreground);
    
    draw_formula_box(b, x, y);
    
    XSetForeground(Dpy, Gc, Foreground);
    XSetBackground(Dpy, Gc, Background);
    
    draw_inverted_operators(b, x, y);
    
    /*
      XSetFillStyle(Dpy, Win, FillSolid);
      */
    
}  /* draw_formula_box_inverted */

/*************************
*  
*     void display_formula(p1)
*
*****************************/

void display_formula(p1)
struct formula_ptr_2 *p1;
{
    /* set up the formula box locations and sizes for the display */
    /* the top box is global because it must be accessed thru event handlers */
    
    kill_message();
    
    if (B)
	free_formula_box_tree(B);
    
    B = arrange_box(p1->f);
    
    /* draw the formula on the canvas */
    
    XClearWindow(Dpy, Win);
    
    draw_formula_box(B,5,5);
    
    Sel_area = B;
    Highlighted = 0;
    
}  /* end display_formula */


/*************************
*  
*     struct formula_box *do_box_geometry(b, op)
*
*****************************/

struct formula_box *do_box_geometry(b, op)
struct formula_box *b;
int op;
{
    struct formula_box *b1;
    int height, length, n, x_center_line, y_center_line, x_current, y_current;
    
    if (op == NOT_FORM) {
	
	/*** set up negation type geometry ***/
	
	b1 = b->first_child;    /* op_box */
	
	length = Spacing + b1->length + Spacing + (b1->next)->length + Spacing;
	
	if (b1->height > (b1-> next)->height)
	    height = b1->height;
	else
	    height = (b1->next)->height;
	height = Spacing + height + Spacing;
	
	b->length = length;
	b->height = height;
	
	y_center_line = height / 2;
	
	b1->x_off  = Spacing;
	b1->y_off  = y_center_line - b1->height/2;
	(b1->next)->x_off = Spacing + b1->length + Spacing;
	(b1->next)->y_off =  y_center_line - (b1->next)->height/2;
	
	}
    
    else if (op == AND_FORM) {
	
	/*** set up conjunction type geometry ***/
	
	length = height = 0;
	for (b1 = b->first_child, n=0; b1; b1 = b1->next, n++) {
	    height += b1->height;
	    length = (b1->length > length ? b1->length : length);
	    }
	
	height = Spacing + height + (n-1)*Spacing + Spacing;
	length = Spacing + length + Spacing;
	
	b->length = length;
	b->height = height;
	
	x_center_line = length / 2;
	y_current = 0;
	
	for (b1 = b->first_child; b1; b1 = b1->next) {
	    b1->y_off = y_current + Spacing;
	    y_current += Spacing + b1->height;
	    if (0)  /* center conjuncts */
		b1->x_off = x_center_line - b1->length/2;
	    else    /* left justify conjuncts */
		b1->x_off = Spacing;
	    }
	}
    
    else {
	
	/*** set up disjunction type of geometry (includes iff,imp,quant's) ***/
	
	length = height = 0;
	for (b1 = b->first_child, n=0; b1; b1 = b1->next, n++) {
	    length += b1->length;
	    height = (b1->height > height ? b1->height : height);
	    }
	
	length = Spacing + length + (n-1)*Spacing + Spacing;
	height = Spacing + height + Spacing;
	
	b->length = length;
	b->height = height;
	
	y_center_line = height / 2;
	x_current = 0;
	
	for (b1 = b->first_child; b1; b1 = b1->next) {
	    b1->x_off = x_current + Spacing;
	    x_current += Spacing + b1->length;
	    b1->y_off = y_center_line - b1->height/2;
	    }
	}
    
    return(b);
    
}  /* end do_box_geometry */

/*************
 *
 *    static struct formula_box *arrange_box_term(t)
 *
 *************/

static struct formula_box *arrange_box_term(t)
struct term *t;
{
    struct formula_box *b;
    char str[1000];
    int len;
    
    b = get_formula_box();
    b->type = FORMULA;         /* change to TERM later */
    b->subtype = ATOM_FORM;
    b->f = NULL;
    
    /* get the string */
    sprint_term(str, t);

    len = strlen(str);

    if (len >= 100)
	strcpy( b->str,"*** STRING TOO LONG ***");
    else
	strcpy( b->str,str);

    /* HERE find size of string and assign to length and width */
    b->length = XTextWidth(Font_struct, b->str, strlen(b->str));
    b->height = Font_char_ht;
    
    return(b);
}  /* arrange_box_term */

/*************
 *
 *    static struct formula_box *arrange_box_symbol(op)
 *
 *************/

static struct formula_box *arrange_box_symbol(op)
int op;
{
    struct formula_box *b;
    
    b = get_formula_box();
    b->type = OPERATOR;
    b->subtype = op;
    
    switch (op) {
      case OR_OP:
        b->length = Or_width;
        b->height = Or_height;
        break;
      case AND_OP:
        b->length = And_width;
        b->height = And_height;
        break;
      case NOT_OP:
        b->length = Not_width;
        b->height = Not_height;
        break;
      case IMP_OP:
        b->length = Imp_width;
        b->height = Imp_height;
        break;
      case IFF_OP:
        b->length = Iff_width;
        b->height = Iff_height;
        break;
      case ALL_OP:
        b->length = All_width;
        b->height = All_height;
        break;
      case EXISTS_OP:
        b->length = Exists_width;
        b->height = Exists_height;
        break;
      default:
        strcpy(Error_string, "arrange_box_symbol: unknown operator.\n");
        user_error(Error_string);
        }
    
    return(b);
    
}  /* arrange_box_symbol */

/*************
 *
 *    static struct formula_box *arrange_box_atom(f)
 *
 *************/

static struct formula_box *arrange_box_atom(f)
struct formula *f;
{
    struct formula_box *b;
    char str[1000];
    int len;
    
    b = get_formula_box();
    b->type = FORMULA;
    b->subtype = ATOM_FORM;
    b->f = f;

    /* get the string */
    sprint_term(str, f->t);

    len = strlen(str);

    if (len >= 100)
	strcpy( b->str,"*** STRING TOO LONG ***");
    else
	strcpy( b->str,str);

    /* HERE find size of string and assign to length and width */
    b->length = XTextWidth(Font_struct, b->str, strlen(b->str));
    b->height = Font_char_ht;
    
    return(b);
}  /* arrange_box_atom */

/*************
 *
 *    static struct formula_box *arrange_box_or(f)
 *
 *************/

static struct formula_box *arrange_box_or(f)
struct formula *f;
{
    struct formula_box *b, *sub_box, *op_box;
    struct formula *f1;
    
    b = get_formula_box();
    b->type = FORMULA;
    b->subtype = OR_FORM;
    b->f = f;

    if (f->first_child == NULL) {
	/* speicial case: empty disjunction is FALSE */
	strcpy( b->str,"FALSE");
	b->length = XTextWidth(Font_struct, b->str, strlen(b->str));
	b->height = Font_char_ht;
	}
    
    else {
	for (f1 = f->first_child, op_box = NULL; f1; f1 = f1->next) {
	    sub_box = arrange_box(f1);
	    sub_box->parent = b;
	    if (op_box)
		op_box->next = sub_box;
	    else
		b->first_child = sub_box;
	    if (f1->next) {
		op_box = arrange_box_symbol(OR_OP);
		sub_box->next = op_box;
		op_box->parent = b;
		}
	    }
	
	b = do_box_geometry(b, OR_FORM);
	}
    
    return(b);
    
}  /* arrange_box_or */

/*************
 *
 *    static struct formula_box *arrange_box_and(f)
 *
 *************/

static struct formula_box *arrange_box_and(f)
struct formula *f;
{
    struct formula_box *b, *sub_box, *op_box;
    struct formula *f1;
    
    b = get_formula_box();
    b->type = FORMULA;
    b->subtype = AND_FORM;
    b->f = f;
    
    if (f->first_child == NULL) {
	/* speicial case: empty conjunction is TRUE */
	strcpy( b->str,"TRUE");
	b->length = XTextWidth(Font_struct, b->str, strlen(b->str));
	b->height = Font_char_ht;
	}
    
    else {
	for (f1 = f->first_child, op_box = NULL; f1; f1 = f1->next) {
	    sub_box = arrange_box(f1);
	    sub_box->parent = b;
	    if (op_box)
		op_box->next = sub_box;
	    else
		b->first_child = sub_box;
	    if (0 && f1->next) {  /* display '&' */
		op_box = arrange_box_symbol(AND_OP);
		sub_box->next = op_box;
		op_box->parent = b;
		}
	    else  /* don't display '&' */
		op_box = sub_box;
	    }
	
	b = do_box_geometry(b, AND_FORM);
	}    
    return(b);
    
}  /* arrange_box_and */

/*************
 *
 *    static struct formula_box *arrange_box_not(f)
 *
 *************/

static struct formula_box *arrange_box_not(f)
struct formula *f;
{
    struct formula_box *b, *sub_box, *op_box;
    
    b = get_formula_box();
    b->type = FORMULA;
    b->subtype = NOT_FORM;
    b->f = f;
    
    op_box = arrange_box_symbol(NOT_OP);
    sub_box = arrange_box(f->first_child);
    op_box->parent = b;
    sub_box->parent = b;
    b->first_child = op_box;
    op_box->next = sub_box;
    
    b = do_box_geometry(b, NOT_FORM);
    
    return(b);
    
}  /* arrange_box_not */

/*************
 *
 *    static struct formula_box *arrange_box_imp(f)
 *
 *************/

static struct formula_box *arrange_box_imp(f)
struct formula *f;
{
    struct formula_box *b, *sub_box, *op_box;
    struct formula *f1;
    
    b = get_formula_box();
    b->type = FORMULA;
    b->subtype = IMP_FORM;
    b->f = f;
    
    for (f1 = f->first_child, op_box = NULL; f1; f1 = f1->next) {
	sub_box = arrange_box(f1);
        sub_box->parent = b;
        if (op_box)
            op_box->next = sub_box;
	else
	    b->first_child = sub_box;
	if (f1->next) {
	    op_box = arrange_box_symbol(IMP_OP);
            sub_box->next = op_box;
            op_box->parent = b;
            }
	}
    
    b = do_box_geometry(b, IMP_FORM);
    
    return(b);
    
}  /* arrange_box_imp */

/*************
 *
 *    static struct formula_box *arrange_box_iff(f)
 *
 *************/

static struct formula_box *arrange_box_iff(f)
struct formula *f;
{
    struct formula_box *b, *sub_box, *op_box;
    struct formula *f1;
    
    b = get_formula_box();
    b->type = FORMULA;
    b->subtype = IFF_FORM;
    b->f = f;
    
    for (f1 = f->first_child, op_box = NULL; f1; f1 = f1->next) {
	sub_box = arrange_box(f1);
        sub_box->parent = b;
        if (op_box)
            op_box->next = sub_box;
	else
	    b->first_child = sub_box;
	if (f1->next) {
	    op_box = arrange_box_symbol(IFF_OP);
            sub_box->next = op_box;
            op_box->parent = b;
            }
	}
    
    b = do_box_geometry(b, IFF_FORM);
    
    return(b);
    
}  /* arrange_box_iff */


/*************
 *
 *    static struct formula_box *arrange_box_quant(f)
 *
 *************/

static struct formula_box *arrange_box_quant(f)
struct formula *f;
{
    struct formula_box *b, *sub_box, *op_box, *b1;
    struct quantifier *q;
    
    b = get_formula_box();
    b->type = FORMULA;
    b->subtype = QUANT_FORM;
    b->f = f;
    
    /* pick off quantifier and its variable */
    
    if (f->quant_type == ALL_QUANT)
	op_box = arrange_box_symbol(ALL_OP);
    else
	op_box = arrange_box_symbol(EXISTS_OP);
    
    op_box->parent = b;
    
    b->first_child = op_box;
    
    sub_box = arrange_box_term(f->t);
    sub_box->parent = b;
    op_box->next = sub_box;
    
    b1 = sub_box;  
    sub_box = arrange_box(f->first_child);
    sub_box->parent = b;

    if (b1)
	b1->next = sub_box;
    else
	b->first_child = sub_box;
    
    b = do_box_geometry(b, QUANT_FORM);
    
    return(b);
    
}  /* arrange_box_quant */

/*************
 *
 *    struct formula_box *arrange_box(f)
 *
 *************/

struct formula_box *arrange_box(f)
struct formula *f;
{
    switch (f->type) {
      case ATOM_FORM:
	return(arrange_box_atom(f));
	break;
      case NOT_FORM:
	return(arrange_box_not(f));
	break;
      case AND_FORM:
	return(arrange_box_and(f));
	break;
      case OR_FORM:
	return(arrange_box_or(f));
	break;
      case IMP_FORM:
	return(arrange_box_imp(f));
	break;
      case IFF_FORM:
	return(arrange_box_iff(f));
	break;
      case QUANT_FORM:
	return(arrange_box_quant(f));
	break;
      default:
	strcpy(Error_string, "arrange_box: unknown operator.\n");
	user_error(Error_string);
	exit(1);
	return(NULL);
	break;
	}
}  /* arrange_box */

/*************
 *
 *    draw_formula_box(b, x, y)
 *
 *************/

void draw_formula_box(b, x, y)
struct formula_box *b;
int x;
int y;
{
    struct formula_box *b1;
    
    /* save absolute address of the box in the Window */
    b->abs_x_loc = x;
    b->abs_y_loc = y;
    
    if (b->type == OPERATOR) {
        /*  operator at position (x,y) */
	switch(b->subtype) {
	  case OR_OP:
	    XCopyArea(Dpy, Or, Win, Gc, 0,0, Or_width, 
		      Or_height, x, y);
	    break;
	  case AND_OP:
	    XCopyArea(Dpy, And, Win, Gc, 0, 0, And_width,
		      And_height, x, y);
	    break;
	  case NOT_OP:
	    XCopyArea(Dpy, Not, Win, Gc, 0, 0, Not_width,
		      Not_height, x, y);
	    break;
	  case IMP_OP:
	    XCopyArea(Dpy, Imp, Win, Gc, 0, 0, Imp_width, 
		      Imp_height, x, y);
	    break;
	  case IFF_OP:
	    XCopyArea(Dpy, Iff, Win, Gc, 0, 0, Iff_width, 
		      Iff_height, x, y);
	    break;
	  case EXISTS_OP:
	    XCopyArea(Dpy, Exists, Win, Gc, 0, 0, Exists_width, 
                      Exists_height, x, y);
	    break;
	  case ALL_OP:
	    XCopyArea(Dpy, All, Win, Gc, 0, 0, All_width, 
		      All_height, x, y);
	    break;
	  default:
	    strcpy(Error_string, "draw_formula_box: unknown operator.\n");
	    user_error(Error_string);
	    }
	}
    else if (b->subtype == ATOM_FORM || b->first_child == NULL) {
	/* draw string b->str at position (x,y) */
	
	XDrawImageString(Dpy, Win, Gc, x, y + Font_ascent,
			 b->str, strlen(b->str));
	
  	}
    else {  /* non-atomic formula */

	if (b->subtype != NOT_FORM && b->subtype != QUANT_FORM)
	    XDrawRectangle(Dpy, Win, Gc, x, y, b->length, b->height);
	
	for (b1 = b->first_child; b1; b1 = b1->next)
	    draw_formula_box(b1, x + b1->x_off, y + b1->y_off);
	}
    
}  /* draw_formula_box */

/*************
 *
 *    draw_inverted_operators(b, x, y)
 *
 *************/

void draw_inverted_operators(b, x, y)
struct formula_box *b;
int x;
int y;
{
    struct formula_box *b1;
    
    if (b->type == OPERATOR) {
        /*  operator at position (x,y) */
	switch(b->subtype) {
	  case OR_OP:
	    XCopyArea(Dpy, Or_invert, Win, Gc, 0,0, Or_width, 
		      Or_height, x, y);
	    break;
	  case AND_OP:
	    XCopyArea(Dpy, And_invert, Win, Gc, 0, 0, And_width,
		      And_height, x, y);
	    break;
	  case NOT_OP:
	    XCopyArea(Dpy, Not_invert, Win, Gc, 0, 0, Not_width,
		      Not_height, x, y);
	    break;
	  case IMP_OP:
	    XCopyArea(Dpy, Imp_invert, Win, Gc, 0, 0, Imp_width, 
		      Imp_height, x, y);
	    break;
	  case IFF_OP:
	    XCopyArea(Dpy, Iff_invert, Win, Gc, 0, 0, Iff_width, 
		      Iff_height, x, y);
	    break;
	  case EXISTS_OP:
	    XCopyArea(Dpy, Exists_invert, Win, Gc, 0, 0, Exists_width, 
                      Exists_height, x, y);
	    break;
	  case ALL_OP:
	    XCopyArea(Dpy, All_invert, Win, Gc, 0, 0, All_width, 
		      All_height, x, y);
	    break;
	  default:
	    strcpy(Error_string, "draw_inverted_operators: unknown operator.\n");
	    user_error(Error_string);
	    }
	}
    else if (b->subtype == ATOM_FORM) {
	; /* do nothing */
  	}
    else {  /* non-atomic formula */
	
	for (b1 = b->first_child; b1; b1 = b1->next)
	    draw_inverted_operators(b1, x + b1->x_off, y + b1->y_off);
	}
    
}  /* draw_inverted_operators */

/*************
 *
 *    struct formula_box *find_point(b, x, y, xb, yb)
 *
 *    (x,y) is the point to find.
 *    (xb,yb) is the address of b.
 *
 *************/

struct formula_box *find_point(b, x, y, xb, yb)
struct formula_box *b;
int x;
int y;
int xb;
int yb;
{
    struct formula_box *b1, *b_xy;
    
    if (x < xb || x > xb+b->length || y < yb || y > yb+b->height)
        return(NULL);  /* out of bounds */
    else {
	for (b1 = b->first_child, b_xy = NULL; b1 && !b_xy; b1 = b1->next)
	    b_xy = find_point(b1, x, y, xb+b1->x_off, yb+b1->y_off);
	return(b_xy ? b_xy : b);
     	}
    
    
}  /* find_formula_box */

/*************
 *
 *    void install_up_pointers(f)
 *
 *************/

void install_up_pointers(f)
struct formula *f;
{
    struct formula *f1;
    
    for (f1 = f->first_child; f1; f1 = f1->next) {
	f1->parent = f;
	install_up_pointers(f1);
	}
    
}  /* install_up_pointers */

/*************
 *
 *    void free_formula_box_tree(b)
 *
 *************/

void free_formula_box_tree(b)
struct formula_box *b;
{
    struct formula_box *g1, *g2;
    
    g1 = b->first_child;
    while (g1) {
	g2 = g1->next;
	free_formula_box_tree(g1);
	g1 = g2;
	}
    free_formula_box(b);
    
}  /* free_formula_box_tree */

/*************
 *
 *    void transform(f, trans_proc)
 *
 *************/

void transform(f, trans_proc)
struct formula *f;
struct formula *(*trans_proc)();
{
    struct formula *parent, *prev, *f1;
    struct formula *copy;
    struct formula_ptr_2 *p;
    struct formula_box *sub_box;
    
    parent = f->parent;
    if (parent) {
	prev = NULL;
	for (f1 = parent->first_child; f1 != f; f1 = f1->next)
	    prev = f1;
	}
    
    copy = copy_formula(Crnt_transform->f);
    install_up_pointers(copy);
    
    f = trans_proc(f);
    install_up_pointers(f);
    
    if (parent) { /* if not at the top */
	if (prev)
	    prev->next = f;
	else
	    parent->first_child = f;
	f->parent = parent;
	}
    else {
	Crnt_transform->f = f;
	f->parent = NULL;
	}
    

    if (formula_ident(Crnt_transform->f, copy)) {
	user_error("That transformation wouldn't do anything.\n");
	zap_formula(copy);
	}

    else {
	
	/* free any undone/unedited formulas */
	if(Crnt_transform->right) {
	    free_formulas(Crnt_transform->right);
	    Crnt_transform->right = NULL;
	    }
	else if(Crnt_transform->up) {
	    free_formulas(Crnt_transform->up);
	    Crnt_transform->up = NULL;
	    }
	
	p = get_formula_ptr_2();
	p->f = Crnt_transform->f;
	
	Crnt_transform->right = p;
	p->left = Crnt_transform;
	Crnt_transform->f = copy;
	Crnt_transform = p;
	
	display_formula(Crnt_transform); 
	
	/* now highlight (and select) the transformed subterm */
	
	sub_box = find_sub_box(B, f);
	if (sub_box && sub_box != B) {
	    Sel_area = sub_box;
	    Highlighted = 1;
	    draw_formula_box_inverted(Sel_area, Sel_area->abs_x_loc, Sel_area->abs_y_loc);
	    }
	}
    
}  /* transform */

/*************
 *
 *    struct formula_box *find_sub_box(b, f)
 *
 *************/

struct formula_box *find_sub_box(b, f)
struct formula_box *b;
struct formula *f;
{
    struct formula_box *b1, *b2;
    
    if (b->f == f)
	b2 = b;
    else {
	for (b1 = b->first_child, b2 = NULL; b1 && !b2; b1 = b1->next)
	    b2 = find_sub_box(b1, f);
	}
    return(b2);
}  /* find_sub_box */

/*************
 *
 *    void free_formulas(p)
 *
 *************/

void free_formulas(p)
struct formula_ptr_2 *p;
{
    
    struct formula_ptr_2 *q;
    
    while(p) {
	q = p;
	if(p->right)
	    p = p->right;
	else 
	    p = p->up;
	
	zap_formula(q->f);
	free_formula_ptr_2(q);
	
	}
    
}    /* free_formulas */
