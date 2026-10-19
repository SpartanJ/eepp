#include "location.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/vfs/providers/archiveprovider.hpp>
#include <eepp/vfs/providers/zipprovider.hpp>
#include <string_view>

using namespace EE;
using namespace EE::System;
using namespace EE::VFS;

namespace eefm {

namespace {

constexpr std::string_view TarSuffixes[] = { ".tar.gz", ".tar.bz2", ".tar.xz",	".tar.lzma",
											 ".tar.lz", ".tar.zst", ".tar.lz4", ".tar.z" };

} // namespace

URI Location::parse( const std::string& text ) {
	std::string trimmed = String::trim( text );
	if ( trimmed.find( "://" ) != std::string::npos || trimmed.starts_with( "zip:" ) ||
		 trimmed.starts_with( "archive:" ) ) {
		URI uri( trimmed );
		if ( uri.getPath().empty() )
			uri.setRawPath( "/" );
		return uri;
	}
	return URI::fromFSPath( FileSystem::expandTilde( trimmed ) );
}

URI Location::archiveRoot( const URI& file ) {
	if ( file.getScheme() != "file" )
		return {};
	const std::string extension = String::toLower( FileSystem::fileExtension( file.getPath() ) );
	if ( extension == "zip" || extension == "jar" || extension == "apk" || extension == "epub" )
		return ZipProvider::makeURI( file );
	if ( extension == "tar" || extension == "tgz" || extension == "tbz" || extension == "tbz2" ||
		 extension == "txz" || extension == "tlz" || extension == "tzst" || extension == "7z" ||
		 extension == "rar" || extension == "iso" || extension == "cab" || extension == "cpio" ||
		 extension == "ar" || extension == "deb" || extension == "xar" || extension == "lha" ||
		 extension == "lzh" )
		return ArchiveProvider::makeURI( file );
	const std::string name = String::toLower( file.getLastPathSegment() );
	for ( const auto suffix : TarSuffixes ) {
		if ( name.ends_with( suffix ) )
			return ArchiveProvider::makeURI( file );
	}
	return {};
}

std::string Location::archiveFolderName( const URI& file ) {
	std::string name = file.getLastPathSegment();
	const std::string lower = String::toLower( name );
	size_t suffix = name.find_last_of( '.' );
	for ( const auto tarSuffix : TarSuffixes ) {
		if ( lower.ends_with( tarSuffix ) ) {
			suffix = name.size() - tarSuffix.size();
			break;
		}
	}
	if ( suffix != std::string::npos )
		name.resize( suffix );
	if ( name.empty() || name == "." || name == ".." )
		return "archive";
	return name;
}

URI Location::parent( const URI& uri ) {
	if ( uri.getScheme() == "zip" || uri.getScheme() == "archive" ) {
		const bool zip = uri.getScheme() == "zip";
		auto archive = zip ? ZipProvider::archiveURI( uri ) : ArchiveProvider::archiveURI( uri );
		auto member = zip ? ZipProvider::memberPath( uri ) : ArchiveProvider::memberPath( uri );
		if ( archive && member ) {
			if ( member.value() == "/" )
				return archive.value().getParent();
			URI entry;
			entry.setRawPath( std::move( member ).value() );
			return zip ? ZipProvider::makeURI( archive.value(), entry.getParent().getPath() )
					   : ArchiveProvider::makeURI( archive.value(), entry.getParent().getPath() );
		}
	}
	return uri.getParent();
}

std::string Location::entryName( const URI& uri ) {
	if ( uri.getScheme() == "zip" || uri.getScheme() == "archive" ) {
		const bool zip = uri.getScheme() == "zip";
		auto archive = zip ? ZipProvider::archiveURI( uri ) : ArchiveProvider::archiveURI( uri );
		auto member = zip ? ZipProvider::memberPath( uri ) : ArchiveProvider::memberPath( uri );
		if ( archive && member && member.value() == "/" )
			return archive.value().getLastPathSegment();
	}
	return uri.getLastPathSegment();
}

String Location::displayName( const URI& uri ) {
	std::string name = entryName( uri );
	if ( ( uri.getScheme() == "zip" || uri.getScheme() == "archive" ) && !name.empty() )
		return String::fromUtf8( name );
	if ( name.empty() || uri.getPath() == "/" )
		name = uri.getScheme() == "file" ? std::string( "/" ) : uri.getHost();
	return String::fromUtf8( name );
}

String Location::text( const URI& uri ) {
	if ( uri.getScheme() == "zip" || uri.getScheme() == "archive" ) {
		// URI::toString() omits an empty authority for non-file schemes. Spell it explicitly
		// in the location bar; both spellings parse into the same provider URI.
		return String::fromUtf8( uri.getSchemeAndAuthority() + uri.getPathEtc() );
	}
	if ( uri.getScheme() == "file" ) {
		std::string path = uri.getFSPath();
		FileSystem::dirAddSlashAtEnd( path );
		return String::fromUtf8( path );
	}
	return String::fromUtf8( uri.toString() );
}

std::string Location::clipboardText( const URI& uri ) {
	if ( uri.getScheme() != "file" )
		return uri.toString();
	std::string path = uri.getFSPath();
	if ( path.size() > 1 && path.back() == '/' )
		path.pop_back();
	return path;
}

} // namespace eefm
