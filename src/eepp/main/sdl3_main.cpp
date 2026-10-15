#if defined( _WIN32 ) && defined( _MSC_VER )

// Compile once per executable. SDL supplies console and GUI entry points and UTF-8 arguments.
#include <SDL3/SDL_main.h>

#endif
