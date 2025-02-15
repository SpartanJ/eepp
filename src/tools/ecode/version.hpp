#ifndef ECODE_VERSION_HPP
#define ECODE_VERSION_HPP

#include <eepp/config.hpp>
#include <string>

using namespace EE;

#define ECODE_MAJOR_VERSION 0
#define ECODE_MINOR_VERSION 7
#define ECODE_PATCH_LEVEL 1
/* ECODE_COMMIT_NUMBER 9999 is used for official releases, nightly builds (pre-releases) will
 * contain the number of commits after the last official release
 */
#define ECODE_COMMIT_NUMBER 9999
#define ECODE_CODENAME "Vastiva"

/** The compiled version of the library */
#define ECODE_VERSION( x )              \
	{                                   \
		x.major = ECODE_MAJOR_VERSION;  \
		x.minor = ECODE_MINOR_VERSION;  \
		x.patch = ECODE_PATCH_LEVEL;    \
		x.commit = ECODE_COMMIT_NUMBER; \
	}

#define ECODE_VERSIONNUM( X, Y, Z, C ) \
	( ( X ) * 100000000 + ( Y ) * 1000000 + ( Z ) * 10000 + ( C ) )

#define ECODE_COMPILEDVERSION                                                      \
	ECODE_VERSIONNUM( ECODE_MAJOR_VERSION, ECODE_MINOR_VERSION, ECODE_PATCH_LEVEL, \
					  ECODE_COMMIT_NUMBER )

#define ECODE_VERSION_ATLEAST( X, Y, Z, C ) \
	( ECODE_COMPILEDVERSION >= ECODE_VERSIONNUM( X, Y, Z, C ) )

namespace ecode {

class Version {
  public:
	Uint64 major;  /**< major version */
	Uint64 minor;  /**< minor version */
	Uint64 patch;  /**< update version */
	Uint64 commit; /**< commit number, used for nightly builds */

	/** @return The linked version of the library */
	static Version getVersion();

	/** @return The linked version number of the library */
	static Uint64 getVersionNum();

	/** @return The linked version number of the library */
	static std::string getVersionNumString();

	/** @return The library version name: "ecode version major.minor.patch" */
	static std::string getVersionFullName();

	/** @return The version codename */
	static std::string getCodename();

	/** @return The build time of the library */
	static inline std::string getBuildTime() {
		return std::string( __DATE__ ) + " " + std::string( __TIME__ );
	}

	static Uint64 getVersionNumFromTag( const std::string& tag );
};

} // namespace ecode

#endif
