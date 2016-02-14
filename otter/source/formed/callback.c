/*
 *  callbacks.c -- Callbacks for the FormEd program.
 *
 */

#include "../header.h"
#include "formed.h"

#include "help_str.h"

Widget replace_button();
Widget conjoin_button();
Widget disjoin_button();
Widget add_quantifiers_button();
Widget insert_button();
Widget create_one_line_text_widget();

/*************************
*  
*     void edit_menu_callback(w, client_data, call_data)
*
*****************************/

void edit_menu_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    int same_display;
    
    same_display = 0;
    
    /* unmanage the current child, or list of children if not the placeholder */
    switch(Crnt_button_menu) {
      case PLACE_HOLDER:
	XtUnmanageChild(Place_holder);
	break;
      case EDIT_MENU:
	user_error("Edit panel already displayed.\n");
	same_display = 1;
	break;
      case LOGIC_MENU:
	XtUnmanageChildren(Logic_buttons, NUM_LOGIC_BUTTONS);
	break;
	}
    
    if (!same_display) {
	XtManageChildren(Edit_buttons, NUM_EDIT_BUTTONS);
	Crnt_button_menu = EDIT_MENU;
	}
    
} /* edit_menu_callback */


/*************************
*  
*     void logic_menu_callback(w, client_data, call_data)
*
*****************************/

void logic_menu_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    int same_display;
    
    same_display = 0;
    
    /* unmanage the current child, or list of children if not the placeholder */
    switch(Crnt_button_menu) {
      case PLACE_HOLDER:
	XtUnmanageChild(Place_holder);
	break;
      case LOGIC_MENU:
	user_error("Logic panel already displayed.\n");
	same_display = 1;
	break;
      case EDIT_MENU:
	XtUnmanageChildren(Edit_buttons, NUM_EDIT_BUTTONS);
	break;
	}
    
    if (!same_display) {
	XtManageChildren(Logic_buttons, NUM_LOGIC_BUTTONS);
	Crnt_button_menu = LOGIC_MENU;
	}
    
} /* logic_menu_callback */

/*************************
*  
*     void comp_redo_callback(w, client_data, call_data)
*
*****************************/

void comp_redo_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    if (Crnt_formula) {
	if (Crnt_formula->right == NULL && Crnt_formula->up == NULL)
	    user_error("Nothing exists to be redone.\n");
	else {
	    Crnt_transform = find_end(Crnt_formula);
	    display_formula(Crnt_transform);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end comp_redo_callback */

/*************************
*  
*     void comp_undo_callback(w, client_data, call_data)
*
*****************************/

void comp_undo_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    if (Crnt_formula) {
	Crnt_transform = Crnt_formula;
	display_formula(Crnt_transform);
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end comp_undo_callback */

/*************************
*
*     void next_callback(w, client_data, call_data)
*
*****************************/

void next_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	/* display next formula, if have one */
	
	if (Crnt_formula->next) {
	    Crnt_formula = Crnt_formula->next;
	    Crnt_transform = find_end(Crnt_formula);
	    display_formula(Crnt_transform);
	    }
	else 
	    user_error("End of formula list.\n");
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* next_callback */

/*************************
*
*     void previous_callback(w, client_data, call_data)
*
*****************************/

void previous_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	
	/* display previous formula, if have one */
	
	if (Crnt_formula->prev) {
	    Crnt_formula = Crnt_formula->prev;
	    Crnt_transform = find_end(Crnt_formula);
	    display_formula(Crnt_transform);
	    }
	else 
	    user_error("Beginning of formula list.\n");
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end previous_callback */

/*************************
*  
*     void font_callback(w, client_data, call_data)
*
*****************************/

void font_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    XtDestroyWidget(Popup);
    
    /* redisplay formula if have one */
    
    setup_font((char *) client_data);
    
    if (Crnt_formula)
	display_formula(Crnt_transform);
    
}   /* end font_callback */


/*************************
*  
*     void quit_callback(w, client_data, call_data)
*
*****************************/

void quit_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    print_mem(stdout);
    exit(0);
    
}   /* end quit_callback */

/*************************
*  
*     void help_callback(w, client_data, call_data)
*
*****************************/

void help_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Widget help_menu_form;
    int n;
    Arg arg[10];
    
    
    create_menu_popup("Help", w);  /* create the popup shell */
    
    n = 0;
    XtSetArg(arg[n], XtNbackground, Background); n++;
    help_menu_form = XtCreateManagedWidget("help_menu_form", formWidgetClass, 
					   Popup, arg, n);
    
    
    /* command buttons */
    
    /* how to use help */
    n = 0;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNlabel, "How to use Help");n++;
    Help_info = XtCreateManagedWidget("help_info",commandWidgetClass, 
				      help_menu_form,arg,n);
    XtAddCallback(Help_info, XtNcallback, help_info_callback, Help_info);
    
    /* Edit buttons */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, Help_info); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Edit Menu Buttons");n++;
    Edit_help = XtCreateManagedWidget("edit_help", commandWidgetClass,
				      help_menu_form, arg, n);
    XtAddCallback(Edit_help, XtNcallback, help_info_callback, Edit_help);
    
    /* Logic buttons */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, Edit_help); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Logic Menu Buttons");n++;
    Logic_help = XtCreateManagedWidget("logic_help", commandWidgetClass,
				       help_menu_form, arg, n);
    XtAddCallback(Logic_help, XtNcallback, help_info_callback, Logic_help);
    
    /* formula control buttons */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, Logic_help); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Formula Control Buttons");n++;
    Formula_control_help = XtCreateManagedWidget("formula_control_help", 
						 commandWidgetClass,
						 help_menu_form, arg, n);
    XtAddCallback(Formula_control_help, XtNcallback, help_info_callback, 
		  Formula_control_help);
    
    /* selecting formula area information */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, Formula_control_help); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Selecting Formula Areas");n++;
    Select_area_help = XtCreateManagedWidget("select_area_help", 
					     commandWidgetClass,
					     help_menu_form, arg, n);
    XtAddCallback(Select_area_help, XtNcallback, help_info_callback, 
		  Select_area_help);
    
    /* cancel */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, Select_area_help); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Cancel"); n++;
    Cancel =  XtCreateManagedWidget("cancel",commandWidgetClass,
				    help_menu_form, arg, n);
    XtAddCallback(Cancel, XtNcallback, destroy_popup, Popup);
    
    XtPopup(Popup, XtGrabNone);
    
}   /* end help_callback */


/*************************
*  
*     void help_info_callback(entry, client_data, call_data)
*
*****************************/

void help_info_callback(entry,client_data, call_data)
Widget entry;
XtPointer client_data;
XtPointer call_data;
{
    Widget selected_help = (Widget) client_data;
    int n;
    Arg arg[10];
    Dimension width, height;
    Position x, y;
    Widget help_label, help_form, help_return, help_list;
    
    /* make the buttons in the main menu insensitive */
    /* until a return to menu has been issued */
    
    XtSetArg(arg[0], XtNsensitive, False); 
    XtSetValues(Help_info, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, False); 
    XtSetValues(Edit_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, False); 
    XtSetValues(Logic_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, False); 
    XtSetValues(Formula_control_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, False); 
    XtSetValues(Select_area_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, False); 
    XtSetValues(Cancel, arg, 1);
    
    /* create the shell */
    n = 0;
    XtSetArg(arg[n], XtNwidth, &width); n++;
    XtSetArg(arg[n], XtNheight, &height); n++;
    XtGetValues(Outline, arg, n);
    XtTranslateCoords(Outline, (Position)((width/2)-200), 
		      (Position)(height/2-150), &x, &y);
    
    n = 0;
    XtSetArg(arg[n], XtNx, x); n++;
    XtSetArg(arg[n], XtNy, y); n++;
    Help_popup = XtCreatePopupShell("Help_popup", transientShellWidgetClass,
				    Outline, arg, n);
    
    /* form widget to hold buttons & text */
    n = 0;
    XtSetArg(arg[n], XtNforeground, Foreground); n++;
    help_form = XtCreateManagedWidget("help_form", formWidgetClass,
				      Help_popup, arg, n);
    
    /* help label */
    n = 0;
    XtSetArg(arg[n], XtNlabel, "Help"); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNbackground, Background); n++;
    XtSetArg(arg[n], XtNwidth, 300); n++;
    XtSetArg(arg[n], XtNborderWidth, 0); n++;
    help_label = XtCreateManagedWidget("help_label", labelWidgetClass, 
				       help_form, arg, n);
    
    /* return to help menu selection */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, help_label); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Return to Help Menu"); n++;
    help_return = XtCreateManagedWidget("help_return", commandWidgetClass,
					help_form, arg, n);
    XtAddCallback(help_return, XtNcallback, return_help_menu, NULL);
    
    /* list of buttons to choose for information on */
    if (selected_help == Help_info)
	strcpy(Help_str, Help_info_text);
    else if (selected_help == Select_area_help)
	strcpy(Help_str, Select_area_help_text);
    else {
	n = 0;
	XtSetArg(arg[n], XtNfromVert, help_return); n++;
	if (selected_help == Edit_help) {
	    XtSetArg(arg[n], XtNdefaultColumns, 3); n++;
	    XtSetArg(arg[n], XtNlist, Edit_help_items); n++;
	    }
	else if (selected_help == Logic_help) {
	    XtSetArg(arg[n], XtNdefaultColumns, 4); n++;
	    XtSetArg(arg[n], XtNlist, Logic_help_items); n++;
	    }
	else if (selected_help == Formula_control_help) {
	    XtSetArg(arg[n], XtNdefaultColumns, 4); n++;
	    XtSetArg(arg[n], XtNlist, Formula_control_help_items); n++;
	    }
	
	help_list = XtCreateManagedWidget("help_list", listWidgetClass, help_form, 
					  arg, n);
	XtAddCallback(help_list, XtNcallback, set_help_string, selected_help);
	
	strcpy(Help_str, "  ");
	
	}
    
    /* text box */
    
    n = 0;
    if ((selected_help == Help_info) || (selected_help == Select_area_help)) {
	XtSetArg(arg[n], XtNfromVert, help_return); n++;
	}
    else {
	XtSetArg(arg[n], XtNfromVert, help_list); n++;
	}
    XtSetArg(arg[n], XtNforeground, Foreground); n++;
    XtSetArg(arg[n], XtNbackground, Background); n++;
    XtSetArg(arg[n], XtNstring, Help_str); n++;
    XtSetArg(arg[n], XtNwrap, XawtextWrapWord); n++;
    XtSetArg(arg[n], XtNdisplayCaret, False); n++;
    XtSetArg(arg[n], XtNwidth, 300); n++;
    XtSetArg(arg[n], XtNheight, 275); n++;
    Help_text = XtCreateManagedWidget("help_text",asciiTextWidgetClass,
				      help_form, arg, n);
    
    XtPopup(Help_popup, XtGrabNone);
    
    
} /* help_info_callback */


/*************************
*  
*     void return_help_menu(entry, client_data, call_data)
*
*****************************/

void return_help_menu(entry,client_data, call_data)
Widget entry;
XtPointer client_data;
XtPointer call_data;
{
    Arg arg[1];
    
    /* reset the sensitivity of the help menu buttons */
    XtSetArg(arg[0], XtNsensitive, True); 
    XtSetValues(Help_info, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, True); 
    XtSetValues(Edit_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, True); 
    XtSetValues(Logic_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, True); 
    XtSetValues(Formula_control_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, True); 
    XtSetValues(Select_area_help, arg, 1);
    
    XtSetArg(arg[0], XtNsensitive, True); 
    XtSetValues(Cancel, arg, 1);
    
    /* destroy the help popup */
    XtDestroyWidget(Help_popup);
    
    
    /* redisplay formula if have one */
    if (Crnt_formula)
	display_formula(Crnt_transform);
    
    
}  /* return_help_menu */

/*************************
*  
*     void set_help_string(entry, client_data, call_data)
*
*****************************/

void set_help_string(entry,client_data, call_data)
Widget entry;
XtPointer client_data;
XtPointer call_data;
{
    
    XawListReturnStruct *item = (XawListReturnStruct *)call_data;
    Widget help_set = (Widget) client_data;    /* which set of help selected */
    
    Arg arg[1];
    
    /* get appropriate string  */
    
    if (help_set == Edit_help)
	strcpy(Help_str, Edit_help_text[item->list_index]);
    else if (help_set == Logic_help)
	strcpy(Help_str, Logic_help_text[item->list_index]);
    else 
	strcpy(Help_str, Formula_control_help_text[item->list_index]);
    
    
    /* reset the string in the widget */
    XtSetArg(arg[0], XtNstring, Help_str);
    XtSetValues(Help_text, arg, 1);
    
}  /* set_help_string */


/*************************
*  
*     void destroy_popup(w, client_data, call_data)
*
*****************************/

void destroy_popup(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    XtDestroyWidget(client_data);
    
    /* redisplay formula if have one */
    if (Crnt_formula)
	display_formula(Crnt_transform);
    
    
}  /* destroy_popup */

/*************************
*  
*     void load_callback(entry, client_data, call_data)
*
*****************************/

void load_callback(entry,client_data, call_data)
Widget entry;
XtPointer client_data;
XtPointer call_data;
{
    Widget load_form, load_label, file_text, load_file, cancel;
    Arg arg[10];
    int n;
    
    create_menu_popup("Load", entry);  /* create the popup shell */
    
    n = 0;
    XtSetArg(arg[n], XtNbackground, Background); n++;
    load_form = XtCreateManagedWidget("load_form", formWidgetClass, 
				      Popup, arg, n);
    
    n = 0;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNbackground, Background); n++;
    XtSetArg(arg[n], XtNborderWidth, 0); n++;
    XtSetArg(arg[n], XtNlabel, "Filename:");n++;
    load_label = XtCreateManagedWidget("load_label", labelWidgetClass,
				       load_form,arg,n);
    
    /* file name */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, load_label); n++;
    XtSetArg(arg[n], XtNforeground, Foreground); n++;
    XtSetArg(arg[n], XtNeditType, XawtextEdit); n++;
    XtSetArg(arg[n], XtNwidth, 176); n++;
    XtSetArg(arg[n], XtNstring, Crnt_file); n++;
    file_text = XtCreateManagedWidget("file_text", asciiTextWidgetClass,
				      load_form, arg, n);
    
    n = 0;
    XtSetArg(arg[n], XtNfromVert, file_text); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Load file");n++;
    load_file = XtCreateManagedWidget("load_file",commandWidgetClass,
				      load_form,arg,n);
    XtAddCallback(load_file, XtNcallback, set_load_file, file_text);
    
    n = 0;
    XtSetArg(arg[n], XtNfromVert, file_text); n++;
    XtSetArg(arg[n], XtNfromHoriz, load_file); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Cancel");n++;
    cancel = XtCreateManagedWidget("cancel",commandWidgetClass,
				   load_form,arg,n);
    XtAddCallback(cancel, XtNcallback, destroy_popup, Popup);
    
    
    XtPopup(Popup, XtGrabNone);
    
}   /* end load_callback */


/*************************
*  
*     void set_load_file(button, client_data, call_data)
*
*****************************/

void set_load_file(button, client_data, call_data)
Widget button;
XtPointer client_data;
XtPointer call_data;
{
    Widget file_name = (Widget) client_data;
    int c;
    Arg arg[1];
    
    XtSetArg(arg[0], XtNstring, &File_str);
    XtGetValues(file_name, arg, 1);
    
    strcpy(Crnt_file, File_str);
    
    /* attempt to save list, if any errors, return to popup */
    /* if no errors, set up pointers and display first formula */
    
    c = load_formula_list(Crnt_file);
    if (c) {
	XtDestroyWidget(Popup);   /* remove load file popup */
	
	/* display first formula */
	Top_formula = Crnt_formula;
	Crnt_transform = Crnt_formula;
	display_formula(Crnt_transform);
	if (Display_setup)
	    user_error("File loaded.\n");
	}
    
}  /* set_load_file */


/*************************
*  
*     int load_formula_list(filename)  
*
*****************************/

int load_formula_list(filename)
char filename[];
{
    int errors;
    struct formula_ptr *p1, *p2;
    struct formula_ptr_2 *q1, *q2;
    struct formula *f;
    FILE *fp;
    
    fp = fopen(Crnt_file, "r");
    if (fp == NULL && Display_setup) {
        strcpy(Error_string, "Error: Cannot open ");
	strcat(Error_string, Crnt_file);
	strcat(Error_string, ".\n");
      	user_error(Error_string);
	return(0);
	}
    else {
	p1 = read_formula_list(fp, &errors);
	
	if (errors)
	    exit(1);
	
	q2 = NULL;
	while (p1) {
	    q1 = get_formula_ptr_2();
	    q1->f = p1->f;
	    install_up_pointers(q1->f);
	    q1->f->parent = NULL;
	    q1->prev = q2;
	    if (q2)
		q2->next = q1;
	    else
		Crnt_formula = q1;
	    q2 = q1;
	    p2 = p1;
	    p1 = p1->next;
	    free_formula_ptr(p2);
	    }
	
	/* print out formulas loaded in to the screen */
	
	q2 = Crnt_formula;
	while (q2 != NULL) {
	    print_formula(stdout, q2->f);
	    fprintf(stdout, ".\n");
	    q2 = q2->next;
	    }
	
	fclose(fp);
	
	return(1);
	
	}
    
}  /* load_formula_list */

/*************************
*  
*     void save_callback(w, client_data, call_data)
*
*****************************/

void save_callback(entry,client_data, call_data)
Widget entry;
XtPointer client_data;
XtPointer call_data;
{
    
    Widget save_form, save_label, file_text, save_file, cancel;
    Arg arg[10];
    int n;
    
    
    if (Crnt_formula) {
	
	create_menu_popup("Save", entry);   /* create the popup shell */
	
	n = 0;
	XtSetArg(arg[n], XtNbackground, Background); n++;
	save_form = XtCreateManagedWidget("save_form", formWidgetClass, 
					  Popup, arg, n);
	
	n = 0;
	XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
	XtSetArg(arg[n], XtNbackground, Background); n++;
	XtSetArg(arg[n], XtNborderWidth, 0); n++;
	XtSetArg(arg[n], XtNlabel, "Filename:");n++;
	save_label = XtCreateManagedWidget("save_label", labelWidgetClass,
					   save_form,arg,n);
	
	/* file name */
	n = 0;
	XtSetArg(arg[n], XtNfromVert, save_label); n++;
	XtSetArg(arg[n], XtNforeground, Foreground); n++;
	XtSetArg(arg[n], XtNeditType, XawtextEdit); n++;
	XtSetArg(arg[n], XtNwidth, 176); n++;
	XtSetArg(arg[n], XtNstring, Crnt_file); n++;
	file_text = XtCreateManagedWidget("file_text", asciiTextWidgetClass,
					  save_form, arg, n);
	
	n = 0;
	XtSetArg(arg[n], XtNfromVert, file_text); n++;
	XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
	XtSetArg(arg[n], XtNlabel, "Save file");n++;
	save_file = XtCreateManagedWidget("save_file",commandWidgetClass,
					  save_form,arg,n);
	XtAddCallback(save_file, XtNcallback, set_save_file, file_text);
	
	n = 0;
	XtSetArg(arg[n], XtNfromVert, file_text); n++;
	XtSetArg(arg[n], XtNfromHoriz, save_file); n++;
	XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
	XtSetArg(arg[n], XtNlabel, "Cancel");n++;
	cancel = XtCreateManagedWidget("cancel",commandWidgetClass,
				       save_form,arg,n);
	XtAddCallback(cancel, XtNcallback, destroy_popup, Popup);
	
	XtPopup(Popup, XtGrabNone);
	}
    
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end save_callback */

/*************************
*  
*     void set_save_file(button, client_data, call_data)
*
*****************************/

void set_save_file(button, client_data, call_data)
Widget button;
XtPointer client_data;
XtPointer call_data;
{
    
    Widget file_name = (Widget) client_data;
    int c;
    Arg arg[1];
    
    XtSetArg(arg[0], XtNstring, &File_str);
    XtGetValues(file_name, arg, 1);
    
    strcpy(Crnt_file, File_str);
    
    /* attempt to save list, if any errors, return to popup */
    
    c = save_formula_list(Crnt_file);

    if (c) {

	XtDestroyWidget(Popup);
	
	/* redisplay formula if have one */
	if (Crnt_formula)
	    display_formula(Crnt_transform);
	
	if (Display_setup)
	    user_error("File saved.\n");
	}
    
}  /* set_save_file */


/*************************
*  
*     int save_formula_list(filename)
*
*****************************/

int save_formula_list(filename)
char filename[];
{
    FILE *fp;
    struct formula_ptr_2 *p, *q;
    
    if ((fp = fopen(filename,"w")) == NULL && Display_setup) {
	strcpy(Error_string, "Error: cannot open ");
	strcat(Error_string, filename);
	strcat(Error_string, ".\n");
	user_error(Error_string);
	return(0);
	}
    else {
	
	/* save the formula list pointed to by Top_formula */
	
	p = Top_formula;
	while(p) {
	    q = find_end(p);   /* find end of current transformation list */
	    print_formula(fp, q->f);
	    fprintf(fp,".\n");
	    p = p->next;
	    }
	
	
	fclose(fp);
	return(1);
	}
    
}  /* save_formula_list */


/*******************
*  LOGIC CALLBACKS
********************/

/*************************
*  
*     void clausify_callback(w, client_data, call_data)
*
*****************************/

void clausify_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	if (Sel_area != B)
	    user_error("Clausify is done on the entire formula,\nno selections necessary.\n");
	else {
	    f = Sel_area->f;
	    transform(f, clausify_formed);
	    user_error("WARNING: further logic transformations may be unsound,\nbecuase universal quantifiers are gone.\n");
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end clausify_callback */


/*************************
*  
*     void operate_callback(w, client_data, call_data)
*
*****************************/

void operate_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    struct formula *f;
    int n;
    Arg arg[10];
    Widget to_conj, to_disj, cancel, iff_form;
    
    
    if (Crnt_formula) {
	if (Sel_area->type != OPERATOR)
	    user_error("Operate requires selection of\nan operator (not, imp, iff).\n");
	else {
	    /* depending on type of operator, perform transformation */
	    switch(Sel_area->subtype) {
	      case NOT_OP:
		f = Sel_area->parent->f;
		transform(f, negation_inward);
		break;
		
	      case IMP_OP:
		f = Sel_area->parent->f;
		transform(f, expand_imp);
		break;
		
	      case IFF_OP:  
		/* create a popup widget, for to conj or disj */
		
		create_menu_popup("Iff_op", w);
		
		n = 0;
		XtSetArg(arg[n], XtNbackground, Background); n++;
		iff_form = XtCreateManagedWidget("iff_form", formWidgetClass, 
						 Popup, arg, n);
		
		/* command buttons */
		n = 0;
		XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
		XtSetArg(arg[n], XtNlabel, "To Conj");n++;
		to_conj = XtCreateManagedWidget("to_conj",commandWidgetClass,
						iff_form,arg,n);
		XtAddCallback(to_conj, XtNcallback, expandConj_callback, NULL);
		
		n = 0;
		XtSetArg(arg[n], XtNfromVert, to_conj); n++;
		XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
		XtSetArg(arg[n], XtNlabel, "To Disj");n++;
		to_disj = XtCreateManagedWidget("to_disj",commandWidgetClass,
						iff_form,arg,n);
		XtAddCallback(to_disj, XtNcallback, expandDisj_callback, NULL);
		
		n = 0;
		XtSetArg(arg[n], XtNfromVert, to_disj); n++;
		XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT));n++;
		XtSetArg(arg[n], XtNlabel, "Cancel");n++;
		cancel = XtCreateManagedWidget("cancel",commandWidgetClass,
					       iff_form,arg,n);
		XtAddCallback(cancel, XtNcallback, destroy_popup, Popup);
		
		XtPopup(Popup, XtGrabNone);
		
		break;
		
	      default:      
		user_error("Operate requires selection of not, imp, or iff operators.");
		break;
		}
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}  /* operate_callback */

/*************************
*  
*     void nnf_callback(w, client_data, call_data)
*
*****************************/

void nnf_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type != FORMULA)
	    user_error("NNF requires a formula (or subformula) as a selection.\n");
	else {
	    f = Sel_area->f;
	    transform(f, nnf);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end nnf_callback */

/*************************
*  
*     void skolemize_callback(w, client_data, call_data)
*
*****************************/

void skolemize_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area != B)
	    user_error("Skolemize is done on the entire formula,\nno selections necessary.\n"); 
	else {
	    f = Sel_area->f;
	    transform(f, nnf_skolemize);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end skolemize_callback */

/*************************
*  
*     void cnf_callback(w, client_data, call_data)
*
*****************************/

void cnf_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type != FORMULA)
	    user_error("CNF requires a formula (or subformula) as a selection.\n");
	else {
	    f = Sel_area->f;
	    transform(f, nnf_cnf);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end cnf_callback */

/*************************
*  
*     void cnf_simp_callback(w, client_data, call_data)
*
*****************************/

void cnf_simp_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type != FORMULA)
	    user_error("CNF requires a formula (or subformula) as a selection.\n");
	else {
	    f = Sel_area->f;
	    Flags[SIMPLIFY_FOL].val = 1;
	    transform(f, rms_cnf);       /* nnf_cnf, or rms_cnf for quants */
	    Flags[SIMPLIFY_FOL].val = 0;
	    }
	}
    else 
	user_error("No formulas are currently loaded.\n");
    
}   /* end cnf_simp_callback */

/*************************
*  
*     void rms_cnf_callback(w, client_data, call_data)
*
*****************************/

void rms_cnf_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type != FORMULA)
	    user_error("RMS CNF requires a formula (or subformula) as a selection.\n");
	else {
	    f = Sel_area->f;
	    transform(f, rms_cnf);
	    }
	}
    else 
	user_error("No formulas are currently loaded.\n");
    
}   /* end rms_cnf_callback */

/*************************
*  
*     void rms_dnf_callback(w, client_data, call_data)
*
*****************************/

void rms_dnf_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type != FORMULA)
	    user_error("RMS DNF requires a formula (or subformula) as a selection.\n");
	else {
	    f = Sel_area->f;
	    transform(f, rms_dnf);
	    }
	}
    else 
	user_error("No formulas are currently loaded.\n");
    
}   /* end rms_dnf_callback */

/*************************
*  
*     void dnf_callback(w, client_data, call_data)
*
*****************************/

void dnf_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type != FORMULA)
	    user_error("DNF requires a formula (or subformula) as a selection.\n");
	else {
	    f = Sel_area->f;
	    transform(f, nnf_dnf);
	    }
	}
    else 
	user_error("No formulas are currently loaded.\n");
    
}   /* end dnf_callback */

/*************************
*  
*     void dnf_simp_callback(w, client_data, call_data)
*
*****************************/

void dnf_simp_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type != FORMULA)
	    user_error("DNF requires a formula (or subformula) as a selection.\n");
	else {
	    f = Sel_area->f;
	    Flags[SIMPLIFY_FOL].val = 1;
	    transform(f, rms_dnf);       /* nnf_dnf, or rms_dnf for quants */
	    Flags[SIMPLIFY_FOL].val = 0;
	    }
	}
    else 
	user_error("No formulas are currently loaded.\n");
}   /* end dnf_simp_callback */

/*************************
*  
*     void redo_callback(w, client_data, call_data)
*
*****************************/

void redo_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    if (Crnt_formula) {
	if (Crnt_transform->right == NULL)
	    user_error("Nothing more to be redone.\n");
	else {
	    Crnt_transform = Crnt_transform->right;
	    display_formula(Crnt_transform);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end redo_callback */

/*************************
*  
*     void undo_callback(w, client_data, call_data)
*
*****************************/

void undo_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	if (Crnt_transform->left == NULL)
	    user_error("Nothing more to be undone.\n");
	else {
	    Crnt_transform = Crnt_transform->left;
	    display_formula(Crnt_transform);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end undo_callback */

/*************************
*  
*     void expandConj_callback(w, client_data, call_data)
*
*****************************/

void expandConj_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    XtDestroyWidget(Popup);   /* remove the popup with iff options */
    
    f = Sel_area->parent->f;
    transform(f, iff_to_conj);
    
}   /* end expandConj_callback */

/*************************
*  
*     void expandDisj_callback(w, client_data, call_data)
*
*****************************/

void expandDisj_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    XtDestroyWidget(Popup);    /* remove the popup with iff options */
    
    f = Sel_area->parent->f;
    transform(f, iff_to_disj);
    
}   /* end expandDisj_callback */

/*************************
*  
*     void edit_callback(w, client_data, call_data)
*
*****************************/

void edit_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    
    strcpy(Edit_str, "\0");
    
    
    if (Crnt_formula) {
	
	if (Sel_area->type == FORMULA) {
	    
	    /* get the selected area in a string */
	    f = Sel_area->f;
	    sprint_formula(Edit_str, f);
	    
	    /* create a popup text widget with a replace button */
	    create_edit_popup(replace_button);
	    }
	else 
	    user_error("Edit requires a formula (or subformula) as a selection.\n");
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end edit_callback */

/*************************
*  
*     void abbreviate_callback(w, client_data, call_data)
*
*****************************/

void abbreviate_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;

    printf("called abbreviate_callback.\n");

}  /* abbreviate_callback */

/*************************
*  
*     void replace_callback(w, client_data, call_data)
*
*****************************/

void replace_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Arg arg[1];
    
    struct formula *f1 = (struct formula *) client_data;
    
    
    XtSetArg(arg[0], XtNstring, &Str);
    XtGetValues(Edit_text, arg, 1);
    
    edit_transform(f1, replace_text);
    
    if (!Have_message)
	XtDestroyWidget(Edit_popup);
    
}   /* end replace_callback */

/*************************
*  
*     void edit_transform(f, trans_proc, flag)
*
*****************************/

void edit_transform(f, trans_proc, flag)
struct formula *f;
struct formula *(*trans_proc)();
int flag;
{
    struct formula *copy, *parent, *prev, *f1, *save_next;
    struct formula_ptr_2 *p, *q;
    struct formula_box *sub_box;
    
    parent = f->parent;
    if (parent) {
	prev = NULL;
	for (f1 = parent->first_child; f1 != f; f1 = f1->next)
	    prev = f1;
	}
    
    copy = copy_formula(Crnt_transform->f);
    install_up_pointers(copy);
    
    save_next = f->next;
    f = trans_proc(f, flag);
    
    /* f == NULL means failure, so do not update */
    
    if (f != NULL) {
	f->next = save_next;
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
	
	/* free any undone/unedited formulas */
	if (Crnt_transform->right) {
	    free_formulas(Crnt_transform->right);
	    Crnt_transform->right = NULL;
	    }
	else if (Crnt_transform->up) {
	    free_formulas(Crnt_transform->up);
	    Crnt_transform->up = NULL;
	    }
	
	p = get_formula_ptr_2();
	p->f = Crnt_transform->f;
	
	Crnt_transform->up = p;
	p->down = Crnt_transform;
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
    
}  /* edit_transform */

/*************
 *
 *    struct formula *str_2_formula(buf)
 *
 *************/

struct formula *str_2_formula(buf)
     char *buf;
{
    struct term *t;
    struct formula *f;
    int i = 0;

    t = str_to_term(buf, &i, 0);
    if (t) {
	skip_white(buf, &i);
	if (buf[i] != '\0' && buf[i] != '.') {
	    fprintf(stderr, "end of formula not at end of string (missing parentheses?)\n");
	    zap_term(t);
	    f = NULL;
	    }
	else {
	    t = term_fixup(t);
	    f = term_to_formula(t);
	    zap_term(t);
	    }
	}
    else
	f = NULL;
    return(f);

}  /* str_2_formula */

/*************************
*  
*     struct formula *replace_text(f, flag)
*
*****************************/

struct formula *replace_text(f, flag)
struct formula *f;
int flag;
{
    f = str_2_formula(Str);  /* convert the string to a formula */
    if (f == NULL)
	user_error("Error in converting string to formula.\nSee standard output for more details.");
    return(f);
    
}   /* replace_text */

/*************************
*  
*     struct formula *make_deleted(f, flag)
*
*****************************/

struct formula *make_deleted(f, flag)
struct formula *f;
int flag;
{
    struct formula *f1, *prev;
    int i;
    
    /* flag is index of subformula to be deleted */
    f1 = f->first_child;
    i = 1;
    prev = NULL;
    while (i != flag) {
	prev = f1;
	f1 = f1->next;
	i++;
	}
    if (prev)
	prev->next = f1->next;
    else
	f->first_child = f1->next;
    
    zap_formula(f1);
    
    if (f->first_child && f->first_child->next == NULL) {
	/* if just one remaining */
	f1 = f;
	f = f->first_child;
	f->next = f1->next;
        free_formula(f1);
	}
    return(f);
    
}   /* make_deleted */

/*************************
*  
*     struct formula *join_formulas(f, flag)
*
*****************************/

struct formula *join_formulas(f, flag)
struct formula *f;
int flag;
{
    f = str_2_formula(Edit_str);   /* convert new string to formula */
    
    if (f == NULL)
	user_error("Error in converting string to formula.\nSee standard output for more details.");
    return(f);
    
}   /* join_formulas */

/*************************
*  
*     struct formula_ptr_2 *find_end(q)
*
*****************************/

struct formula_ptr_2 *find_end(q)
struct formula_ptr_2 *q;
{
    while( q->right || q->up) {
	if (q->right)
	    q = find_end(q->right);
	else
	    q = find_end(q->up);
	}
    return(q);
    
}  /* find_end */

/*************************
*  
*     void clear_text_callback(w, client_data, call_data)
*
*****************************/

void clear_text_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Arg arg[1];
    
    Widget text = (Widget) client_data;
    
    
    XtSetArg(arg[0], XtNstring, "");
    XtSetValues(text, arg, 1);
    
}   /* end clear_text_callback */


/*************************
*  
*     void conjoin_callback(w, client_data, call_data)
*
*****************************/

void conjoin_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	if (Sel_area->type == FORMULA) {
	    
	    strcpy(Edit_str, "\0");
	    
	    /* create a popup text widget with a conjoin button */
	    create_edit_popup(conjoin_button);
	    
	    }
	else
	    user_error("Conjoin requires a formula (or subformula)\nas a selection.\n");
	
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end conjoin_callback */


/*************************
*  
*     void conjoin_with_callback(w, client_data, call_data)
*
*****************************/

void conjoin_with_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    char temp_str[TEXT_LENGTH];
    int i;
    Arg arg[1];
    
    
    /* build a string consisting of "( Sel_area & New_text )" */
    strcpy(Edit_str, "(");
    
    i = sprint_formula(temp_str, Sel_area->f);
    strcat(Edit_str, temp_str);
    strcat(Edit_str, " & ");
    
    XtSetArg(arg[0], XtNstring, &Str);
    XtGetValues(Edit_text, arg, 1);
    strcat(Edit_str, Str);
    strcat(Edit_str, ")");
    
    edit_transform(Sel_area->f, join_formulas, 0);
    
    /* if no message, conjoin was successful, destroy popup */
    /* o.w. leave it for possible correction */
    
    if (!Have_message)
	XtDestroyWidget(Edit_popup);
    
}  /* end conjoin_with_callback */

 
/*************************
*  
*     void disjoin_callback(w, client_data, call_data)
*
*****************************/

void disjoin_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	if (Sel_area->type == FORMULA) {
	    
	    strcpy(Edit_str, "\0");
	    
	    /* create a popup text widget with a disjoin button */
	    create_edit_popup(disjoin_button);
	    
	    }
	else
	    user_error("Disjoin requires a formula (or subformula)\nas a selection.\n");
	
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end disjoin_callback */

/*************************
*  
*     void disjoin_with_callback(w, client_data, call_data)
*
*****************************/

void disjoin_with_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    char temp_str[TEXT_LENGTH];
    int i;
    Arg arg[1];
    
    
    /* build a string consisting of "( Sel_area | New_text )" */
    strcpy(Edit_str, "(");
    
    i = sprint_formula(temp_str, Sel_area->f);
    strcat(Edit_str, temp_str);
    strcat(Edit_str, " | ");
    
    XtSetArg(arg[0], XtNstring, &Str);
    XtGetValues(Edit_text, arg, 1);
    strcat(Edit_str, Str);
    strcat(Edit_str, ")");
    
    edit_transform(Sel_area->f, join_formulas, 0);
    
    /* if no message, disjoin was successful, destroy popup */
    /* o.w. leave it for possible correction */
    
    if (!Have_message)
	XtDestroyWidget(Edit_popup);
    
}  /* end disjoin_with_callback */

/*************************
*  
*     void quantify_callback(w, client_data, call_data)
*
*****************************/

void quantify_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	if (Sel_area->type == FORMULA) {
	    
	    strcpy(Edit_str, "\0");
	    
	    /* create a popup text widget with an add_quantifiers button */
	    create_edit_popup(add_quantifiers_button);
	    
	    }
	else
	    user_error("Quantify requires a formula (or subformula)\nas a selection.\n");
	
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end quantify_callback */


/*************************
*  
*     void add_quantify_callback(w, client_data, call_data)
*
*****************************/

void add_quantify_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    char temp_str[TEXT_LENGTH];
    int i;
    Arg arg[1];
    
    
    /* build a string consisting of "(New_quantifiers Sel_area)" */
    strcpy(Edit_str, "(");
    
    XtSetArg(arg[0], XtNstring, &Str);
    XtGetValues(Edit_text, arg, 1);
    strcat(Edit_str, Str);
    strcat(Edit_str, " ");
    
    i = sprint_formula(temp_str, Sel_area->f);
    strcat(Edit_str, temp_str);
    strcat(Edit_str, ")");
    
    edit_transform(Sel_area->f, join_formulas, 0);
    
    /* if no message, quantification was successful, destroy popup */
    /* o.w. leave it for possible correction */
    
    if (!Have_message)
	XtDestroyWidget(Edit_popup);
    
}  /* end add_quantify_callback */


/*************************
*  
*     void negate_callback(w, client_data, call_data)
*
*****************************/

void negate_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    struct formula *f;
    
    
    if (Crnt_formula) {
	if (Sel_area->type == FORMULA) {
	    
	    f = Sel_area->f;
	    
	    /* negate the formula (or subformula) and place in list */
	    edit_transform(f, negate_formula, 0);
	    
	    }
	else
	    user_error("Negate requires a formula (or subformula)\nas a selection.\n");
	
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end negate_callback */

/*************************
*  
*     void new_formula_callback(w, client_data, call_data)
*
*****************************/

void new_formula_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    strcpy(Edit_str, "\0");
    
    /* create a popup text widget with an insert button */
    create_edit_popup(insert_button);
    
}   /* end new_formula_callback */


/*************************
*  
*     void insert_formula_callback(w, client_data, call_data)
*
*****************************/

void insert_formula_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula *f;
    struct formula_ptr_2 *q;
    int i;
    
    Arg arg[1];
    
    
    XtSetArg(arg[0], XtNstring, &Str);
    XtGetValues(Edit_text, arg, 1);
    
    f = str_2_formula(Str); /* convert string to formula form */
    
    if (f) {
	/* setup in a formula list node */
	install_up_pointers(f);
	f->parent = NULL;
	q = get_formula_ptr_2();
	q->f = f;
	
	/* insert into formula list after current formula  */
	if (Crnt_formula) {
	    q->next = Crnt_formula->next;
	    Crnt_formula->next = q;
	    }
	else
	    Top_formula = q;    
	
	if (q->next)
	    q->next->prev = q;
	q->prev = Crnt_formula;
	
	Crnt_formula = q;
	Crnt_transform = q;
	
	XtDestroyWidget(Edit_popup);
	
	display_formula(Crnt_transform);
	}
    else
	user_error("Error in converting string to formula.\nSee standard output for more details.");
    
}   /* end insert_formula_callback */

/*************************
*  
*     void delete_formula_callback(w, client_data, call_data)
*
*****************************/

void delete_formula_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    struct formula_ptr_2 *p;
    struct formula *f, *f1;
    int i;
    
    
    if (Crnt_formula) {
	
	if (Sel_area == B) {
	    
	    if (Crnt_formula == Crnt_transform) {
		
		p = Crnt_formula;  /* hold for freeing nodes */
		
		/* remove formula from list */
		
		if (Crnt_formula->prev)    
		    Crnt_formula->prev->next = Crnt_formula->next;
		else
		    Top_formula = Crnt_formula->next;
		
		if (Crnt_formula->next) {
		    Crnt_formula->next->prev = Crnt_formula->prev;
		    Crnt_formula = Crnt_formula->next;
		    }
		else
		    Crnt_formula = Crnt_formula->prev;
		
		Crnt_transform = Crnt_formula;
		
		if (Top_formula == NULL) {
		    XClearWindow(Dpy,Win);
		    user_error("Empty formula list.\n");
		    }
		else display_formula(Crnt_transform);
		
		free_formulas(p);   /* kill all the transformations of the */
		/* deleted formula */
		
		}
	    else
		user_error("Delete formula removes from the original formula list,\nnot from the edit/logic transformed formulas.");
	    }
	else if (Sel_area->type == OPERATOR)
	    user_error("Delete formula cannot delete an operator.");
	else if (Sel_area->parent->subtype != AND_FORM  &&
		 Sel_area->parent->subtype != OR_FORM)
	    user_error("Deletion of a subformula must be from a\nconjunction or a disjunction.");
	else {
	    f = Sel_area->parent->f;  /* t is AND or OR */
	    /* find index of subformula to be deleted, and send that in */
	    i = 1;
	    for (f1 = f->first_child; f1 != Sel_area->f; f1 = f1->next)
		i++;
	    edit_transform(f, make_deleted, i);
	    }
	}
    else
	user_error("No formulas are currently loaded.");
    
}   /* end delete_formula_callback */

/*************************
*  
*     void unedit_callback(w, client_data, call_data)
*
*****************************/

void unedit_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	if (Crnt_transform->down == NULL)
	    user_error("Nothing more to be un-edited.\n");
	else {
	    Crnt_transform = Crnt_transform->down;
	    display_formula(Crnt_transform);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end unedit_callback */

/*************************
*  
*     void reedit_callback(w, client_data, call_data)
*
*****************************/

void reedit_callback(w,client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    
    if (Crnt_formula) {
	if (Crnt_transform->up == NULL)
	    user_error("Nothing more to be re-edited.\n");
	else {
	    Crnt_transform = Crnt_transform->up;
	    display_formula(Crnt_transform);
	    }
	}
    else
	user_error("No formulas are currently loaded.\n");
    
}   /* end reedit_callback */

/*************************
*  
*     void font_menu_callback(w, client_data, call_data)
*
*****************************/

void font_menu_callback(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    
    Widget font_form, smallfont, medfont, largefont, cancel;
    int n;
    Arg arg[10];
    
    
    create_menu_popup("fonts", w);    /* create the popup shell */
    
    n = 0;
    XtSetArg(arg[n], XtNbackground, Background); n++;
    font_form = XtCreateManagedWidget("font_form", formWidgetClass, Popup,
				      arg, n);
    
    /* command buttons */
    
    /* small font */
    n = 0;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, SMALLFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Small");n++;
    smallfont = XtCreateManagedWidget("smallfont",commandWidgetClass, 
				      font_form,arg,n);
    XtAddCallback(smallfont, XtNcallback, font_callback, SMALLFONT);
    
    /* medium font */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, smallfont); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, MEDFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Medium");n++;
    medfont = XtCreateManagedWidget("medfont", commandWidgetClass,
				    font_form, arg, n);
    XtAddCallback(medfont, XtNcallback, font_callback, MEDFONT);
    
    /* large font */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, medfont); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LARGEFONT));n++;
    XtSetArg(arg[n], XtNlabel, "Large");n++;
    largefont = XtCreateManagedWidget("largefont", commandWidgetClass,
				      font_form, arg, n);
    XtAddCallback(largefont, XtNcallback, font_callback, LARGEFONT);
    
    /* cancel */
    n = 0;
    XtSetArg(arg[n], XtNfromVert,largefont); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Cancel"); n++;
    cancel =  XtCreateManagedWidget("cancel",commandWidgetClass,
				    font_form, arg, n);
    XtAddCallback(cancel, XtNcallback, destroy_popup, Popup);
    
    XtPopup(Popup, XtGrabNone);
    
    
}  /* font_menu_callback */

/*************************
*  
*     void create_menu_popup(name, parent)
*
*****************************/

void create_menu_popup(name, parent)
char name[];
Widget parent;
{
    
    Dimension width, height;  
    Position x, y;
    int n;
    Arg arg[5];
    
    n = 0;
    XtSetArg(arg[n], XtNwidth, &width); n++;
    XtSetArg(arg[n], XtNheight, &height); n++;
    XtGetValues(parent, arg, n);
    XtTranslateCoords(parent, (Position) (width/2), (Position) (height/2),
		      &x, &y);
    
    n = 0;
    XtSetArg(arg[n], XtNx, x); n++;
    XtSetArg(arg[n], XtNy, y); n++;
    XtSetArg(arg[n], XtNborderWidth, 1);n++;
    Popup = XtCreatePopupShell(name,transientShellWidgetClass, Outline,
			       arg,n);
    
}   /* create_menu_popup */


/*************************
*  
*     void create_edit_popup(create_text_op_button)
*
*****************************/

void create_edit_popup(create_text_op_button)
Widget (*create_text_op_button)();
{
    
    Dimension width, height;
    Position x,y;
    int n;
    Arg arg[15];
    Widget edit_form, text_op_button, clear_text, cancel_edit;
    
    /* create the shell */
    n = 0;
    XtSetArg(arg[n], XtNwidth, &width); n++;
    XtSetArg(arg[n], XtNheight, &height); n++;
    XtGetValues(Outline, arg, n);
    XtTranslateCoords(Outline, (Position)((width/2)-200), 
		      (Position)(height/2), &x, &y);
    
    n = 0; 
    XtSetArg(arg[n], XtNx, x); n++;
    XtSetArg(arg[n], XtNy, y); n++;
    Edit_popup = XtCreatePopupShell("Edit_popup", transientShellWidgetClass,
				    Outline, arg, n);
    
    /* form widget to hold the buttons & text */
    n = 0;
    XtSetArg(arg[n], XtNbackground, Foreground); n++;
    XtSetArg(arg[n], XtNforeground, Foreground); n++;
    edit_form = XtCreateManagedWidget("edit_form",formWidgetClass,
				      Edit_popup, arg, n);
    
    /* text_op_button */
    text_op_button = create_text_op_button(edit_form);
    
    /* clear text button */
    n = 0;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfromVert, text_op_button); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Clear"); n++;
    clear_text = XtCreateManagedWidget("clear_text", commandWidgetClass,
				       edit_form, arg, n);
    
    /* cancel button */
    n = 0;
    XtSetArg(arg[n], XtNfromVert, clear_text); n++;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Cancel"); n++;
    cancel_edit = XtCreateManagedWidget("cancel", commandWidgetClass,
					edit_form, arg, n);
    XtAddCallback(cancel_edit, XtNcallback, destroy_popup, Edit_popup);
    
    /* text widget */
    n = 0;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfromHoriz, text_op_button); n++;
    XtSetArg(arg[n], XtNforeground, Foreground); n++;
    XtSetArg(arg[n], XtNeditType, XawtextEdit); n++;
    XtSetArg(arg[n], XtNscrollVertical, XawtextScrollWhenNeeded); n++;
    XtSetArg(arg[n], XtNautoFill, True); n++;
    XtSetArg(arg[n], XtNheight, 150); n++;
    XtSetArg(arg[n], XtNwidth, 300); n++;
    XtSetArg(arg[n], XtNwrap, XawtextWrapWord); n++;
    XtSetArg(arg[n], XtNstring, Edit_str); n++;
    Edit_text = XtCreateManagedWidget("edit_text", asciiTextWidgetClass,
				      edit_form, arg, n);
    
    XtAddCallback(clear_text, XtNcallback, clear_text_callback,
		  (XtPointer) Edit_text);
    
    XtPopup(Edit_popup, XtGrabNone);
    
}   /* create_edit_popup */


/*************************
*  
*     Widget replace_button(parent)
*
*****************************/

Widget replace_button(parent)
Widget parent;
{
    
    Widget button;
    int n;
    Arg arg[5];
    
    n = 0;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Replace"); n++;
    button = XtCreateManagedWidget("replace", commandWidgetClass, parent,
				   arg, n);
    
    XtAddCallback(button, XtNcallback, replace_callback,
		  (XtPointer)(Sel_area->f));
    
    return(button);
    
}   /* replace_button */

/*************************
*  
*     Widget conjoin_button(parent)
*
*****************************/

Widget conjoin_button(parent)
Widget parent;
{
    
    Widget button;
    int n;
    Arg arg[5];
    
    n = 0;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Conjoin with"); n++;
    button = XtCreateManagedWidget("conjoin_with", commandWidgetClass, parent,
				   arg, n);
    
    XtAddCallback(button, XtNcallback, conjoin_with_callback, NULL);
    
    return(button);
    
}   /* conjoin_button */


/*************************
*  
*     Widget disjoin_button(parent)
*
*****************************/

Widget disjoin_button(parent)
Widget parent;
{
    
    Widget button;
    int n;
    Arg arg[5];
    
    n = 0;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Disjoin with"); n++;
    button = XtCreateManagedWidget("disjoin_with", commandWidgetClass, parent,
				   arg, n);
    
    XtAddCallback(button, XtNcallback, disjoin_with_callback, NULL);
    
    return(button);
    
}   /* disjoin_button */


/*************************
*  
*     Widget add_quantifiers_button(parent)
*
*****************************/

Widget add_quantifiers_button(parent)
Widget parent;
{
    
    Widget button;
    int n;
    Arg arg[5];
    
    n = 0;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "     Add\nQuantifiers"); n++;
    button = XtCreateManagedWidget("add_quantifiers", commandWidgetClass, parent,
				   arg, n);
    
    XtAddCallback(button, XtNcallback, add_quantify_callback, NULL);
    
    return(button);
    
}   /* add_quantifiers_button */


/*************************
*  
*     Widget insert_button(parent)
*
*****************************/

Widget insert_button(parent)
Widget parent;
{
    
    Widget button;
    int n;
    Arg arg[5];
    
    n = 0;
    XtSetArg(arg[n], XtNhighlightThickness, 0); n++;
    XtSetArg(arg[n], XtNfont, XLoadQueryFont(Dpy, LABELFONT)); n++;
    XtSetArg(arg[n], XtNlabel, "Insert"); n++;
    button = XtCreateManagedWidget("insert", commandWidgetClass, parent,
				   arg, n);
    
    XtAddCallback(button, XtNcallback, insert_formula_callback, NULL);
    
    return(button);
    
}   /* insert_button */

