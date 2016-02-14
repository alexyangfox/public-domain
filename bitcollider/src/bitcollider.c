/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: bitcollider.c,v 1.23 2004/02/02 01:11:53 mayhemchaos Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "bitcollider.h"
#include "dirsearch.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <config.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define MAX_EXT_LEN 64

void usage()
{
   char agentString[80];

   get_agent_string(agentString);
   printf("%s\n\n", agentString);
   printf("Usage: bitcollider [options] <file|dir> [file|dir] ...\n");
   printf("       bitcollider [options] -f <tag file>\n\n");
   printf("Options:\n");
   printf(" -p        - print the submission info, without submitting\n");
   printf(" -n        - show submission in browser, without submitting\n");
   printf(" -u [url]  - send submission to alternate URL\n");
   printf(" -e [ext]  - treat file as if it had a given extension\n");
   printf(" -d        - debug print during plugin loading\n");
   printf(" -r        - recurse into directories.\n");
   printf(" -a        - analyze all files for a given directory.\n");
   printf("             (Default is to analyze only known extensions)\n");
   printf(" -f <file> - read tags from <file> instead of analyzing\n");
   printf(" --md5      - calculate MD5\n");
   printf(" --crc32    - calculate CRC32\n");
#if !defined(_WIN32) && !defined(DARWIN)
   printf(" --konqueror   - use the Konqueror browser to submit\n");
   printf(" --lynx        - use the lynx browser to submit\n");
   printf(" --mozilla     - use the Mozilla browser to submit\n");
   printf(" --opera       - use the Opera browser to submit\n");
   printf("\nBy default Netscape will be used. You may also set the\n");
   printf("BROWSER environment variable to specify your browser of choice.\n");
   printf("Check http://www.tuxedo.org/~esr/BROWSER/index.html for details.\n");
#endif

   printf("\nFor more information, go to http://bitzi.com/bitcollider\n");
}

void print_error(Bitcollider *bc, 
                 char        *message)
{
   char *error;

   if( ! bc )
   {
     fprintf(stderr, "%s\n", message);
     return;
   }

   error = get_error(bc);

   fprintf(stderr, "%s:\n", message);
   if (error)
      fprintf(stderr, "   Error: %s\n\n", error);
   else
      fprintf(stderr, "   Unknown error.\n\n");
}

void print_warning(Bitcollider *bc)
{
   char *warning;

   warning = get_warning(bc);
   if (warning)
      fprintf(stderr, "Warning: %s\n\n", warning);
}

extern void win32_progress_callback(int percentComplete, const char *fileName, const char *message);
void progress(int percentComplete, const char *fileName, const char *message)
{
   if (fileName)
   {
      fprintf(stderr, "  %s: ", fileName);
      fflush(stdout);
   }

   if (message)
      fprintf(stderr, "%s \n", message);      
   else
   {
      fprintf(stderr, "%-3d%%\b\b\b\b", percentComplete);
      fflush(stdout);
   }
}

/* This var is global so that the atexit() handler can grab it and
   cleanly shutdown the Berkeley DB cache */
static Bitcollider           *bc = NULL;

void exit_cleanup(void)
{
   if (bc)
   {
      bitcollider_shutdown(bc);
   }
}

/* Catch a SIGINT (^C) and then exit normally, letting the atexit
   handler do the actual cleanup */
void signal_cleanup(int sig)
{
    exit(0);
}

int main(int argc, char *argv[])
{
  const char            *submitTarget;
  char                   checkAsExt[MAX_EXT_LEN];
  char			 tagFile[MAX_PATH];
  BitcolliderSubmission *submission;
  int                    argIndex = 1, count;
  BrowserEnum            browser = eBrowserNetscape;
  FileType               type;
  b_bool                 dump = false;
  b_bool                 autoSubmit = true;
  b_bool                 debugPlugins = false;
  b_bool                 analyzeAll = false;
  b_bool                 recurse = false;
  b_bool                 quiet = false;
  b_bool                 clearCache = false;
  b_bool                 calculateMD5 = false;
  b_bool                 calculateCRC32 = false;
  b_bool                 ret;
  char                   fileName[MAX_PATH];


  if (argc < 2) 
  {
     usage();
     exit(EXIT_FAILURE);
  }
  checkAsExt[0] = 0;
  tagFile[0] = 0;

  submitTarget = getenv("BITPRINT_SUBMIT_URL");
  if (submitTarget && strlen(submitTarget) == 0)
     submitTarget = NULL;

  for(; argIndex < argc; argIndex++)
  {
#ifndef _WIN32
      if (strcmp(argv[argIndex], "--mozilla") == 0)
      {
          browser = eBrowserMozilla;
      }
      else
      if (strcmp(argv[argIndex], "--konqueror") == 0)
      {
          browser = eBrowserKonqueror;
      }
      else
      if (strcmp(argv[argIndex], "--opera") == 0)
      {
          browser = eBrowserOpera;
      }
      else
      if (strcmp(argv[argIndex], "--lynx") == 0)
      {
          browser = eBrowserLynx;
      }
      else
#endif
      if (strcmp(argv[argIndex], "-u") == 0)
      {
          submitTarget = argv[++argIndex];
      }
      else
      if (strcmp(argv[argIndex], "-d") == 0)
      {
          debugPlugins = true;
      }
      else
      if (strcmp(argv[argIndex], "-p") == 0)
      {
          dump = true;
      }
      else
      if (strcmp(argv[argIndex], "-r") == 0)
      {
          recurse = true;
      }
      else
      if (strcmp(argv[argIndex], "-a") == 0)
      {
          analyzeAll = true;
      }
      else
      if (strcmp(argv[argIndex], "-q") == 0)
      {
          quiet = true;
      }
      else
      if (strcmp(argv[argIndex], "-f") == 0)
      {
          strcpy(tagFile, argv[++argIndex]);
      }
      else
      if (strcmp(argv[argIndex], "-n") == 0)
      {
          autoSubmit = false;
      }
      else
      if (strcmp(argv[argIndex], "-c") == 0)
      {
          clearCache = true;
      }
      else
      if (strcmp(argv[argIndex], "--md5") == 0)
      {
          calculateMD5 = true;
      }
      else
      if (strcmp(argv[argIndex], "--crc32") == 0)
      {
          calculateCRC32 = true;
      }
      else
      if (strcmp(argv[argIndex], "-e") == 0)
      {
          argIndex++;
          if (argv[argIndex][0] != '.')
              sprintf(checkAsExt, ".%s", argv[argIndex]);
          else
              strcpy(checkAsExt, argv[argIndex]);
      }
      else
          break;
  }
  if (! tagFile[0] && !clearCache && argIndex == argc)
  {
      fprintf(stderr, "You must specify a file to bitcollide.\n");
      exit(-1);
  }

  if (submitTarget)
  {
      if (!strncmp(submitTarget, "http://", 7) || !strncmp(submitTarget, "https://", 8))
      {
          fprintf(stderr, "Submitting to: %s\n\n", submitTarget);
      }
      else
      {
          fprintf(stderr, "'%s' is not a valid URL. Ignoring.\n\n", submitTarget);
          submitTarget = NULL;
      }
  }

  /* set the cleanup handler */
  atexit(exit_cleanup);
  signal(SIGINT, signal_cleanup);

  bc = bitcollider_init(debugPlugins);
  if (!bc)
  {
      fprintf(stderr, "Cannot create bitcollider.\n");
      exit(-1);
  }

  set_calculateMD5(bc, calculateMD5);
  set_calculateCRC32(bc, calculateCRC32);

  if (clearCache)
  {
      clear_bitprint_cache(bc);
      bitcollider_shutdown(bc);
      bc = NULL;
      fprintf(stderr, "Bitprint cache cleared.\n");
      exit(0);
  }
  
  if( tagFile[0] )
  {
      submission = read_submission_from_file(bc, tagFile);
      if (!submission)
      {
          fprintf(stderr, "Cannot create submission.\n");
          bitcollider_shutdown(bc);
          bc = NULL;
          exit(-1);
      }
      if (submission->numBitprints == 0)
      {
          print_error(submission->bc, NULL);
          delete_submission(submission);
          bitcollider_shutdown(bc);
          bc = NULL;
          exit(-1);
      }
		  
      if (autoSubmit)
          set_auto_submit(submission, true);
  }
  else
  {
      submission = create_submission(bc);
      if (!submission)
      {
          fprintf(stderr, "Cannot create submission.\n");
          bitcollider_shutdown(bc);
          bc = NULL;
          exit(-1);
      }
  
      if (checkAsExt[0])
         set_check_as(submission, checkAsExt);
  
      if (autoSubmit)
         set_auto_submit(submission, true);
  
      if (!quiet)
         set_progress_callback(bc, progress);
  
      /* Check the file type and existence and then print out error info
         or analyze the file or dir */
      for(; argIndex < argc; argIndex++)
      {
         strcpy(fileName, argv[argIndex]);

         type = check_file_type(fileName);
         if (type == eOther)
         {
             fprintf(stderr, "%s is not a regular file. Skipping.\n", fileName);
             continue;
         }
         if (type == eNotFound)
         {
             fprintf(stderr, "Cannot find file/dir %s. Skipping.\n", fileName);
             continue;
         }

         if (type == eFile)
         {
             ret = analyze_file(submission, fileName, false);
             if (!ret)
             {
                 fprintf(stderr, "%s problem: %s\n", fileName, get_error(bc));
		 continue;
             }
         }
         else
         {
             if (!quiet)
                 fprintf(stderr, "Finding files:\n");
             count = recurse_dir(submission, fileName, analyzeAll, recurse);
             if (!quiet)
                 fprintf(stderr, "Analyzed %d files.\n\n", count);
         }
  
         print_warning(bc);
      }
  }
  
  if (!dump)
  {
      if (!submit_submission(submission, submitTarget, browser))
      {
          print_error(bc, "Submission failed");
          bitcollider_shutdown(bc);
          bc = NULL;
          exit(-1);
      }
      print_warning(bc);
      fprintf(stderr, "Information submitted. Thank you!\n");
  }
  else
  {
      fprintf(stderr, "\n");
      print_submission(submission);
  }
  delete_submission(submission);
  bitcollider_shutdown(bc);

  /* Set the bitcolider object to NULL, so that the cleanup handler doesn't
     clean it up again */
  bc = NULL;

  return 0;
}
