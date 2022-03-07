#include <eepp/window/backend/SDL2/base.hpp>
#include <eepp/window/backend/SDL2/platformhelpersdl2.hpp>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <jni.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

PlatformHelperSDL2::PlatformHelperSDL2() {}

bool PlatformHelperSDL2::openURL( const std::string& url ) {
#if SDL_VERSION_ATLEAST(2,0,14)
	return SDL_OpenURL( url.c_str() ) == 0;
#else
	return false;
#endif
}

#if EE_PLATFORM == EE_PLATFORM_ANDROID
void* PlatformHelperSDL2::getActivity() {
	return SDL_AndroidGetActivity();
}

void* PlatformHelperSDL2::getJNIEnv() {
	return SDL_AndroidGetJNIEnv();
}

std::string PlatformHelperSDL2::getExternalStoragePath() {
	return std::string( SDL_AndroidGetExternalStoragePath() );
}

std::string PlatformHelperSDL2::getInternalStoragePath() {
	return std::string( SDL_AndroidGetInternalStoragePath() );
}

std::string PlatformHelperSDL2::getApkPath() {
	static std::string apkPath = "";

	if ( "" == apkPath ) {
		jmethodID mid;
		jobject context;
		jstring fileObject;
		const char* path;

		JNIEnv* env = (JNIEnv*)getJNIEnv();

		jobject activity = (jobject)getActivity();
		jclass ActivityClass = env->GetObjectClass( activity );

		// context = SDLActivity.getContext();
		mid = env->GetStaticMethodID( ActivityClass, "getContext", "()Landroid/content/Context;" );

		context = env->CallStaticObjectMethod( ActivityClass, mid );

		// fileObj = context.getFilesDir();
		mid = env->GetMethodID( env->GetObjectClass( context ), "getPackageCodePath",
								"()Ljava/lang/String;" );

		fileObject = (jstring)env->CallObjectMethod( context, mid );

		jboolean isCopy;
		path = env->GetStringUTFChars( fileObject, &isCopy );

		apkPath = std::string( path );

		env->ReleaseStringUTFChars( fileObject, path );
		env->DeleteLocalRef( activity );
		env->DeleteLocalRef( ActivityClass );
	}

	return apkPath;
}

bool PlatformHelperSDL2::isExternalStorageReadable() {
	return 0 != ( SDL_AndroidGetExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_READ );
}

bool PlatformHelperSDL2::isExternalStorageWritable() {
	return 0 != ( SDL_AndroidGetExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_WRITE );
}

#endif

}}}} // namespace EE::Window::Backend::SDL2
