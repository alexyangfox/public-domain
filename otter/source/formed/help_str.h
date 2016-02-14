/*
 *  help_strings.h -- For FormEd X Winwods Program.
 *
 */

String Edit_help_items[] = {
  "Edit",
  "Conjoin",
  "Disjoin",
  "Quantify",
  "Negate",
  "New formula",
  "Delete formula",
  "Re-edit",
  "Unedit",
  NULL
  };

String Logic_help_items[] = {
  "Clausify",
  "Operate",
  "NNF",
  "Skolemize",
  "CNF",
  "CNF simp",
  "DNF",
  "DNF simp",
  "Redo",
  "Undo",
  NULL
  };

String Formula_control_help_items[] = {
  "Edit Menu",
  "Logic Menu",
  "Redo All",
  "Undo All",
  "Next",
  "Previous",
  "Save",
  "Load",
  "Font",
  "Quit",
  NULL
  };

char Help_info_text[] = "\tHow to use Help\n\nTo use the help facility, click on a set of information from the menu panel.  A window will appear containing information on that topic.\n\tThe Edit, Logic, and Formula Control help topics also contain subtopics which may be clicked on for more specific information.\n\tYou must return to the help menu and click on Cancel to exit the help facility.";

char Select_area_help_text[] = "\tSelecting Formula Areas\n\nFor some operations, a part of the displayed formula may be selected on which to operate.  Simply clicking in the formula display area will cause an area, corresponding to the location of the mouse, to be \"highlighted\". To unselect an area, simply click again in the same area.\n\tIf no area is highlighted, the default selected area is the entire formula and subsequent operations will act on the entire formula.";


String Edit_help_text[] = {
  "\t\tEdit\n\nUse:  To replace selected text in the displayed formula.\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  A window in which the text may be edited (Emacs style), containing buttons for replacement of the text, clearing of the text, and cancelling of the command appears.  After replacement, the new formula will be displayed.",
  "\t\tConjoin\n\nUse:  To conjoin text with selected text from the displayed formula.\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  A window in which the text may be edited (Emacs style), containing buttons for conjoining the new text with the selected area,  clearing of the text, and cancelling of the command appears.  After conjoining the two, the new formula will be displayed.",
  "\t\tDisjoin\n\nUse:  To disjoin text with selected text from the displayed formula.\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  A window in which the text may be edited (Emacs style), containing buttons for disjoining the new text with the selected area,  clearing of the text, and cancelling of the command appears.  After disjoining of the two, the new formula will be displayed.",  
  "\t\tQuantify\n\nUse:  To quantify selected text from the displayed formula.\n\nInput format:  [all var_name] or [exists var_name]\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  A window in which the quantifiers may be edited (Emacs style), containing buttons for adding the quantifiers,  clearing of the text, and cancelling of the command appears.  After adding the quantifiers, the new formula will be displayed.", 
 "\t\tNegate\n\nUse:  To negate selected text from the displayed formula.\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  After negation the new formula will be displayed.",
  "\t\tNew formula\n\nUse:  To add a new formula to the current list of formulas.\n\nResults:  A window in which the text may be edited (Emacs style), containing buttons for insertion of the formula, clearing the text, and cancelling the command appears.  The formula is inserted after the current formula and will then be displayed.",
  "\t\tDelete formula\n\nUse:  To remove the currently displayed formula from the current list of formulas or to delete a subformula from a conjunction or a disjunction.\n\nResults:  If the entire formula is selected, it is deleted.  If an immediate subformula of a conjunction or a disjunction is selected, that subformula is deleted form the formula.",
  "\t\tRe-edit\n\nUse:  To redo an edit transformation that has already been done to a formula.\n\nResults:  The re-edited formula is displayed, and it becomes the current formula.",
  "\t\tUnedit\n\nUse:  To undo an edit transformation that has already been done to a formula.\n\nResults:  The un-edited formula is displayed, and it becomes the current formula.",
  NULL
  };

String Logic_help_text[] = {
  "\t\tClausify\n\nUse:  To completely clausify the displayed formula.\n\nSelection requirements:  Operates on entire formula, no selections should be made.\n\nResults:  The clausified formula is displayed.",
  "\t\tOperate\n\nUse:  To expand the implicational and biconditional operators, and move negations inward.\n\nSelection requirements:  An implicational, biconditional, or negation operator must be selected.\n\nResults:  The transformed formula (wrt the operator selected) is displayed.",
  "\t\tNNF\n\nUse:  To transform the selected area to its negation normal form.\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  The formula with its selected area in negation normal form is displayed.",
  "\t\tSkolemize\n\nUse:  To skolemize the displayed formula.\n\nSelection requirements:  Operates on entire formula, no selections should be made.\n\nResults:  The skolemized formula is displayed.",
  "\t\tCNF\n\nUse: To transform the selected area to its conjunctive normal form.\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  The formula with its selected area in conjunctive normal form is displayed.",
  "\t\tCNF simp\n\nUse:  To transform the selected area to its conjunctive normal form with any simplifications that can be made done (i.e. (p | -p) simplifies to True).\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  The formula with its selected area in conjunctive normal form (simplified) is displayed.",
  "\t\tDNF\n\nUse: To transform the selected area to its disjunctive normal form.\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  The formula with its selected area in disjunctive normal form is displayed.",
  "\t\tDNF simp\n\nUse:  To transform the selected area to its disjunctive normal form with any simplifications that can be made done (i.e. (p | -p) simplifies to True).\n\nSelection requirements:  Any piece of the formula may be selected, provided it contains more than just an operator.\n\nResults:  The formula with its selected area in disjunctive normal form (simplified) is displayed.",
  "\t\tRedo\n\nUse:  To redo a logic transformation that has already been done to a formula.\n\nResults:  The re-done formula is displayed, and it becomes the current formula.",
 "\t\tUndo\n\nUse:  To undo a logic transformation that has already been done to a formula.\n\nResults:  The undone formula is displayed, and it becomes the current formula.",
  NULL
  };

String Formula_control_help_text[] = {
  "\t\tEdit Menu\n\nUse:  Brings up a set of buttons for edit-type transformations to the current formula in the left panel.",
  "\t\tLogic Menu\n\nUse: Brings up a set of buttons for logic transformations to the current formula in the left panel.",
  "\t\tRedo All\n\nUse:  To completely redo the edit and logic transformations that have been done to the current formula and display that formula.",
  "\t\tUndo All\n\nUse:  To completely undo the edit and logic transformations that have been done to the current formula, i.e. display the original formula.",
  "\t\tNext\n\nUse:  To display the next formula in the list of formulas.",
  "\t\tPrevious\n\nUse:  To display the previous formula in the list of formulas.",
  "\t\tSave\n\nUse:  To save the current list of formulas in a specified file.  The current state of the formula (after all edit & logic transformations) is the version saved.",
  "\t\tLoad\n\nUse:  To load a list of formulas from a specified file.",
  "\t\tFont\n\nUse:  To change the font being used in the display.",
  "\t\tQuit\n\nUse:  To exit the formula editor.",
  NULL
  };


