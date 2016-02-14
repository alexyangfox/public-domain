/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: plugin_man.h,v 1.4 2001/03/27 00:33:21 mayhemchaos Exp $
 */
#ifndef PLUGIN_MAN_H
#define PLUGIN_MAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bitcollider.h"

/*-------------------------------------------------------------------------*/

Bitcollider   *init_plugins    (void);
void           shutdown_plugins(Bitcollider *bc);

int            load_plugins    (Bitcollider *bc, const char *path, 
                                b_bool printDebugInfo);
void           unload_plugins  (Bitcollider *bc);

int            get_num_plugins (Bitcollider *bc);
PluginMethods *get_plugin      (Bitcollider *bc, const char *extension);

#ifdef WIN32
void set_plugin_dir(char *path);
#endif

/*-------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
