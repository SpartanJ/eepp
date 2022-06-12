#include "version.hpp"
#include <eepp/core/string.hpp>

namespace ecode {

Version Version::getVersion() {
	Version ver;
	ECODE_VERSION( &ver );
	return ver;
}

Uint32 Version::getVersionNum() {
	Version ver = getVersion();
	return ECODE_VERSIONNUM( ver.major, ver.minor, ver.patch );
}

std::string Version::getVersionName() {
	Version ver = getVersion();
	return String::format( "ecode version %d.%d.%d", ver.major, ver.minor, ver.patch );
}

std::string Version::getCodename() {
	return std::string( ECODE_CODENAME );
}

} // namespace ecode
