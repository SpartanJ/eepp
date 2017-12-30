#include <eepp/window/backend/SFML/platformhelpersfml.hpp>

namespace EE { namespace Window { namespace Backend { namespace SFML {

PlatformHelperSFML::PlatformHelperSFML()
{
}

#if EE_PLATFORM == EE_PLATFORM_ANDROID
void * PlatformHelperSFML::getActivity() {
	return NULL;
}

void * PlatformHelperSFML::getJNIEnv() {
	return NULL;
}

std::string PlatformHelperSFML::getExternalStoragePath() {
	return std::string();
}

std::string PlatformHelperSFML::getInternalStoragePath() {
	return std::string();
}

std::string PlatformHelperSFML::getApkPath() {
	return std::string();
}

bool PlatformHelperSFML::isExternalStorageReadable() {
	return true;
}

bool PlatformHelperSFML::isExternalStorageWritable() {
	return true;
}
#endif

}}}}
