#include <eepp/vfs/path.hpp>

#include <eepp/core/string.hpp>

namespace EE { namespace VFS {

bool VFSPath::validURI( const URI& uri ) {
	return uri.getPath().find( '\0' ) == std::string::npos &&
		   uri.getHost().find( '\0' ) == std::string::npos &&
		   uri.getUserInfo().find( '\0' ) == std::string::npos;
}

bool VFSPath::validName( std::string_view name, const URI& directory ) {
	if ( name.empty() || name == "." || name == ".." ||
		 name.find_first_of( std::string_view( "/\0", 2 ) ) != std::string_view::npos )
		return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( directory.getScheme() == "file" ) {
		if ( name.back() == '.' || name.back() == ' ' ||
			 name.find_first_of( "\\:<>\"|?*" ) != std::string_view::npos )
			return false;
		for ( const unsigned char c : name ) {
			if ( c < 32 )
				return false;
		}
		// Windows recognizes device names even with an extension (for example CON.txt).
		auto base = name.substr( 0, name.find( '.' ) );
		while ( !base.empty() && base.back() == ' ' )
			base.remove_suffix( 1 );
		for ( const auto reserved : { "CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$" } ) {
			if ( String::iequals( base, reserved ) )
				return false;
		}
		if ( base.size() >= 4 && ( String::iequals( base.substr( 0, 3 ), "COM" ) ||
								   String::iequals( base.substr( 0, 3 ), "LPT" ) ) ) {
			const auto number = base.substr( 3 );
			if ( ( number.size() == 1 && number[0] >= '1' && number[0] <= '9' ) || number == "¹" ||
				 number == "²" || number == "³" )
				return false;
		}
	}
#else
	(void)directory;
#endif
	return true;
}

}} // namespace EE::VFS
