/*************************************************************************
 * Header file for recursion module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *************************************************************************/

typedef int (*DirRecurseMatch)(char *filename, void *data);
int recurse_directory( char *dirname, DirRecurseMatch func, void *data );

