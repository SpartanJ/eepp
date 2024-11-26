#include "version.hpp"
#include <cinttypes>
#include <eepp/core/string.hpp>

namespace ecode {

Version Version::getVersion() {
	Version ver;
	ECODE_VERSION( ver );
	return ver;
}

Uint64 Version::getVersionNum() {
	Version ver = getVersion();
	return ECODE_VERSIONNUM( ver.major, ver.minor, ver.patch, ver.commit );
}
std::string Version::getVersionNumString() {
	Version ver = getVersion();
	if ( ver.commit > 0 && ver.commit < 9999 )
		return String::format( "%" PRIu64 ".%" PRIu64 ".%" PRIu64 "-%" PRIu64, ver.major, ver.minor,
							   ver.patch, ver.commit );
	return String::format( "%" PRIu64 ".%" PRIu64 ".%" PRIu64, ver.major, ver.minor, ver.patch );
}

std::string Version::getVersionFullName() {
	Version ver = getVersion();
	if ( ver.commit > 0 && ver.commit < 9999 )
		return String::format( "ecode version %s nightly", getVersionNumString() );
	return String::format( "ecode version %s", getVersionNumString() );
}

std::string Version::getCodename() {
	return std::string( ECODE_CODENAME );
}

Uint64 Version::getVersionNumFromTag( const std::string& tag ) {
	auto tagPart = String::split( tag, '-' );
	if ( tagPart.size() == 2 ) {
		auto versionPart = String::split( tagPart[1], '.' );
		if ( versionPart.size() == 3 ) {
			Version ver;
			if ( String::fromString( ver.major, versionPart[0] ) &&
				 String::fromString( ver.minor, versionPart[1] ) &&
				 String::fromString( ver.patch, versionPart[2] ) ) {
				return ECODE_VERSIONNUM( ver.major, ver.minor, ver.patch,
										 /* tags don't count commits, all stable releases will
											always have 9999 as commit number */
										 9999 );
			}
		}
	}
	return 0;
}

} // namespace ecode
