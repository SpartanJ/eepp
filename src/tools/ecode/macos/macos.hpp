#ifndef ECODE_MACOS_MACOS
#define ECODE_MACOS_MACOS

#ifdef __cplusplus
extern "C" {
#endif

void macOS_createApplicationMenus();

void macOS_enableScrollMomentum();

void macOS_removeTitleBarSeparator( void* nsWindow );

#ifdef __cplusplus
}
#endif

#endif
