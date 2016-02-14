/*
 *  display.c -- These routines are part of the FormEd program.
 *
 */

#include "../header.h"
#include "formed.h"

String Fallback_resources[] = {
    "*Command.height:  20",
    "*Command.width:   85",
    "*to_conj.width: 70",
    "*to_disj.width: 70",
    "*iff_form.cancel.width: 70",
    "*smallfont.width: 75",
    "*medfont.width: 75",
    "*largefont.height: 30",
    "*largefont.width: 75",
    "*font_form.cancel.width: 75",
    "*help.width:  75",
    "*font_menu.width:  75",
    "*save.width:  75",
    "*load.width:  75",
    "*quit.width:  75",
    "*edit.width:  75",
    "*unedit.width: 85",
    "*reedit.width: 85",
    "*new_formula.width:  95",
    "*delete_formula.width: 95",
    "*add_quantifiers.height: 30",
    "*help_menu_form*width: 150",
    "*help_return.width: 128",
    NULL
    };


/*************************
*  
*     void setup_display(argc, argv)
*
*****************************/

void setup_display(argc, argv)
int argc;

char *argv[];
{

  Widget toplevel, title, button_form, view;  /* general form */
                                       /* also includes Outline, Canvas */
  Widget button_menus;   /* can contain Place_holder */   
  Widget displayButtons, edit_menu, logic_menu, comp_redo, comp_undo,
         next, previous, save, load, help, font_menu, quit;

  int i;
  int n;
  Arg arg[25];

  /*** set up initial shell ***/
  toplevel = XtAppInitialize(&App_con,"Toplevel",NULL,0,&argc,argv, 
			  Fallback_resources, NULL, 0);

  /* set display information, for initial font & color info */
  Dpy = XtDisplay(toplevel);
  Def_screen = DefaultScreen(Dpy);
  Frame_depth = DefaultDepth(Dpy, Def_screen);
  Cmap = DefaultColormap(Dpy, Def_screen);

  /* set foreground & background variables */
  if (Frame_depth > 1) {
    if (Fore_set)
      Foreground = convert_color(User_fore);  /* note: convert_color uses */
    else                                    /* values of Dpy,Cmap,Def_screen*/
      Foreground = convert_color("black");
    if (Back_set)
      Background = convert_color(User_back);
    else
      Background = convert_color("white");
  }
  else{
    Foreground = convert_color("black");
    Background = convert_color("white");
  }

  /*** the containing form ***/
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background); n++;
  XtSetArg(arg[n], XtNforeground, Foreground); n++;
  Outline = XtCreateManagedWidget("Outline",formWidgetClass,toplevel,arg,n);

  /*** title ***/
  n = 0;
  XtSetArg(arg[n], XtNhorizDistance, 450); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, MEDFONT)); n++;
  XtSetArg(arg[n], XtNlabel, "FormEd"); n++;
  XtSetArg(arg[n], XtNbackground, Background); n++;
  XtSetArg(arg[n], XtNborderWidth, 0); n++;
  title = XtCreateManagedWidget("title",labelWidgetClass, Outline, arg, n);

  /*** button holder ***/
  n = 0;
  XtSetArg(arg[n], XtNfromVert, title); n++;
  XtSetArg(arg[n], XtNborderWidth, 3); n++;
  XtSetArg(arg[n], XtNbackground, Foreground); n++;
  button_form = XtCreateManagedWidget("button_form",formWidgetClass, 
				      Outline, arg, n);

  /** button menus (logic, edit, place_holder will go here) **/
  n = 0;
  XtSetArg(arg[n], XtNborderWidth, 1); n++;
  button_menus = XtCreateManagedWidget("button_menus",
				       formWidgetClass,button_form, arg, n);
  

  /* create the logic & edit widgets, but do not manage them */
  /* also, put the widgets in appropriate lists, so they can be  */
  /* managed & unmanaged with one call to Xt(un)manageChildren */
  /* use accum_size to determine how big the initial place holder */
  /* needs to be  */

  Logic_buttons = (WidgetList) XtMalloc(NUM_LOGIC_BUTTONS * sizeof(Widget));

  i = 0;
  Logic_area_width = Logic_area_height = 0;

  /* clausify button */
  n = 0;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNlabel, "Clausify");n++;
  Logic_buttons[i] = XtCreateWidget("clausify",commandWidgetClass,button_menus,
				    arg, n); 
  XtAddCallback(Logic_buttons[i], XtNcallback, clausify_callback, NULL);
  accum_size(Logic_buttons[i], ADD, ADD, LOGIC_MENU);
  i++;

  /* operate button */
  n = 0;
  XtSetArg(arg[n], XtNfromVert, Logic_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Operate");n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Logic_buttons[i] = XtCreateWidget("operate",commandWidgetClass,button_menus,
				    arg, n); 
  XtAddCallback(Logic_buttons[i], XtNcallback, operate_callback, NULL);
  accum_size(Logic_buttons[i], IGNORE, ADD, LOGIC_MENU);
  i++;

  /* nnf button */
  n = 0;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "NNF");n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Logic_buttons[i] = XtCreateWidget("nnf",commandWidgetClass,
				     button_menus,arg,n);
  XtAddCallback(Logic_buttons[i], XtNcallback, nnf_callback, NULL);
  accum_size(Logic_buttons[i], ADD, IGNORE, LOGIC_MENU);
  i++;

  /* skolemize button */
  n = 0;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Logic_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Skolemize");n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Logic_buttons[i] = XtCreateWidget("skolemize",commandWidgetClass,
				     button_menus,arg,n); 
  XtAddCallback(Logic_buttons[i], XtNcallback, skolemize_callback, NULL);
  i++;

  /* cnf button */
  n = 0;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "CNF");n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Logic_buttons[i] = XtCreateWidget("cnf",commandWidgetClass,
				     button_menus,arg,n); 
  XtAddCallback(Logic_buttons[i], XtNcallback, cnf_callback, NULL);
  accum_size(Logic_buttons[i], ADD, IGNORE, LOGIC_MENU);
  i++;

  /* cnf_simp button */
  n = 0;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Logic_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "CNF simp");n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Logic_buttons[i] = XtCreateWidget("cnf_simp",commandWidgetClass,
				    button_menus, arg, n); 
  XtAddCallback(Logic_buttons[i], XtNcallback, cnf_simp_callback, NULL);
  i++;

  /* dnf button */
  n = 0;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "DNF");n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Logic_buttons[i] = XtCreateWidget("dnf",commandWidgetClass,
					   button_menus, arg, n);
  XtAddCallback(Logic_buttons[i], XtNcallback, dnf_callback, NULL);
  accum_size(Logic_buttons[i], ADD, IGNORE, LOGIC_MENU);
  i++;

  /* dnf_simp button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Logic_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "DNF simp");n++;
  Logic_buttons[i] = XtCreateWidget("dnf_simp",commandWidgetClass,
				    button_menus, arg, n);
  XtAddCallback(Logic_buttons[i], XtNcallback, dnf_simp_callback, NULL);
  i++;

  /* redo button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Redo");n++;
  Logic_buttons[i] = XtCreateWidget("redo",commandWidgetClass,
				    button_menus, arg, n);
  XtAddCallback(Logic_buttons[i], XtNcallback, redo_callback, NULL);
  accum_size(Logic_buttons[i], ADD, IGNORE, LOGIC_MENU);
  i++;

  /* undo button */
  n = 0;
  XtSetArg(arg[n], XtNfromHoriz, Logic_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Logic_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Undo");n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Logic_buttons[i] = XtCreateWidget("undo",commandWidgetClass,
				    button_menus, arg, n); 
  XtAddCallback(Logic_buttons[i], XtNcallback, undo_callback, NULL);
  i++;

  /** edit buttons **/

  Edit_buttons = (WidgetList) XtMalloc(NUM_EDIT_BUTTONS * sizeof(Widget));

  i = 0;
  Edit_area_width = Edit_area_height = 0;

  /* edit button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Edit");n++;
  Edit_buttons[i] = XtCreateWidget("edit",commandWidgetClass,button_menus,
				    arg, n); 
  XtAddCallback(Edit_buttons[i], XtNcallback, edit_callback, NULL);
  accum_size(Edit_buttons[i], ADD, ADD, EDIT_MENU);
  i++;

  /* abbreviate button */
  n = 0;
  XtSetArg(arg[n], XtNfromVert, Edit_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Abbreviate");n++;
  Edit_buttons[i] = XtCreateWidget("edit",commandWidgetClass,button_menus,
				    arg, n); 
  XtAddCallback(Edit_buttons[i], XtNcallback, abbreviate_callback, NULL);
  accum_size(Edit_buttons[i], IGNORE, ADD, EDIT_MENU);
  i++;


  /* conjoin button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Conjoin");n++;
  Edit_buttons[i] = XtCreateWidget("conjoin",commandWidgetClass,
				     button_menus,arg,n);
  XtAddCallback(Edit_buttons[i], XtNcallback, conjoin_callback, NULL);
  accum_size(Edit_buttons[i], ADD, IGNORE, EDIT_MENU);
  i++;

  /* disjoin button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Edit_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Disjoin");n++;
  Edit_buttons[i] = XtCreateWidget("disjoin",commandWidgetClass,
				     button_menus,arg,n); 
  XtAddCallback(Edit_buttons[i], XtNcallback, disjoin_callback, NULL);
  i++;

  /* quantify button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Quantify");n++;
  Edit_buttons[i] = XtCreateWidget("quantify",commandWidgetClass,
				     button_menus,arg,n); 
  XtAddCallback(Edit_buttons[i], XtNcallback, quantify_callback, NULL);
  accum_size(Edit_buttons[i], ADD, IGNORE, EDIT_MENU);
  i++;

  /* negate button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Edit_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Negate");n++;
  Edit_buttons[i] = XtCreateWidget("negate",commandWidgetClass,
				    button_menus, arg, n); 
  XtAddCallback(Edit_buttons[i], XtNcallback, negate_callback, NULL);
  i++;

  /* new formula button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "New formula");n++;
  Edit_buttons[i] = XtCreateWidget("new_formula",commandWidgetClass,
					   button_menus, arg, n);
  XtAddCallback(Edit_buttons[i], XtNcallback, new_formula_callback, NULL);
  accum_size(Edit_buttons[i], ADD, IGNORE, EDIT_MENU);
  i++;

  /* delete formula button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Edit_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Delete formula");n++;
  Edit_buttons[i] = XtCreateWidget("delete_formula",commandWidgetClass,
				    button_menus, arg, n);
  XtAddCallback(Edit_buttons[i], XtNcallback, delete_formula_callback, NULL);
  i++;

  /* reedit button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-2]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Re-edit");n++;
  Edit_buttons[i] = XtCreateWidget("reedit",commandWidgetClass,
				    button_menus, arg, n);
  XtAddCallback(Edit_buttons[i], XtNcallback, reedit_callback, NULL);
  accum_size(Edit_buttons[i], ADD, IGNORE, EDIT_MENU);
  i++;

  /* unedit button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, Edit_buttons[i-3]); n++;
  XtSetArg(arg[n], XtNfromVert, Edit_buttons[i-1]); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Unedit");n++;
  Edit_buttons[i] = XtCreateWidget("unedit",commandWidgetClass,
				    button_menus, arg, n); 
  XtAddCallback(Edit_buttons[i], XtNcallback, unedit_callback, NULL);
  i++;


  /** Place holder for button menus **/
  n = 0;
  XtSetArg(arg[n], XtNborderWidth, 1); n++;

  if (Logic_area_width > Edit_area_width) {
    XtSetArg(arg[n], XtNwidth, Logic_area_width + 25); n++;
  }
  else{
    XtSetArg(arg[n], XtNwidth, Edit_area_width + 25); n++;
  }

  if (Logic_area_height > Edit_area_height) {
    XtSetArg(arg[n], XtNheight, Logic_area_height + 6); n++;
  }
  else{
    XtSetArg(arg[n], XtNheight, Edit_area_height + 6); n++;
  }

  XtSetArg(arg[n], XtNlabel, "Welcome to FormEd"); n++;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  Place_holder = XtCreateManagedWidget("Place_holder",labelWidgetClass,
				       button_menus, arg, n);
  Crnt_button_menu = PLACE_HOLDER;

  
  /** display buttons **/
  n = 0;
  XtSetArg(arg[n], XtNborderWidth, 1); n++;
  XtSetArg(arg[n], XtNfromHoriz, button_menus); n++;
  displayButtons = XtCreateManagedWidget("displayButtons", formWidgetClass,
                                          button_form, arg, n);

  /* edit menu button */
  n = 0; 
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNshapeStyle,XmuShapeRoundedRectangle); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Edit Menu");n++;
  edit_menu = XtCreateManagedWidget("edit_menu",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(edit_menu, XtNcallback, edit_menu_callback, NULL);

  /* logic menu button */
  n = 0; 
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromVert, edit_menu); n++;
  XtSetArg(arg[n], XtNshapeStyle, XmuShapeRoundedRectangle); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Logic Menu");n++;
  logic_menu = XtCreateManagedWidget("logic_menu",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(logic_menu, XtNcallback, logic_menu_callback, NULL);

  /* complete redo action button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, edit_menu); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Redo All");n++;
  comp_redo = XtCreateManagedWidget("comp_redo",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(comp_redo, XtNcallback, comp_redo_callback, NULL);

  /* comp_undo action button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, edit_menu); n++;
  XtSetArg(arg[n], XtNfromVert, comp_redo); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Undo All");n++;
  comp_undo = XtCreateManagedWidget("comp_undo",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(comp_undo, XtNcallback, comp_undo_callback, NULL);
  
  /* next formula button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, comp_redo); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Next");n++;
  next = XtCreateManagedWidget("next",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(next, XtNcallback, next_callback, NULL);

  /* previous formula button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, comp_redo); n++;
  XtSetArg(arg[n], XtNfromVert, next); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Previous");n++;
  previous = XtCreateManagedWidget("previous",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(previous, XtNcallback, previous_callback, NULL);

  /* save button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, next); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Save");n++;
  save = XtCreateManagedWidget("save",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(save, XtNcallback, save_callback, NULL);

  /* load button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, next); n++;
  XtSetArg(arg[n], XtNfromVert, save); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Load");n++;
  load = XtCreateManagedWidget("load",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(load, XtNcallback, load_callback, NULL);
  

  /* help button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, save); n++;
  XtSetArg(arg[n], XtNborderWidth, 1); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Help");n++;
  help = XtCreateManagedWidget("help",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(help, XtNcallback, help_callback, NULL);


  /* font menu */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, save); n++;
  XtSetArg(arg[n], XtNfromVert, help); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
  XtSetArg(arg[n], XtNlabel, "Font"); n++;
  font_menu=XtCreateManagedWidget("font_menu", commandWidgetClass,
					displayButtons, arg, n);
  XtAddCallback(font_menu, XtNcallback, font_menu_callback,  NULL);



  /* quit button */
  n = 0;
  XtSetArg(arg[n], XtNbackground, Background);n++;
  XtSetArg(arg[n], XtNfromHoriz, help); n++;
  XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
  XtSetArg(arg[n], XtNlabel, "Quit");n++;
  quit = XtCreateManagedWidget("quit",commandWidgetClass,
				     displayButtons,arg,n);
  XtAddCallback(quit, XtNcallback, quit_callback, NULL);
  

  /*** the viewport ***/
  n = 0;
  XtSetArg(arg[n], XtNfromVert, button_form); n++;
  XtSetArg(arg[n], XtNallowHoriz, True); n++;
  XtSetArg(arg[n], XtNallowVert, True); n++;
  XtSetArg(arg[n], XtNheight, VIEWPORT_HEIGHT); n++;
  XtSetArg(arg[n], XtNwidth, VIEWPORT_WIDTH); n++;
  XtSetArg(arg[n], XtNborderWidth, 3); n++;
  view = XtCreateManagedWidget("view",viewportWidgetClass,Outline,arg,n);

  /* the canvas - made a composite widget because a form widget without */
  /*              children at this point will not make scrollbars */

  n =  0;
  XtSetArg(arg[n], XtNforeground, Foreground); n++;
  XtSetArg(arg[n], XtNbackground, Background); n++;
  XtSetArg(arg[n], XtNborderWidth, 0); n++;
  XtSetArg(arg[n], XtNheight, CANVAS_HEIGHT); n++;
  XtSetArg(arg[n], XtNwidth, CANVAS_WIDTH); n++;
  Canvas = XtCreateManagedWidget("canvas",compositeWidgetClass, view, arg, n);

  XtRealizeWidget(toplevel);


  /* get window attributes */
  Win = XtWindow(Canvas);

  /* set the graphics context */
  Gc = XCreateGC(Dpy, Win, (unsigned long) 0, NULL);
  XSetForeground(Dpy, Gc, Foreground);
  XSetBackground(Dpy, Gc, Background);

  /* set up font information - default is currently MEDFONT */
  /* the pixmaps are set up within this according to font */
  setup_font(MEDFONT);

  /* for highlighting */

/*
  Pattern = XCreateBitmapFromData(Dpy, Win, pattern_bits, pattern_width,
				   pattern_height);
*/

  /* needed to save window contents for reexposure */
  W_attr.backing_store = Always;
  W_attr.save_under = True;
  XChangeWindowAttributes(Dpy, Win,(CWBackingStore|CWSaveUnder), &W_attr);

  /* miscellaneous initializations */

  XtAddEventHandler(Canvas, ButtonPressMask, False, select_area, NULL);

  Display_setup = 1;        /* set so know if have an initial display */


}  /* end setup_display */

/*************
 *
 *    setup_operator_pixmaps(font_string)
 *
 *************/

setup_operator_pixmaps(font_string)
char font_string[];
{

    if (strcmp(font_string, SMALLFONT) == 0) {
	
	Spacing = SMALL_SPACING;
	
	Or_bits = or_sm_bits; And_bits = and_sm_bits; Not_bits = not_sm_bits;
	Imp_bits = imp_sm_bits; Iff_bits = iff_sm_bits;
	Exists_bits = exists_sm_bits; All_bits = all_sm_bits;
	
	Or_width = or_sm_width; And_width = and_sm_width; Not_width = not_sm_width;
	Imp_width = imp_sm_width; Iff_width = iff_sm_width;
	Exists_width = exists_sm_width; All_width = all_sm_width;
	
	Or_height = or_sm_height; And_height = and_sm_height; Not_height = not_sm_height;
	Imp_height = imp_sm_height; Iff_height = iff_sm_height;
	Exists_height = exists_sm_height; All_height = all_sm_height;
	}
    
    else if (strcmp(font_string,MEDFONT) == 0) {
	
	Spacing = MED_SPACING;
	
	Or_bits = or_md_bits; And_bits = and_md_bits; Not_bits = not_md_bits;
	Imp_bits = imp_md_bits; Iff_bits = iff_md_bits;
	Exists_bits = exists_md_bits; All_bits = all_md_bits;
	
	Or_width = or_md_width; And_width = and_md_width; Not_width = not_md_width;
	Imp_width = imp_md_width; Iff_width = iff_md_width;
	Exists_width = exists_md_width; All_width = all_md_width;
	
	Or_height = or_md_height; And_height = and_md_height; Not_height = not_md_height;
	Imp_height = imp_md_height; Iff_height = iff_md_height;
	Exists_height = exists_md_height; All_height = all_md_height;
	}
    else {  /* LARGEFONT */
	
	Spacing = LARGE_SPACING;
	
	Or_bits = or_lg_bits; And_bits = and_lg_bits; Not_bits = not_lg_bits;
	Imp_bits = imp_lg_bits; Iff_bits = iff_lg_bits;
	Exists_bits = exists_lg_bits; All_bits = all_lg_bits;
	
	Or_width = or_lg_width; And_width = and_lg_width; Not_width = not_lg_width;
	Imp_width = imp_lg_width; Iff_width = iff_lg_width;
	Exists_width = exists_lg_width; All_width = all_lg_width;
	
	Or_height = or_lg_height; And_height = and_lg_height; Not_height = not_lg_height;
	Imp_height = imp_lg_height; Iff_height = iff_lg_height;
	Exists_height = exists_lg_height; All_height = all_lg_height;
	}
    
    
    Or = XCreatePixmapFromBitmapData(Dpy, Win, Or_bits, Or_width,
				      Or_height, Foreground, 
				      Background, Frame_depth);
    And = XCreatePixmapFromBitmapData(Dpy, Win, And_bits, And_width,
				      And_height, Foreground, 
				      Background, Frame_depth);
    Not = XCreatePixmapFromBitmapData(Dpy, Win, Not_bits, Not_width,
				      Not_height, Foreground, 
				      Background, Frame_depth);
    Imp = XCreatePixmapFromBitmapData(Dpy, Win, Imp_bits, Imp_width,
				      Imp_height, Foreground,
				      Background, Frame_depth);
    Iff = XCreatePixmapFromBitmapData(Dpy, Win, Iff_bits, Iff_width, 
				      Iff_height, Foreground, 
				      Background, Frame_depth);
    Exists = XCreatePixmapFromBitmapData(Dpy, Win, Exists_bits, 
					 Exists_width, Exists_height,
					 Foreground, Background, Frame_depth);
    All = XCreatePixmapFromBitmapData(Dpy, Win, All_bits, All_width,
				      All_height, Foreground, 
				      Background, Frame_depth);
    
    Or_invert = XCreatePixmapFromBitmapData(Dpy, Win, Or_bits, Or_width,
					  Or_height, Background, 
					  Foreground, Frame_depth);
    And_invert = XCreatePixmapFromBitmapData(Dpy, Win, And_bits, And_width,
					   And_height, Background, 
					   Foreground, Frame_depth);
    Not_invert = XCreatePixmapFromBitmapData(Dpy, Win, Not_bits, Not_width,
					   Not_height, Background, 
					   Foreground, Frame_depth);
    Imp_invert = XCreatePixmapFromBitmapData(Dpy, Win, Imp_bits, Imp_width,
					   Imp_height, Background,
					   Foreground, Frame_depth);
    Iff_invert = XCreatePixmapFromBitmapData(Dpy, Win, Iff_bits, Iff_width, 
					   Iff_height, Background, 
					   Foreground, Frame_depth);
    Exists_invert = XCreatePixmapFromBitmapData(Dpy, Win, Exists_bits, 
					      Exists_width, Exists_height,
					      Background, Foreground, Frame_depth);
    All_invert = XCreatePixmapFromBitmapData(Dpy, Win, All_bits, All_width,
					   All_height, Background, 
					   Foreground, Frame_depth);
    
}  /* setup_operator_pixmaps */


/*************
 *
 *    convert_color(colorname)
 *
 *************/

int convert_color(colorname)
char *colorname;
{
  XColor color, ignore;

  if (XAllocNamedColor(Dpy, Cmap, colorname, &color, &ignore))
    return(color.pixel);

  else{
    printf("Warning:  couldn't allocate color %s\n", colorname);
    return(BlackPixel(Dpy, Def_screen));
  }

}  /* convert_color */

/*************
 *
 *    void user_error(message_string)
 *
 *************/

void user_error(message_string)
char message_string[];
{
  int n;
  Arg arg[10];
  
  Dimension width, height;
  Position x, y;
  Widget message_form, message_text, message_label;
  

  kill_message();
  
  /* create a popup widget for the message */
  /* place it in the bottom center of the viewport */

  /* get coordinates */
  n = 0;
  XtSetArg(arg[n], XtNwidth, &width); n++;
  XtSetArg(arg[n], XtNheight, &height); n++;
  XtGetValues(Outline, arg, n);
  XtTranslateCoords(Outline, (Position)(275), (Position)(height-100), 
		    &x, &y);

  /* create the popup */
  n = 0;
  XtSetArg(arg[n], XtNx, x); n++;
  XtSetArg(arg[n], XtNy, y); n++;
  Message = XtCreatePopupShell("Message", transientShellWidgetClass, 
			     Outline, arg, n);

  /* form widget to hold the text & label */
  n = 0; 
  message_form = XtCreateManagedWidget("message_form", formWidgetClass,
				       Message, arg, n);

  /* message label */
  n = 0;
  XtSetArg(arg[n], XtNlabel, "Messages");n++;
  XtSetArg(arg[n], XtNwidth, 400); n++;
  XtSetArg(arg[n], XtNborderWidth, 0); n++;
  message_label = XtCreateManagedWidget("message_label", labelWidgetClass,
					message_form, arg,  n);

  /* text widget */
  n = 0;
  XtSetArg(arg[n], XtNfromVert, message_label); n++;
  XtSetArg(arg[n], XtNforeground, Foreground); n++;
  XtSetArg(arg[n], XtNbackground, Background); n++;
  XtSetArg(arg[n], XtNstring, message_string); n++;
  XtSetArg(arg[n], XtNwidth, 400); n++;
  XtSetArg(arg[n], XtNheight, 40); n++;
  message_text = XtCreateManagedWidget("message_text", asciiTextWidgetClass, 
				       message_form, arg, n);

  Have_message = 1;

  XtPopup(Message, XtGrabNone);

}   /* user_error */ 


/*************
 *
 *    void setup_font(font_string)
 *
 *************/

void setup_font(font_string)
char font_string[];
{
  Font fid;

  /* get font ID */
  fid = XLoadFont(Dpy, font_string);

  /* get font structure */
  Font_struct = XQueryFont(Dpy, fid);

  /* set max height of characters in font */
  Font_char_ht = (Font_struct->max_bounds).ascent 
                      + (Font_struct->max_bounds).descent;
  Font_ascent = (Font_struct -> max_bounds).ascent;

  /* set the font in the graphics context */
  XSetFont(Dpy, Gc, fid);

  /* set up the appropriate operator pixmaps */
  setup_operator_pixmaps(font_string);
  
}   /* setup_font */

/*************
 *
 *    void kill_message()
 *
 *************/

void kill_message()
{
    if (Have_message) {
	XtDestroyWidget(Message);
	Have_message = 0;
	}
}   /* kill message */

/*************************
*  
*     void accum_size(w, add_width, add_height, flag)
*
*****************************/

void accum_size(w, add_width, add_height, flag)
Widget w;
int add_width;
int add_height;
int flag;
{
    
    int n;
    Arg arg[2];
    Dimension width, height;
    
    n =  0;
    XtSetArg(arg[n], XtNwidth, &width); n++;
    XtSetArg(arg[n], XtNheight, &height); n++;
    XtGetValues(w, arg, n);
    
    switch (flag) {
      case LOGIC_MENU:
	if (add_width)
	    Logic_area_width += width;
	if (add_height)
	    Logic_area_height += height;
	break;
      case EDIT_MENU:
	if (add_width)
	    Edit_area_width += width;
	if (add_height)
	    Edit_area_height += height;
	break;
	}
    
}  /* accum_size */




