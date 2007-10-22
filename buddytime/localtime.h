/*************************************************************************
 * Header file for timezone module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *************************************************************************/

#include <time.h>

struct state;

struct state *timezone_load (const char * name);
struct tm *	gmtsub (const time_t * timep, long offset,
				struct tm * tmp);
struct tm *	localsub (const time_t * timep, long offset,
				struct tm * tmp, struct state *sp);
int tz_init(const char *zoneinfo_dir);
