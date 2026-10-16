#ifndef EE_VFS_PROVIDERS_LOCALPATH_HPP
#define EE_VFS_PROVIDERS_LOCALPATH_HPP

#include <string>
#include <string_view>

namespace EE { namespace VFS {

/** Native path helpers shared by the LocalProvider platform implementations. */
class LocalPath {
  public:
	/** @return The last component of a native path, trailing separators ignored ("/" for the
	 * root). */
	static std::string fileName( const std::string& path ) {
		std::string_view view( path );
		while ( view.size() > 1 && ( view.back() == '/' || view.back() == '\\' ) )
			view.remove_suffix( 1 );
		size_t pos = view.find_last_of( "/\\" );
		if ( pos == std::string_view::npos || view.size() == 1 )
			return std::string( view );
		return std::string( view.substr( pos + 1 ) );
	}
};

}} // namespace EE::VFS

#endif
