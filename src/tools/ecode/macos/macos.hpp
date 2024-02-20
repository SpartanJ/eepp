#ifndef ECODE_MACOS_MACOS
#define ECODE_MACOS_MACOS

#ifdef __cplusplus
extern "C" {
#endif

void macOS_createApplicationMenus();

void macOS_enableScrollMomentum();

void macOS_removeTitleBarSeparator( void* nsWindow );

void macOS_changeTitleBarColor( void* window, double red, double green, double blue );

int macOS_isDarkModeEnabled();

#ifdef __cplusplus
}
#endif

#endif
