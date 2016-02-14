#ifndef TP_MACE_HEADER_H
#define TP_MACE_HEADER_H

/************ BASIC INCLUDES ************/

#define IN_MAIN   /* so that global vars in ../header.h will not be external */
#include "../source/header.h"  /* Otter header */

#include "Opts.h"
#include "Stats.h"
#include "Avail.h"
#include "Clock.h"
#include "Miscellany.h"
#include "Flatten.h"
#include "Print.h"
#include "Dp.h"
#include "Generate.h"

/******** Types of exit ********/

#define MACE_ABEND_EXIT         11
#define UNSATISFIABLE_EXIT      12
#define MACE_MAX_SECONDS_EXIT   13
#define MACE_MAX_MEM_EXIT       14
#define MAX_MODELS_EXIT         15
#define ALL_MODELS_EXIT         16
#define MACE_SIGINT_EXIT        17
#define MACE_SEGV_EXIT          18
#define MACE_INPUT_ERROR_EXIT   19

#endif  /* ! TP_MACE_HEADER_H */
