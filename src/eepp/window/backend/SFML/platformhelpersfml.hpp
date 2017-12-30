#ifndef EE_PLATFORMHELPERSFML_HPP
#define EE_PLATFORMHELPERSFML_HPP

#include <eepp/window/platformhelper.hpp>
#include <string>

namespace EE { namespace Window { namespace Backend { namespace SFML {

class PlatformHelperSFML : public PlatformHelper
{
	public:
		PlatformHelperSFML();

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
