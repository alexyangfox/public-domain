/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: browser.c,v 1.3 2004/02/02 01:11:53 mayhemchaos Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "../config.h"
#include "bitcollider.h"

b_bool launch(const char *url, const char *command);
b_bool launch_using_envvar(const char *url);
b_bool is_netscape_running(void);

b_bool launch_browser(const char* url, BrowserEnum browser)
{
    char         command[1024];
    char        *browser_env;

#if DARWIN
    strcpy(command, "open %s");
#else

    browser_env = getenv("BROWSER");
    if (browser_env && strlen(browser_env) > 0)
        return launch_using_envvar(url);

    switch(browser)
    {
        case eBrowserNetscape:
           if (is_netscape_running())
                strcpy(command, "netscape -raise -remote "
                                "\"openURL(file://%s,new-window)\""); 
           else
                strcpy(command, "netscape file://%s &");

           break;
        case eBrowserMozilla:
           strcpy(command, "mozilla file://%s &");
           break;
        case eBrowserKonqueror:
           strcpy(command, "konqueror file://%s &");
           break;
        case eBrowserOpera:
           strcpy(command, "opera file://%s &");
           break;
        case eBrowserLynx:
           strcpy(command, "lynx file://%s");
           break;
    }
#endif

    return launch(url, command);
}

int launch_using_envvar(const char *url)
{
    char *browser, *token;
    int   ret = 0;

    browser = strdup(getenv("BROWSER"));
    token = strtok(browser, ":");
    while(*token)
    {
        ret = launch(url, token);
        if (ret)
           break;

        token = strtok(NULL, ":");
    }
    free(browser);

    return ret;
}

int launch(const char *url, const char *browser)
{
    char *command;
    int   ret;

    command = malloc(strlen(browser) + strlen(url) + 10);
    sprintf(command, browser, url);

    ret = system(command) >> 8;
    if (ret == 127)
       ret = 0;
    else
       ret = 1;

    free(command);
    return ret;
}

b_bool is_netscape_running(void)
{
    struct stat  sb;
    char        *home, lockfile[1024];

    home = getenv("HOME");
    if (!home) 
        return false;

    sprintf(lockfile,"%.200s/.netscape/lock",home);
    return (lstat(lockfile, &sb) != -1);
}
