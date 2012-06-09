#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32) && !defined(_XBOX)
 #if defined(AL_BUILD_LIBRARY)
  #define AL_API __declspec(dllexport)
 #else
  #define AL_API __declspec(dllimport)
 #endif
#else
 #if defined(AL_BUILD_LIBRARY) && defined(HAVE_GCC_VISIBILITY)
  #define AL_API __attribute__((visibility("protected")))
 #else
  #define AL_API extern
 #endif
#endif

#if defined(_WIN32)
 #define AL_APIENTRY __cdecl
#else
 #define AL_APIENTRY
#endif

#if defined(TARGET_OS_MAC) && TARGET_OS_MAC
 #pragma export on
#endif 

AL_API void AL_APIENTRY al_android_pause_playback();
AL_API void AL_APIENTRY al_android_resume_playback();

AL_API void AL_SetJavaVM( void * vm );

#if defined(TARGET_OS_MAC) && TARGET_OS_MAC
 #pragma export off
#endif

#if defined(__cplusplus)
}  /* extern "C" */
#endif