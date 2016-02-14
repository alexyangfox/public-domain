/*
 * This code is in an include file because some compilers may not handle
 * the large function and switch statement which happens in exec.c.
 * See ici_op_binop() in arith.c if you have this problem.
 *
 * This is where run-time binary operator "arithmetic" happens.
 */

/*
 * On entry, o = ici_xs.a_top[-1], technically, but ici_xs.a_top has been pre-decremented
 * so it isn't really there.
 */
{
    register ici_obj_t  *o0;
    register ici_obj_t  *o1;
    register long       i;
    double              f;
    int                 can_temp;

#define SWAP()          (o = o0, o0 = o1, o1 = o)
    o0 = ici_os.a_top[-2];
    o1 = ici_os.a_top[-1];
    can_temp = opof(o)->op_ecode == OP_BINOP_FOR_TEMP;
    if (o0->o_tcode > TC_MAX_BINOP || o1->o_tcode > TC_MAX_BINOP)
        goto others;
    switch (TRI(o0->o_tcode, o1->o_tcode, opof(o)->op_code))
    {
    /*
     * Pure integer operations.
     */
    case TRI(TC_INT, TC_INT, T_ASTERIX):
    case TRI(TC_INT, TC_INT, T_ASTERIXEQ):
        i = intof(o0)->i_value * intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_SLASH):
    case TRI(TC_INT, TC_INT, T_SLASHEQ):
        if (intof(o1)->i_value == 0)
        {
            ici_error = "division by 0";
            goto fail;
        }
        i = intof(o0)->i_value / intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_PERCENT):
    case TRI(TC_INT, TC_INT, T_PERCENTEQ):
        if (intof(o1)->i_value == 0)
        {
            ici_error = "modulus by 0";
            goto fail;
        }
        i = intof(o0)->i_value % intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_PLUS):
    case TRI(TC_INT, TC_INT, T_PLUSEQ):
        i = intof(o0)->i_value + intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_MINUS):
    case TRI(TC_INT, TC_INT, T_MINUSEQ):
        i = intof(o0)->i_value - intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_GRTGRT):
    case TRI(TC_INT, TC_INT, T_GRTGRTEQ):
        i = intof(o0)->i_value >> intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_LESSLESS):
    case TRI(TC_INT, TC_INT, T_LESSLESSEQ):
        i = intof(o0)->i_value << intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_LESS):
        if (intof(o0)->i_value < intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_INT, T_GRT):
        if (intof(o0)->i_value > intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_INT, T_LESSEQ):
        if (intof(o0)->i_value <= intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_INT, T_GRTEQ):
        if (intof(o0)->i_value >= intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_INT, T_EQEQ):
        if (intof(o0)->i_value == intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_INT, T_EXCLAMEQ):
        if (intof(o0)->i_value != intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_INT, T_AND):
    case TRI(TC_INT, TC_INT, T_ANDEQ):
        i = intof(o0)->i_value & intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_CARET):
    case TRI(TC_INT, TC_INT, T_CARETEQ):
        i = intof(o0)->i_value ^ intof(o1)->i_value;
        goto usei;

    case TRI(TC_INT, TC_INT, T_BAR):
    case TRI(TC_INT, TC_INT, T_BAREQ):
        i = intof(o0)->i_value | intof(o1)->i_value;
        goto usei;

    /*
     * Pure floating point and mixed float, int operations...
     */
    case TRI(TC_FLOAT, TC_FLOAT, T_ASTERIX):
    case TRI(TC_FLOAT, TC_FLOAT, T_ASTERIXEQ):
        f = floatof(o0)->f_value * floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_INT, T_ASTERIX):
    case TRI(TC_FLOAT, TC_INT, T_ASTERIXEQ):
        f = floatof(o0)->f_value * intof(o1)->i_value;
        goto usef;

    case TRI(TC_INT, TC_FLOAT, T_ASTERIX):
    case TRI(TC_INT, TC_FLOAT, T_ASTERIXEQ):
        f = intof(o0)->i_value * floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_FLOAT, T_SLASH):
    case TRI(TC_FLOAT, TC_FLOAT, T_SLASHEQ):
        if (floatof(o1)->f_value == 0)
        {
            ici_error = "division by 0.0";
            goto fail;
        }
        f = floatof(o0)->f_value / floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_INT, T_SLASH):
    case TRI(TC_FLOAT, TC_INT, T_SLASHEQ):
        if (intof(o1)->i_value == 0)
        {
            ici_error = "division by 0";
            goto fail;
        }
        f = floatof(o0)->f_value / intof(o1)->i_value;
        goto usef;

    case TRI(TC_INT, TC_FLOAT, T_SLASH):
    case TRI(TC_INT, TC_FLOAT, T_SLASHEQ):
        if (floatof(o1)->f_value == 0)
        {
            ici_error = "division by 0.0";
            goto fail;
        }
        f = intof(o0)->i_value / floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_FLOAT, T_PLUS):
    case TRI(TC_FLOAT, TC_FLOAT, T_PLUSEQ):
        f = floatof(o0)->f_value + floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_INT, T_PLUS):
    case TRI(TC_FLOAT, TC_INT, T_PLUSEQ):
        f = floatof(o0)->f_value + intof(o1)->i_value;
        goto usef;

    case TRI(TC_INT, TC_FLOAT, T_PLUS):
    case TRI(TC_INT, TC_FLOAT, T_PLUSEQ):
        f = intof(o0)->i_value + floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_FLOAT, T_MINUS):
    case TRI(TC_FLOAT, TC_FLOAT, T_MINUSEQ):
        f = floatof(o0)->f_value - floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_INT, T_MINUS):
    case TRI(TC_FLOAT, TC_INT, T_MINUSEQ):
        f = floatof(o0)->f_value - intof(o1)->i_value;
        goto usef;

    case TRI(TC_INT, TC_FLOAT, T_MINUS):
    case TRI(TC_INT, TC_FLOAT, T_MINUSEQ):
        f = intof(o0)->i_value - floatof(o1)->f_value;
        goto usef;

    case TRI(TC_FLOAT, TC_FLOAT, T_LESS):
        if (floatof(o0)->f_value < floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_INT, T_LESS):
        if (floatof(o0)->f_value < intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_FLOAT, T_LESS):
        if (intof(o0)->i_value < floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_FLOAT, T_GRT):
        if (floatof(o0)->f_value > floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_INT, T_GRT):
        if (floatof(o0)->f_value > intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_FLOAT, T_GRT):
        if (intof(o0)->i_value > floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_FLOAT, T_LESSEQ):
        if (floatof(o0)->f_value <= floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_INT, T_LESSEQ):
        if (floatof(o0)->f_value <= intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_FLOAT, T_LESSEQ):
        if (intof(o0)->i_value <= floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_FLOAT, T_GRTEQ):
        if (floatof(o0)->f_value >= floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_INT, T_GRTEQ):
        if (floatof(o0)->f_value >= intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_FLOAT, T_GRTEQ):
        if (intof(o0)->i_value >= floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_FLOAT, T_EQEQ):
        if (floatof(o0)->f_value == floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_INT, T_EQEQ):
        if (floatof(o0)->f_value == intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_FLOAT, T_EQEQ):
        if (intof(o0)->i_value == floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_FLOAT, T_EXCLAMEQ):
        if (floatof(o0)->f_value != floatof(o1)->f_value)
            goto use1;
        goto use0;

    case TRI(TC_FLOAT, TC_INT, T_EXCLAMEQ):
        if (floatof(o0)->f_value != intof(o1)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_INT, TC_FLOAT, T_EXCLAMEQ):
        if (intof(o0)->i_value != floatof(o1)->f_value)
            goto use1;
        goto use0;

    /*
     * Regular expression operators...
     */
    case TRI(TC_STRING, TC_REGEXP, T_TILDE):
        SWAP();
    case TRI(TC_REGEXP, TC_STRING, T_TILDE):
        if
        (
            pcre_exec
            (
                regexpof(o0)->r_re,
                regexpof(o0)->r_rex,
                stringof(o1)->s_chars,
                stringof(o1)->s_nchars,
                0,
                0,
                re_bra,
                nels(re_bra)
            )
            >=
            0
        )
            goto use1;
        goto use0;

    case TRI(TC_STRING, TC_REGEXP, T_EXCLAMTILDE):
        SWAP();
    case TRI(TC_REGEXP, TC_STRING, T_EXCLAMTILDE):
        if
        (
            pcre_exec
            (
                regexpof(o0)->r_re,
                regexpof(o0)->r_rex,
                stringof(o1)->s_chars,
                stringof(o1)->s_nchars,
                0,
                0,
                re_bra,
                nels(re_bra)
            )
            <
            0
        )
            goto use1;
        goto use0;

    case TRI(TC_STRING, TC_REGEXP, T_2TILDE):
    case TRI(TC_STRING, TC_REGEXP, T_2TILDEEQ):
        SWAP();
    case TRI(TC_REGEXP, TC_STRING, T_2TILDE):
    case TRI(TC_REGEXP, TC_STRING, T_2TILDEEQ):
        memset(re_bra, 0, sizeof re_bra);
        if
        (
            pcre_exec
            (
                regexpof(o0)->r_re,
                regexpof(o0)->r_rex,
                stringof(o1)->s_chars,
                stringof(o1)->s_nchars,
                0,
                0,
                re_bra,
                nels(re_bra)
            )
            <
            0
            ||
            re_bra[2] < 0 || re_bra[3] < 0
        )
        {
            o = objof(&o_null);
            goto useo;
        }
        else
        {
            o = objof
                (
                    ici_str_new
                    (
                        stringof(o1)->s_chars + re_bra[2],
                        re_bra[3] - re_bra[2]
                    )
                );
            if (o == NULL)
                goto fail;
        }
        goto looseo;

    case TRI(TC_STRING, TC_REGEXP, T_3TILDE):
        SWAP();
    case TRI(TC_REGEXP, TC_STRING, T_3TILDE):
        memset(re_bra, 0, sizeof re_bra);
        re_nbra = pcre_exec
                  (
                      regexpof(o0)->r_re,
                      regexpof(o0)->r_rex,
                      stringof(o1)->s_chars,
                      stringof(o1)->s_nchars,
                      0,
                      0,
                      re_bra,
                      nels(re_bra)
                  );
        if (re_nbra < 0)
        {
            o = objof(&o_null);
            goto useo;
        }
        if ((o = objof(ici_array_new(re_nbra))) == NULL)
            goto fail;
        for (i = 1; i < re_nbra; ++i)
        {
            if (re_bra[i*2] == -1)
            {
                if ((*arrayof(o)->a_top = objof(ici_str_alloc(0))) == NULL)
                {
                    goto fail;
                }
            }
            else if
            (
                (
                    *arrayof(o)->a_top
                    =
                    objof
                    (
                        ici_str_new
                        (
                            stringof(o1)->s_chars + re_bra[i*2],
                            re_bra[(i * 2) + 1 ] - re_bra[i * 2]
                        )
                    )
                )
                ==
                NULL
            )
            {
                goto fail;
            }
            ici_decref(*arrayof(o)->a_top);
            ++arrayof(o)->a_top;
        }
        goto looseo;

    /*
     * Everything else...
     */
    case TRI(TC_PTR, TC_INT, T_MINUS):
    case TRI(TC_PTR, TC_INT, T_MINUSEQ):
        if (!isint(ptrof(o0)->p_key))
            goto mismatch;
        {
            ici_obj_t   *i;

            if ((i = objof(ici_int_new(intof(ptrof(o0)->p_key)->i_value
                    - intof(o1)->i_value))) == NULL)
                goto fail;
            if ((o = objof(ici_ptr_new(ptrof(o0)->p_aggr, i))) == NULL)
                goto fail;
            ici_decref(i);
        }
        goto looseo;
        
    case TRI(TC_INT, TC_PTR, T_PLUS):
    case TRI(TC_INT, TC_PTR, T_PLUSEQ):
        if (!isint(ptrof(o1)->p_key))
            goto mismatch;
        SWAP();
    case TRI(TC_PTR, TC_INT, T_PLUS):
    case TRI(TC_PTR, TC_INT, T_PLUSEQ):
        if (!isint(ptrof(o0)->p_key))
            goto mismatch;
        {
            ici_obj_t   *i;

            if ((i = objof(ici_int_new(intof(ptrof(o0)->p_key)->i_value
                    + intof(o1)->i_value))) == NULL)
                goto fail;
            if ((o = objof(ici_ptr_new(ptrof(o0)->p_aggr, i))) == NULL)
                goto fail;
            ici_decref(i);
        }
        goto looseo;

    case TRI(TC_STRING, TC_STRING, T_PLUS):
    case TRI(TC_STRING, TC_STRING, T_PLUSEQ):
        if ((o = objof(ici_str_alloc(stringof(o1)->s_nchars
            + stringof(o0)->s_nchars))) == NULL)
            goto fail;
        memcpy
        (
            stringof(o)->s_chars,
            stringof(o0)->s_chars,
            stringof(o0)->s_nchars
        );
        memcpy
        (
            stringof(o)->s_chars + stringof(o0)->s_nchars,
            stringof(o1)->s_chars,
            stringof(o1)->s_nchars + 1
        );
        o = ici_atom(o, 1);
        goto looseo;

    case TRI(TC_ARRAY, TC_ARRAY, T_PLUS):
    case TRI(TC_ARRAY, TC_ARRAY, T_PLUSEQ):
        {
            ici_array_t *a;
            ptrdiff_t   z0;
            ptrdiff_t   z1;

            z0 = ici_array_nels(arrayof(o0));
            z1 = ici_array_nels(arrayof(o1));
            if ((a = ici_array_new(z0 + z1)) == NULL)
                goto fail;
            ici_array_gather(a->a_top, arrayof(o0), 0, z0);
            a->a_top += z0;
            ici_array_gather(a->a_top, arrayof(o1), 0, z1);
            a->a_top += z1;
            o = objof(a);
        }
        goto looseo;

    case TRI(TC_STRUCT, TC_STRUCT, T_PLUS):
    case TRI(TC_STRUCT, TC_STRUCT, T_PLUSEQ):
        {
            register ici_struct_t   *s;
            register ici_sslot_t *sl;
            register int        i;

            if ((s = structof(copy(o0))) == NULL)
                goto fail;
            sl = structof(o1)->s_slots;
            for (i = 0; i < structof(o1)->s_nslots; ++i, ++sl)
            {
                if (sl->sl_key == NULL)
                    continue;
                if (ici_assign(s, sl->sl_key, sl->sl_value))
                {
                    ici_decref(s);
                    goto fail;
                }
            }
            o = objof(s);
        }
        goto looseo;

    case TRI(TC_SET, TC_SET, T_PLUS):
    case TRI(TC_SET, TC_SET, T_PLUSEQ):
        {
            register ici_set_t  *s;
            register ici_obj_t  **sl;
            register int        i;

            if ((s = setof(copy(o0))) == NULL)
                goto fail;
            sl = setof(o1)->s_slots;
            for (i = 0; i < setof(o1)->s_nslots; ++i, ++sl)
            {
                if (*sl == NULL)
                    continue;
                if (ici_assign(s, *sl, ici_one))
                {
                    ici_decref(s);
                    goto fail;
                }
            }
            o = objof(s);
        }
        goto looseo;

    case TRI(TC_SET, TC_SET, T_MINUS):
    case TRI(TC_SET, TC_SET, T_MINUSEQ):
        {
            register ici_set_t  *s;
            register ici_obj_t  **sl;
            register int        i;

            if ((s = setof(copy(o0))) == NULL)
                goto fail;
            sl = setof(o1)->s_slots;
            for (i = 0; i < setof(o1)->s_nslots; ++i, ++sl)
            {
                if (*sl == NULL)
                    continue;
                if (ici_assign(s, *sl, &o_null))
                {
                    ici_decref(s);
                    goto fail;
                }
            }
            o = objof(s);
        }
        goto looseo;

    case TRI(TC_SET, TC_SET, T_ASTERIX):
    case TRI(TC_SET, TC_SET, T_ASTERIXEQ):
        {
            ici_set_t  *s;
            ici_obj_t  **sl;
            int        i;

            if (setof(o0)->s_nels > setof(o1)->s_nels)
                SWAP();
            if ((s = ici_set_new()) == NULL)
                goto fail;
            sl = setof(o0)->s_slots;
            for (i = 0; i < setof(o0)->s_nslots; ++i, ++sl)
            {
                if (*sl == NULL)
                    continue;
                if
                (
                    ici_fetch(o1, *sl) != objof(&o_null)
                    &&
                    ici_assign(s, *sl, ici_one)
                )
                {
                    ici_decref(s);
                    goto fail;
                }
            }
            o = objof(s);
        }
        goto looseo;

    case TRI(TC_SET, TC_SET, T_GRTEQ):
        o = set_issubset(setof(o1), setof(o0)) ? objof(ici_one) : objof(ici_zero);
        goto useo;

    case TRI(TC_SET, TC_SET, T_LESSEQ):
        o = set_issubset(setof(o0), setof(o1)) ? objof(ici_one) : objof(ici_zero);
        goto useo;

    case TRI(TC_SET, TC_SET, T_GRT):
        o = set_ispropersubset(setof(o1), setof(o0)) ? objof(ici_one) : objof(ici_zero);
        goto useo;

    case TRI(TC_SET, TC_SET, T_LESS):
        o = set_ispropersubset(setof(o0), setof(o1)) ? objof(ici_one) : objof(ici_zero);
        goto useo;

    case TRI(TC_PTR, TC_PTR, T_MINUS):
    case TRI(TC_PTR, TC_PTR, T_MINUSEQ):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key))
            goto mismatch;
        if ((o = objof(ici_int_new(intof(ptrof(o0)->p_key)->i_value
              - intof(ptrof(o1)->p_key)->i_value))) == NULL)
              goto fail;
        goto looseo;

    case TRI(TC_STRING, TC_STRING, T_LESS):
    case TRI(TC_STRING, TC_STRING, T_GRT):
    case TRI(TC_STRING, TC_STRING, T_LESSEQ):
    case TRI(TC_STRING, TC_STRING, T_GRTEQ):
        {
            int         compare;
            ici_str_t   *s1;
            ici_str_t   *s2;

            s1 = stringof(o0);
            s2 = stringof(o1);
            compare = s1->s_nchars < s2->s_nchars ? s1->s_nchars : s2->s_nchars;
            compare = memcmp(s1->s_chars, s2->s_chars, compare);
            if (compare == 0)
            {
                if (s1->s_nchars < s2->s_nchars)
                    compare = -1;
                else if (s1->s_nchars > s2->s_nchars)
                    compare = 1;
            }
            switch (opof(o)->op_code)
            {
            case t_subtype(T_LESS):   if (compare < 0) goto use1; break;
            case t_subtype(T_GRT):    if (compare > 0) goto use1; break;
            case t_subtype(T_LESSEQ): if (compare <= 0) goto use1; break;
            case t_subtype(T_GRTEQ):  if (compare >= 0) goto use1; break;
            }
        }
        goto use0;

    case TRI(TC_PTR, TC_PTR, T_LESS):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key))
            goto mismatch;
        if (intof(ptrof(o0)->p_key)->i_value < intof(ptrof(o1)->p_key)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_PTR, TC_PTR, T_GRTEQ):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key))
            goto mismatch;
        if (intof(ptrof(o0)->p_key)->i_value >=intof(ptrof(o1)->p_key)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_PTR, TC_PTR, T_LESSEQ):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key))
            goto mismatch;
        if (intof(ptrof(o0)->p_key)->i_value <=intof(ptrof(o1)->p_key)->i_value)
            goto use1;
        goto use0;

    case TRI(TC_PTR, TC_PTR, T_GRT):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key))
            goto mismatch;
        if (intof(ptrof(o0)->p_key)->i_value > intof(ptrof(o1)->p_key)->i_value)
            goto use1;
        goto use0;

    default:
    others:
        switch (opof(o)->op_code)
        {
        case t_subtype(T_EQEQ):
            if (ici_typeof(o0) == ici_typeof(o1) && cmp(o0, o1) == 0)
                goto use1;
            goto use0;

        case t_subtype(T_EXCLAMEQ):
            if (!(ici_typeof(o0) == ici_typeof(o1) && cmp(o0, o1) == 0))
                goto use1;
            goto use0;
        }
        /* Fall through. */
    mismatch:
        {
            char        n1[30];
            char        n2[30];

            sprintf(buf, "attempt to perform \"%s %s %s\"",
                ici_objname(n1, o0),
                ici_binop_name(opof(o)->op_code),
                ici_objname(n2, o1));
        }
        ici_error = buf;
        goto fail;
    }

use0:
    ici_os.a_top[-2] = objof(ici_zero);
    goto done;

use1:
    ici_os.a_top[-2] = objof(ici_one);
    goto done;

usef:
    if (can_temp)
    {
        int             n;

        n = &ici_os.a_top[-2] - ici_os.a_base;
        if (ici_stk_probe(ici_exec->x_os_temp_cache, n))
            goto fail;
        if ((o = ici_exec->x_os_temp_cache->a_base[n]) == objof(&o_null))
        {
            if ((o = objof(ici_talloc(ici_ostemp_t))) == NULL)
                goto fail;
            ici_exec->x_os_temp_cache->a_base[n] = o;
            ici_rego(o);
        }
        ICI_OBJ_SET_TFNZ(o, TC_FLOAT, O_TEMP, 0, sizeof(ici_ostemp_t));
        floatof(o)->f_value = f;
        goto useo;
    }
    /*
     * The following in-line expansion of float creation replaces, and
     * this should be equivalent to, this old code:
     *
     * if ((o = objof(ici_float_new(f))) == NULL)
     *     goto fail;
     * goto looseo;
     */

    /*
     * In-line expansion of float creation.
     */
    {
        ici_obj_t               **po;
        union
        {
            double              f;
            long                l[2];
        }
            v;
        unsigned long           h;

        /*
         * If this assert fails, we should switch back to a an explicit call
         * to hash_float().
         */
        assert(sizeof floatof(o)->f_value == 2 * sizeof(unsigned long));
        v.f = f;
        h = FLOAT_PRIME + v.l[0] + v.l[1] * 31;
        h ^= (h >> 12) ^ (h >> 24);
        for
        (
            po = &atoms[ici_atom_hash_index(h)];
            (o = *po) != NULL;
            --po < atoms ? po = atoms + atomsz - 1 : NULL
        )
        {
            if (isfloat(o) && DBL_BIT_CMP(&floatof(o)->f_value, &v.l))
                goto useo;
        }
        ++ici_supress_collect;
        if ((o = objof(ici_talloc(ici_float_t))) == NULL)
        {
            --ici_supress_collect;
            goto fail;
        }
        ICI_OBJ_SET_TFNZ(o, TC_FLOAT, O_ATOM, 1, sizeof(ici_float_t));
        floatof(o)->f_value = f;
        ici_rego(o);
        assert(h == hash(o));
        --ici_supress_collect;
        ICI_STORE_ATOM_AND_COUNT(po, o);
        goto looseo;
    }

usei:
    if (can_temp)
    {
        int             n;

        n = &ici_os.a_top[-2] - ici_os.a_base;
        if (ici_stk_probe(ici_exec->x_os_temp_cache, n))
            goto fail;
        if ((o = ici_exec->x_os_temp_cache->a_base[n]) == objof(&o_null))
        {
            if ((o = objof(ici_talloc(ici_ostemp_t))) == NULL)
                goto fail;
            ici_exec->x_os_temp_cache->a_base[n] = o;
            ici_rego(o);
        }
        ICI_OBJ_SET_TFNZ(o, TC_INT, O_TEMP, 0, sizeof(ici_ostemp_t));
        intof(o)->i_value = i;
        goto useo;
    }
    /*
     * In-line expansion of atom_int() from object.c. Following that, and
     * merged with it, is in-line atom creation.
     */
    if ((i & ~ICI_SMALL_INT_MASK) == 0)
    {
        o = objof(ici_small_ints[i]);
        goto useo;
    }
    {
        register ici_obj_t      **po;

        for
        (
            po = &atoms[ici_atom_hash_index((unsigned long)i * INT_PRIME)];
            (o = *po) != NULL;
            --po < atoms ? po = atoms + atomsz - 1 : NULL
        )
        {
            if (isint(o) && intof(o)->i_value == i)
                goto useo;
        }
        ++ici_supress_collect;
        if ((o = objof(ici_talloc(ici_int_t))) == NULL)
        {
            --ici_supress_collect;
            goto fail;
        }
        ICI_OBJ_SET_TFNZ(o, TC_INT, O_ATOM, 1, sizeof(ici_int_t));
        intof(o)->i_value = i;
        ici_rego(o);
        --ici_supress_collect;
        ICI_STORE_ATOM_AND_COUNT(po, o);
    }

looseo:
    ici_decref(o);
useo:
    ici_os.a_top[-2] = o;
done:
    --ici_os.a_top;
    /*--ici_xs.a_top; Don't do this because it has been pre-done. */
}
