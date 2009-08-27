/*************************************************************************
 * Recursion test module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * Code to test the recursion module.
 *************************************************************************/

#include <stdio.h>

#include "recurse.h"

int
process_entry(char *str, void *ptr)
{
    printf("%s\n", str);
    return 0;
}

int
main()
{
    recurse_directory("/usr/share/zoneinfo", process_entry, main);
    return 0;
}
