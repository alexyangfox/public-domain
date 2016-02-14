/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: plugin_man.c,v 1.16 2004/02/03 01:11:07 mayhemchaos Exp $
 */
/*------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#ifndef NO_PLUGINS
#include "libltdl/ltdl.h"
#endif

#include "bitcollider.h"
#include "plugin_man.h"

/*------------------------------------------------------------------------- */

#define MAX_PATH    1024
#define DB printf("%s:%d\n", __FILE__, __LINE__);

/*------------------------------------------------------------------------- */
#define ERROR_FILENOTFOUND   "File not found."

/*------------------------------------------------------------------------- */

Bitcollider *init_plugins(void)
{
   Bitcollider *bc;

#ifndef NO_PLUGINS
   lt_dlinit();
#endif

   bc = malloc(sizeof(Bitcollider));
   memset(bc, 0, sizeof(Bitcollider));

   return bc;
}

void shutdown_plugins(Bitcollider *bc)
{
   free(bc);
#ifndef NO_PLUGINS
   lt_dlexit();
#endif
}

#ifndef NO_PLUGINS
int load_plugins(Bitcollider *bc, const char *path, b_bool printDebugInfo)
{
   DIR           *dir;
   struct dirent *entry;
   int            count = 0, j;
   char          *ptr, file[MAX_PATH], init_func[255];
   PluginMethods *(*init_function)(void);

   dir = opendir(path);
   if (dir == NULL)
      return 0;

   for(;;)
   {
       /* Scan the given dir for plugins */
       entry = readdir(dir);
       if (!entry)
          break;

       ptr = strrchr(entry->d_name, '.');
       if (!ptr || strcasecmp(ptr, ".bcp"))
          continue;

       if (printDebugInfo)
           fprintf(stderr, "  %s: ", entry->d_name);
       sprintf(file, "%s/%s", path, entry->d_name);

       /* Found one, lets open it */
       bc->plugins[bc->numPluginsLoaded].handle = lt_dlopen(file);
       if (bc->plugins[bc->numPluginsLoaded].handle == NULL)
       {
           if (printDebugInfo)
               fprintf(stderr, "Cannot load plugin %s. (%s)\n", 
               file, lt_dlerror());
           continue;
       }
       bc->plugins[bc->numPluginsLoaded].file = strdup(entry->d_name);
       strcpy(init_func, entry->d_name);
       ptr = strrchr(init_func, '.');
       if (ptr)
           *ptr = 0;
       strcat(init_func, "_init_plugin");

       /* Opened plugin ok, now locate our entry function */
       init_function = lt_dlsym(bc->plugins[bc->numPluginsLoaded].handle, init_func);
       if (init_function == NULL)
       {
           if (printDebugInfo)
               fprintf(stderr, "Cannot find entry point in %s (%s).\n", file, lt_dlerror());
           lt_dlclose(bc->plugins[bc->numPluginsLoaded].handle);
           continue;
       }

       /* Init the plugin and get the methods provided by the plugin */
       bc->plugins[bc->numPluginsLoaded].methods = (*init_function)();
       if (bc->plugins[bc->numPluginsLoaded].methods == NULL)
       {
           lt_dlclose(bc->plugins[bc->numPluginsLoaded].handle);
           if (printDebugInfo)
               fprintf(stderr, "Cannot retrieve supported methods from %s.\n", 
                               file);
           continue;
       }

       /* Now get the formats handled by the plugin */
       bc->plugins[bc->numPluginsLoaded].formats = 
           bc->plugins[bc->numPluginsLoaded].methods->get_supported_formats();

       if (printDebugInfo)
       {
           fprintf(stderr, "%s ", bc->plugins[bc->numPluginsLoaded].
                                  methods->get_name());
           fprintf(stderr, "(%s)\n", bc->plugins[bc->numPluginsLoaded].
                                  methods->get_version());

       }

       /* Check to make sure that the given plugin hasnt already been loaded */
       for(j = 0; j < bc->numPluginsLoaded; j++)
       {
           if (!strcmp(bc->plugins[j].file, 
                       bc->plugins[bc->numPluginsLoaded].file))
           {
               if (printDebugInfo)
                  fprintf(stderr, "  [Plugin %s has already been loaded. "
                       "Skipping.]\n", bc->plugins[bc->numPluginsLoaded].file);
                
               bc->plugins[bc->numPluginsLoaded].methods->shutdown_plugin();
               lt_dlclose(bc->plugins[bc->numPluginsLoaded].handle);
               bc->plugins[bc->numPluginsLoaded].handle = NULL;
               bc->plugins[bc->numPluginsLoaded].methods = NULL;
               free(bc->plugins[bc->numPluginsLoaded].file);
               bc->plugins[bc->numPluginsLoaded].file = NULL;

               break;
           }
       }

       /* If we didn't already this plugin loaded, increment our counters */
       if (j == bc->numPluginsLoaded)
       {
          bc->numPluginsLoaded++;
          count++;
       }
   }
   closedir(dir);

   return count;
}

void unload_plugins(Bitcollider *bc)
{
   for(;;)
   {
       bc->numPluginsLoaded--;
       if (bc->numPluginsLoaded < 0)
          break;

       if (bc->plugins[bc->numPluginsLoaded].handle)
       {
           bc->plugins[bc->numPluginsLoaded].methods->shutdown_plugin();
           lt_dlclose(bc->plugins[bc->numPluginsLoaded].handle);
           bc->plugins[bc->numPluginsLoaded].handle = NULL;
           bc->plugins[bc->numPluginsLoaded].methods = NULL;
           free(bc->plugins[bc->numPluginsLoaded].file);
           bc->plugins[bc->numPluginsLoaded].file = NULL;
       }
   }
}
#else
int load_plugins(Bitcollider *bc, const char *path, b_bool printDebugInfo)
{
   return 0;
}

void unload_plugins(Bitcollider *bc)
{
}
#endif

int get_num_plugins(Bitcollider *bc)
{
   return bc->numPluginsLoaded;
}

PluginMethods *get_plugin(Bitcollider *bc, const char *extension)
{
   int              i;
   SupportedFormat *format;

   for(i = 0; i < bc->numPluginsLoaded; i++)
   {
      for(format = bc->plugins[i].formats; format && 
          format->fileExtension; format++)
      {
         if (strcasecmp(format->fileExtension, extension) == 0)
             return bc->plugins[i].methods;
      }
   }

   return NULL;
}
