/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: dirsearch.c,v 1.4 2001/06/16 03:28:43 mayhemchaos Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitcollider.h"
#include "dirsearch.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#ifndef WIN32
// check the type and do the long name lookup on the file
FileType check_file_type(const char *path)
{
   struct stat sbuf;

   if (lstat(path, &sbuf) == 0)
   {
       if (S_ISCHR(sbuf.st_mode) || S_ISBLK(sbuf.st_mode) || 
           S_ISFIFO(sbuf.st_mode) || S_ISSOCK(sbuf.st_mode))
          return eOther;

       if (S_ISREG(sbuf.st_mode))
          return eFile;
       else
       if (S_ISDIR(sbuf.st_mode) && !S_ISLNK(sbuf.st_mode))
          return eDir;
       else
          return eOther;
   }
   else
       return eNotFound;
}

int  recurse_dir(BitcolliderSubmission *sub, 
                 const char *path, 
                 b_bool analyzeAll,
                 b_bool recurseDeep)
{
   DIR           *dir;
   struct dirent *entry;
   char           newPath[MAX_PATH];
   int            count = 0;
   struct stat    sbuf;

   dir = opendir(path);
   if (dir == NULL)
   {
       return 0;
   }

   for(;;)
   {
       if (sub->bc->exitNow)
           break;

       /* Scan the given dir for plugins */
       entry = readdir(dir);
       if (!entry)
          break;

       /* Skip the . and .. dirs */
       if (strcmp(entry->d_name, ".") == 0 ||
           strcmp(entry->d_name, "..") == 0)
          continue;

       sprintf(newPath, "%s/%s", path, entry->d_name);
       if (lstat(newPath, &sbuf) == 0)
       {
           if (S_ISDIR(sbuf.st_mode) && !S_ISLNK(sbuf.st_mode) && recurseDeep)
           {
               count += recurse_dir(sub, newPath, analyzeAll, recurseDeep);
           }
           else
           if (S_ISREG(sbuf.st_mode))
           {
               fflush(stdout);

               if (analyze_file(sub, newPath, !analyzeAll))
               {
                  count++;
               }
           }
           else
           {
               if (sub->bc->progressCallback)
                  sub->bc->progressCallback(0, newPath, 
                             "skipped. (not a regular file)");
           }
       }
   }
   closedir(dir);

   return count;
}

#else

// check the type and do the long name lookup on the file
FileType check_file_type(const char *path)
{
   DWORD type;

   type = (int)GetFileAttributes(path);
   if ((int)type < 0)
      return eNotFound;

   if (type & FILE_ATTRIBUTE_DIRECTORY)
      return eDir;

   if ((type & FILE_ATTRIBUTE_HIDDEN) ||
       (type & FILE_ATTRIBUTE_OFFLINE) ||
       (type & FILE_ATTRIBUTE_TEMPORARY))
      return eOther;
      
   return eFile;
}

int recurse_dir(BitcolliderSubmission *sub, 
                const char *path, 
                b_bool analyzeAll,
                b_bool recurseDeep)
{
   WIN32_FIND_DATA  find;
   HANDLE           hFind;
   int              count = 0, j;
   char             newPath[MAX_PATH], newFile[MAX_PATH], savedPath[MAX_PATH];
   FileType         type;

   strcpy(newPath, path);
   if (newPath[strlen(newPath) - 1] == '\\')
      newPath[strlen(newPath) - 1] = 0;

   strcpy(savedPath, newPath);

   /* if a path was specified, then add a \*.* */
   if ((GetFileAttributes(newPath) & FILE_ATTRIBUTE_DIRECTORY) != 0)
      strcat(newPath, "\\*.*");

   hFind = FindFirstFile(newPath, &find);
   if (hFind == INVALID_HANDLE_VALUE)
      return -1;

   /* If its not a directory, then remove everything after the last slash */
   if ((GetFileAttributes(savedPath) & FILE_ATTRIBUTE_DIRECTORY) == 0)
   {
      char *ptr;

      ptr = strrchr(savedPath, '\\');
      if (ptr)
         *ptr = 0;
   }

   for(j = 0;; j++)
   {
       if (sub->bc->exitNow)
           break;

       if (j > 0)
          if (!FindNextFile(hFind, &find))
              break;

       /* Skip the . and .. dirs */
       if (strcmp(find.cFileName, ".") == 0 ||
           strcmp(find.cFileName, "..") == 0)
          continue;

       sprintf(newPath, "%s\\%s", savedPath, find.cFileName);
       type = check_file_type(newPath);
       if (type == eDir && recurseDeep)
       {
          count += recurse_dir(sub, newPath, analyzeAll, recurseDeep);
       }
       if (type == eFile)
       {
          getLongPathName(newPath, MAX_PATH, newFile);
          if (analyze_file(sub, newFile, !analyzeAll))
             count++;
       }
       else
       {
          if (sub->bc->progressCallback)
             sub->bc->progressCallback(0, newPath, 
                         "skipped. (not a regular file)");
       }
   }
   FindClose(hFind);

   return count;

}

#endif
