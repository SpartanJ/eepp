#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/backend/SDL3/platformhelpersdl3.hpp>
#include <eepp/system/log.hpp>

using namespace EE::System;

#ifdef EE_BACKEND_SDL3

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
EM_JS( void, emscripten_open_url, ( const char* msg ),
	   { window.open( UTF8ToString( msg ), 'blank' ); } );
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

PlatformHelperSDL3::PlatformHelperSDL3() {}

bool PlatformHelperSDL3::openURL( const std::string& url ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	emscripten_open_url( url.c_str() );
	return true;
#else
	bool res = SDL_OpenURL( url.c_str() );
	if ( !res ) {
		Log::error( "PlatformHelperSDL3::openURL: Failed with error - %s", SDL_GetError() );
	}
	return res;
#endif
}

char* PlatformHelperSDL3::iconv( const char* tocode, const char* fromcode, const char* inbuf,
								 size_t inbytesleft ) {
	return SDL_iconv_string( tocode, fromcode, inbuf, inbytesleft );
}

void PlatformHelperSDL3::iconvFree( char* buf ) {
	SDL_free( buf );
}

#if EE_PLATFORM == EE_PLATFORM_ANDROID
void* PlatformHelperSDL3::getActivity() {
	return SDL_GetAndroidActivity();
}

void* PlatformHelperSDL3::getJNIEnv() {
	return (void*)SDL_GetAndroidJNIEnv();
}

std::string PlatformHelperSDL3::getExternalStoragePath() {
	return std::string( SDL_GetAndroidExternalStoragePath() );
}

std::string PlatformHelperSDL3::getInternalStoragePath() {
	return std::string( SDL_GetAndroidInternalStoragePath() );
}

std::string PlatformHelperSDL3::getApkPath() {
	return "";
}

bool PlatformHelperSDL3::isExternalStorageReadable() {
	return 0 != ( SDL_GetAndroidExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_READ );
}

bool PlatformHelperSDL3::isExternalStorageWritable() {
	return 0 != ( SDL_GetAndroidExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_WRITE );
}
#endif

}}}} // namespace EE::Window::Backend::SDL3

#endif
