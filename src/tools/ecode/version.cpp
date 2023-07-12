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

std::string Version::getVersionNumString() {
	Version ver = getVersion();
	return String::format( "%d.%d.%d", ver.major, ver.minor, ver.patch );
}

std::string Version::getVersionFullName() {
	Version ver = getVersion();
	return String::format( "ecode version %d.%d.%d", ver.major, ver.minor, ver.patch );
}

std::string Version::getTagName() {
	Version ver = getVersion();
	return String::format( "ecode-%d.%d.%d", ver.major, ver.minor, ver.patch );
}

std::string Version::getCodename() {
	return std::string( ECODE_CODENAME );
}

Uint32 Version::getVersionNumFromTag( const std::string& tag ) {
	auto tagPart = String::split( tag, '-' );
	if ( tagPart.size() == 2 ) {
		auto versionPart = String::split( tagPart[1], '.' );
		if ( versionPart.size() == 3 ) {
			Version ver;
			if ( String::fromString( ver.major, versionPart[0] ) &&
				 String::fromString( ver.minor, versionPart[1] ) &&
				 String::fromString( ver.patch, versionPart[2] ) ) {
				return ECODE_VERSIONNUM( ver.major, ver.minor, ver.patch );
			}
		}
	}
	return 0;
}

} // namespace ecode
