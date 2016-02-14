/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: plugin.h,v 1.7 2001/06/18 07:27:17 mayhemchaos Exp $
 */
#ifndef PLUGIN_H
#define PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------*/

typedef struct _SupportedFormat
{
    char *fileExtension;
    char *formatName;
} SupportedFormat;

typedef struct _Attribute
{
    char *key;
    char *value;
} Attribute;

typedef void Context;

typedef struct _PluginMethods
{
    void              (*shutdown_plugin)      (void);

    const char       *(*get_version)          (void);
    const char       *(*get_name)             (void);
    SupportedFormat  *(*get_supported_formats)(void);
   
    Attribute        *(*file_analyze)         (const char *fileName);

    Context          *(*mem_analyze_init)     (void);
    void              (*mem_analyze_update)   (Context             *context, 
                                               const unsigned char *buf,
                                               unsigned             bufLen);
    Attribute        *(*mem_analyze_final)    (Context *context);

    void              (*free_attributes)      (Attribute *attrList);

    const char       *(*get_error)            (void);
} PluginMethods;

/*-------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
