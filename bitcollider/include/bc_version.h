/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 */
#ifndef BC_VERSION_H
#define BC_VERSION_H

/* These values are reported with each submission.
 *
 * If you make any modifications, you may want to the agentname 
 * to another name of your choice; please do not use our 
 * trademark "Bitcollider". 
 */
#define BC_AGENTNAME     "Bitprinter"

/* You may want to change this to a build identifier of your own, instead
   of using a timestamp */
#define BC_AGENTBUILD    __DATE__ " " __TIME__

/* This indicates the version of the official submission spec */
#define BC_SUBMITSPECVER "0.4"

/* Your agent-version string; should be #[.#[.#[etc]]] format */
#define BC_VERSION       "0.6.0"

#endif
