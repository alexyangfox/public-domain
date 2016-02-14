/*
 *  formed.h -- FormEd (Formula Editor) X Windows program.
 *
 */

/* C Includes */

#include <stdio.h>
#include <string.h>

/* X Includes */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/TextSrc.h>
#include <X11/Xaw/List.h>

/* bitmaps */

#define all_lg_width 16
#define all_lg_height 16
static char all_lg_bits[] = {
   0x03, 0xc0, 0x03, 0xc0, 0x06, 0x60, 0x06, 0x60, 0x0c, 0x30, 0x0c, 0x30,
   0xf8, 0x1f, 0xf8, 0x1f, 0x30, 0x0c, 0x30, 0x0c, 0x60, 0x06, 0x60, 0x06,
   0xc0, 0x03, 0xc0, 0x03, 0x80, 0x01, 0x80, 0x01};
#define all_md_width 12
#define all_md_height 12
static char all_md_bits[] = {
   0x03, 0x0c, 0x03, 0x0c, 0x06, 0x06, 0x06, 0x06, 0xfc, 0x03, 0xfc, 0x03,
   0x98, 0x01, 0x98, 0x01, 0xf0, 0x00, 0xf0, 0x00, 0x60, 0x00, 0x60, 0x00};
#define all_sm_width 8
#define all_sm_height 8
static char all_sm_bits[] = {
   0x81, 0x81, 0x42, 0x7e, 0x24, 0x24, 0x18, 0x18};
#define and_lg_width 16
#define and_lg_height 16
static char and_lg_bits[] = {
   0xc0, 0x01, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x40, 0x01, 0xc0, 0x00,
   0xc0, 0x00, 0xe0, 0x01, 0x30, 0x33, 0x18, 0x33, 0x0c, 0x26, 0x0c, 0x1c,
   0x0c, 0x0c, 0x0c, 0x1c, 0xf8, 0x37, 0xf0, 0x33};
#define and_md_width 12
#define and_md_height 12
static char and_md_bits[] = {
   0x70, 0x00, 0x58, 0x00, 0x58, 0x00, 0x30, 0x00, 0x30, 0x00, 0x68, 0x00,
   0xcc, 0x06, 0x86, 0x04, 0x86, 0x03, 0x86, 0x01, 0xc6, 0x02, 0x7c, 0x06};
#define and_sm_width 8
#define and_sm_height 8
static char and_sm_bits[] = {
   0x08, 0x14, 0x14, 0x08, 0x14, 0x52, 0x22, 0x5c};
#define exists_lg_width 14
#define exists_lg_height 18
static char exists_lg_bits[] = {
   0x00, 0x00, 0xfe, 0x1f, 0xfe, 0x1f, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18,
   0x00, 0x18, 0x00, 0x18, 0xf8, 0x1f, 0xf8, 0x1f, 0x00, 0x18, 0x00, 0x18,
   0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0xfe, 0x1f, 0xfe, 0x1f, 0x00, 0x00};
#define exists_md_width 12
#define exists_md_height 14
static char exists_md_bits[] = {
   0x00, 0x00, 0xfe, 0x07, 0xfe, 0x07, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04,
   0xf8, 0x07, 0xf8, 0x07, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0xfe, 0x07,
   0xfe, 0x07, 0x00, 0x00};
#define exists_sm_width 8
#define exists_sm_height 11
static char exists_sm_bits[] = {
   0x00, 0x7e, 0x40, 0x40, 0x40, 0x7c, 0x40, 0x40, 0x40, 0x7e, 0x00};
#define iff_lg_width 16
#define iff_lg_height 16
static char iff_lg_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x0c, 0x30,
   0xfe, 0x7f, 0xff, 0xff, 0xfe, 0x7f, 0x0c, 0x30, 0x08, 0x10, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define iff_md_width 12
#define iff_md_height 12
static char iff_md_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x06, 0x06, 0xff, 0x0f, 0xff, 0x0f,
   0x06, 0x06, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define iff_sm_width 8
#define iff_sm_height 8
static char iff_sm_bits[] = {
   0x00, 0x24, 0x42, 0xff, 0x42, 0x24, 0x00, 0x00};
#define imp_lg_width 16
#define imp_lg_height 16
static char imp_lg_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00, 0x38,
   0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0x00, 0x38, 0x00, 0x18, 0x00, 0x08,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define imp_md_width 12
#define imp_md_height 12
static char imp_md_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x06, 0xff, 0x0f,
   0xff, 0x0f, 0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define imp_sm_width 8
#define imp_sm_height 8
static char imp_sm_bits[] = {
   0x00, 0x20, 0x40, 0xff, 0x40, 0x20, 0x00, 0x00};
#define not_lg_width 16
#define not_lg_height 16
static char not_lg_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define not_md_width 12
#define not_md_height 12
static char not_md_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x0f,
   0xff, 0x0f, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define not_sm_width 8
#define not_sm_height 8
static char not_sm_bits[] = {
   0x00, 0x00, 0x00, 0xff, 0x80, 0x80, 0x00, 0x00};
#define or_lg_width 16
#define or_lg_height 16
static char or_lg_bits[] = {
   0x03, 0xc0, 0x03, 0xc0, 0x06, 0x60, 0x06, 0x60, 0x0c, 0x30, 0x0c, 0x30,
   0x18, 0x18, 0x18, 0x18, 0x30, 0x0c, 0x30, 0x0c, 0x60, 0x06, 0x60, 0x06,
   0xc0, 0x03, 0xc0, 0x03, 0x80, 0x01, 0x80, 0x01};
#define or_md_width 12
#define or_md_height 12
static char or_md_bits[] = {
   0x03, 0x0c, 0x03, 0x0c, 0x06, 0x06, 0x06, 0x06, 0x0c, 0x03, 0x0c, 0x03,
   0x98, 0x01, 0x98, 0x01, 0xf0, 0x00, 0xf0, 0x00, 0x60, 0x00, 0x60, 0x00};
#define or_sm_width 8
#define or_sm_height 8
static char or_sm_bits[] = {
   0x81, 0x81, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18};
#define pattern_width 4
#define pattern_height 4
static char pattern_bits[] = {
   0x05, 0x0a, 0x05, 0x0a};


#define LABELFONT "-*-helvetica-*-r-*-*-*-120-*-*-*-*-*-*"
#define LARGEFONT "-*-helvetica-*-r-*-*-*-240-*-*-*-*-*-*"
#define MEDFONT "-*-helvetica-*-r-*-*-*-140-*-*-*-*-*-*"
#define SMALLFONT "-*-helvetica-medium-r-*-*-*-100-*-*-*-*-*-*"

#define SMALLFONT_DIM 8
#define MEDFONT_DIM 12
#define LARGEFONT_DIM 16

#define SMALL_SPACING 3
#define MED_SPACING 5
#define LARGE_SPACING 7

#define VIEWPORT_HEIGHT 600
#define VIEWPORT_WIDTH  1000

#define CANVAS_HEIGHT 1000
#define CANVAS_WIDTH  1000

#define FORMULA      1
#define OPERATOR     2

#define OR_OP        1  /* subtypes of operator */
#define AND_OP       2
#define NOT_OP       3
#define EXISTS_OP    4
#define ALL_OP       5
#define IFF_OP       6
#define IMP_OP       7

#define TEXT_LENGTH 500

#define PLACE_HOLDER 0
#define LOGIC_MENU 1
#define EDIT_MENU 2

#define NUM_LOGIC_BUTTONS 10
#define NUM_EDIT_BUTTONS 10

#define ADD 1          /* for use in accum_size */
#define IGNORE 0

        /* The rest of this file is communal (global) variables.    */
        /* Names of global variables have first letter capitalized. */

#ifdef IN_MAIN
#define CLASS         /* empty string if included by main program */
#else
#define CLASS extern  /* extern if included by anything else */
#endif

CLASS XtAppContext App_con;
  
CLASS Display *Dpy;        /* variables for window setup */
CLASS Window Win;
CLASS XSetWindowAttributes W_attr;
CLASS GC Gc;
CLASS int Def_screen;
CLASS int Frame_depth;
CLASS Colormap Cmap;
CLASS int Foreground, Background;
CLASS int Display_setup;    /* flag for if display has been initialized */

CLASS int Fore_set, Back_set, Load_file;   /* command line options */
CLASS char User_fore[25], User_back[25];
CLASS char Crnt_file[25];     /* the current filename in use */

CLASS int Have_message;        /* for if have a message widget displayed */
CLASS char Error_string[50];   /* for building error messages */

CLASS XFontStruct *Font_struct;    /* structure holding font information */
CLASS int Font_char_ht;  /* max height for characters in the font */
CLASS int Font_ascent;  /* max baseline to top edge of character */

  /* pixel maps for operators and inversions */
CLASS Pixmap And, Or, Not, All, Exists, Imp, Iff;
CLASS Pixmap And_invert, Or_invert, Not_invert, All_invert, Exists_invert, Imp_invert, Iff_invert;

CLASS Pixmap Pattern;

  /* when font size is selected, following are set */
CLASS int Spacing;
CLASS char *Or_bits, *And_bits, *Not_bits, *Imp_bits, *Iff_bits, *Exists_bits, *All_bits;
CLASS int Or_width, And_width, Not_width, Imp_width, Iff_width, Exists_width, All_width;     
CLASS int Or_height, And_height, Not_height, Imp_height, Iff_height, Exists_height, All_height;     

CLASS struct formula_ptr_2 *Crnt_formula;  /* the top level formula */
CLASS struct formula_ptr_2 *Top_formula;   /* the head of the formula list */
CLASS struct formula_ptr_2 *Crnt_transform; /*the current formula displayed*/

CLASS struct formula_box *B;        /* the top level formula box */
CLASS int Highlighted;              /* is a formula currently highlighted? */
CLASS struct formula_box *Sel_area; /* the currently selected formula box */

CLASS Widget Outline;           /* widgets referenced in callbacks */
CLASS Widget Canvas;  
CLASS Widget Popup, Edit_popup, Font_popup, Message;  /* popups */
CLASS Widget Place_holder;
CLASS Widget Edit_text;
CLASS Widget Help_text;

CLASS char Edit_str[TEXT_LENGTH*2];   /* for conjoining, disjoining of */
                                      /* formulas */

CLASS int Crnt_button_menu;   /* the child(ren) currently displayed in the */
                                  /* button menu */

/* the list of children to manage in button_menu */
CLASS WidgetList Logic_buttons;    
CLASS WidgetList Edit_buttons;

/* space needed for logic & edit buttons */
CLASS int Logic_area_width, Logic_area_height;
CLASS int Edit_area_width, Edit_area_height;

CLASS String Str;   /* string in the edit window */
CLASS String File_str;  /* string in the file window */
CLASS char Help_str[500];  /* string in the help window */

CLASS Widget Help_popup;     /* help widgets */
/* these need to be global so can reset sensitivities */
CLASS Widget Help_info, Edit_help, Logic_help, 
             Formula_control_help, Select_area_help,
             Cancel; 

/**** function prototypes for formed.c, display.c, and callbacks.c. ****/

/* formed.c */

void proc_command_line();
int sprint_formula();
void select_area();
void draw_formula_box_inverted();
void display_formula();
struct formula_box *do_box_geometry();
struct formula_box *arrange_box();
void draw_formula_box();
void draw_inverted_operators();
struct formula_box *find_point();
void install_up_pointers();
void free_formula_box_tree();
void transform();
struct formula_box *find_sub_box();
void free_formulas();

/* display.c */

void setup_display();
int convert_color();
void user_error();
void setup_font();
void kill_message();
void accum_size();

/* callbacks.c */

void edit_menu_callback();
void logic_menu_callback();
void comp_redo_callback();
void comp_undo_callback();
void next_callback();
void previous_callback();
void font_callback();
void quit_callback();
void help_callback();
void help_info_callback();
void return_help_menu();
void set_help_string();
void destroy_popup();
void load_callback();
void set_load_file();
int load_formula_list();
void save_callback();
void set_save_file();
int save_formula_list();
void clausify_callback();
void operate_callback();
void nnf_callback();
void skolemize_callback();
void cnf_callback();
void cnf_simp_callback();
void rms_cnf_callback();
void rms_dnf_callback();
void dnf_callback();
void dnf_simp_callback();
void redo_callback();
void undo_callback();
void expandConj_callback();
void expandDisj_callback();
void edit_callback();
void abbreviate_callback();
void replace_callback();
void edit_transform();
struct formula *str_to_formula();
struct formula *replace_text();
struct formula *make_deleted();
struct formula *join_formulas();
struct formula_ptr_2 *find_end();
void clear_text_callback();
void conjoin_callback();
void conjoin_with_callback();
void disjoin_callback();
void disjoin_with_callback();
void quantify_callback();
void add_quantify_callback();
void negate_callback();
void new_formula_callback();
void insert_formula_callback();
void delete_formula_callback();
void unedit_callback();
void reedit_callback();
void font_menu_callback();
void create_menu_popup();
void create_edit_popup();
