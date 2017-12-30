#ifndef EE_PLATFORMHELPERSDL2_HPP
#define EE_PLATFORMHELPERSDL2_HPP

#include <eepp/window/platformhelper.hpp>
#include <string>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class PlatformHelperSDL2 : public PlatformHelper
{
	public:
		PlatformHelperSDL2();

#if EE_PLATFORM == EE_PLATFORM_ANDROID
		void * getActivity();

		void * getJNIEnv();

		std::string getExternalStoragePath();

		std::string getInternalStoragePath();

		std::string getApkPath();

		bool isExternalStorageReadable();

		bool isExternalStorageWritable();
#endif
};

}}}}

#endif
