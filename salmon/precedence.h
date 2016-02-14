/* file "precedence.h" */

/*
 *  This file contains types specifying expression and type expression parsing
 *  precedence, which is used both for parsing and for knowing when parentheses
 *  need to be written when writing out expressions and types.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef PRECEDENCE_H
#define PRECEDENCE_H


typedef enum expression_parsing_precedence
  {
    EPP_TOP = 0,
    EPP_FORCE,
    EPP_CONDITIONAL,
    EPP_LOGICAL_OR,
    EPP_LOGICAL_AND,
    EPP_BITWISE_OR,
    EPP_BITWISE_XOR,
    EPP_BITWISE_AND,
    EPP_EQUALITY,
    EPP_RELATIONAL,
    EPP_SHIFT,
    EPP_CONCATENATE,
    EPP_ADDITIVE,
    EPP_MULTIPLICATIVE,
    EPP_UNARY,
    EPP_POSTFIX,
    EPP_TAGALONG
  } expression_parsing_precedence;

typedef enum type_expression_parsing_precedence
  {
    TEPP_TOP = 0,
    TEPP_OR,
    TEPP_XOR,
    TEPP_AND,
    TEPP_ARRAY,
    TEPP_MAP,
    TEPP_ROUTINE,
    TEPP_TYPE,
    TEPP_POINTER,
    TEPP_NOT
  } type_expression_parsing_precedence;


#endif /* PRECEDENCE_H */
