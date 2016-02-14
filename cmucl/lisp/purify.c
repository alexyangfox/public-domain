/* Purify. */

/* $Header: purify.c,v 1.6 93/03/29 03:39:58 wlott Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>

#include "lisp.h"
#include "os.h"
#include "internals.h"
#include "globals.h"
#include "validate.h"
#include "interrupt.h"
#include "purify.h"
#include "interr.h"

#define gc_abort() lose("GC invariant lost!  File \"%s\", line %d\n", \
			__FILE__, __LINE__)

#if 0
#define gc_assert(ex) do { \
	if (!(ex)) gc_abort(); \
} while (0)
#else
#define gc_assert(ex)
#endif


/* These hold the original end of the read_only and static spaces so we can */
/* tell what are forwarding pointers. */

static lispobj *read_only_end, *static_end;

static lispobj *read_only_free, *static_free;
static lispobj *pscav(lispobj *addr, int nwords, boolean constant);

#define LATERBLOCKSIZE 1020
#define LATERMAXCOUNT 10

static struct later {
    struct later *next;
    union {
        lispobj *ptr;
        int count;
    } u[LATERBLOCKSIZE];
} *later_blocks = NULL;
static int later_count = 0;

#define CEILING(x,y) (((x) + ((y) - 1)) & (~((y) - 1)))
#define NWORDS(x,y) (CEILING((x),(y)) / (y))

#ifdef sparc
#define RAW_ADDR_OFFSET 0
#else
#define RAW_ADDR_OFFSET (6*sizeof(lispobj) - type_FunctionPointer)
#endif

static boolean forwarding_pointer_p(lispobj obj)
{
    lispobj *ptr;

    ptr = (lispobj *)obj;

    return ((static_end <= ptr && ptr <= static_free) ||
            (read_only_end <= ptr && ptr <= read_only_free));
}

static boolean dynamic_pointer_p(lispobj ptr)
{
    return ptr >= (lispobj)dynamic_0_space;
}

static void pscav_later(lispobj *where, int count)
{
    struct later *new;

    if (count > LATERMAXCOUNT) {
        while (count > LATERMAXCOUNT) {
            pscav_later(where, LATERMAXCOUNT);
            count -= LATERMAXCOUNT;
            where += LATERMAXCOUNT;
        }
    }
    else {
        if (later_blocks == NULL || later_count == LATERBLOCKSIZE ||
            (later_count == LATERBLOCKSIZE-1 && count > 1)) {
            new  = (struct later *)malloc(sizeof(struct later));
            new->next = later_blocks;
            if (later_blocks && later_count < LATERBLOCKSIZE)
                later_blocks->u[later_count].ptr = NULL;
            later_blocks = new;
            later_count = 0;
        }

        if (count != 1)
            later_blocks->u[later_count++].count = count;
        later_blocks->u[later_count++].ptr = where;
    }
}

static lispobj ptrans_boxed(lispobj thing, lispobj header, boolean constant)
{
    int nwords;
    lispobj result, *new, *old;

    nwords = 1 + HeaderValue(header);

    /* Allocate it */
    old = (lispobj *)PTR(thing);
    if (constant) {
        new = read_only_free;
        read_only_free += CEILING(nwords, 2);
    }
    else {
        new = static_free;
        static_free += CEILING(nwords, 2);
    }

    /* Copy it. */
    bcopy(old, new, nwords * sizeof(lispobj));

    /* Deposit forwarding pointer. */
    result = (lispobj)new | LowtagOf(thing);
    *old = result;
        
    /* Scavenge it. */
    pscav(new, nwords, constant);

    return result;
}

/* need to look at the layout to see if it is a pure structure class, and
   only then can we transport as constant.  If it is pure, we can
   ALWAYS transport as a constant */

static lispobj ptrans_instance(lispobj thing, lispobj header, boolean constant)
{
    lispobj layout = ((struct instance *)PTR(thing))->slots[0];
    return ptrans_boxed(thing, header,
			(((struct instance *)PTR(layout))->slots[15])
			!= NIL);
}
    
static lispobj ptrans_fdefn(lispobj thing, lispobj header)
{
    int nwords;
    lispobj result, *new, *old, oldfn;
    struct fdefn *fdefn;

    nwords = 1 + HeaderValue(header);

    /* Allocate it */
    old = (lispobj *)PTR(thing);
    new = static_free;
    static_free += CEILING(nwords, 2);

    /* Copy it. */
    bcopy(old, new, nwords * sizeof(lispobj));

    /* Deposit forwarding pointer. */
    result = (lispobj)new | LowtagOf(thing);
    *old = result;

    /* Scavenge the function. */
    fdefn = (struct fdefn *)new;
    oldfn = fdefn->function;
    pscav(&fdefn->function, 1, FALSE);
    if ((char *)oldfn + RAW_ADDR_OFFSET == fdefn->raw_addr)
        fdefn->raw_addr = (char *)fdefn->function + RAW_ADDR_OFFSET;

    return result;
}

static lispobj ptrans_unboxed(lispobj thing, lispobj header)
{
    int nwords;
    lispobj result, *new, *old;

    nwords = 1 + HeaderValue(header);

    /* Allocate it */
    old = (lispobj *)PTR(thing);
    new = read_only_free;
    read_only_free += CEILING(nwords, 2);

    /* Copy it. */
    bcopy(old, new, nwords * sizeof(lispobj));

    /* Deposit forwarding pointer. */
    result = (lispobj)new | LowtagOf(thing);
    *old = result;

    return result;
}

static lispobj ptrans_vector(lispobj thing, int bits, int extra,
			     boolean boxed, boolean constant)
{
    struct vector *vector;
    int nwords;
    lispobj result, *new;

    vector = (struct vector *)PTR(thing);
    nwords = 2 + (CEILING((fixnum_value(vector->length)+extra)*bits,32)>>5);

    if (boxed && !constant) {
        new = static_free;
        static_free += CEILING(nwords, 2);
    }
    else {
        new = read_only_free;
        read_only_free += CEILING(nwords, 2);
    }

    bcopy(vector, new, nwords * sizeof(lispobj));

    result = (lispobj)new | LowtagOf(thing);
    vector->header = result;

    if (boxed)
        pscav(new, nwords, constant);

    return result;
}


static lispobj ptrans_code(lispobj thing)
{
    struct code *code, *new;
    int nwords;
    lispobj func, result;

    code = (struct code *)PTR(thing);
    nwords = HeaderValue(code->header) + fixnum_value(code->code_size);

    new = (struct code *)read_only_free;
    read_only_free += CEILING(nwords, 2);

    bcopy(code, new, nwords * sizeof(lispobj));
    
    result = (lispobj)new | type_OtherPointer;

    /* Stick in a forwarding pointer for the code object. */
    *(lispobj *)code = result;

    /* Put in forwarding pointers for all the functions. */
    for (func = code->entry_points;
         func != NIL;
         func = ((struct function *)PTR(func))->next) {

        gc_assert(LowtagOf(func) == type_FunctionPointer);

        *(lispobj *)PTR(func) = result + (func - thing);
    }

    /* Arrange to scavenge the debug info later. */
    pscav_later(&new->debug_info, 1);

    /* Scavenge the constants. */
    pscav(new->constants, HeaderValue(new->header)-5, TRUE);

    /* Scavenge all the functions. */
    pscav(&new->entry_points, 1, TRUE);
    for (func = new->entry_points;
         func != NIL;
         func = ((struct function *)PTR(func))->next) {
        gc_assert(LowtagOf(func) == type_FunctionPointer);
        gc_assert(!dynamic_pointer_p(func));
        pscav(&((struct function *)PTR(func))->self, 2, TRUE);
        pscav_later(&((struct function *)PTR(func))->name, 3);
    }

    return result;
}

static lispobj ptrans_func(lispobj thing, lispobj header)
{
    int nwords;
    lispobj code, *new, *old, result;
    struct function *function;

    /* THING can either be a function header, a closure function header, */
    /* a closure, or a funcallable-instance.  If it's a closure or a */
    /* funcallable-instance, we do the same as ptrans_boxed. */
    /* Otherwise we have to do something strange, 'cause it is buried inside */
    /* a code object. */

    if (TypeOf(header) == type_FunctionHeader ||
        TypeOf(header) == type_ClosureFunctionHeader) {

	/* We can only end up here if the code object has not been */
        /* scavenged, because if it had been scavenged, forwarding pointers */
        /* would have been left behind for all the entry points. */

        function = (struct function *)PTR(thing);
        code = (PTR(thing)-(HeaderValue(function->header)*sizeof(lispobj))) |
            type_OtherPointer;

        /* This will cause the function's header to be replaced with a */
        /* forwarding pointer. */
        ptrans_code(code);

        /* So we can just return that. */
        return function->header;
    }
    else {
	/* It's some kind of closure-like thing. */
        nwords = 1 + HeaderValue(header);
        old = (lispobj *)PTR(thing);

	/* Allocate the new one. */
	if (TypeOf(header) == type_FuncallableInstanceHeader) {
	    /* FINs *must* not go in read_only space. */
	    new = static_free;
	    static_free += CEILING(nwords, 2);
	}
	else {
	    /* Closures can always go in read-only space, 'caues */
	    /* they never change. */
	    new = read_only_free;
	    read_only_free += CEILING(nwords, 2);
	}

        /* Copy it. */
        bcopy(old, new, nwords * sizeof(lispobj));

        /* Deposit forwarding pointer. */
        result = (lispobj)new | LowtagOf(thing);
        *old = result;

        /* Scavenge it. */
        pscav(new, nwords, FALSE);

        return result;
    }
}

static lispobj ptrans_returnpc(lispobj thing, lispobj header)
{
    lispobj code, new;

    /* Find the corresponding code object. */
    code = thing - HeaderValue(header)*sizeof(lispobj);

    /* Make sure it's been transported. */
    new = *(lispobj *)PTR(code);
    if (!forwarding_pointer_p(new))
        new = ptrans_code(code);

    /* Maintain the offset: */
    return new + (thing - code);
}

#define WORDS_PER_CONS CEILING(sizeof(struct cons) / sizeof(lispobj), 2)

static lispobj ptrans_list(lispobj thing, boolean constant)
{
    struct cons *old, *new, *orig;
    int length;

    if (constant)
        orig = (struct cons *)read_only_free;
    else
        orig = (struct cons *)static_free;
    length = 0;

    do {
        /* Allocate a new cons cell. */
        old = (struct cons *)PTR(thing);
        if (constant) {
            new = (struct cons *)read_only_free;
            read_only_free += WORDS_PER_CONS;
        }
        else {
            new = (struct cons *)static_free;
            static_free += WORDS_PER_CONS;
        }

        /* Copy the cons cell and keep a pointer to the cdr. */
        new->car = old->car;
        thing = new->cdr = old->cdr;

        /* Set up the forwarding pointer. */
        *(lispobj *)old = ((lispobj)new) | type_ListPointer;

        /* And count this cell. */
        length++;
    } while (LowtagOf(thing) == type_ListPointer &&
             dynamic_pointer_p(thing) &&
             !(forwarding_pointer_p(*(lispobj *)PTR(thing))));

    /* Scavenge the list we just copied. */
    pscav((lispobj *)orig, length * WORDS_PER_CONS, constant);

    return ((lispobj)orig) | type_ListPointer;
}

static lispobj ptrans_otherptr(lispobj thing, lispobj header, boolean constant)
{
    switch (TypeOf(header)) {
      case type_Bignum:
      case type_SingleFloat:
      case type_DoubleFloat:
      case type_Sap:
        return ptrans_unboxed(thing, header);

      case type_Ratio:
      case type_Complex:
      case type_SimpleArray:
      case type_ComplexString:
      case type_ComplexVector:
      case type_ComplexArray:
        return ptrans_boxed(thing, header, constant);

      case type_ValueCellHeader:
      case type_WeakPointer:
        return ptrans_boxed(thing, header, FALSE);

      case type_SymbolHeader:
        return ptrans_boxed(thing, header, FALSE);

      case type_SimpleString:
        return ptrans_vector(thing, 8, 1, FALSE, constant);

      case type_SimpleBitVector:
        return ptrans_vector(thing, 1, 0, FALSE, constant);

      case type_SimpleVector:
        return ptrans_vector(thing, 32, 0, TRUE, constant);

      case type_SimpleArrayUnsignedByte2:
        return ptrans_vector(thing, 2, 0, FALSE, constant);

      case type_SimpleArrayUnsignedByte4:
        return ptrans_vector(thing, 4, 0, FALSE, constant);

      case type_SimpleArrayUnsignedByte8:
        return ptrans_vector(thing, 8, 0, FALSE, constant);

      case type_SimpleArrayUnsignedByte16:
        return ptrans_vector(thing, 16, 0, FALSE, constant);

      case type_SimpleArrayUnsignedByte32:
        return ptrans_vector(thing, 32, 0, FALSE, constant);

      case type_SimpleArraySingleFloat:
        return ptrans_vector(thing, 32, 0, FALSE, constant);

      case type_SimpleArrayDoubleFloat:
        return ptrans_vector(thing, 64, 0, FALSE, constant);

      case type_CodeHeader:
        return ptrans_code(thing);

      case type_ReturnPcHeader:
        return ptrans_returnpc(thing, header);

      case type_Fdefn:
	return ptrans_fdefn(thing, header);

      default:
        /* Should only come across other pointers to the above stuff. */
        gc_abort();
	return NIL;
    }
}

static int pscav_fdefn(struct fdefn *fdefn)
{
    boolean fix_func;

    fix_func = ((char *)(fdefn->function+RAW_ADDR_OFFSET) == fdefn->raw_addr);
    pscav(&fdefn->name, 1, TRUE);
    pscav(&fdefn->function, 1, FALSE);
    if (fix_func)
        fdefn->raw_addr = (char *)(fdefn->function + RAW_ADDR_OFFSET);
    return sizeof(struct fdefn) / sizeof(lispobj);
}

#ifdef i386
static int pscav_closure_header(closure)
     struct closure *closure;
{
    lispobj fun = closure->function - RAW_ADDR_OFFSET;
    int nwords = HeaderValue(closure->header);

    pscav(&fun, 1, TRUE);
    pscav(closure->info, nwords-1, TRUE);
    return nwords+1;
}
#endif

static lispobj *pscav(lispobj *addr, int nwords, boolean constant)
{
    lispobj thing, *thingp, header;
    int count;
    struct vector *vector;

    while (nwords > 0) {
        thing = *addr;
        if (Pointerp(thing)) {
            /* It's a pointer.  Is it something we might have to move? */
            if (dynamic_pointer_p(thing)) {
                /* Maybe.  Have we already moved it? */
                thingp = (lispobj *)PTR(thing);
                header = *thingp;
                if (Pointerp(header) && forwarding_pointer_p(header))
                    /* Yep, so just copy the forwarding pointer. */
                    thing = header;
                else {
                    /* Nope, copy the object. */
                    switch (LowtagOf(thing)) {
                      case type_FunctionPointer:
                        thing = ptrans_func(thing, header);
                        break;
                    
                      case type_ListPointer:
                        thing = ptrans_list(thing, constant);
                        break;
                    
                      case type_InstancePointer:
                        thing = ptrans_instance(thing, header, constant);
                        break;
                    
                      case type_OtherPointer:
                        thing = ptrans_otherptr(thing, header, constant);
                        break;

                      default:
                        /* It was a pointer, but not one of them? */
                        gc_abort();
                    }
                }
                *addr = thing;
            }
            count = 1;
        }
        else if (thing & 3) {
            /* It's an other immediate.  Maybe the header for an unboxed */
            /* object. */
            switch (TypeOf(thing)) {
              case type_Bignum:
              case type_SingleFloat:
              case type_DoubleFloat:
              case type_Sap:
                /* It's an unboxed simple object. */
                count = HeaderValue(thing)+1;
                break;

              case type_SimpleVector:
                if (HeaderValue(thing) == subtype_VectorValidHashing)
                    *addr = (subtype_VectorMustRehash<<type_Bits) |
                        type_SimpleVector;
                count = 1;
                break;

              case type_SimpleString:
                vector = (struct vector *)addr;
                count = CEILING(NWORDS(fixnum_value(vector->length)+1,4)+2,2);
                break;

              case type_SimpleBitVector:
                vector = (struct vector *)addr;
                count = CEILING(NWORDS(fixnum_value(vector->length),32)+2,2);
                break;

              case type_SimpleArrayUnsignedByte2:
                vector = (struct vector *)addr;
                count = CEILING(NWORDS(fixnum_value(vector->length),16)+2,2);
                break;

              case type_SimpleArrayUnsignedByte4:
                vector = (struct vector *)addr;
                count = CEILING(NWORDS(fixnum_value(vector->length),8)+2,2);
                break;

              case type_SimpleArrayUnsignedByte8:
                vector = (struct vector *)addr;
                count = CEILING(NWORDS(fixnum_value(vector->length),4)+2,2);
                break;

              case type_SimpleArrayUnsignedByte16:
                vector = (struct vector *)addr;
                count = CEILING(NWORDS(fixnum_value(vector->length),2)+2,2);
                break;

              case type_SimpleArrayUnsignedByte32:
                vector = (struct vector *)addr;
                count = CEILING(fixnum_value(vector->length)+2,2);
                break;

              case type_SimpleArraySingleFloat:
                vector = (struct vector *)addr;
                count = CEILING(fixnum_value(vector->length)+2,2);
                break;

              case type_SimpleArrayDoubleFloat:
                vector = (struct vector *)addr;
                count = fixnum_value(vector->length)*2+2;
                break;

              case type_CodeHeader:
                gc_abort(); /* No code headers in static space */
                break;

              case type_FunctionHeader:
              case type_ClosureFunctionHeader:
              case type_ReturnPcHeader:
                /* We should never hit any of these, 'cause they occure */
                /* buried in the middle of code objects. */
                gc_abort();

#ifdef i386
	      case type_ClosureHeader:
	      case type_FuncallableInstanceHeader:
	      case type_ByteCodeFunction:
	      case type_ByteCodeClosure:
	      case type_DylanFunctionHeader:
		count = pscav_closure_header((struct closure *)addr);
		break;
#endif

              case type_WeakPointer:
                /* Weak pointers get preserved during purify, 'cause I don't */
                /* feel like figuring out how to break them. */
                pscav(addr+1, 2, constant);
                count = 4;
                break;

	      case type_Fdefn:
		/* We have to handle fdefn objects specially, so we can fix */
		/* up the raw function address. */
		count = pscav_fdefn((struct fdefn *)addr);
		break;

              default:
                count = 1;
                break;
            }
        }
        else {
            /* It's a fixnum. */
            count = 1;
        }
            
        addr += count;
        nwords -= count;
    }

    return addr;
}

int purify(lispobj static_roots, lispobj read_only_roots)
{
    lispobj *clean;
    int count, i;
    struct later *laters, *next;

#ifdef PRINTNOISE
    printf("[Doing purification:");
    fflush(stdout);
#endif

    if (fixnum_value(SymbolValue(FREE_INTERRUPT_CONTEXT_INDEX)) != 0) {
        printf(" Ack! Can't purify interrupt contexts. ");
        fflush(stdout);
        return 0;
    }

    read_only_end = read_only_free =
        (lispobj *)SymbolValue(READ_ONLY_SPACE_FREE_POINTER);
    static_end = static_free =
        (lispobj *)SymbolValue(STATIC_SPACE_FREE_POINTER);

#ifdef PRINTNOISE
    printf(" roots");
    fflush(stdout);
#endif
    pscav(&static_roots, 1, FALSE);
    pscav(&read_only_roots, 1, TRUE);

#ifdef PRINTNOISE
    printf(" handlers");
    fflush(stdout);
#endif
    pscav((lispobj *) interrupt_handlers,
          sizeof(interrupt_handlers) / sizeof(lispobj),
          FALSE);

#ifdef PRINTNOISE
    printf(" stack");
    fflush(stdout);
#endif
    pscav(control_stack, current_control_stack_pointer - control_stack, FALSE);

#ifdef PRINTNOISE
    printf(" bindings");
    fflush(stdout);
#endif
#ifndef ibmrt
    pscav(binding_stack, current_binding_stack_pointer - binding_stack, FALSE);
#else
    pscav(binding_stack, (lispobj *)SymbolValue(BINDING_STACK_POINTER) - binding_stack, FALSE);
#endif

#ifdef PRINTNOISE
    printf(" static");
    fflush(stdout);
#endif
    clean = static_space;
    do {
        while (clean != static_free)
            clean = pscav(clean, static_free - clean, FALSE);
        laters = later_blocks;
        count = later_count;
        later_blocks = NULL;
        later_count = 0;
        while (laters != NULL) {
            for (i = 0; i < count; i++) {
                if (laters->u[i].count == 0)
                    ;
                else if (laters->u[i].count <= LATERMAXCOUNT) {
                    pscav(laters->u[i+1].ptr, laters->u[i].count, TRUE);
                    i++;
                }
                else
                    pscav(laters->u[i].ptr, 1, TRUE);
            }
            next = laters->next;
            free(laters);
            laters = next;
            count = LATERBLOCKSIZE;
        }
    } while (clean != static_free || later_blocks != NULL);


#ifdef PRINTNOISE
    printf(" cleanup");
    fflush(stdout);
#endif
    os_zero((os_vm_address_t) current_dynamic_space,
            (os_vm_size_t) DYNAMIC_SPACE_SIZE);

    /* Zero stack. */
    os_zero((os_vm_address_t) current_control_stack_pointer,
            (os_vm_size_t) (CONTROL_STACK_SIZE -
                            ((current_control_stack_pointer - control_stack) *
                             sizeof(lispobj))));

#ifndef ibmrt
    current_dynamic_space_free_pointer = current_dynamic_space;
#else
    SetSymbolValue(ALLOCATION_POINTER, (lispobj)current_dynamic_space);
#endif
    SetSymbolValue(READ_ONLY_SPACE_FREE_POINTER, (lispobj)read_only_free);
    SetSymbolValue(STATIC_SPACE_FREE_POINTER, (lispobj)static_free);

#ifdef PRINTNOISE
    printf(" Done.]\n");
    fflush(stdout);
#endif

    return 0;
}
