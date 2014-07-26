#include <eepp/version.hpp>
#include <eepp/core/string.hpp>

namespace EE {

Version Version::GetVersion() {
	Version ver;
	EEPP_VERSION(&ver);
	return ver;
}

Uint32 Version::GetVersionNum() {
	Version ver = GetVersion();
	return EEPP_VERSIONNUM(ver.major, ver.minor, ver.patch);
}

std::string Version::GetVersionName() {
	Version ver = GetVersion();
	return String::StrFormated( "eepp version %d.%d.%d", ver.major, ver.minor, ver.patch );
}

std::string Version::GetCodename() {
	return std::string( EEPP_CODENAME );
}

std::string Version::GetBuildTime() {
	return std::string( __DATE__ ) + " " + std::string( __TIME__ );
}

} 
