#define ICI_CORE
#include "exec.h"
#include "op.h"
#include "catch.h"
#include "ptr.h"
#include "func.h"
#include "str.h"
#include "buf.h"
#include "pc.h"
#include "int.h"
#include "struct.h"
#include "set.h"
#include "parse.h"
#include "float.h"
#include "re.h"
#include "src.h"
#include "null.h"
#include "forall.h"
#include "primes.h"
#ifndef NOTRACE
#include "trace.h"
#endif
#ifndef NOSIGNALS
# include <signal.h>
#endif

/*
 * List of all active execution structures.
 */
ici_exec_t      *ici_execs;

/*
 * The global pointer to the current execution context and cached pointers
 * to the three stacks (execution, operand and variable (scope)). These are
 * set every time we switch ICI threads.
 */
ici_exec_t      *ici_exec;

/*
 * A cached copy of ici_exec->x_count for the current thread.
 */
int             ici_exec_count;

/*
 * The arrays that form the current execution, operand and variable (scope)
 * stacks. These are actually swapped-in copies of the stacks that are
 * referenced from the current execution context. They get copied into
 * these fixed-address locations when we switch into a particular context
 * to save a level of indirection on top-of-stack references (it can make
 * up to 20% difference in CPU time). While we are executing in a particular
 * context, the "real" arrays have their a_base pointer zapped to NULL so
 * those copies of the base, limit and top pointers. When we switch away
 * from a particular context, we copies these stacks back to real array
 * structs (see thread.c).
 */
ici_array_t     ici_xs;
ici_array_t     ici_os;
ici_array_t     ici_vs;

ici_int_t       *ici_zero;
ici_int_t       *ici_one;

/*
 * Set this to non-zero to cause an "aborted" failure even when the ICI
 * program is stuck in an infinite loop.  For use in embeded systems etc
 * to provide protection against badly behaved user programs.  How it gets
 * set is not addressed here (by an interupt perhaps).  Remember to clear
 * it before re-running any ICI code.
 */
volatile int    ici_aborted;

#ifndef NOSIGNALS
# ifdef SUNOS5
int sigisempty(sigset_t *s) {
  char *sptr, *eptr;
  for (eptr = (sptr = (char *) s) + sizeof(sigset_t); sptr < eptr; sptr++)
    if (*sptr) return 0;
  return 1;
}
# else
#  define sigisempty(s) (*(s))
# endif
#endif

/*
 * Mark this and referenced unmarked objects, return memory costs.
 * See comments on t_mark() in object.h.
 */
static unsigned long
mark_exec(ici_obj_t *o)
{
    ici_exec_t          *x;

    o->o_flags |= O_MARK;
    x = execof(o);
    return sizeof(ici_exec_t)
       + (x->x_xs != NULL ? ici_mark(x->x_xs) : 0)
       + (x->x_os != NULL ? ici_mark(x->x_os) : 0)
       + (x->x_vs != NULL ? ici_mark(x->x_vs) : 0)
       + ici_mark(x->x_src)
       + (x->x_pc_closet != NULL ? ici_mark(x->x_pc_closet) : 0)
       + (x->x_os_temp_cache != NULL ? ici_mark(x->x_os_temp_cache) : 0)
       + (x->x_waitfor != NULL ? ici_mark(x->x_waitfor) : 0)
       + (x->x_result != NULL ? ici_mark(x->x_result) : 0);
}

static void
free_exec(ici_obj_t *o)
{
    ici_exec_t          *x;
    ici_exec_t          **xp;

    for (xp = &ici_execs; (x = *xp) != NULL; xp = &x->x_next)
    {
        if (x == execof(o))
        {
            *xp = x->x_next;
            break;
        }
    }
    assert(x != NULL);
#ifdef ICI_USE_WIN32_THREADS
    if (x->x_thread_handle != NULL)
        CloseHandle(x->x_thread_handle);
#endif
#ifdef ICI_USE_POSIX_THREADS
    if ((void *)x->x_thread_handle != NULL)
        pthread_join(x->x_thread_handle, NULL);
    (void)sem_destroy(&x->x_semaphore);
#endif
    ici_tfree(o, ici_exec_t);
}

/*
 * Return the object at key k of the obejct o, or NULL on error.
 * See the comment on t_fetch in object.h.
 */
static ici_obj_t *
fetch_exec(ici_obj_t *o, ici_obj_t *k)
{
    ici_exec_t          *x;

    x = execof(o);
    if (k == SSO(result))
    {
        switch (x->x_state)
        {
        case XS_ACTIVE:
            return objof(&o_null);

        case XS_RETURNED:
            return x->x_result;

        case XS_FAILED:
            if (x->x_result == NULL)
                ici_error = "failed";
            else
            {
                if (ici_chkbuf(stringof(x->x_result)->s_nchars))
                    return NULL;
                strcpy(buf, stringof(x->x_result)->s_chars);
                ici_error = buf;
            }
            return NULL;

        default:
            assert(0);
        }
    }
    else if (k == SSO(status))
    {
        switch (x->x_state)
        {
        case XS_ACTIVE:     return SSO(active);
        case XS_RETURNED:   return SSO(finished);
        case XS_FAILED:     return SSO(failed);
        default:            assert(0);
        }
    }
    return objof(&o_null);
}

/*
 * Create and return a pointer to a new ICI execution context.
 * On return, all stacks (execution, operand and variable) are empty.
 * Returns NULL on failure, in which case error has been set.
 * The new exec struct is linked onto the global list of all exec
 * structs (ici_execs).
 */
ici_exec_t *
ici_new_exec(void)
{
    ici_exec_t          *x;
    static ici_src_t    default_src = {OBJ(TC_SRC), 0, NULL};

    if ((x = ici_talloc(ici_exec_t)) ==  NULL)
        return NULL;
    memset(x, 0, sizeof *x);
    ICI_OBJ_SET_TFNZ(x, TC_EXEC, 0, 1, 0);
    ici_rego(x);
    x->x_src = &default_src;
    if ((x->x_xs = ici_array_new(80)) == NULL)
        goto fail;
    ici_decref(x->x_xs);
    if ((x->x_os = ici_array_new(80)) == NULL)
        goto fail;
    ici_decref(x->x_os);
    if ((x->x_vs = ici_array_new(80)) == NULL)
        goto fail;
    ici_decref(x->x_vs);
    if ((x->x_pc_closet = ici_array_new(80)) == NULL)
        goto fail;
    ici_decref(x->x_pc_closet);
    if ((x->x_os_temp_cache = ici_array_new(80)) == NULL)
        goto fail;
    ici_decref(x->x_os_temp_cache);
#ifdef ICI_USE_WIN32_THREADS
    if ((x->x_semaphore = CreateSemaphore(NULL, 0, 10000, NULL)) == NULL)
    {
        ici_get_last_win32_error();
        goto fail;
    }
#endif
#ifdef ICI_USE_POSIX_THREADS
    if (sem_init(&x->x_semaphore, 0, 0) == -1)
    {
        ici_get_last_errno("sem_init", NULL);
        goto fail;
    }
#endif
    x->x_state = XS_ACTIVE;
    x->x_count = 20;
    x->x_n_engine_recurse = 0;
    x->x_next = ici_execs;
    ici_execs = x;
    return x;

fail:
    return NULL;
}

/*
 * The main execution loop avoids continuous checks for available space
 * on its three stacks (operand, execution, and scope) by knowing that
 * no ordinary operation increase the stack depth by more than 3 levels
 * at a time. Knowing this, every N times round the loop it checks that
 * there is room for N * 3 objects.
 */
int
ici_engine_stack_check(void)
{
    ici_array_t         *pcs;
    int                 depth;

    if (ici_stk_push_chk(&ici_xs, 60))
        return 1;
    if (ici_stk_push_chk(&ici_os, 60))
        return 1;
    if (ici_stk_push_chk(&ici_vs, 60))
        return 1;
    pcs = ici_exec->x_pc_closet;
    depth = (ici_xs.a_top - ici_xs.a_base) + 60;
    if ((depth -= (pcs->a_top - pcs->a_base)) > 0)
    {
        if (ici_stk_push_chk(pcs, depth))
            return 1;
        while (pcs->a_top < pcs->a_limit)
        {
            if ((*pcs->a_top = objof(new_pc())) == NULL)
                return 1;
            ++pcs->a_top;
        }
    }
    return 0;
}

void
get_pc(ici_array_t *code, ici_obj_t **xs)
{
    ici_pc_t            *pc;

    pc = pcof(*xs = ici_exec->x_pc_closet->a_base[xs - ici_xs.a_base]);
    pc->pc_code = code;
    pc->pc_next = code->a_base;
}

/*
 * Execute 'code' (any object, normally an ici_array_t of code or a
 * ici_parse_t).  The execution procedes on top of the current stacks
 * (execution, operand and variable).  This call to evaluate will return when
 * the execution stack again returns to the level it was when entered.  It
 * then returns the object left on the operand stack, or &o_null if there
 * wasn't one.  The returned object is ici_incref()ed.  Returns NULL on error,
 * usual conventions (i.e.  'ici_error' points to the error message).
 *
 * n_operands is the number of objects on the operand stack that are arguments
 * to this call.  They will all be poped off before ici_evaluate returns.
 *
 * The execution loop knows when the execution stack returns to its origional
 * level because it puts a ici_catch_t object on it.  This object also records
 * the levels of the other two stacks that match.
 *
 * This is the main execution loop.  All of the nasty optimisations are
 * concentrated here.  It used to be clean, elegant and 20 lines long.  Now it
 * goes faster.
 *
 * Originally each type had an execution method, which was a function pointer
 * like all the other type specific methods.  This was unrolled into a switch
 * within the main loop for speed and variable optimisation.  Then many of the
 * op_type operations got unrolled in the loop in the same way.  Gotos got
 * added to short-circuit some of the steps where possible.  Then they got
 * removed, apart from the fail exits.  Then one got added again.  And again.
 *
 * Note that binop.h is included half way down this function.
 */
ici_obj_t *
ici_evaluate(ici_obj_t *code, int n_operands)
{
    register ici_obj_t  *o;
    ici_obj_t           *pc;
    int                 flags;
    ici_catch_t         frame;
    ici_src_t           *src;
#define FETCH(s, k) \
                        isstring(objof(k)) \
                            && stringof(k)->s_struct == structof(s) \
                            && stringof(k)->s_vsver == ici_vsver \
                        ? stringof(k)->s_slot->sl_value \
                        : ici_fetch(s, k)

    if (++ici_exec->x_n_engine_recurse > 50)
    {
        ici_error = "excessive recursive invocations of the main interpreter";
        goto badfail;
    }

    if (ici_engine_stack_check())
        goto badfail;
    /*
     * This is pretty scary.  An object on the C stack.  But it should be OK
     * because it's only on the execution stack and there should be no way for
     * it to escape into the world at large, so no-one should be able to have
     * a reference to it after we return.  It will get poped off before we do.
     * It's not registered with the garbage collector, so after that, it's
     * just gone.  We do this to save allocation/collection of an object on
     * every call from C to ICI.  This object *will* be seen by the mark phase
     * of the garbage collection, which may occur in a thread other than this
     * one.  This is likely to cause a good memory integrity checking system
     * to complain.
     */
    ICI_OBJ_SET_TFNZ(&frame, TC_CATCH, CF_EVAL_BASE, 0, 0);
    frame.c_catcher = NULL;
    frame.c_odepth = (ici_os.a_top - ici_os.a_base) - n_operands;
    frame.c_vdepth = ici_vs.a_top - ici_vs.a_base;
    *ici_xs.a_top++ = objof(&frame);

    if (isarray(code))
        get_pc(arrayof(code), ici_xs.a_top);
    else
        *ici_xs.a_top = code;
    ++ici_xs.a_top;

    /*
     * The execution loop.
     */
    for (;;)
    {
        if (--ici_exec_count == 0)
        {
            if (ici_aborted)
            {
                ici_error = "aborted";
                goto fail;
            }
            /*
             * Ensure that there is enough room on all stacks for at
             * least 20 more worst case operations.  See also f_call().
             */
            if (ici_engine_stack_check())
                goto fail;
            ici_exec_count = 20;
            if (++ici_exec->x_yield_count > 10)
            {
                ici_yield();
                ici_exec->x_yield_count = 0;
            }
        }

        /*
         * Places which would be inclined to continue in this loop, that
         * know that they have not increased any stack depths, can just
         * goto this label to avoid the check above.
         */
    stable_stacks_continue:
        /*
         * In principle our execution model is pretty simple. We execute
         * the thing on the top of the execution stack. When that is a pc
         * we push the thing the pc points to (with post increment) onto
         * the execution stack and continue.
         *
         * So technically the following test for a pc on top of the
         * execution stack should just be part of the main switch (as it
         * once was). But 90% of the time we execute: pc then other;
         * pc then other etc... Doing this test here means that normally
         * we run these two operations into one another without going
         * round the main loop. And then, enough of the things we do are
         * operators to make a short-circuit of the switch worth while.
         *
         * Code arrays never contain pcs, so the thing picked up from
         * indirecting the pc is not a pc, so there is no general case
         * in the switch.
         */
#ifndef NOSIGNALS
        if (sigisempty(&ici_signals_pending))
            ici_signals_invoke_handlers();
#endif
        assert(ici_os.a_top >= ici_os.a_base);
        if (ispc(pc = ici_xs.a_top[-1]))
        {
    continue_with_same_pc:
            o = *pcof(pc)->pc_next++;
            if (isop(o))
                goto an_op;
        }
        else
        {
            o = pc;
            --ici_xs.a_top;
        }

        /*
         * Formally, the thing being executed should be on top of the
         * execution stack when we do this switch. But the value is
         * known to be pointed to by o, and most things pop it off.
         * We get a net gain if we assume it is pre-popped, because we
         * can avoid pushing things that are comming out of code arrays
         * on at all. Some of the cases below must push it on to restore
         * the formal model. The code just above here assumes this, but
         * has to explicitly pop the stack in the non-pc case.
         */
        switch (o->o_tcode)
        {
        case TC_SRC:
            ici_exec->x_src = srcof(o);
            if (ici_debug_active)
	    {
		*ici_xs.a_top++ = o; /* Restore formal state. */
                ici_debug->idbg_src(srcof(o));
		--ici_xs.a_top;
		continue;
	    }
            goto stable_stacks_continue;

        case TC_PARSE:
            *ici_xs.a_top++ = o; /* Restore formal state. */
            if (parse_exec())
                goto fail;
            continue;

        case TC_STRING:
            /*
             * Executing a string is the operation of variable lookup.
             * Look up the value of the string on the execution stack
             * in the current scope and push the value onto the operand
             * stack.
             *
             * First check for lookup lookaside.
             */
            if
            (
                stringof(o)->s_struct == structof(ici_vs.a_top[-1])
                &&
                stringof(o)->s_vsver == ici_vsver
            )
            {
                /*
                 * We know directly where the value is because we have
                 * looked up this name since the last change to the scope
                 * or structures etc.
                 */
                assert(ici_fetch_super(ici_vs.a_top[-1], o, ici_os.a_top, NULL) == 1);
                assert(*ici_os.a_top == stringof(o)->s_slot->sl_value);
                *ici_os.a_top++ = stringof(o)->s_slot->sl_value;
            }
            else
            {
                ici_obj_t   *f;

                /*
                 * This is an in-line version of fetch_struct because
                 * (a) we know that the top of the variable stack is
                 * always a struct, and (b) we want to detect when the
                 * value is not found so we can do auto-loading.
                 */
                switch
                (
                    ici_fetch_super
                    (
                        ici_vs.a_top[-1],
                        o,
                        ici_os.a_top,
                        structof(ici_vs.a_top[-1])
                    )
                )
                {
                case -1:
                    goto fail;

                case 0:
                    /*
                     * We failed to find that name on first lookup.
                     * Try to load a library of that name and repeat
                     * the lookup before deciding it is undefined.
                     */
                    if ((f = ici_fetch(ici_vs.a_top[-1], SSO(load))) == objof(&o_null))
                    {
                        if (ici_chkbuf(stringof(o)->s_nchars + 20))
                            goto fail;
                        sprintf(buf, "\"%s\" undefined", stringof(o)->s_chars);
                        ici_error = buf;
                        goto fail;
                    }
                    *ici_xs.a_top++ = o; /* Temp restore formal state. */
                    src = ici_exec->x_src;
                    ici_incref(src);
                    if (ici_func(f, "o", o))
                    {
                        ici_decref(src);
                        goto fail;
                    }
                    ici_exec->x_src = src;
                    ici_decref(src);
                    --ici_xs.a_top;
                    switch
                    (
                        ici_fetch_super
                        (
                            ici_vs.a_top[-1],
                            o,
                            ici_os.a_top,
                            structof(ici_vs.a_top[-1])
                        )
                    )
                    {
                    case -1:
                        goto fail;

                    case 0:
                        if (ici_chkbuf(stringof(o)->s_nchars + 20))
                            goto fail;
                        sprintf(buf, "load() failed to define \"%s\"",
                            stringof(o)->s_chars);
                        ici_error = buf;
                        goto fail;
                    }
                }
                ++ici_os.a_top;
            }
            continue;

        case TC_CATCH:
            /*
             * This can either be an error catcher which is being poped
             * off (having done its job, but it never got used) or it
             * can be the guard catch (frame marker) indicating it is time
             * to return from ici_evaluate().
             *
             * First note the top of the operand stack, if there is anything
             * on it, it becomes the return value, else we return &o_null.
             * The caller knows if there is really a value to return.
             */
            *ici_xs.a_top++ = o; /* Restore formal state. */
            if (o->o_flags & CF_EVAL_BASE)
            {
                /*
                 * This is the base of a call to ici_evaluate().  It is now
                 * time to return.
                 */
                if (catchof(o)->c_odepth < ici_os.a_top - ici_os.a_base)
                    o = ici_os.a_top[-1];
                else
                    o = objof(&o_null);
                ici_incref(o);
                ici_unwind();
                --ici_exec->x_n_engine_recurse;
                return o;
            }
            if (o->o_flags & CF_CRIT_SECT)
            {
                --ici_exec->x_critsect;
                /*
                 * Force a check for a yield (see top of loop). If we
                 * don't do this, there is a chance a loop that spends
                 * much of its time in critsects could sync with the
                 * stack check countdown and never yield.
                 */
                ici_exec_count = 1;
            }
            ici_unwind();
            goto stable_stacks_continue;

        case TC_FORALL:
            *ici_xs.a_top++ = o; /* Restore formal state. */
            if (exec_forall())
                goto fail;
            continue;

        default:
            *ici_os.a_top++ = o;
            continue;

        case TC_OP:
        an_op:
            switch (opof(o)->op_ecode)
            {
            case OP_OTHER:
                *ici_xs.a_top++ = o; /* Restore to formal state. */
                if ((*opof(o)->op_func)())
                    goto fail;
                continue;

            case OP_SUPER_CALL:
                flags = OPC_COLON_CALL | OPC_COLON_CARET;
                goto do_colon;

            case OP_METHOD_CALL:
                flags = OPC_COLON_CALL;
                goto do_colon;

            case OP_COLON:
                /*
                 * aggr key => method (os) (normal case)
                 */
                {
                    ici_method_t        *m;
                    ici_obj_t           *o1;
                    ici_obj_t           *t;
                    char                n1[30];

                    flags = opof(o)->op_code;
                do_colon:
                    o1 = o;
                    t = ici_os.a_top[-2];
                    if (flags & OPC_COLON_CARET)
                    {
                        if ((o = FETCH(ici_vs.a_top[-1], SS(class))) == NULL)
                            goto fail;
                        if (!hassuper(o))
                        {
                            sprintf(buf, "\"class\" evaluated to %s in :^ operation",
                                ici_objname(n1, o));
                            ici_error = buf;
                            goto fail;
                        }
                        if ((t = objof(objwsupof(o)->o_super)) == NULL)
                        {
                            ici_error = "class has no super class in :^ operation";
                            goto fail;
                        }
                    }
                    if (ici_typeof(t)->t_fetch_method != NULL)
                    {
                        if ((o = (*ici_typeof(t)->t_fetch_method)(t, ici_os.a_top[-1])) == NULL)
                            goto fail;
                    }
                    else
                    {
                        if ((o = FETCH(t, ici_os.a_top[-1])) == NULL)
                            goto fail;
                    }
                    if ((flags & OPC_COLON_CALL) == 0)
                    {
                        if ((m = ici_method_new(ici_os.a_top[-2], o)) == NULL)
                            goto fail;
                        --ici_os.a_top;
                        ici_os.a_top[-1] = objof(m);
                        ici_decref(m);
                        goto stable_stacks_continue;
                    }
                    /*
                     * This is a direct call, don't form the method object.
                     */
                    *ici_xs.a_top++ = o1;  /* Restore xs to formal state. */
                    o1 = ici_os.a_top[-2]; /* The subject object. */
                    --ici_os.a_top;
                    ici_os.a_top[-1] = o;  /* The callable object. */
                    o = o1;
                    ici_incref(o);
                    goto do_call;
                }

            case OP_CALL:
                *ici_xs.a_top++ = o;        /* Restore to formal state. */
                o = NULL;                   /* No subject object. */
            do_call:
                if (ici_typeof(ici_os.a_top[-1])->t_call == NULL)
                {
                    char    n1[30];

                    sprintf(buf, "attempt to call %s", ici_objname(n1, ici_os.a_top[-1]));
                    ici_error = buf;
                    if (o != NULL)
                        ici_decref(o);
                    goto fail;
                }
                if (ici_debug_active)
                    ici_debug->idbg_fncall(ici_os.a_top[-1], ARGS(), NARGS());
                if ((*ici_typeof(ici_os.a_top[-1])->t_call)(ici_os.a_top[-1], o))
                {
                    if (o != NULL)
                        ici_decref(o);
                    goto fail;
                }
                if (o != NULL)
                    ici_decref(o);
                continue;

            case OP_QUOTE:
                /*
                 * pc           => pc+1 (xs)
                 *              => *pc (os)
                 */
                o = ici_xs.a_top[-1];
                *ici_os.a_top++ = *pcof(o)->pc_next++;
                continue;

            case OP_AT:
                /*
                 * obj => obj (os)
                 */
                ici_os.a_top[-1] = ici_atom(ici_os.a_top[-1], 0);
                goto stable_stacks_continue;

            case OP_NAMELVALUE:
                /*
                 * pc (xs)      => pc+1 (xs)
                 *              => struct *pc (os)
                 * (Ie. the next thing in the code array is a name, put its
                 * lvalue on the operand stack.)
                 */
                *ici_os.a_top++ = ici_vs.a_top[-1];
                *ici_os.a_top++ = *pcof(ici_xs.a_top[-1])->pc_next++;
                continue;

            case OP_DOT:
                /*
                 * aggr key => value (os)
                 */
                if ((o = FETCH(ici_os.a_top[-2], ici_os.a_top[-1])) == NULL)
                    goto fail;
                --ici_os.a_top;
                ici_os.a_top[-1] = o;
                goto stable_stacks_continue;

            case OP_DOTKEEP:
                /*
                 * aggr key => aggr key value (os)
                 */
                if ((o = FETCH(ici_os.a_top[-2], ici_os.a_top[-1])) == NULL)
                    goto fail;
                *ici_os.a_top++ = o;
                continue;

            case OP_DOTRKEEP:
                /*
                 * aggr key => value aggr key value (os)
                 *
                 * Used in postfix ++/-- for value.
                 */
                if ((o = FETCH(ici_os.a_top[-2], ici_os.a_top[-1])) == NULL)
                    goto fail;
                ici_os.a_top += 2;
                ici_os.a_top[-1] = o;
                ici_os.a_top[-2] = ici_os.a_top[-3];
                ici_os.a_top[-3] = ici_os.a_top[-4];
                ici_os.a_top[-4] = o;
                continue;

            case OP_ASSIGNLOCALVAR:
                /*
                 * name value => - (os, for effect)
                 *                => value (os, for value)
                 *                => aggr key (os, for lvalue)
                 */
                if (assign_base(ici_vs.a_top[-1], ici_os.a_top[-2],ici_os.a_top[-1]))
                    goto fail;
                switch (opof(o)->op_code)
                {
                case FOR_EFFECT:
                    ici_os.a_top -= 2;
                    break;

                case FOR_VALUE:
                    ici_os.a_top[-2] = ici_os.a_top[-1];
                    --ici_os.a_top;
                    break;

                case FOR_LVALUE:
                    ici_os.a_top[-1] = ici_os.a_top[-2];
                    ici_os.a_top[-2] = ici_vs.a_top[-1];
                    break;
                }
                continue;

            case OP_ASSIGN_TO_NAME:
                /*
                 * value on os, next item in code is name.
                 */
                ici_os.a_top += 2;
                ici_os.a_top[-1] = ici_os.a_top[-3];
                ici_os.a_top[-2] = *pcof(ici_xs.a_top[-1])->pc_next++;
                ici_os.a_top[-3] = ici_vs.a_top[-1];
                /* Fall through. */
            case OP_ASSIGN:
                /*
                 * aggr key value => - (os, for effect)
                 *                => value (os, for value)
                 *                => aggr key (os, for lvalue)
                 */
                if
                (
                    stringof(ici_os.a_top[-2])->s_struct == structof(ici_os.a_top[-3])
                    &&
                    stringof(ici_os.a_top[-2])->s_vsver == ici_vsver
                    &&
                    isstring(ici_os.a_top[-2])
                    &&
                    (objof(ici_os.a_top[-2])->o_flags & S_LOOKASIDE_IS_ATOM) == 0
                )
                {
                    stringof(ici_os.a_top[-2])->s_slot->sl_value = ici_os.a_top[-1];
                    goto assign_finish;
                }
                if (ici_assign(ici_os.a_top[-3], ici_os.a_top[-2], ici_os.a_top[-1]))
                    goto fail;
                goto assign_finish;

            case OP_ASSIGNLOCAL:
                /*
                 * aggr key value => - (os, for effect)
                 *                => value (os, for value)
                 *                => aggr key (os, for lvalue)
                 */
                if (hassuper(ici_os.a_top[-3]))
                {
                    if (assign_base(ici_os.a_top[-3], ici_os.a_top[-2], ici_os.a_top[-1]))
                        goto fail;
                }
                else
                {
                    if (ici_assign(ici_os.a_top[-3], ici_os.a_top[-2], ici_os.a_top[-1]))
                        goto fail;
                }
            assign_finish:
                switch (opof(o)->op_code)
                {
                case FOR_EFFECT:
                    ici_os.a_top -= 3;
                    break;

                case FOR_VALUE:
                    ici_os.a_top[-3] = ici_os.a_top[-1];
                    ici_os.a_top -= 2;
                    break;

                case FOR_LVALUE:
                    --ici_os.a_top;
                    break;
                }
                continue;

            case OP_SWAP:
                /*
                 * aggr1 key1 aggr2 key2        =>
                 *                              => value1
                 *                              => aggr1 key1
                 */
                {
                    register ici_obj_t  *v1;
                    register ici_obj_t  *v2;

                    if ((v1 = ici_fetch(ici_os.a_top[-4], ici_os.a_top[-3])) == NULL)
                        goto fail;
                    ici_incref(v1);
                    if ((v2 = ici_fetch(ici_os.a_top[-2], ici_os.a_top[-1])) == NULL)
                    {
                        ici_decref(v1);
                        goto fail;
                    }
                    ici_incref(v2);
                    if (ici_assign(ici_os.a_top[-2], ici_os.a_top[-1], v1))
                    {
                        ici_decref(v1);
                        ici_decref(v2);
                        goto fail;
                    }
                    if (ici_assign(ici_os.a_top[-4], ici_os.a_top[-3], v2))
                    {
                        ici_decref(v1);
                        ici_decref(v2);
                        goto fail;
                    }
                    switch (opof(o)->op_code)
                    {
                    case FOR_EFFECT:
                        ici_os.a_top -= 4;
                        break;

                    case FOR_VALUE:
                        ici_os.a_top[-4] = v2;
                        ici_os.a_top -= 3;
                        break;

                    case FOR_LVALUE:
                        ici_os.a_top -= 2;
                        break;
                    }
                    ici_decref(v1);
                    ici_decref(v2);
                }
                continue;

            case OP_IF:
                /*
                 * bool => - (os)
                 * 
                 */
                if (isfalse(ici_os.a_top[-1]))
                {
                    --ici_os.a_top;
                    ++pcof(ici_xs.a_top[-1])->pc_next;
                    goto stable_stacks_continue;
                }
                o = *pcof(ici_xs.a_top[-1])->pc_next++;
                get_pc(arrayof(o), ici_xs.a_top);
                --ici_os.a_top;
                ++ici_xs.a_top;
                continue;

            case OP_IFELSE:
                /*
                 * bool => -
                 */
                if (isfalse(ici_os.a_top[-1]))
                {
                    ++pcof(ici_xs.a_top[-1])->pc_next;
                    o = *pcof(ici_xs.a_top[-1])->pc_next++;
                }
                else
                {
                    o = *pcof(ici_xs.a_top[-1])->pc_next++;
                    ++pcof(ici_xs.a_top[-1])->pc_next;
                }
                get_pc(arrayof(o), ici_xs.a_top);
                --ici_os.a_top;
                ++ici_xs.a_top;
                goto stable_stacks_continue;

            case OP_IFBREAK:
                /*
                 * bool => - (os)
                 *      => [o_break] (xs)
                 */
                if (isfalse(ici_os.a_top[-1]))
                {
                    --ici_os.a_top;
                    continue;
                }
                --ici_os.a_top;
                goto do_break;

             case OP_IFNOTBREAK:
                /*
                 * bool => - (os)
                 *      => [o_break] (xs)
                 */
                if (!isfalse(ici_os.a_top[-1]))
                {
                    --ici_os.a_top;
                    continue;
                }
                --ici_os.a_top;
                /* Falling through. */
            case OP_BREAK:
            do_break:
                /*
                 * Pop the execution stack until a looper or switcher
                 * is found and disgard it (and the thing under it,
                 * which is the code array that is the body of the loop).
                 * Oh, and forall as well.
                 */
                {
                    register ici_obj_t  **s;

                    for (s = ici_xs.a_top; s > ici_xs.a_base + 1; --s)
                    {
                        if (iscatch(s[-1]))
                        {
                            if (s[-1]->o_flags & CF_CRIT_SECT)
                            {
                                --ici_exec->x_critsect;
                                ici_exec_count = 1;
                            }
                            else if (s[-1]->o_flags & CF_EVAL_BASE)
                                break;
                        }
                        else if
                        (
                            s[-1] == objof(&o_looper)
                            ||
                            s[-1] == objof(&o_switcher)
                        )
                        {
                            ici_xs.a_top = s - 2;
                            goto stable_stacks_continue;
                        }
                        else if (isforall(s[-1]))
                        {
                            ici_xs.a_top = s - 1;
                            goto stable_stacks_continue;
                        }
                    }
                }
                ici_error = "break not within loop or switch";
                goto fail;

            case OP_ANDAND:
                /*
                 * bool obj => bool (os) OR pc (xs)
                 */
                {
                    int         c;

                    if ((c = !isfalse(ici_os.a_top[-2])) == opof(o)->op_code)
                    {
                        /*
                         * Have to test next part of the condition.
                         */
                        get_pc(arrayof(ici_os.a_top[-1]), ici_xs.a_top);
                        ++ici_xs.a_top;
                        ici_os.a_top -= 2;
                        goto stable_stacks_continue;
                    }
                    /*
                     * This is the old behaviour of ICI 4.0.3 and before
                     * where the value was reduced to 0 or 1 exactly. 
                     *
                     * ici_os.a_top[-2] = objof(c ? ici_one : ici_zero);
                     */
                    --ici_os.a_top;
                }
                goto stable_stacks_continue;
            
            case OP_CONTINUE:
                /*
                 * Pop the execution stack until a looper is found.
                 */
                {
                    ici_obj_t   **s;

                    for (s = ici_xs.a_top; s > ici_xs.a_base + 1; --s)
                    {
                        if (iscatch(s[-1]))
                        {
                            if (s[-1]->o_flags & CF_EVAL_BASE)
                                break;
                            if (s[-1]->o_flags & CF_CRIT_SECT)
                            {
                                --ici_exec->x_critsect;
                                ici_exec_count = 1;
                            }
                        }
                        if (s[-1] == objof(&o_looper) || isforall(s[-1]))
                        {
                            ici_xs.a_top = s;
                            goto stable_stacks_continue;
                        }
                    }
                }
                ici_error = "continue not within loop";
                goto fail;

            case OP_REWIND:
                /*
                 * This is the end of a code array that is the subject
                 * of a loop. Rewind the pc back to its start.
                 */
                o = ici_xs.a_top[-1];
                pcof(o)->pc_next = pcof(o)->pc_code->a_base;
                goto continue_with_same_pc;

            case OP_LOOPER:
                /*
                 * obj self     => obj self pc (xs)
                 *              => (os)
                 *
                 * We have fallen out of a code array that is the subject of
                 * a loop. Push a new pc pointing to the start of the code array
                 * back on the stack. This doesn't happen very often now, because
                 * the OP_REWIND (above) does the common case of comming to the
                 * end of a code array that should loop.
                 */
                *ici_xs.a_top++ = o; /* Restore formal state.*/
                get_pc(arrayof(ici_xs.a_top[-2]), ici_xs.a_top);
                ++ici_xs.a_top;
                goto stable_stacks_continue;

            case OP_ENDCODE:
                /*
                 * pc => - (xs)
                 */
                --ici_xs.a_top;
                goto stable_stacks_continue;

            case OP_LOOP:
                o = *pcof(ici_xs.a_top[-1])->pc_next++;
                *ici_xs.a_top++ = o;
                *ici_xs.a_top++ = objof(&o_looper);
                get_pc(arrayof(o), ici_xs.a_top);
                ++ici_xs.a_top;
                break;

            case OP_EXEC:
                /*
                 * array => - (os)
                 *       => pc (xs)
                 */
                get_pc(arrayof(ici_os.a_top[-1]), ici_xs.a_top);
                ++ici_xs.a_top;
                --ici_os.a_top;
                continue;

            case OP_SWITCHER:
                /*
                 * NULL self (xs) =>
                 *
                 * This only happens when we fall of the bottom of a switch
                 * without a break.
                 */
                --ici_xs.a_top;
                goto stable_stacks_continue;

            case OP_SWITCH:
                /*
                 * value array struct => (os)
                 *           => NULL switcher (pc(array) + struct.value) (xs)
                 */
                {
                    register ici_sslot_t *sl;

                    if ((sl = find_raw_slot(structof(ici_os.a_top[-1]), ici_os.a_top[-3]))->sl_key == NULL)
                    {
                        if ((sl = find_raw_slot(structof(ici_os.a_top[-1]), objof(&o_mark)))->sl_key == NULL)
                        {
                            /*
                             * No matching case, no default. Pop everything off and
                             * continue;
                             */
                            ici_os.a_top -= 3;
                            goto stable_stacks_continue;
                        }
                    }
                    *ici_xs.a_top++ = objof(&o_null);
                    *ici_xs.a_top++ = objof(&o_switcher);
                    get_pc(arrayof(ici_os.a_top[-2]), ici_xs.a_top);
                    pcof(*ici_xs.a_top)->pc_next += intof(sl->sl_value)->i_value;
                    ++ici_xs.a_top;
                    ici_os.a_top -= 3;
                }
                goto stable_stacks_continue;

            case OP_CRITSECT:
                {
                    *ici_xs.a_top = (ici_obj_t *)new_catch
                    (
                        NULL,
                        (ici_os.a_top - ici_os.a_base) - 1,
                        ici_vs.a_top - ici_vs.a_base,
                        CF_CRIT_SECT
                    );
                    if (*ici_xs.a_top == NULL)
                        goto fail;
                    ++ici_xs.a_top;
                    get_pc(arrayof(ici_os.a_top[-1]), ici_xs.a_top);
                    ++ici_xs.a_top;
                    --ici_os.a_top;
                    ++ici_exec->x_critsect;
                }
                continue;


            case OP_WAITFOR:
                /*
                 * obj => - (os)
                 */
                --ici_exec->x_critsect;
                ici_waitfor(ici_os.a_top[-1]);
                ++ici_exec->x_critsect;
                --ici_os.a_top;
                goto stable_stacks_continue;

            case OP_POP:
                --ici_os.a_top;
                goto stable_stacks_continue;

            case OP_BINOP:
            case OP_BINOP_FOR_TEMP:
#ifndef BINOPFUNC
#include        "binop.h"
#else
                if (ici_op_binop(o))
                    goto fail;
#endif
                goto continue_with_same_pc;

            default:
                assert(0);
            }
            continue;
        }

    fail:
        {
            ici_catch_t *c;

            if (ici_error == NULL)
                ici_error = "error";
            for (;;)
            {
                if ((c = ici_unwind()) == NULL || objof(c)->o_flags & CF_EVAL_BASE)
                    goto badfail;
                if (objof(c)->o_flags & CF_CRIT_SECT)
                {
                    --ici_exec->x_critsect;
                    ici_exec_count = 1;
                    continue;
                }
                break;
            }
            ici_incref(c);
            if (ici_set_val(objwsupof(ici_vs.a_top[-1]), SS(error), 's', ici_error))
            {
                ici_decref(c);
                goto badfail;
            }
            get_pc(arrayof(c->c_catcher), ici_xs.a_top);
            ++ici_xs.a_top;
            ici_decref(c);
            continue;

        badfail:
#ifndef NODEBUGGING
            /*
             * This is not such a useful place to hop into the debugger on
             * error, because we have already unwound the stack. So the user's
             * scope for debugging is very limited. But if it was earlier, we
             * would be breaking on every type of error, even caught ones.
             */
            if (ici_debug_active && !ici_debug_ign_err)
                ici_debug->idbg_error(ici_error, ici_exec->x_src);
#endif
            expand_error(ici_exec->x_src->s_lineno, ici_exec->x_src->s_filename);
            --ici_exec->x_n_engine_recurse;
            return NULL;
        }
    }
}

/*
 * Evaluate 'name' as if it was a variable in a script in the currently
 * prevailing scope, and return its value. If the name is undefined, this
 * will attempt to load extension modules in an attemot to get it defined.
 *
 * This is slightly different from fetching the name from the top element
 * of the scope stack (i.e. 'ici_vs.a_top[-1]') because it will attempt to
 * auto-load, and fail if the name is not defined.
 *
 * The returned object has had it's reference count incremented.
 *
 * Returns NULL on error, usual conventions.
 *
 * This --func-- forms part of the --ici-api--.
 */
ici_obj_t *
ici_eval(ici_str_t *name)
{
    assert(isstring(objof(name)));
    return ici_evaluate(objof(name), 0);
}

ici_type_t  ici_exec_type =
{
    mark_exec,
    free_exec,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    fetch_exec,
    "exec"
};

#if 0
ici_code_t *
ici_code_new(ici_array_t *a)
{
    ici_code_t          *c;

    if ((c = ici_talloc(ici_code_t)) == NULL)
        return NULL;
    ICI_OBJ_SET_TFNZ(c, TC_CODE, 0, 1, 0);
    c->c_code = a;
    return c;
}

static unsigned long
mark_code(ici_obj_t *o)
{
    return sizeof(ici_code_t) + ici_mark(codeof(o)->c_code);
}

static void
free_code(ici_obj_t *o)
{
    ici_tfree(o, ici_code_t);
}

/*
 * nargs code     => code-array (os)
 *       call     => o_exec     (xs)
 */
static int
call_code(ici_obj_t *o, ici_obj_t *s)
{
    if (NARGS() != 0)
        return ici_argcount(0);
    ici_xs.a_top[-1] = objof(&o_exec);
    --ici_os.a_top;
    ici_os.a_top[-1] = objof(codeof(o)->c_code);
    return 0;
}

ici_type_t  ici_code_type =
{
    mark_code,
    free_code,
    ici_hash_unique,
    ici_cmp_unique,
    ici_copy_simple,
    ici_assign_fail,
    ici_fetch_fail,
    "codefrag",
    NULL,
    call_code
};
#endif

ici_op_t    o_quote         = {OBJ(TC_OP), NULL, OP_QUOTE};
