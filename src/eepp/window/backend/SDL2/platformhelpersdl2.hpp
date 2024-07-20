#ifndef EE_PLATFORMHELPERSDL2_HPP
#define EE_PLATFORMHELPERSDL2_HPP

#include <eepp/window/platformhelper.hpp>
#include <string>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API PlatformHelperSDL2 : public PlatformHelper {
  public:
	PlatformHelperSDL2();

	bool openURL( const std::string& url );

	char* iconv( const char* tocode, const char* fromcode, const char* inbuf, size_t inbytesleft );

	void iconvFree( char* buf );

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	void* getActivity();

	void* getJNIEnv();

	std::string getExternalStoragePath();

	std::string getInternalStoragePath();

	std::string getApkPath();

	bool isExternalStorageReadable();

	bool isExternalStorageWritable();
#endif
};

}}}} // namespace EE::Window::Backend::SDL2

#endif
