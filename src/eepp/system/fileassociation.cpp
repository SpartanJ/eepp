#include <eepp/system/fileassociation.hpp>

#include <algorithm>
#include <cctype>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <windows.h>

#include <shlobj.h>
#elif EE_PLATFORM == EE_PLATFORM_MACOS
#include <CoreServices/CoreServices.h>
#endif

namespace EE::System {

namespace {

static std::vector<std::string> normalizeExtensions( const std::vector<std::string>& extensions ) {
	std::vector<std::string> normalized;
	normalized.reserve( extensions.size() );
	for ( const auto& extension : extensions ) {
		auto value = FileAssociation::normalizeExtension( extension );
		if ( !value.empty() )
			normalized.emplace_back( std::move( value ) );
	}
	std::sort( normalized.begin(), normalized.end() );
	normalized.erase( std::unique( normalized.begin(), normalized.end() ), normalized.end() );
	return normalized;
}

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD

static std::string applicationSlug( std::string_view id ) {
	std::string slug;
	slug.reserve( id.size() );
	for ( unsigned char character : id ) {
		if ( std::isalnum( character ) )
			slug += static_cast<char>( std::tolower( character ) );
		else if ( slug.empty() || slug.back() != '-' )
			slug += '-';
	}
	while ( !slug.empty() && slug.back() == '-' )
		slug.pop_back();
	return slug.empty() ? "application" : slug;
}

static std::string xdgDataHome() {
	auto path = Sys::getEnv( "XDG_DATA_HOME" );
	if ( path.empty() ) {
		path = Sys::getUserDirectory();
		FileSystem::dirAddSlashAtEnd( path );
		path += ".local/share";
	}
	FileSystem::dirRemoveSlashAtEnd( path );
	return path;
}

static std::string xdgConfigHome() {
	auto path = Sys::getEnv( "XDG_CONFIG_HOME" );
	if ( path.empty() ) {
		path = Sys::getUserDirectory();
		FileSystem::dirAddSlashAtEnd( path );
		path += ".config";
	}
	FileSystem::dirRemoveSlashAtEnd( path );
	return path;
}

static std::string desktopEntryName( const FileAssociationApplication& application ) {
	return applicationSlug( application.id ) + ".desktop";
}

static std::string desktopEntryPath( const FileAssociationApplication& application ) {
	return xdgDataHome() + "/applications/" + desktopEntryName( application );
}

static std::string mimePackagePath( const FileAssociationApplication& application ) {
	return xdgDataHome() + "/mime/packages/" + applicationSlug( application.id ) + ".xml";
}

static std::string desktopEscape( std::string_view value ) {
	std::string escaped;
	escaped.reserve( value.size() + 8 );
	for ( char character : value ) {
		switch ( character ) {
			case '\\':
			case '"':
			case '`':
			case '$':
				escaped += '\\';
				[[fallthrough]];
			default:
				escaped += character;
		}
	}
	return escaped;
}

static std::string desktopExecPath( std::string_view path ) {
	if ( path.find_first_of( " \t\n\"'\\><~|&;$*?#()`" ) == std::string_view::npos )
		return std::string( path );
	auto environment = Sys::which( "env" );
	if ( environment.empty() )
		environment = "env";
	return environment + " \"" + desktopEscape( path ) + '"';
}

static std::string xmlEscape( std::string_view value ) {
	std::string escaped;
	escaped.reserve( value.size() );
	for ( char character : value ) {
		switch ( character ) {
			case '&':
				escaped += "&amp;";
				break;
			case '<':
				escaped += "&lt;";
				break;
			case '>':
				escaped += "&gt;";
				break;
			case '"':
				escaped += "&quot;";
				break;
			default:
				escaped += character;
		}
	}
	return escaped;
}

static std::vector<std::string> desktopExtensions( const FileAssociationApplication& application ) {
	std::string contents;
	if ( !FileSystem::fileGet( desktopEntryPath( application ), contents ) )
		return {};
	constexpr std::string_view key = "X-EEPP-File-Extensions=";
	auto position = contents.find( key );
	if ( position == std::string::npos )
		return {};
	position += key.size();
	auto end = contents.find( '\n', position );
	auto values = String::split( contents.substr( position, end - position ), ';' );
	return normalizeExtensions( values );
}

static bool desktopEntryVisible( const FileAssociationApplication& application ) {
	std::string contents;
	if ( !FileSystem::fileGet( desktopEntryPath( application ), contents ) )
		return false;
	return contents.find( "NoDisplay=false" ) != std::string::npos;
}

static void loadMimeGlobs( const std::string& path, const std::unordered_set<std::string>& wanted,
						   std::unordered_map<std::string, std::pair<int, std::string>>& types ) {
	std::string contents;
	if ( !FileSystem::fileGet( path, contents ) )
		return;
	for ( const auto& line : String::split( contents, '\n' ) ) {
		if ( line.empty() || line[0] == '#' )
			continue;
		auto fields = String::split( line, ':' );
		if ( fields.size() < 2 )
			continue;
		int weight = 50;
		std::string mime;
		std::string glob;
		if ( fields.size() >= 3 ) {
			Int32 parsedWeight = weight;
			String::fromString( parsedWeight, fields[0] );
			weight = parsedWeight;
			mime = fields[1];
			glob = fields[2];
		} else {
			mime = fields[0];
			glob = fields[1];
		}
		if ( !String::startsWith( glob, "*." ) )
			continue;
		auto extension = FileAssociation::normalizeExtension( glob.substr( 2 ) );
		if ( extension.empty() || !wanted.contains( extension ) )
			continue;
		auto found = types.find( extension );
		if ( found == types.end() || found->second.first < weight )
			types[extension] = { weight, std::move( mime ) };
	}
}

static std::unordered_map<std::string, std::string>
mimeTypesForExtensions( const std::vector<std::string>& extensions,
						const FileAssociationApplication& application ) {
	std::unordered_set<std::string> wanted( extensions.begin(), extensions.end() );
	std::unordered_map<std::string, std::pair<int, std::string>> weightedTypes;
	std::vector<std::string> dataDirectories{ xdgDataHome() };
	auto systemDirectories = Sys::getEnv( "XDG_DATA_DIRS" );
	if ( systemDirectories.empty() )
		systemDirectories = "/usr/local/share:/usr/share";
	for ( auto& directory : String::split( systemDirectories, ':' ) )
		dataDirectories.emplace_back( std::move( directory ) );
	for ( auto& directory : dataDirectories ) {
		FileSystem::dirRemoveSlashAtEnd( directory );
		loadMimeGlobs( directory + "/mime/globs2", wanted, weightedTypes );
		loadMimeGlobs( directory + "/mime/globs", wanted, weightedTypes );
	}

	std::unordered_map<std::string, std::string> types;
	types.reserve( extensions.size() );
	const auto slug = applicationSlug( application.id );
	for ( const auto& extension : extensions ) {
		auto found = weightedTypes.find( extension );
		types[extension] = found != weightedTypes.end()
							   ? std::move( found->second.second )
							   : "application/x-" + slug + '-' + applicationSlug( extension );
	}
	return types;
}

static bool runDatabaseUpdate( const std::string& command, const std::string& directory ) {
	auto executable = Sys::which( command );
	if ( executable.empty() )
		return true;
	Process process;
	if ( !process.create( executable, std::vector<std::string>{ directory },
						  Process::getDefaultOptions() ) )
		return false;
	int exitCode = -1;
	return process.join( &exitCode ) && exitCode == 0;
}

static bool writeFileIfChanged( const std::string& path, const std::string& contents,
								bool& changed ) {
	std::string current;
	if ( FileSystem::fileExists( path ) ) {
		if ( !FileSystem::fileGet( path, current ) )
			return false;
		if ( current == contents )
			return true;
	}
	if ( !FileSystem::fileWrite( path, contents ) )
		return false;
	changed = true;
	return true;
}

static bool removeFileIfExists( const std::string& path, bool& changed ) {
	if ( !FileSystem::fileExists( path ) )
		return true;
	if ( !FileSystem::fileRemove( path ) )
		return false;
	changed = true;
	return true;
}

static void updateMimeAppsSection( std::vector<std::string>& lines, std::string_view sectionName,
								   const std::unordered_set<std::string>& selected,
								   const std::unordered_set<std::string>& removed,
								   std::string_view desktopName, bool addSelected ) {
	const std::string header = '[' + std::string( sectionName ) + ']';
	auto section = std::find( lines.begin(), lines.end(), header );
	if ( section == lines.end() ) {
		if ( !addSelected || selected.empty() )
			return;
		if ( !lines.empty() && !lines.back().empty() )
			lines.emplace_back();
		lines.emplace_back( header );
		section = std::prev( lines.end() );
	}

	auto sectionEnd =
		std::find_if( std::next( section ), lines.end(), []( const std::string& line ) {
			return line.size() >= 2 && line.front() == '[' && line.back() == ']';
		} );
	std::unordered_set<std::string> found;
	for ( auto line = std::next( section ); line != sectionEnd; ) {
		const auto separator = line->find( '=' );
		if ( separator == std::string::npos ) {
			++line;
			continue;
		}
		const auto mime = line->substr( 0, separator );
		const bool isSelected = selected.contains( mime );
		if ( !isSelected && !removed.contains( mime ) ) {
			++line;
			continue;
		}
		found.emplace( mime );
		auto applications = String::split( line->substr( separator + 1 ), ';' );
		applications.erase( std::remove( applications.begin(), applications.end(), desktopName ),
							applications.end() );
		if ( addSelected && isSelected )
			applications.insert( applications.begin(), std::string( desktopName ) );
		if ( applications.empty() ) {
			line = lines.erase( line );
			sectionEnd = std::find_if( line, lines.end(), []( const std::string& value ) {
				return value.size() >= 2 && value.front() == '[' && value.back() == ']';
			} );
			continue;
		}
		*line = mime + '=';
		for ( const auto& application : applications )
			*line += application + ';';
		++line;
	}

	if ( addSelected ) {
		std::vector<std::string> missing;
		for ( const auto& mime : selected ) {
			if ( !found.contains( mime ) )
				missing.emplace_back( mime );
		}
		std::sort( missing.begin(), missing.end() );
		for ( const auto& mime : missing ) {
			sectionEnd = lines.insert( sectionEnd, mime + '=' + std::string( desktopName ) + ';' );
			++sectionEnd;
		}
	}
}

static bool updateMimeAppsFile( const std::string& path,
								const std::unordered_set<std::string>& selected,
								const std::unordered_set<std::string>& removed,
								std::string_view desktopName, std::string& error ) {
	std::string contents;
	if ( FileSystem::fileExists( path ) && !FileSystem::fileGet( path, contents ) ) {
		error = "Could not read the XDG MIME application preferences.";
		return false;
	}
	auto lines = String::split( contents, '\n', true );
	if ( lines.size() == 1 && lines.front().empty() )
		lines.clear();
	updateMimeAppsSection( lines, "Default Applications", selected, removed, desktopName, true );
	updateMimeAppsSection( lines, "Added Associations", selected, removed, desktopName, true );
	updateMimeAppsSection( lines, "Removed Associations", selected, removed, desktopName, false );
	std::string updated;
	for ( const auto& line : lines )
		updated += line + '\n';
	if ( updated == contents )
		return true;
	if ( !FileSystem::fileWrite( path, updated ) ) {
		error = "Could not write the XDG MIME application preferences.";
		return false;
	}
	return true;
}

static bool updateMimeAppsRegistration( const FileAssociationApplication& application,
										const std::unordered_set<std::string>& selected,
										const std::unordered_set<std::string>& removed,
										std::string& error ) {
	const auto configHome = xdgConfigHome();
	if ( !FileSystem::isDirectory( configHome ) && !FileSystem::makeDir( configHome, true ) ) {
		error = "Could not create the XDG configuration directory.";
		return false;
	}
	std::vector<std::string> paths{ configHome + "/mimeapps.list" };
	for ( auto desktop : String::split( Sys::getEnv( "XDG_CURRENT_DESKTOP" ), ':' ) ) {
		String::toLowerInPlace( desktop );
		if ( !desktop.empty() )
			paths.emplace_back( configHome + '/' + desktop + "-mimeapps.list" );
	}
	const auto legacyPath = xdgDataHome() + "/applications/mimeapps.list";
	if ( FileSystem::fileExists( legacyPath ) )
		paths.emplace_back( legacyPath );
	std::sort( paths.begin(), paths.end() );
	paths.erase( std::unique( paths.begin(), paths.end() ), paths.end() );
	for ( const auto& path : paths ) {
		if ( !updateMimeAppsFile( path, selected, removed, desktopEntryName( application ),
								  error ) )
			return false;
	}
	return true;
}

static bool writeXdgRegistration( const FileAssociationApplication& application,
								  const std::vector<std::string>& extensions, bool visible,
								  std::string& error ) {
	const auto previousExtensions = desktopExtensions( application );
	const auto dataHome = xdgDataHome();
	const auto applicationsDirectory = dataHome + "/applications";
	const auto mimeDirectory = dataHome + "/mime";
	const auto packageDirectory = mimeDirectory + "/packages";
	if ( ( !FileSystem::isDirectory( applicationsDirectory ) &&
		   !FileSystem::makeDir( applicationsDirectory, true ) ) ||
		 ( !FileSystem::isDirectory( packageDirectory ) &&
		   !FileSystem::makeDir( packageDirectory, true ) ) ) {
		error = "Could not create the XDG application directories.";
		return false;
	}

	const auto previousMimeTypes = mimeTypesForExtensions( previousExtensions, application );
	const auto mimeTypes = mimeTypesForExtensions( extensions, application );
	std::unordered_set<std::string> uniqueMimeTypes;
	std::string package =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n";
	const auto slug = applicationSlug( application.id );
	bool hasCustomMimeTypes = false;
	for ( const auto& extension : extensions ) {
		const auto& mime = mimeTypes.at( extension );
		uniqueMimeTypes.insert( mime );
		if ( !String::startsWith( mime, "application/x-" + slug + '-' ) )
			continue;
		hasCustomMimeTypes = true;
		package += "  <mime-type type=\"" + mime + "\">\n";
		package += "    <comment>" + xmlEscape( application.name ) + " " + extension +
				   " document</comment>\n";
		package += "    <glob pattern=\"*." + extension + "\"/>\n";
		package += "  </mime-type>\n";
	}
	package += "</mime-info>\n";
	const auto packagePath = mimePackagePath( application );
	bool mimeDatabaseChanged = false;
	if ( hasCustomMimeTypes ) {
		if ( !writeFileIfChanged( packagePath, package, mimeDatabaseChanged ) ) {
			error = "Could not write the XDG MIME package.";
			return false;
		}
	} else if ( !removeFileIfExists( packagePath, mimeDatabaseChanged ) ) {
		error = "Could not remove the obsolete XDG MIME package.";
		return false;
	}

	const auto desktopPath = desktopEntryPath( application );
	bool desktopDatabaseChanged = false;
	if ( extensions.empty() && !visible ) {
		if ( !removeFileIfExists( desktopPath, desktopDatabaseChanged ) ) {
			error = "Could not remove the XDG desktop entry.";
			return false;
		}
	} else {
		std::vector<std::string> sortedMimeTypes( uniqueMimeTypes.begin(), uniqueMimeTypes.end() );
		std::sort( sortedMimeTypes.begin(), sortedMimeTypes.end() );
		std::string desktop = "[Desktop Entry]\nType=Application\nVersion=1.0\n";
		desktop += "Name=" + desktopEscape( application.name ) + "\n";
		desktop += "Comment=Edit source code and text files\n";
		desktop += "Exec=" + desktopExecPath( application.executablePath ) + " %F\n";
		desktop += "TryExec=" + application.executablePath + "\n";
		if ( !application.iconPath.empty() )
			desktop += "Icon=" + application.iconPath + "\n";
		desktop += "Terminal=false\nCategories=Development;TextEditor;\n";
		desktop += std::string( "NoDisplay=" ) + ( visible ? "false\n" : "true\n" );
		desktop += "MimeType=";
		for ( const auto& mime : sortedMimeTypes )
			desktop += mime + ';';
		desktop += "\nX-EEPP-File-Extensions=";
		for ( const auto& extension : extensions )
			desktop += extension + ';';
		desktop += '\n';
		if ( !writeFileIfChanged( desktopPath, desktop, desktopDatabaseChanged ) ) {
			error = "Could not write the XDG desktop entry.";
			return false;
		}
	}

	if ( ( mimeDatabaseChanged && !runDatabaseUpdate( "update-mime-database", mimeDirectory ) ) ||
		 ( desktopDatabaseChanged &&
		   !runDatabaseUpdate( "update-desktop-database", applicationsDirectory ) ) ) {
		error = "The XDG registration was written, but a desktop database update failed.";
		return false;
	}
	std::unordered_set<std::string> selectedMimeTypes;
	for ( const auto& [extension, mime] : mimeTypes )
		selectedMimeTypes.emplace( mime );
	std::unordered_set<std::string> removedMimeTypes;
	for ( const auto& [extension, mime] : previousMimeTypes ) {
		if ( !selectedMimeTypes.contains( mime ) )
			removedMimeTypes.emplace( mime );
	}
	if ( !updateMimeAppsRegistration( application, selectedMimeTypes, removedMimeTypes, error ) )
		return false;
	return true;
}

#elif EE_PLATFORM == EE_PLATFORM_WIN

static std::wstring toWide( std::string_view value ) {
	return String::fromUtf8( std::string( value ) ).toWideString();
}

static bool setRegistryString( HKEY root, const std::wstring& key, const wchar_t* valueName,
							   const std::wstring& value, std::string& error ) {
	HKEY handle = nullptr;
	auto status = RegCreateKeyExW( root, key.c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr,
								   &handle, nullptr );
	if ( status != ERROR_SUCCESS ) {
		error = "Could not create a file-association registry key (error " +
				std::to_string( status ) + ").";
		return false;
	}
	status = RegSetValueExW( handle, valueName, 0, REG_SZ,
							 reinterpret_cast<const BYTE*>( value.c_str() ),
							 static_cast<DWORD>( ( value.size() + 1 ) * sizeof( wchar_t ) ) );
	RegCloseKey( handle );
	if ( status != ERROR_SUCCESS ) {
		error = "Could not write a file-association registry value (error " +
				std::to_string( status ) + ").";
		return false;
	}
	return true;
}

static bool registryValueExists( const std::wstring& key, const std::wstring& valueName ) {
	HKEY handle = nullptr;
	if ( RegOpenKeyExW( HKEY_CURRENT_USER, key.c_str(), 0, KEY_QUERY_VALUE, &handle ) !=
		 ERROR_SUCCESS )
		return false;
	const auto status =
		RegQueryValueExW( handle, valueName.c_str(), nullptr, nullptr, nullptr, nullptr );
	RegCloseKey( handle );
	return status == ERROR_SUCCESS;
}

static bool deleteRegistryValue( const std::wstring& key, const std::wstring& valueName,
								 std::string& error ) {
	HKEY handle = nullptr;
	if ( RegOpenKeyExW( HKEY_CURRENT_USER, key.c_str(), 0, KEY_SET_VALUE, &handle ) !=
		 ERROR_SUCCESS )
		return true;
	const auto status = RegDeleteValueW( handle, valueName.c_str() );
	RegCloseKey( handle );
	if ( status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND ) {
		error = "Could not remove a file-association registry value (error " +
				std::to_string( status ) + ").";
		return false;
	}
	return true;
}

#elif EE_PLATFORM == EE_PLATFORM_MACOS

class CFRef {
  public:
	explicit CFRef( CFTypeRef value = nullptr ) : mValue( value ) {}
	~CFRef() {
		if ( mValue )
			CFRelease( mValue );
	}
	CFRef( const CFRef& ) = delete;
	CFRef& operator=( const CFRef& ) = delete;
	CFTypeRef get() const { return mValue; }

  private:
	CFTypeRef mValue;
};

static CFStringRef cfString( const std::string& value ) {
	return CFStringCreateWithCString( kCFAllocatorDefault, value.c_str(), kCFStringEncodingUTF8 );
}

static CFStringRef typeForExtension( const std::string& extension ) {
	CFRef extensionString( cfString( extension ) );
	if ( !extensionString.get() )
		return nullptr;
	return UTTypeCreatePreferredIdentifierForTag(
		kUTTagClassFilenameExtension, static_cast<CFStringRef>( extensionString.get() ), nullptr );
}

#endif

} // namespace

FileAssociation::FileAssociation( FileAssociationApplication application ) :
	mApplication( std::move( application ) ) {}

bool FileAssociation::isSupported() {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	return true;
#else
	return false;
#endif
}

bool FileAssociation::supportsDesktopEntries() {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	return true;
#else
	return false;
#endif
}

std::string FileAssociation::normalizeExtension( std::string_view extension ) {
	while ( !extension.empty() && extension.front() == '.' )
		extension.remove_prefix( 1 );
	if ( extension.empty() || extension.size() > 64 )
		return {};
	std::string normalized;
	normalized.reserve( extension.size() );
	for ( unsigned char character : extension ) {
		if ( !std::isalnum( character ) && character != '_' && character != '-' &&
			 character != '+' && character != '.' )
			return {};
		normalized += static_cast<char>( std::tolower( character ) );
	}
	return normalized;
}

std::vector<std::string> FileAssociation::getRegisteredExtensions(
	const std::vector<std::string>& supportedExtensions ) const {
	mLastError.clear();
	const auto supported = normalizeExtensions( supportedExtensions );
	std::vector<std::string> registered;
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	const auto current = desktopExtensions( mApplication );
	std::set_intersection( supported.begin(), supported.end(), current.begin(), current.end(),
						   std::back_inserter( registered ) );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	const auto progId = toWide( mApplication.id );
	for ( const auto& extension : supported ) {
		if ( registryValueExists(
				 L"Software\\Classes\\." + toWide( extension ) + L"\\OpenWithProgids", progId ) )
			registered.emplace_back( extension );
	}
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	CFRef applicationId( cfString( mApplication.id ) );
	if ( !applicationId.get() ) {
		mLastError = "The application identifier is not valid UTF-8.";
		return {};
	}
	for ( const auto& extension : supported ) {
		CFRef type( typeForExtension( extension ) );
		if ( !type.get() )
			continue;
		CFRef handler( LSCopyDefaultRoleHandlerForContentType(
			static_cast<CFStringRef>( type.get() ), kLSRolesEditor | kLSRolesViewer ) );
		if ( handler.get() && CFEqual( handler.get(), applicationId.get() ) )
			registered.emplace_back( extension );
	}
#else
	(void)supported;
	mLastError = "File associations are not supported on this platform.";
#endif
	return registered;
}

bool FileAssociation::setRegisteredExtensions(
	const std::vector<std::string>& registeredExtensions,
	const std::vector<std::string>& supportedExtensions ) {
	return setRegisteredExtensions( registeredExtensions, supportedExtensions,
									supportsDesktopEntries() ? isDesktopEntryInstalled() : false );
}

bool FileAssociation::setRegisteredExtensions( const std::vector<std::string>& registeredExtensions,
											   const std::vector<std::string>& supportedExtensions,
											   bool desktopEntryInstalled ) {
	mLastError.clear();
	const auto supported = normalizeExtensions( supportedExtensions );
	const auto requested = normalizeExtensions( registeredExtensions );
	std::vector<std::string> selected;
	selected.reserve( std::min( requested.size(), supported.size() ) );
	std::set_intersection( requested.begin(), requested.end(), supported.begin(), supported.end(),
						   std::back_inserter( selected ) );
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	return writeXdgRegistration( mApplication, selected, desktopEntryInstalled, mLastError );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	(void)desktopEntryInstalled;
	const auto progId = toWide( mApplication.id );
	const auto executable = toWide( mApplication.executablePath );
	const auto executableName =
		toWide( FileSystem::fileNameFromPath( mApplication.executablePath ) );
	const auto classes = std::wstring( L"Software\\Classes\\" );
	const auto applicationKey = classes + L"Applications\\" + executableName;
	const auto command = L"\"" + executable + L"\" \"%1\"";
	if ( !setRegistryString( HKEY_CURRENT_USER, classes + progId, nullptr,
							 toWide( mApplication.name + " document" ), mLastError ) ||
		 !setRegistryString( HKEY_CURRENT_USER, applicationKey, L"FriendlyAppName",
							 toWide( mApplication.name ), mLastError ) ||
		 !setRegistryString( HKEY_CURRENT_USER, classes + progId + L"\\shell\\open\\command",
							 nullptr, command, mLastError ) ||
		 !setRegistryString( HKEY_CURRENT_USER, applicationKey + L"\\shell\\open\\command", nullptr,
							 command, mLastError ) )
		return false;
	if ( !mApplication.iconPath.empty() &&
		 ( !setRegistryString( HKEY_CURRENT_USER, classes + progId + L"\\DefaultIcon", nullptr,
							   toWide( mApplication.iconPath ), mLastError ) ||
		   !setRegistryString( HKEY_CURRENT_USER, applicationKey + L"\\DefaultIcon", nullptr,
							   toWide( mApplication.iconPath ), mLastError ) ) )
		return false;
	for ( const auto& extension : supported ) {
		const bool shouldRegister =
			std::binary_search( selected.begin(), selected.end(), extension );
		const auto dotExtension = toWide( '.' + extension );
		const auto openWithKey = classes + dotExtension + L"\\OpenWithProgids";
		const auto supportedTypesKey = applicationKey + L"\\SupportedTypes";
		if ( shouldRegister ) {
			if ( !setRegistryString( HKEY_CURRENT_USER, openWithKey, progId.c_str(), L"",
									 mLastError ) ||
				 !setRegistryString( HKEY_CURRENT_USER, supportedTypesKey, dotExtension.c_str(),
									 L"", mLastError ) )
				return false;
		} else if ( !deleteRegistryValue( openWithKey, progId, mLastError ) ||
					!deleteRegistryValue( supportedTypesKey, dotExtension, mLastError ) ) {
			return false;
		}
	}
	SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr );
	return true;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	(void)desktopEntryInstalled;
	CFRef bundleUrl( CFBundleCopyBundleURL( CFBundleGetMainBundle() ) );
	if ( !bundleUrl.get() ||
		 LSRegisterURL( static_cast<CFURLRef>( bundleUrl.get() ), true ) != noErr ) {
		mLastError = "Launch Services could not register the application bundle.";
		return false;
	}
	CFRef applicationId( cfString( mApplication.id ) );
	if ( !applicationId.get() ) {
		mLastError = "The application identifier is not valid UTF-8.";
		return false;
	}
	for ( const auto& extension : supported ) {
		CFRef type( typeForExtension( extension ) );
		if ( !type.get() )
			continue;
		const bool shouldRegister =
			std::binary_search( selected.begin(), selected.end(), extension );
		if ( shouldRegister ) {
			const auto status = LSSetDefaultRoleHandlerForContentType(
				static_cast<CFStringRef>( type.get() ), kLSRolesEditor | kLSRolesViewer,
				static_cast<CFStringRef>( applicationId.get() ) );
			if ( status != noErr ) {
				mLastError = "Launch Services could not set a file association (error " +
							 std::to_string( status ) + ").";
				return false;
			}
			continue;
		}
		CFRef current( LSCopyDefaultRoleHandlerForContentType(
			static_cast<CFStringRef>( type.get() ), kLSRolesEditor | kLSRolesViewer ) );
		if ( !current.get() || !CFEqual( current.get(), applicationId.get() ) )
			continue;
		CFRef handlers( LSCopyAllRoleHandlersForContentType( static_cast<CFStringRef>( type.get() ),
															 kLSRolesEditor | kLSRolesViewer ) );
		if ( !handlers.get() )
			continue;
		auto array = static_cast<CFArrayRef>( handlers.get() );
		bool reassigned = false;
		for ( CFIndex index = 0; index < CFArrayGetCount( array ); ++index ) {
			auto handler = static_cast<CFStringRef>( CFArrayGetValueAtIndex( array, index ) );
			if ( CFEqual( handler, applicationId.get() ) )
				continue;
			if ( LSSetDefaultRoleHandlerForContentType( static_cast<CFStringRef>( type.get() ),
														kLSRolesEditor | kLSRolesViewer,
														handler ) == noErr ) {
				reassigned = true;
				break;
			}
		}
		if ( !reassigned ) {
			mLastError = "Launch Services has no alternative handler for ." + extension + ".";
			return false;
		}
	}
	return true;
#else
	(void)selected;
	(void)desktopEntryInstalled;
	(void)supported;
	mLastError = "File associations are not supported on this platform.";
	return false;
#endif
}

bool FileAssociation::isDesktopEntryInstalled() const {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	return desktopEntryVisible( mApplication );
#else
	return false;
#endif
}

bool FileAssociation::setDesktopEntryInstalled( bool installed ) {
	mLastError.clear();
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	return writeXdgRegistration( mApplication, desktopExtensions( mApplication ), installed,
								 mLastError );
#else
	(void)installed;
	mLastError = "Desktop entry installation is not supported on this platform.";
	return false;
#endif
}

} // namespace EE::System
