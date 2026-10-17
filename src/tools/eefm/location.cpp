#include "location.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/vfs/providers/zipprovider.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::VFS;

namespace eefm {

URI Location::parse( const std::string& text ) {
	std::string trimmed = String::trim( text );
	if ( trimmed.find( "://" ) != std::string::npos || trimmed.starts_with( "zip:" ) ) {
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
	return {};
}

URI Location::parent( const URI& uri ) {
	if ( uri.getScheme() == "zip" ) {
		auto archive = ZipProvider::archiveURI( uri );
		auto member = ZipProvider::memberPath( uri );
		if ( archive && member ) {
			if ( member.value() == "/" )
				return archive.value().getParent();
			URI entry;
			entry.setRawPath( std::move( member ).value() );
			return ZipProvider::makeURI( archive.value(), entry.getParent().getPath() );
		}
	}
	return uri.getParent();
}

std::string Location::entryName( const URI& uri ) {
	if ( uri.getScheme() == "zip" ) {
		auto archive = ZipProvider::archiveURI( uri );
		auto member = ZipProvider::memberPath( uri );
		if ( archive && member && member.value() == "/" )
			return archive.value().getLastPathSegment();
	}
	return uri.getLastPathSegment();
}

String Location::displayName( const URI& uri ) {
	std::string name = entryName( uri );
	if ( uri.getScheme() == "zip" && !name.empty() )
		return String::fromUtf8( name );
	if ( name.empty() || uri.getPath() == "/" )
		name = uri.getScheme() == "file" ? std::string( "/" ) : uri.getHost();
	return String::fromUtf8( name );
}

String Location::text( const URI& uri ) {
	if ( uri.getScheme() == "zip" ) {
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
