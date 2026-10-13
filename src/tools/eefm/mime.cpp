#include "mime.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

namespace eefm {

namespace {

struct BuiltinType {
	const char* extension;
	const char* mime;
	const char* description;
};

// Used where shared-mime-info is not installed (Windows, macOS) and as a description fallback.
constexpr BuiltinType BuiltinTypes[] = {
	{ "txt", "text/plain", "Plain text document" },
	{ "md", "text/markdown", "Markdown document" },
	{ "pdf", "application/pdf", "PDF document" },
	{ "png", "image/png", "PNG image" },
	{ "jpg", "image/jpeg", "JPEG image" },
	{ "jpeg", "image/jpeg", "JPEG image" },
	{ "gif", "image/gif", "GIF image" },
	{ "webp", "image/webp", "WebP image" },
	{ "svg", "image/svg+xml", "SVG image" },
	{ "bmp", "image/bmp", "Windows BMP image" },
	{ "mp3", "audio/mpeg", "MP3 audio" },
	{ "ogg", "audio/ogg", "Ogg audio" },
	{ "flac", "audio/flac", "FLAC audio" },
	{ "wav", "audio/x-wav", "WAV audio" },
	{ "mp4", "video/mp4", "MPEG-4 video" },
	{ "mkv", "video/x-matroska", "Matroska video" },
	{ "webm", "video/webm", "WebM video" },
	{ "zip", "application/zip", "Zip archive" },
	{ "gz", "application/gzip", "Gzip archive" },
	{ "tar", "application/x-tar", "Tar archive" },
	{ "7z", "application/x-7z-compressed", "7-zip archive" },
	{ "rar", "application/vnd.rar", "RAR archive" },
	{ "xz", "application/x-xz", "XZ archive" },
	{ "doc", "application/msword", "Word document" },
	{ "docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
	  "Word document" },
	{ "xls", "application/vnd.ms-excel", "Excel spreadsheet" },
	{ "xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
	  "Excel worksheet" },
	{ "ppt", "application/vnd.ms-powerpoint", "PowerPoint presentation" },
	{ "pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation",
	  "PowerPoint presentation" },
	{ "odt", "application/vnd.oasis.opendocument.text", "ODT document" },
	{ "ods", "application/vnd.oasis.opendocument.spreadsheet", "ODS spreadsheet" },
	{ "csv", "text/csv", "CSV document" },
	{ "html", "text/html", "HTML document" },
	{ "htm", "text/html", "HTML document" },
	{ "xml", "application/xml", "XML document" },
	{ "json", "application/json", "JSON document" },
	{ "c", "text/x-csrc", "C source code" },
	{ "h", "text/x-chdr", "C header" },
	{ "cpp", "text/x-c++src", "C++ source code" },
	{ "hpp", "text/x-c++hdr", "C++ header" },
	{ "py", "text/x-python", "Python script" },
	{ "js", "application/javascript", "JavaScript program" },
	{ "ts", "application/x-typescript", "TypeScript program" },
	{ "lua", "text/x-lua", "Lua script" },
	{ "sh", "application/x-shellscript", "Shell script" },
	{ "exe", "application/x-msdownload", "Windows executable" },
	{ "iso", "application/x-cd-image", "Disk image" },
};

/** Value of the first <comment> without xml:lang in a shared-mime-info type file. */
std::string readComment( const std::string& path ) {
	std::string xml;
	if ( !FileSystem::fileGet( path, xml ) )
		return {};
	size_t pos = 0;
	while ( ( pos = xml.find( "<comment", pos ) ) != std::string::npos ) {
		size_t close = xml.find( '>', pos );
		if ( close == std::string::npos )
			break;
		std::string_view tag( xml.data() + pos, close - pos );
		size_t end = xml.find( "</comment>", close );
		if ( end == std::string::npos )
			break;
		if ( tag.find( "xml:lang" ) == std::string_view::npos ) {
			std::string text = xml.substr( close + 1, end - close - 1 );
			String::replaceAll( text, "&amp;", "&" );
			String::replaceAll( text, "&lt;", "<" );
			String::replaceAll( text, "&gt;", ">" );
			String::replaceAll( text, "&apos;", "'" );
			String::replaceAll( text, "&quot;", "\"" );
			return text;
		}
		pos = end;
	}
	return {};
}

std::string lowerExtension( const std::string& name ) {
	size_t dot = name.find_last_of( '.' );
	if ( dot == std::string::npos || dot == 0 || dot + 1 == name.size() )
		return {};
	return String::toLower( name.substr( dot + 1 ) );
}

bool startsWith( const std::string& text, std::string_view prefix ) {
	return text.size() >= prefix.size() && text.compare( 0, prefix.size(), prefix ) == 0;
}

bool contains( const std::string& text, std::string_view needle ) {
	return text.find( needle ) != std::string::npos;
}

} // namespace

MimeDatabase& MimeDatabase::instance() {
	static MimeDatabase database;
	return database;
}

MimeDatabase::MimeDatabase() {}

void MimeDatabase::load() {
	if ( mLoaded )
		return;
	mLoaded = true;
	for ( const auto& builtin : BuiltinTypes )
		mExtensions[builtin.extension] = Glob{ 0, builtin.mime };
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_HAIKU
	std::string dataHome = Sys::getEnv( "XDG_DATA_HOME" );
	if ( dataHome.empty() ) {
		dataHome = Sys::getUserDirectory();
		FileSystem::dirAddSlashAtEnd( dataHome );
		dataHome += ".local/share";
	}
	mDataDirs.push_back( dataHome );
	std::string dataDirs = Sys::getEnv( "XDG_DATA_DIRS" );
	if ( dataDirs.empty() )
		dataDirs = "/usr/local/share:/usr/share";
	for ( const auto& dir : String::split( dataDirs, ':' ) )
		mDataDirs.push_back( dir );
	// Lower priority directories first, so user definitions override system ones.
	for ( auto it = mDataDirs.rbegin(); it != mDataDirs.rend(); ++it ) {
		std::string base = *it;
		FileSystem::dirAddSlashAtEnd( base );
		std::string globs;
		if ( FileSystem::fileGet( base + "mime/globs2", globs ) ) {
			mHasDatabase = true;
			for ( const auto& line : String::split( globs, '\n' ) ) {
				if ( line.empty() || line[0] == '#' )
					continue;
				auto fields = String::split( line, ':', true );
				if ( fields.size() < 3 )
					continue;
				int weight = 50;
				String::fromString( weight, fields[0] );
				const std::string& mime = fields[1];
				const std::string& glob = fields[2];
				auto keep = [&]( std::unordered_map<std::string, Glob>& map,
								 const std::string& key ) {
					auto found = map.find( key );
					if ( found == map.end() || found->second.weight <= weight )
						map[key] = Glob{ weight, mime };
				};
				if ( glob.size() > 2 && glob[0] == '*' && glob[1] == '.' &&
					 glob.find_first_of( "*?[", 2 ) == std::string::npos ) {
					keep( mExtensions, String::toLower( glob.substr( 2 ) ) );
				} else if ( glob.find_first_of( "*?[" ) == std::string::npos ) {
					keep( mNames, glob );
				} else {
					mPatterns.emplace_back( glob, Glob{ weight, mime } );
				}
			}
		}
		std::string subclasses;
		if ( FileSystem::fileGet( base + "mime/subclasses", subclasses ) ) {
			for ( const auto& line : String::split( subclasses, '\n' ) ) {
				auto fields = String::split( line, ' ' );
				if ( fields.size() == 2 )
					mParents[fields[0]].push_back( fields[1] );
			}
		}
	}
#endif
}

std::string MimeDatabase::typeForName( const std::string& name ) {
	std::lock_guard<std::mutex> lock( mMutex );
	load();
	auto exact = mNames.find( name );
	if ( exact != mNames.end() )
		return exact->second.mime;
	// Longest extension first ("tar.gz" before "gz").
	std::string lower = String::toLower( name );
	for ( size_t dot = lower.find( '.', 1 ); dot != std::string::npos;
		  dot = lower.find( '.', dot + 1 ) ) {
		auto found = mExtensions.find( lower.substr( dot + 1 ) );
		if ( found != mExtensions.end() )
			return found->second.mime;
	}
	for ( const auto& pattern : mPatterns )
		if ( String::globMatch( name, pattern.first, true ) )
			return pattern.second.mime;
	return "application/octet-stream";
}

std::string MimeDatabase::typeOf( const VFS::FileInfo& info ) {
	if ( info.isDirectory() )
		return "inode/directory";
	return typeForName( info.name );
}

std::vector<std::string> MimeDatabase::withParents( const std::string& mime ) {
	std::lock_guard<std::mutex> lock( mMutex );
	load();
	std::vector<std::string> out{ mime };
	for ( size_t i = 0; i < out.size() && out.size() < 16; ++i ) {
		auto found = mParents.find( out[i] );
		if ( found == mParents.end() )
			continue;
		for ( const auto& parent : found->second )
			if ( std::find( out.begin(), out.end(), parent ) == out.end() )
				out.push_back( parent );
	}
	if ( startsWith( mime, "text/" ) &&
		 std::find( out.begin(), out.end(), "text/plain" ) == out.end() )
		out.push_back( "text/plain" );
	return out;
}

std::string MimeDatabase::description( const std::string& mime, const std::string& name ) {
	{
		std::lock_guard<std::mutex> lock( mMutex );
		load();
		auto found = mDescriptions.find( mime );
		if ( found != mDescriptions.end() )
			return found->second;
		std::string text;
		if ( mime == "inode/directory" )
			text = "Folder";
		for ( size_t i = 0; text.empty() && i < mDataDirs.size(); ++i ) {
			std::string base = mDataDirs[i];
			FileSystem::dirAddSlashAtEnd( base );
			text = readComment( base + "mime/" + mime + ".xml" );
		}
		for ( size_t i = 0; text.empty() && i < std::size( BuiltinTypes ); ++i )
			if ( mime == BuiltinTypes[i].mime )
				text = BuiltinTypes[i].description;
		if ( !text.empty() ) {
			mDescriptions[mime] = text;
			return text;
		}
	}
	std::string extension = lowerExtension( name );
	if ( !extension.empty() )
		return String::toUpper( extension ) + " file";
	return "File";
}

std::string MimeDatabase::describe( const VFS::FileInfo& info ) {
	if ( info.isDirectory() )
		return "Folder";
	std::string extension = lowerExtension( info.name );
	std::string key = extension.empty() ? "\n" + info.name : extension;
	{
		std::lock_guard<std::mutex> lock( mMutex );
		auto found = mDescriptionByExtension.find( key );
		if ( found != mDescriptionByExtension.end() )
			return found->second;
	}
	std::string text = description( typeOf( info ), info.name );
	std::lock_guard<std::mutex> lock( mMutex );
	if ( mDescriptionByExtension.size() > 4096 )
		mDescriptionByExtension.clear();
	mDescriptionByExtension[key] = text;
	return text;
}

std::string MimeDatabase::iconName( const VFS::FileInfo& info ) {
	if ( info.isDirectory() )
		return "mime-folder";
	std::string mime = typeOf( info );
	if ( mime == "application/pdf" )
		return "mime-pdf";
	if ( mime == "text/markdown" )
		return "mime-markdown";
	if ( contains( mime, "spreadsheet" ) || contains( mime, "excel" ) || mime == "text/csv" )
		return "mime-spreadsheet";
	if ( contains( mime, "presentation" ) || contains( mime, "powerpoint" ) )
		return "mime-presentation";
	if ( contains( mime, "wordprocessing" ) || contains( mime, "msword" ) ||
		 contains( mime, "opendocument.text" ) || mime == "application/rtf" )
		return "mime-document";
	if ( startsWith( mime, "image/" ) )
		return "mime-image";
	if ( startsWith( mime, "audio/" ) )
		return "mime-audio";
	if ( startsWith( mime, "video/" ) )
		return "mime-video";
	if ( contains( mime, "zip" ) || contains( mime, "tar" ) || contains( mime, "rar" ) ||
		 contains( mime, "7z" ) || contains( mime, "compressed" ) || contains( mime, "xz" ) ||
		 contains( mime, "zstd" ) || contains( mime, "bzip" ) || contains( mime, "lzma" ) )
		return "mime-archive";
	if ( mime == "application/x-executable" || mime == "application/x-sharedlib" ||
		 mime == "application/x-msdownload" ||
		 mime == "application/vnd.microsoft.portable-executable" )
		return "mime-executable";
	if ( startsWith( mime, "text/x-" ) || contains( mime, "script" ) || contains( mime, "json" ) ||
		 contains( mime, "xml" ) || mime == "text/html" || mime == "text/css" )
		return "mime-code";
	if ( startsWith( mime, "text/" ) )
		return "mime-text";
	return "mime-generic";
}

} // namespace eefm
