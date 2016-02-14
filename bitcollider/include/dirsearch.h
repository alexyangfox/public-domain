/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: dirsearch.h,v 1.2 2001/04/13 22:20:13 mayhemchaos Exp $
 */
#ifndef DIR_H
#define DIR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _FileType
{
    eFile,
    eDir,
    eOther,
    eNotFound
} FileType;

FileType check_file_type(const char            *path);
int      recurse_dir    (BitcolliderSubmission *sub,
                         const char            *path, 
                         b_bool                 analyzeAll,
                         b_bool                 recurseDeep);

#ifdef __cplusplus
}
#endif

#endif
