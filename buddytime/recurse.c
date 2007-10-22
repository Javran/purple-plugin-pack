/*************************************************************************
 * Recursion module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * Provides a function to recurse a directory and call a callback for each
 * file found.
 *************************************************************************/

#define _GNU_SOURCE
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "recurse.h"

#if 1
/* GLibc specific version. In this version, the entries are sorted */
/* We assume dirname ends in a /, prefix also unless empty */
static int
recurse_directory_int(char *dirname, char *prefix, DirRecurseMatch func, void *data)
{
    struct dirent **namelist;
    int ents;
    struct dirent *ent;
    int i;
    int ret = 0;

    if((ents = scandir(dirname, &namelist, 0, alphasort)) < 0)
        return -1;

    for (i = 0; i < ents; i++)
    {
        char *ptr;
        struct stat s;

        ent = namelist[i];
        asprintf(&ptr, "%s%s", dirname, ent->d_name);
        if(stat(ptr, &s) < 0)
        {
            free(ptr);
            continue;
        }

        if(S_ISREG(s.st_mode))
        {
            free(ptr);
            asprintf(&ptr, "%s%s", prefix, ent->d_name);
            ret = func(ptr, data);
        }
        else if(S_ISDIR(s.st_mode))
        {
            char *newdirname, *newprefix;

            if(ent->d_name[0] != '.')
            {
                asprintf(&newdirname, "%s%s/", dirname, ent->d_name);
                asprintf(&newprefix, "%s%s/", prefix, ent->d_name);
                ret = recurse_directory_int(newdirname, newprefix, func, data);
                free(newdirname);
                free(newprefix);
            }
        }
        free(ptr);
        if(ret < 0)
            break;
    }
    free(namelist);
    return 0;
}
#else
/* generic version, here they are unsorted */
/* We assume dirname ends in a /, prefix also unless empty */
static int
recurse_directory_int(char *dirname, char *prefix, DirRecurseMatch func, void *data)
{
    DIR *dir;
    struct dirent *ent;
    int ret = 0;

    dir = opendir(dirname);
    if(!dir)
        return -1;
    while ((ent = readdir(dir)) != NULL)
    {
        char *ptr;
        struct stat s;

        asprintf(&ptr, "%s%s", dirname, ent->d_name);
        if(stat(ptr, &s) < 0)
        {
            free(ptr);
            continue;
        }

        if(S_ISREG(s.st_mode))
        {
            free(ptr);
            asprintf(&ptr, "%s%s", prefix, ent->d_name);
            ret = func(ptr, data);
        }
        else if(S_ISDIR(s.st_mode))
        {
            char *newdirname, *newprefix;

            if(ent->d_name[0] != '.')
            {
                asprintf(&newdirname, "%s%s/", dirname, ent->d_name);
                asprintf(&newprefix, "%s%s/", prefix, ent->d_name);
                ret = recurse_directory_int(newdirname, newprefix, func, data);
                free(newdirname);
                free(newprefix);
            }
        }
        free(ptr);
        if(ret < 0)
            break;
    }
    closedir(dir);
    return ret;
}
#endif

int
recurse_directory(char *dirname, DirRecurseMatch func, void *data)
{
    char *newdirname = NULL;
    int ret;

    if(dirname[strlen(dirname) - 1] != '/')
        asprintf(&newdirname, "%s/", dirname);

    ret = recurse_directory_int(newdirname ? newdirname : dirname, "", func, data);

    if(newdirname)
        free(newdirname);
    return ret;
}
