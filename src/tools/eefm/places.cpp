#include "places.hpp"
#include "trash.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#include <fstream>
#include <iterator>
#include <map>
#include <nlohmann/json.hpp>

#if defined( EE_PLATFORM_POSIX )
#include <climits> // PATH_MAX
#include <cstdlib>
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD
#include <sys/mount.h>
#include <sys/param.h>
#if defined( __NetBSD__ )
#include <sys/statvfs.h>
#endif
#endif

using json = nlohmann::json;
using namespace EE;
using namespace EE::System;
using namespace EE::VFS;

namespace eefm {

namespace {

std::string withSlash( std::string path ) {
	FileSystem::dirAddSlashAtEnd( path );
	return path;
}

/** XDG user directories (~/.config/user-dirs.dirs), falling back to the English names. */
std::map<std::string, std::string> userDirs( const std::string& home ) {
	std::map<std::string, std::string> dirs = {
		{ "DESKTOP", home + "Desktop" },	{ "DOCUMENTS", home + "Documents" },
		{ "DOWNLOAD", home + "Downloads" }, { "MUSIC", home + "Music" },
		{ "PICTURES", home + "Pictures" },	{ "VIDEOS", home + "Videos" },
	};
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	std::string configHome = Sys::getEnv( "XDG_CONFIG_HOME" );
	if ( configHome.empty() )
		configHome = home + ".config";
	std::string data;
	if ( FileSystem::fileGet( withSlash( configHome ) + "user-dirs.dirs", data ) ) {
		for ( auto line : String::split( data, '\n' ) ) {
			String::trimInPlace( line );
			if ( !String::startsWith( line, "XDG_" ) )
				continue;
			size_t eq = line.find( '=' );
			size_t suffix = line.find( "_DIR" );
			if ( eq == std::string::npos || suffix == std::string::npos || suffix > eq )
				continue;
			std::string key = line.substr( 4, suffix - 4 );
			std::string value = line.substr( eq + 1 );
			if ( value.size() >= 2 && value.front() == '"' && value.back() == '"' )
				value = value.substr( 1, value.size() - 2 );
			String::replaceAll( value, "$HOME/", home );
			String::replaceAll( value, "$HOME", home );
			// A user dir equal to $HOME means "disabled".
			if ( value.empty() || withSlash( value ) == home )
				continue;
			dirs[key] = value;
		}
	}
#endif
	return dirs;
}

/** File systems mounted from another machine: listed in Remote, like Dolphin's network shares. */
bool isNetworkFileSystem( const std::string& type ) {
	static const char* types[] = { "nfs",			"nfs4",			"cifs",			 "smb3",
								   "smbfs",			"afpfs",		"webdav",		 "davfs",
								   "fuse.sshfs",	"fusefs.sshfs", "sshfs",		 "fuse.rclone",
								   "fusefs.rclone", "ceph",			"fuse.glusterfs" };
	for ( const char* known : types ) {
		if ( type == known )
			return true;
	}
	return false;
}

/** "share on server", from the mount source: server:/path (NFS, sshfs), //server/share (SMB) or
 * user@server:/path. Falls back to the mount point's name. */
std::string networkShareName( const std::string& source, const std::string& mountPoint ) {
	std::string server, share;
	if ( String::startsWith( source, "//" ) ) {
		size_t slash = source.find( '/', 2 );
		server = source.substr( 2, slash == std::string::npos ? std::string::npos : slash - 2 );
		if ( slash != std::string::npos )
			share = source.substr( slash + 1 );
	} else if ( size_t colon = source.find( ':' ); colon != std::string::npos ) {
		server = source.substr( 0, colon );
		share = source.substr( colon + 1 );
	}
	if ( size_t at = server.rfind( '@' ); at != std::string::npos )
		server = server.substr( at + 1 );
	while ( !share.empty() && share.back() == '/' )
		share.pop_back();
	share = share.empty() ? std::string() : FileSystem::fileNameFromPath( share );
	if ( share.empty() )
		share = FileSystem::fileNameFromPath( mountPoint );
	if ( server.empty() )
		return share.empty() ? mountPoint : share;
	return share.empty() ? server : share + " on " + server;
}

#if EE_PLATFORM == EE_PLATFORM_LINUX

/** Decodes the octal escapes of /proc/mounts and /dev/disk/by-label ("\040" -> ' '). */
std::string decodeOctal( const std::string& text, bool hex ) {
	std::string out;
	for ( size_t i = 0; i < text.size(); ++i ) {
		if ( text[i] == '\\' && !hex && i + 3 < text.size() + 0 && std::isdigit( text[i + 1] ) ) {
			out += static_cast<char>( ( text[i + 1] - '0' ) * 64 + ( text[i + 2] - '0' ) * 8 +
									  ( text[i + 3] - '0' ) );
			i += 3;
		} else if ( text[i] == '\\' && hex && i + 3 < text.size() && text[i + 1] == 'x' ) {
			out += static_cast<char>( std::stoi( text.substr( i + 2, 2 ), nullptr, 16 ) );
			i += 3;
		} else {
			out += text[i];
		}
	}
	return out;
}

std::string realPath( const std::string& path ) {
	char buffer[PATH_MAX];
	return ::realpath( path.c_str(), buffer ) ? std::string( buffer ) : path;
}

#endif

const char* sectionKey( PlaceSection section ) {
	switch ( section ) {
		case PlaceSection::Places:
			return "places";
		case PlaceSection::Remote:
			return "remote";
		case PlaceSection::Devices:
			return "devices";
	}
	return "";
}

} // namespace

Places::Places( std::string configPath ) : mPath( std::move( configPath ) ) {}

std::string Places::defaultIcon( const URI& uri ) {
	return uri.getScheme() == "file" ? "place-folder" : "place-remote";
}

PlaceSection Places::sectionFor( const URI& uri ) {
	return uri.getScheme() == "file" ? PlaceSection::Places : PlaceSection::Remote;
}

const std::vector<std::string>& Places::iconNames() {
	static const std::vector<std::string> names{
		"place-folder",	   "place-home",   "place-desktop",	 "place-documents",
		"place-downloads", "place-music",  "place-pictures", "place-videos",
		"place-trash",	   "place-remote", "place-network",	 "device-drive",
		"device-root",	   "device-usb",   "device-phone",	 "action-add-place",
	};
	return names;
}

std::vector<Place> Places::builtIn() {
	std::vector<Place> places;
	std::string home = withSlash( Sys::getUserDirectory() );
	Place homePlace{ "Home", URI::fromFSPath( home ), "place-home", PlaceSection::Places, true };
	homePlace.id = "home";
	places.push_back( std::move( homePlace ) );
	auto dirs = userDirs( home );
	struct Known {
		const char* key;
		const char* id;
		const char* name;
		const char* icon;
	};
	for ( const Known& known : std::initializer_list<Known>{
			  { "DESKTOP", "desktop", "Desktop", "place-desktop" },
			  { "DOCUMENTS", "documents", "Documents", "place-documents" },
			  { "DOWNLOAD", "downloads", "Downloads", "place-downloads" },
			  { "MUSIC", "music", "Music", "place-music" },
			  { "PICTURES", "pictures", "Pictures", "place-pictures" },
			  { "VIDEOS", "videos", "Videos", "place-videos" } } ) {
		const std::string& path = dirs[known.key];
		if ( path.empty() || !FileSystem::isDirectory( path ) )
			continue;
		std::string name = FileSystem::fileNameFromPath( path );
		Place place{ name.empty() ? std::string( known.name ) : name,
					 URI::fromFSPath( withSlash( path ) ), known.icon, PlaceSection::Places, true };
		place.id = known.id;
		places.push_back( std::move( place ) );
	}
	if ( auto trashLocation = Trash::location() ) {
		Place place{ "Trash", *trashLocation, "place-trash", PlaceSection::Places, true };
		place.trash = true;
		place.id = "trash";
		places.push_back( std::move( place ) );
	}
	return places;
}

std::vector<URI> Places::sshHosts() {
	std::vector<URI> hosts;
	std::string home = withSlash( Sys::getUserDirectory() );
	std::string config;
	if ( !FileSystem::fileGet( home + ".ssh/config", config ) )
		return hosts;
	auto file = SSHConfigFile::parse( config, home + ".ssh/config" );
	for ( const auto& line : file.lines ) {
		if ( line.keyword != "host" )
			continue;
		// Patterns are not hosts.
		for ( const auto& host : line.args ) {
			if ( host.find_first_of( "*?!" ) != std::string::npos )
				continue;
			URI uri;
			uri.setScheme( "sftp" );
			uri.setHost( host );
			uri.setRawPath( "/" );
			if ( std::find( hosts.begin(), hosts.end(), uri ) == hosts.end() )
				hosts.push_back( std::move( uri ) );
		}
	}
	return hosts;
}

void Places::load() {
	mDefaults = builtIn();
	mPlaces.clear();
	mHiddenSections.clear();
	mShowHidden = false;
	std::vector<bool> used( mDefaults.size(), false );
	std::vector<Place> userPlaces;

	std::string data;
	json document =
		FileSystem::fileGet( mPath, data ) ? json::parse( data, nullptr, false ) : json();
	auto readPlace = [&]( const json& item ) {
		if ( !item.is_object() )
			return;
		std::string id = item.value( "id", "" );
		if ( !id.empty() ) {
			for ( size_t i = 0; i < mDefaults.size(); ++i ) {
				if ( used[i] || mDefaults[i].id != id )
					continue;
				used[i] = true;
				Place place = mDefaults[i];
				place.name = item.value( "name", place.name );
				place.icon = item.value( "icon", place.icon );
				place.hidden = item.value( "hidden", false );
				if ( item.contains( "uri" ) && item["uri"].is_string() ) {
					URI uri( item["uri"].get<std::string>() );
					if ( !uri.getScheme().empty() ) {
						place.uri = uri;
						if ( !place.trash )
							place.section = sectionFor( uri );
					}
				}
				mPlaces.push_back( std::move( place ) );
			}
			return;
		}
		if ( !item.contains( "uri" ) || !item["uri"].is_string() )
			return;
		Place place;
		place.uri = URI( item["uri"].get<std::string>() );
		if ( place.uri.getScheme().empty() )
			return;
		place.name = item.value( "name", place.uri.getLastPathSegment() );
		place.icon = item.value( "icon", defaultIcon( place.uri ) );
		place.hidden = item.value( "hidden", false );
		place.section = sectionFor( place.uri );
		for ( const auto& existing : mPlaces ) {
			if ( existing.uri == place.uri )
				return;
		}
		mPlaces.push_back( std::move( place ) );
	};

	if ( document.is_array() ) {
		// First format: only the user places, after the built-in ones.
		for ( const auto& item : document ) {
			if ( item.is_object() && !item.contains( "id" ) )
				readPlace( item );
		}
	} else if ( document.is_object() ) {
		mShowHidden = document.value( "show_hidden", false );
		if ( document.contains( "hidden_sections" ) && document["hidden_sections"].is_array() ) {
			for ( const auto& key : document["hidden_sections"] ) {
				for ( auto section :
					  { PlaceSection::Places, PlaceSection::Remote, PlaceSection::Devices } ) {
					if ( key.is_string() && key.get<std::string>() == sectionKey( section ) )
						mHiddenSections.push_back( section );
				}
			}
		}
		if ( document.contains( "places" ) && document["places"].is_array() ) {
			for ( const auto& item : document["places"] )
				readPlace( item );
		}
	}

	// Built-in places not in the configuration (first run, or new XDG folders) keep their
	// default position: before the first saved place that comes after them by default.
	for ( size_t i = 0; i < mDefaults.size(); ++i ) {
		if ( used[i] )
			continue;
		auto next = mPlaces.end();
		for ( size_t j = i + 1; j < mDefaults.size() && next == mPlaces.end(); ++j ) {
			if ( used[j] ) {
				next = std::find_if( mPlaces.begin(), mPlaces.end(), [&]( const Place& place ) {
					return place.id == mDefaults[j].id;
				} );
			}
		}
		if ( next == mPlaces.end() ) {
			next = std::find_if( mPlaces.begin(), mPlaces.end(),
								 []( const Place& place ) { return place.id.empty(); } );
		}
		mPlaces.insert( next, mDefaults[i] );
	}
}

bool Places::save() const {
	json places = json::array();
	for ( const auto& place : mPlaces ) {
		json item = json::object();
		const Place* defaults = nullptr;
		for ( const auto& candidate : mDefaults ) {
			if ( !place.id.empty() && candidate.id == place.id )
				defaults = &candidate;
		}
		if ( defaults ) {
			// Only what the user changed, so the defaults (XDG folders) can still evolve.
			item["id"] = place.id;
			if ( place.name != defaults->name )
				item["name"] = place.name;
			if ( place.uri != defaults->uri )
				item["uri"] = place.uri.toString();
			if ( place.icon != defaults->icon )
				item["icon"] = place.icon;
		} else {
			item["name"] = place.name;
			item["uri"] = place.uri.toString();
			item["icon"] = place.icon;
		}
		if ( place.hidden )
			item["hidden"] = true;
		places.push_back( std::move( item ) );
	}
	json hiddenSections = json::array();
	for ( auto section : mHiddenSections )
		hiddenSections.push_back( sectionKey( section ) );
	json document = { { "version", 2 },
					  { "show_hidden", mShowHidden },
					  { "hidden_sections", hiddenSections },
					  { "places", places } };
	std::string dir = FileSystem::fileRemoveFileName( mPath );
	if ( !dir.empty() && !FileSystem::isDirectory( dir ) )
		FileSystem::makeDir( dir, true );
	return FileSystem::fileWrite( mPath, std::string_view( document.dump( 2 ) ) );
}

bool Places::add( Place place, std::optional<size_t> before ) {
	for ( const auto& existing : mPlaces ) {
		if ( existing.uri == place.uri )
			return false;
	}
	place.section = sectionFor( place.uri );
	if ( before && *before < mPlaces.size() && mPlaces[*before].section == place.section ) {
		mPlaces.insert( mPlaces.begin() + *before, std::move( place ) );
		return true;
	}
	// Keep sections together: insert after the last place of the same section.
	auto last = std::find_if( mPlaces.rbegin(), mPlaces.rend(),
							  [&]( const Place& p ) { return p.section == place.section; } );
	mPlaces.insert( last == mPlaces.rend() ? mPlaces.end() : last.base(), std::move( place ) );
	return true;
}

void Places::moveBefore( size_t index, std::optional<size_t> before ) {
	if ( index >= mPlaces.size() || ( before && *before == index ) )
		return;
	PlaceSection section = mPlaces[index].section;
	if ( before && ( *before >= mPlaces.size() || mPlaces[*before].section != section ) )
		return;
	Place place = std::move( mPlaces[index] );
	mPlaces.erase( mPlaces.begin() + index );
	if ( before ) {
		size_t target = *before > index ? *before - 1 : *before;
		mPlaces.insert( mPlaces.begin() + target, std::move( place ) );
		return;
	}
	auto last = std::find_if( mPlaces.rbegin(), mPlaces.rend(),
							  [&]( const Place& p ) { return p.section == section; } );
	mPlaces.insert( last == mPlaces.rend() ? mPlaces.end() : last.base(), std::move( place ) );
}

void Places::remove( size_t index ) {
	if ( index >= mPlaces.size() )
		return;
	if ( mPlaces[index].id.empty() )
		mPlaces.erase( mPlaces.begin() + index );
	else
		mPlaces[index].hidden = true;
}

void Places::edit( size_t index, std::string name, const URI& uri, std::string icon ) {
	if ( index >= mPlaces.size() )
		return;
	Place& place = mPlaces[index];
	if ( !name.empty() )
		place.name = std::move( name );
	if ( !icon.empty() )
		place.icon = std::move( icon );
	if ( uri.getScheme().empty() || uri == place.uri )
		return;
	place.uri = uri;
	PlaceSection section = place.trash ? place.section : sectionFor( uri );
	if ( section == place.section )
		return;
	// A place that changed section goes to the end of its new section.
	Place moved = std::move( place );
	mPlaces.erase( mPlaces.begin() + index );
	moved.section = section;
	auto last = std::find_if( mPlaces.rbegin(), mPlaces.rend(),
							  [&]( const Place& p ) { return p.section == section; } );
	mPlaces.insert( last == mPlaces.rend() ? mPlaces.end() : last.base(), std::move( moved ) );
}

void Places::setHidden( size_t index, bool hidden ) {
	if ( index < mPlaces.size() )
		mPlaces[index].hidden = hidden;
}

std::optional<size_t> Places::neighbor( size_t index, int delta ) const {
	if ( index >= mPlaces.size() || delta == 0 )
		return std::nullopt;
	// Hidden places are skipped while they are not listed.
	for ( size_t i = index; delta < 0 ? i-- > 0 : ++i < mPlaces.size(); ) {
		if ( mPlaces[i].section != mPlaces[index].section || ( mPlaces[i].hidden && !mShowHidden ) )
			continue;
		return i;
	}
	return std::nullopt;
}

bool Places::canMove( size_t index, int delta ) const {
	return neighbor( index, delta ).has_value();
}

void Places::move( size_t index, int delta ) {
	auto target = neighbor( index, delta );
	if ( !target )
		return;
	Place place = std::move( mPlaces[index] );
	mPlaces.erase( mPlaces.begin() + index );
	// After the erase, inserting at target lands before the previous neighbor or after the
	// next one.
	mPlaces.insert( mPlaces.begin() + *target, std::move( place ) );
}

bool Places::isSectionHidden( PlaceSection section ) const {
	return std::find( mHiddenSections.begin(), mHiddenSections.end(), section ) !=
		   mHiddenSections.end();
}

void Places::setSectionHidden( PlaceSection section, bool hidden ) {
	mHiddenSections.erase( std::remove( mHiddenSections.begin(), mHiddenSections.end(), section ),
						   mHiddenSections.end() );
	if ( hidden )
		mHiddenSections.push_back( section );
}

bool Places::hasHidden() const {
	return !mHiddenSections.empty() ||
		   std::any_of( mPlaces.begin(), mPlaces.end(),
						[]( const Place& place ) { return place.hidden; } );
}

std::vector<Place> Places::devices() {
	std::vector<Place> devices;
	auto addDevice = [&]( std::string name, const std::string& path, const char* icon,
						  PlaceSection section = PlaceSection::Devices ) {
		URI uri = URI::fromFSPath( withSlash( path ) );
		for ( const auto& device : devices ) {
			if ( device.uri == uri )
				return;
		}
		devices.push_back( Place{ std::move( name ), uri, icon, section, true } );
	};
	// Network mounts are never touched here (no stat): an unreachable server would block.
	auto addNetworkShare = [&]( const std::string& source, const std::string& mountPoint ) {
		addDevice( networkShareName( source, mountPoint ), mountPoint, "place-network",
				   PlaceSection::Remote );
	};
#if EE_PLATFORM == EE_PLATFORM_LINUX
	// Device node -> label, from the udev by-label links.
	std::map<std::string, std::string> labels;
	const std::string byLabel = "/dev/disk/by-label/";
	if ( FileSystem::isDirectory( byLabel ) ) {
		for ( const auto& label : FileSystem::filesGetInPath( byLabel ) )
			labels[realPath( byLabel + label )] = decodeOctal( label, true );
	}
	// procfs files report a size of 0: read them as a stream.
	std::ifstream stream( "/proc/self/mounts" );
	if ( !stream )
		return devices;
	std::string mounts( ( std::istreambuf_iterator<char>( stream ) ),
						std::istreambuf_iterator<char>() );
	for ( const auto& line : String::split( mounts, '\n' ) ) {
		auto fields = String::split( line, ' ' );
		if ( fields.size() < 3 )
			continue;
		std::string device = decodeOctal( fields[0], false );
		std::string mountPoint = decodeOctal( fields[1], false );
		const std::string& type = fields[2];
		if ( isNetworkFileSystem( type ) ) {
			addNetworkShare( device, mountPoint );
			continue;
		}
		if ( !String::startsWith( device, "/dev/" ) || String::startsWith( device, "/dev/loop" ) ||
			 type == "squashfs" || String::startsWith( mountPoint, "/boot" ) ||
			 String::startsWith( mountPoint, "/snap" ) ||
			 String::startsWith( mountPoint, "/var/" ) ||
			 String::startsWith( mountPoint, "/run/credentials" ) )
			continue;
		bool removable = String::startsWith( mountPoint, "/run/media/" ) ||
						 String::startsWith( mountPoint, "/media/" );
		std::string name;
		auto label = labels.find( realPath( device ) );
		if ( label != labels.end() )
			name = label->second;
		else if ( mountPoint == "/" )
			name = "Root";
		else
			name = FileSystem::fileNameFromPath( mountPoint );
		addDevice( name, mountPoint,
				   mountPoint == "/" ? "device-root"
				   : removable		 ? "device-usb"
									 : "device-drive" );
	}
#elif EE_PLATFORM == EE_PLATFORM_WIN
	for ( const auto& drive : Sys::getLogicalDrives() )
		addDevice( drive, drive, "device-drive" );
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD
#if defined( __NetBSD__ )
	struct statvfs* mounts = nullptr;
#else
	struct statfs* mounts = nullptr;
#endif
	// MNT_NOWAIT: cached information, so unreachable network mounts do not block.
	int count = getmntinfo( &mounts, MNT_NOWAIT );
	for ( int i = 0; i < count; ++i ) {
		std::string type( mounts[i].f_fstypename );
		std::string source( mounts[i].f_mntfromname );
		std::string mountPoint( mounts[i].f_mntonname );
		if ( isNetworkFileSystem( type ) ) {
			addNetworkShare( source, mountPoint );
			continue;
		}
#if EE_PLATFORM == EE_PLATFORM_MACOS
		// The root and the volumes the user sees in Finder.
		if ( mountPoint == "/" )
			addDevice( "Macintosh HD", mountPoint, "device-root" );
		else if ( String::startsWith( mountPoint, "/Volumes/" ) )
			addDevice( FileSystem::fileNameFromPath( mountPoint ), mountPoint, "device-drive" );
#else
		if ( !String::startsWith( source, "/dev/" ) )
			continue;
		addDevice( mountPoint == "/" ? std::string( "Root" )
									 : FileSystem::fileNameFromPath( mountPoint ),
				   mountPoint,
				   mountPoint == "/"							 ? "device-root"
				   : String::startsWith( mountPoint, "/media/" ) ? "device-usb"
																 : "device-drive" );
#endif
	}
	if ( devices.empty() )
		addDevice( "Root", "/", "device-root" );
#else
	addDevice( "Root", "/", "device-root" );
#endif
	return devices;
}

} // namespace eefm
