#ifndef EE_VERSION_HPP
#define EE_VERSION_HPP

#include <eepp/config.hpp>
#include <string>

#define EEPP_MAJOR_VERSION 2
#define EEPP_MINOR_VERSION 8
#define EEPP_PATCH_LEVEL 4
#define EEPP_CODENAME "Siddhi"

/** The compiled version of the library */
#define EEPP_VERSION( x )             \
	{                                 \
		x.major = EEPP_MAJOR_VERSION; \
		x.minor = EEPP_MINOR_VERSION; \
		x.patch = EEPP_PATCH_LEVEL;   \
	}

#define EEPP_VERSIONNUM( X, Y, Z ) ( ( X ) * 1000 + ( Y ) * 100 + ( Z ) )

#define EEPP_COMPILEDVERSION \
	EEPP_VERSIONNUM( EEPP_MAJOR_VERSION, EEPP_MINOR_VERSION, EEPP_PATCH_LEVEL )

#define EEPP_VERSION_ATLEAST( X, Y, Z ) ( EEPP_COMPILEDVERSION >= EEPP_VERSIONNUM( X, Y, Z ) )

namespace EE {

class EE_API Version {
  public:
	Uint8 major; /**< major version */
	Uint8 minor; /**< minor version */
	Uint8 patch; /**< update version */

	/** @return The linked version of the library */
	static Version getVersion();

	/** @return The linked version number of the library */
	static Uint32 getVersionNum();

	/** @return The library version name: "eepp version major.minor.patch" */
	static std::string getVersionName();

	/** @return The version codename */
	static std::string getCodename();

	/** @return The build time of the library */

	static inline std::string getBuildTime() {
		return std::string( __DATE__ ) + " " + std::string( __TIME__ );
	}
};

} // namespace EE

#endif
