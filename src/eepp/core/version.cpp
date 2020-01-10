#include <eepp/core/string.hpp>
#include <eepp/version.hpp>

namespace EE {

Version Version::getVersion() {
	Version ver;
	EEPP_VERSION( &ver );
	return ver;
}

Uint32 Version::getVersionNum() {
	Version ver = getVersion();
	return EEPP_VERSIONNUM( ver.major, ver.minor, ver.patch );
}

std::string Version::getVersionName() {
	Version ver = getVersion();
	return String::format( "eepp version %d.%d.%d", ver.major, ver.minor, ver.patch );
}

std::string Version::getCodename() {
	return std::string( EEPP_CODENAME );
}

std::string Version::getBuildTime() {
	return std::string( __DATE__ ) + " " + std::string( __TIME__ );
}

} // namespace EE
