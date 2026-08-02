#ifndef IWIN_H
#define IWIN_H

// Define all the Windows specific includes in one spot.
// There are problems if things are not defined in order with all of the preprocessor options.

#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include <winsock2.h>
#include <windows.h>

#undef ERROR
#undef TRANSPARENT
#undef CALLBACK

#endif

#endif // IWIN_H
