/*************************************************************************
 * Timezone test module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * Code to test the timezone module.
 *************************************************************************/

//#include "private.h"
//#include "tzfile.h"
#include "localtime.h"

int
main()
{
    struct state *state;
    time_t now = time(NULL);
    struct tm tm;

    state = timezone_load("Australia/Sydney");

    if(!state)
        return 0;

    localsub(&now, 0, &tm, state);
    gmtsub(&now, 0, &tm);
    return 0;
}
