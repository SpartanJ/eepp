#include "location.hpp"
#include <eepp/system/filesystem.hpp>

using namespace EE;
using namespace EE::System;

namespace eefm {

URI Location::parse( const std::string& text ) {
	std::string trimmed = String::trim( text );
	if ( trimmed.find( "://" ) != std::string::npos ) {
		URI uri( trimmed );
		if ( uri.getPath().empty() )
			uri.setRawPath( "/" );
		return uri;
	}
	return URI::fromFSPath( FileSystem::expandTilde( trimmed ) );
}

String Location::displayName( const URI& uri ) {
	std::string name = uri.getLastPathSegment();
	if ( name.empty() || uri.getPath() == "/" )
		name = uri.getScheme() == "file" ? std::string( "/" ) : uri.getHost();
	return String::fromUtf8( name );
}

String Location::text( const URI& uri ) {
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
