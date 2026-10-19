#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <eepp/vfs/providers/localioscheduler.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace EE { namespace VFS {

namespace {

bool isSeparator( char c ) {
	return c == '\\' || c == '/';
}

/** The end of the share in a UNC path ("\\server\share"), counted from its server name. */
size_t uncShareEnd( const std::string& path, size_t server ) {
	size_t slash = path.find_first_of( "\\/", server );
	if ( slash == std::string::npos || slash == server )
		return std::string::npos;
	size_t end = path.find_first_of( "\\/", slash + 1 );
	return end == std::string::npos ? path.size() : end;
}

} // namespace

std::optional<LocalIOScheduler::NetworkMount>
LocalIOScheduler::findNetworkMount( const std::string& path ) {
	// \\?\UNC\server\share\... (long paths) and \\server\share\...
	size_t server = std::string::npos;
	if ( path.size() > 8 && path.compare( 0, 8, "\\\\?\\UNC\\" ) == 0 )
		server = 8;
	else if ( path.size() > 2 && isSeparator( path[0] ) && isSeparator( path[1] ) &&
			  path[2] != '?' && path[2] != '.' )
		server = 2;
	if ( server != std::string::npos ) {
		size_t end = uncShareEnd( path, server );
		if ( end == std::string::npos )
			return std::nullopt;
		// One lane per share, however the path spells it ("//server/share", the long form).
		std::string share = "\\\\" + path.substr( server, end - server );
		for ( auto& c : share ) {
			if ( c == '/' )
				c = '\\';
		}
		return NetworkMount{ share, share };
	}
	// A mapped network drive. GetDriveType answers from the drive table, without the server.
	if ( path.size() >= 2 && path[1] == ':' ) {
		const wchar_t root[] = { static_cast<wchar_t>( path[0] ), L':', L'\\', 0 };
		if ( GetDriveTypeW( root ) == DRIVE_REMOTE )
			return NetworkMount{ path.substr( 0, 2 ), path.substr( 0, 2 ) };
	}
	return std::nullopt;
}

std::vector<std::string> LocalIOScheduler::networkMountsIn( const std::string& ) {
	// Network shares are reached through UNC paths and drive letters, never mounted inside a
	// folder.
	return {};
}

bool LocalIOScheduler::hasNetworkMounts() {
	// resolvePath() does not follow links here yet: nothing to resolve them for.
	return false;
}

std::string LocalIOScheduler::resolvePath( const std::string& path, bool, const MountLookup& ) {
	return path;
}

}} // namespace EE::VFS

#endif
