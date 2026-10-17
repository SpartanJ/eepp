#include <eepp/config.hpp>

#if EE_PLATFORM != EE_PLATFORM_WIN

#include <eepp/vfs/providers/localioscheduler.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#if defined( __linux__ )
#include <eepp/core/string.hpp>
#include <fstream>
#elif defined( __NetBSD__ )
#include <sys/statvfs.h>
#elif defined( __APPLE__ ) || defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || \
	defined( __DragonFly__ )
#include <sys/mount.h>
#include <sys/param.h>
#endif

namespace EE { namespace VFS {

namespace {

using NetworkMount = LocalIOScheduler::NetworkMount;

/** Every mount, local or not: a local file system can be mounted inside a network one. */
struct Mount {
	NetworkMount location;
	bool network;
};

#if defined( __linux__ )

/** File system types served by another machine. */
bool isNetworkType( const std::string& type ) {
	static const char* types[] = {
		"nfs",		   "nfs4",			 "cifs",	  "smb3",		 "smbfs",		  "ncpfs",
		"afs",		   "ceph",			 "glusterfs", "lustre",		 "davfs",		  "fuse.sshfs",
		"fuse.rclone", "fuse.glusterfs", "fuse.s3fs", "fuse.davfs2", "fuse.curlftpfs" };
	for ( const char* known : types ) {
		if ( type == known )
			return true;
	}
	return false;
}

/** mountinfo escapes spaces, tabs, newlines and backslashes as octal ("\040"). */
std::string unescapeMountPath( const std::string& path ) {
	std::string out;
	out.reserve( path.size() );
	for ( size_t i = 0; i < path.size(); ++i ) {
		auto octal = [&]( size_t at ) { return path[at] >= '0' && path[at] <= '7'; };
		if ( path[i] == '\\' && i + 3 < path.size() && octal( i + 1 ) && octal( i + 2 ) &&
			 octal( i + 3 ) ) {
			out += static_cast<char>( ( path[i + 1] - '0' ) * 64 + ( path[i + 2] - '0' ) * 8 +
									  ( path[i + 3] - '0' ) );
			i += 3;
		} else {
			out += path[i];
		}
	}
	return out;
}

/** /proc/self/mountinfo: "id parent major:minor root mountpoint options [tags] - type source
 * superoptions". Reading it never touches the mounts. */
std::vector<Mount> loadMounts() {
	std::vector<Mount> mounts;
	std::ifstream file( "/proc/self/mountinfo" );
	std::string line;
	while ( std::getline( file, line ) ) {
		const size_t separator = line.find( " - " );
		if ( separator == std::string::npos )
			continue;
		auto fields = String::split( line.substr( 0, separator ), ' ' );
		auto tail = String::split( line.substr( separator + 3 ), ' ' );
		if ( fields.size() < 5 || tail.size() < 2 )
			continue;
		mounts.push_back( { { unescapeMountPath( fields[4] ), unescapeMountPath( tail[1] ) },
							isNetworkType( tail[0] ) } );
	}
	return mounts;
}

#elif defined( __NetBSD__ )

std::vector<Mount> loadMounts() {
	std::vector<Mount> mounts;
	int count = getvfsstat( nullptr, 0, ST_NOWAIT );
	if ( count <= 0 )
		return mounts;
	std::vector<struct statvfs> entries( static_cast<size_t>( count ) );
	count = getvfsstat( entries.data(), entries.size() * sizeof( struct statvfs ), ST_NOWAIT );
	for ( int i = 0; i < count; ++i ) {
		mounts.push_back( { { entries[i].f_mntonname, entries[i].f_mntfromname },
							!( entries[i].f_flag & ST_LOCAL ) } );
	}
	return mounts;
}

#elif defined( __APPLE__ ) || defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || \
	defined( __DragonFly__ )

/** MNT_NOWAIT returns the kernel's cached information, without asking any file system. */
std::vector<Mount> loadMounts() {
	std::vector<Mount> mounts;
	int count = getfsstat( nullptr, 0, MNT_NOWAIT );
	if ( count <= 0 )
		return mounts;
	std::vector<struct statfs> entries( static_cast<size_t>( count ) );
	count = getfsstat( entries.data(), static_cast<int>( entries.size() * sizeof( struct statfs ) ),
					   MNT_NOWAIT );
	for ( int i = 0; i < count; ++i ) {
		mounts.push_back( { { entries[i].f_mntonname, entries[i].f_mntfromname },
							!( entries[i].f_flags & MNT_LOCAL ) } );
	}
	return mounts;
}

#else

std::vector<Mount> loadMounts() {
	return {};
}

#endif

/** Whether path is root or inside it. */
bool isUnder( const std::string& path, const std::string& root ) {
	if ( root == "/" )
		return !path.empty() && path[0] == '/';
	return path.compare( 0, root.size(), root ) == 0 &&
		   ( path.size() == root.size() || path[root.size()] == '/' );
}

/** The mount table, deepest mounts first. Mounts change rarely: it is read again at most every two
 * seconds. Calls fn( mounts ) under the table's lock. */
template <typename F> auto withMounts( F&& fn ) {
	static std::mutex mutex;
	static std::vector<Mount> mounts;
	static std::chrono::steady_clock::time_point loaded;
	std::lock_guard<std::mutex> lock( mutex );
	const auto now = std::chrono::steady_clock::now();
	if ( loaded == std::chrono::steady_clock::time_point() ||
		 now - loaded > std::chrono::seconds( 2 ) ) {
		mounts = loadMounts();
		// The deepest mount first: the one a path is on is the first that contains it. The
		// tables list mounts oldest first, and a later mount on the same point hides an earlier
		// one: reversed, the stable sort keeps the latest first.
		std::reverse( mounts.begin(), mounts.end() );
		std::stable_sort( mounts.begin(), mounts.end(), []( const auto& a, const auto& b ) {
			return a.location.root.size() > b.location.root.size();
		} );
		loaded = now;
	}
	return fn( mounts );
}

} // namespace

std::optional<LocalIOScheduler::NetworkMount>
LocalIOScheduler::findNetworkMount( const std::string& path ) {
	return withMounts( [&path]( const std::vector<Mount>& mounts ) -> std::optional<NetworkMount> {
		for ( const auto& mount : mounts ) {
			if ( isUnder( path, mount.location.root ) ) {
				if ( !mount.network )
					return std::nullopt;
				return mount.location;
			}
		}
		return std::nullopt;
	} );
}

std::vector<std::string> LocalIOScheduler::networkMountsIn( const std::string& directory ) {
	std::string parent = directory;
	while ( parent.size() > 1 && parent.back() == '/' )
		parent.pop_back();
	return withMounts( [&parent]( const std::vector<Mount>& mounts ) {
		std::vector<std::string> names;
		for ( const auto& mount : mounts ) {
			const std::string& root = mount.location.root;
			const size_t slash = root.rfind( '/' );
			if ( !mount.network || slash == std::string::npos || slash + 1 >= root.size() )
				continue;
			const std::string rootParent = slash == 0 ? "/" : root.substr( 0, slash );
			if ( rootParent == parent )
				names.push_back( root.substr( slash + 1 ) );
		}
		return names;
	} );
}

bool LocalIOScheduler::hasNetworkMounts() {
	return withMounts( []( const std::vector<Mount>& mounts ) {
		return std::any_of( mounts.begin(), mounts.end(),
							[]( const Mount& mount ) { return mount.network; } );
	} );
}

std::string LocalIOScheduler::resolvePath( const std::string& path, bool followLast,
										   const MountLookup& lookup ) {
	if ( path.empty() || path[0] != '/' || lookup( path ) )
		return path;
	// The components still to walk, the next one last.
	std::vector<std::string> pending;
	auto push = [&pending]( const std::string& names ) {
		std::vector<std::string> parts;
		size_t start = 0;
		while ( start <= names.size() ) {
			size_t end = names.find( '/', start );
			if ( end == std::string::npos )
				end = names.size();
			if ( end > start && names.compare( start, end - start, "." ) != 0 )
				parts.push_back( names.substr( start, end - start ) );
			start = end + 1;
		}
		pending.insert( pending.end(), parts.rbegin(), parts.rend() );
	};
	push( path );
	// The physical path walked so far ("" for the root).
	std::string resolved;
	int links = 0;
	while ( !pending.empty() ) {
		std::string name = std::move( pending.back() );
		pending.pop_back();
		if ( name == ".." ) {
			resolved.erase( resolved.empty() ? 0 : resolved.rfind( '/' ) );
			continue;
		}
		std::string candidate = resolved + "/" + name;
		auto withRest = [&] {
			for ( auto it = pending.rbegin(); it != pending.rend(); ++it )
				candidate += "/" + *it;
			return candidate;
		};
		// On a mount: the rest is the mount's, and asking it could block.
		if ( lookup( candidate ) )
			return withRest();
		if ( pending.empty() && !followLast )
			return candidate;
		struct stat st;
		if ( ::lstat( candidate.c_str(), &st ) != 0 )
			return withRest();
		if ( !S_ISLNK( st.st_mode ) ) {
			resolved = std::move( candidate );
			continue;
		}
		char target[4096];
		const ssize_t length = ::readlink( candidate.c_str(), target, sizeof( target ) );
		// SYMLOOP_MAX is at least 8; Linux follows 40.
		if ( length <= 0 || ++links > 40 )
			return path;
		if ( target[0] == '/' )
			resolved.clear();
		push( std::string( target, static_cast<size_t>( length ) ) );
	}
	return resolved.empty() ? "/" : resolved;
}

}} // namespace EE::VFS

#endif
