/* Launch! 3.79 standalone Configuration program.
 *
 * CONFIG_PROGRAM selects the Configuration-only build of the canonical
 * shared implementation.  Core-only menu, File Open, Explore, SysBar,
 * shutdown, saver engine and command-dispatch blocks are excluded at
 * preprocessing time; !CONFIG is no longer a second copy of !.EXE.
 */
#define CONFIG_PROGRAM 1
#include "LAUNCH.C"
