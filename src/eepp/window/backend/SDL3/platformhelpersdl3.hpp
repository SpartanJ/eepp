#ifndef EE_PLATFORMHELPERSDL3_HPP
#define EE_PLATFORMHELPERSDL3_HPP

#include <eepp/window/platformhelper.hpp>
#include <string>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API PlatformHelperSDL3 : public PlatformHelper {
  public:
	PlatformHelperSDL3();

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

}}}} // namespace EE::Window::Backend::SDL3

#endif
