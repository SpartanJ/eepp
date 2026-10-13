#include "utest.hpp"
#include <eepp/network/uri.hpp>
#include <eepp/system/filesystem.hpp>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;

UTEST( URI, parseAbsolute ) {
	URI uri( "https://example.com/path/to/resource?q=hello#section" );
	EXPECT_STDSTREQ( "https", uri.getScheme() );
	EXPECT_STDSTREQ( "example.com", uri.getHost() );
	EXPECT_STDSTREQ( "/path/to/resource", uri.getPath() );
	EXPECT_STDSTREQ( "q=hello", uri.getQuery() );
	EXPECT_STDSTREQ( "section", uri.getFragment() );
	EXPECT_EQ( 443u, uri.getPort() );

	URI uri2( "http://user:pass@host.com:8080/api/data" );
	EXPECT_STDSTREQ( "http", uri2.getScheme() );
	EXPECT_STDSTREQ( "user:pass", uri2.getUserInfo() );
	EXPECT_STDSTREQ( "host.com", uri2.getHost() );
	EXPECT_EQ( 8080u, uri2.getPort() );
	EXPECT_STDSTREQ( "/api/data", uri2.getPath() );
}

UTEST( URI, parseRelative ) {
	URI uri( "relative/path/file.html" );
	EXPECT_TRUE( uri.getScheme().empty() );
	EXPECT_TRUE( uri.getHost().empty() );
	EXPECT_STDSTREQ( "relative/path/file.html", uri.getPath() );
	EXPECT_TRUE( uri.isRelative() );

	URI uri2( "../parent/file.html" );
	EXPECT_STDSTREQ( "../parent/file.html", uri2.getPath() );

	URI uri3( "/root/relative.html" );
	EXPECT_STDSTREQ( "/root/relative.html", uri3.getPath() );
}

UTEST( URI, parseProtocolRelative ) {
	URI uri( "//cdn.example.com/images/logo.png" );
	EXPECT_TRUE( uri.getScheme().empty() );
	EXPECT_STDSTREQ( "cdn.example.com", uri.getHost() );
	EXPECT_STDSTREQ( "/images/logo.png", uri.getPath() );
	EXPECT_STDSTREQ( "cdn.example.com", uri.getAuthority() );

	URI uri2( "//example.com:8080/api/data" );
	EXPECT_TRUE( uri2.getScheme().empty() );
	EXPECT_STDSTREQ( "example.com", uri2.getHost() );
	EXPECT_EQ( 8080u, uri2.getPort() );
	EXPECT_STDSTREQ( "/api/data", uri2.getPath() );

	URI uri3( "//example.com/search?q=hello" );
	EXPECT_STDSTREQ( "example.com", uri3.getHost() );
	EXPECT_STDSTREQ( "/search", uri3.getPath() );
	EXPECT_STDSTREQ( "q=hello", uri3.getQuery() );

	URI uri4( "//example.com/page#section" );
	EXPECT_STDSTREQ( "example.com", uri4.getHost() );
	EXPECT_STDSTREQ( "/page", uri4.getPath() );
	EXPECT_STDSTREQ( "section", uri4.getFragment() );

	URI uri5( "//user:pass@example.com/path" );
	EXPECT_STDSTREQ( "user:pass", uri5.getUserInfo() );
	EXPECT_STDSTREQ( "example.com", uri5.getHost() );
	EXPECT_STDSTREQ( "/path", uri5.getPath() );
}

UTEST( URI, toString ) {
	URI uri1( "https://example.com/path?q=1#frag" );
	EXPECT_STDSTREQ( "https://example.com/path?q=1#frag", uri1.toString() );

	URI uri2( "relative/path.html" );
	EXPECT_STDSTREQ( "relative/path.html", uri2.toString() );

	URI uri3( "//example.com/images/logo.png" );
	EXPECT_STDSTREQ( "//example.com/images/logo.png", uri3.toString() );

	URI uri4( "//example.com:8080/api" );
	EXPECT_STDSTREQ( "//example.com:8080/api", uri4.toString() );

	URI uri5( "file:///home/user/doc.html" );
	EXPECT_STDSTREQ( "file:///home/user/doc.html", uri5.toString() );
}

#if EE_PLATFORM == EE_PLATFORM_WIN
UTEST( URI, windowsFileURIConvertsToFilesystemPath ) {
	URI uri( "file:///c:/Temp/eepp-uiwebview.html" );
	EXPECT_STDSTREQ( "file", uri.getScheme() );
	EXPECT_STDSTREQ( "/c:/Temp/eepp-uiwebview.html", uri.getPath() );
	EXPECT_STDSTREQ( "C:\\Temp\\eepp-uiwebview.html", uri.getFSPath() );

	URI nativePathURI( "file://C:\\Temp\\eepp-uiwebview.html" );
	EXPECT_STDSTREQ( "file", nativePathURI.getScheme() );
	EXPECT_STDSTREQ( "/c:/Temp/eepp-uiwebview.html", nativePathURI.getPath() );
	EXPECT_STDSTREQ( "C:\\Temp\\eepp-uiwebview.html", nativePathURI.getFSPath() );
}
#endif

UTEST( URI, resolveProtocolRelativeAgainstHttp ) {
	URI base( "https://mysite.com/page.html" );
	URI rel( "//cdn.example.com/img/hero.jpg" );
	base.resolve( rel );

	EXPECT_STDSTREQ( "https", base.getScheme() );
	EXPECT_STDSTREQ( "cdn.example.com", base.getHost() );
	EXPECT_STDSTREQ( "/img/hero.jpg", base.getPath() );
	EXPECT_STDSTREQ( "https://cdn.example.com/img/hero.jpg", base.toString() );
}

UTEST( URI, resolveProtocolRelativeAgainstFile ) {
	URI base( "file:///home/user/page.html" );
	URI rel( "//example.com/image.png" );
	base.resolve( rel );

	EXPECT_STDSTREQ( "file", base.getScheme() );
	EXPECT_STDSTREQ( "example.com", base.getHost() );
	EXPECT_STDSTREQ( "/image.png", base.getPath() );
	EXPECT_STDSTREQ( "file://example.com/image.png", base.toString() );
}

UTEST( URI, resolveProtocolRelativeAgainstEmpty ) {
	URI base;
	URI rel( "//example.com/path" );
	base.resolve( rel );

	EXPECT_TRUE( base.getScheme().empty() );
	EXPECT_STDSTREQ( "example.com", base.getHost() );
	EXPECT_STDSTREQ( "/path", base.getPath() );
	EXPECT_STDSTREQ( "//example.com/path", base.toString() );
}

UTEST( URI, resolveAbsoluteKeepsTarget ) {
	URI base( "https://oldsite.com/oldpath" );
	URI abs( "https://newsite.com/newpath" );
	base.resolve( abs );

	EXPECT_STDSTREQ( "https", base.getScheme() );
	EXPECT_STDSTREQ( "newsite.com", base.getHost() );
	EXPECT_STDSTREQ( "/newpath", base.getPath() );
	EXPECT_STDSTREQ( "https://newsite.com/newpath", base.toString() );
}

UTEST( URI, resolveRootRelative ) {
	URI base( "https://example.com/dir/page.html" );
	URI rel( "/other/file.html" );
	base.resolve( rel );

	EXPECT_STDSTREQ( "https", base.getScheme() );
	EXPECT_STDSTREQ( "example.com", base.getHost() );
	EXPECT_STDSTREQ( "/other/file.html", base.getPath() );
}

UTEST( URI, resolveRelative ) {
	URI base( "https://example.com/dir/page.html" );
	URI rel( "other/file.html" );
	base.resolve( rel );

	EXPECT_STDSTREQ( "https", base.getScheme() );
	EXPECT_STDSTREQ( "example.com", base.getHost() );
	EXPECT_STDSTREQ( "/dir/other/file.html", base.getPath() );
}

UTEST( URI, resolveRelativeWithDots ) {
	URI base( "https://example.com/dir/sub/page.html" );
	URI rel( "../other/file.html" );
	base.resolve( rel );

	EXPECT_STDSTREQ( "/dir/other/file.html", base.getPath() );
}

UTEST( URI, resolveRelativeTrailingSlash ) {
	URI base( "https://example.com/dir/" );
	URI rel( "file.html" );
	base.resolve( rel );

	EXPECT_STDSTREQ( "/dir/file.html", base.getPath() );
}

UTEST( URI, isRelative ) {
	EXPECT_TRUE( URI( "relative/path" ).isRelative() );
	EXPECT_TRUE( URI( "//example.com/path" ).isRelative() );
	EXPECT_FALSE( URI( "https://example.com/path" ).isRelative() );
	EXPECT_FALSE( URI( "file:///path" ).isRelative() );
}

UTEST( URI, empty ) {
	URI empty;
	EXPECT_TRUE( empty.empty() );

	URI notEmpty( "https://example.com" );
	EXPECT_FALSE( notEmpty.empty() );

	URI protocolRelative( "//example.com" );
	EXPECT_FALSE( protocolRelative.empty() );
}

UTEST( URI, pathSegments ) {
	URI uri( "/path/to/resource" );
	std::vector<std::string> segments;
	uri.getPathSegments( segments );
	EXPECT_EQ( 3ul, segments.size() );
	EXPECT_STDSTREQ( "path", segments[0] );
	EXPECT_STDSTREQ( "to", segments[1] );
	EXPECT_STDSTREQ( "resource", segments[2] );
}

UTEST( URI, normalize ) {
	URI uri( "https://example.com/path/../other/./file.html" );
	uri.normalize();
	EXPECT_STDSTREQ( "/other/file.html", uri.getPath() );
}

UTEST( URI, getLastPathSegment ) {
	URI uri( "https://example.com/path/to/file.html?query=1" );
	EXPECT_STDSTREQ( "file.html", uri.getLastPathSegment() );

	URI uri2( "/path/to/dir/" );
	EXPECT_STDSTREQ( "dir", uri2.getLastPathSegment() );
}

UTEST( URI, schemeCaseInsensitive ) {
	URI uri( "HTTPS://Example.COM/Path" );
	EXPECT_STDSTREQ( "https", uri.getScheme() );
	EXPECT_STDSTREQ( "example.com", uri.getHost() );
	EXPECT_STDSTREQ( "/Path", uri.getPath() );
}

UTEST( URI, coreTypeAndNetworkAliasAreTheSame ) {
	static_assert( std::is_same_v<EE::URI, EE::Network::URI> );
	EE::URI a( "sftp://host/a" );
	EE::Network::URI b = a;
	EXPECT_TRUE( a == b );
	EXPECT_EQ( std::hash<EE::URI>()( a ), std::hash<EE::Network::URI>()( b ) );
}

UTEST( URI, explicitPortIsDistinguishableFromSchemeDefault ) {
	URI implicitPort( "sftp://host/path" );
	EXPECT_EQ( 22u, implicitPort.getPort() );
	EXPECT_EQ( 0u, implicitPort.getSpecifiedPort() );

	URI explicitDefault( "sftp://host:22/path" );
	EXPECT_EQ( 22u, explicitDefault.getSpecifiedPort() );

	URI explicitOther( "sftp://user@host:2222/path" );
	EXPECT_EQ( 2222u, explicitOther.getSpecifiedPort() );
	EXPECT_STDSTREQ( "user", explicitOther.getUserInfo() );
	EXPECT_STDSTREQ( "sftp://user@host:2222/path", explicitOther.toString() );

	URI moved( std::move( explicitOther ) );
	EXPECT_EQ( 2222u, moved.getSpecifiedPort() );
	URI assigned;
	assigned = std::move( moved );
	EXPECT_EQ( 2222u, assigned.getSpecifiedPort() );

	URI set( "sftp://host/x" );
	set.setPort( 22 );
	EXPECT_EQ( 22u, set.getSpecifiedPort() );
}

UTEST( URI, parentAndAppendedSegmentsAreVerbatim ) {
	URI base( "sftp://host/home/user" );
	URI child = base.withAppendedSegment( "a #b?c%20" );
	EXPECT_STDSTREQ( "/home/user/a #b?c%20", child.getPath() );
	EXPECT_TRUE( child.getQuery().empty() );
	EXPECT_TRUE( child.getFragment().empty() );
	EXPECT_TRUE( child.getParent() == base );
	EXPECT_STDSTREQ( "/", URI( "sftp://host/a" ).getParent().getPath() );
	EXPECT_STDSTREQ( "/", URI( "sftp://host/" ).getParent().getPath() );
	EXPECT_STDSTREQ( "/a", URI( "sftp://host/a/b/" ).getParent().getPath() );
	EXPECT_STDSTREQ( "/x", URI( "res:///" ).withAppendedSegment( "x" ).getPath() );

	// The encoded form round-trips through the parser.
	URI reparsed( child.toString() );
	EXPECT_STDSTREQ( child.getPath(), reparsed.getPath() );
}

UTEST( URI, isSameOrChildOf ) {
	URI root( "sftp://host/home/user" );
	EXPECT_TRUE( URI( "sftp://host/home/user" ).isSameOrChildOf( root ) );
	EXPECT_TRUE( URI( "sftp://host/home/user/x/y" ).isSameOrChildOf( root ) );
	EXPECT_FALSE( URI( "sftp://host/home/username" ).isSameOrChildOf( root ) );
	EXPECT_FALSE( URI( "sftp://other/home/user/x" ).isSameOrChildOf( root ) );
	EXPECT_FALSE( URI( "file:///home/user/x" ).isSameOrChildOf( root ) );
	EXPECT_TRUE( URI( "sftp://host/anything" ).isSameOrChildOf( URI( "sftp://host/" ) ) );
}

UTEST( URI, fromFSPathPreservesSpecialCharacters ) {
	std::string dir = FileSystem::getCurrentWorkingDirectory();
	FileSystem::dirAddSlashAtEnd( dir );
	std::string native = dir + "weird #name?x%41.txt";
	URI uri = URI::fromFSPath( native );
	EXPECT_STDSTREQ( "file", uri.getScheme() );
	EXPECT_STDSTREQ( native, uri.getFSPath() );
	URI reparsed( uri.toString() );
	EXPECT_STDSTREQ( native, reparsed.getFSPath() );

	// Relative paths are anchored to the working directory.
	EXPECT_STDSTREQ( dir + "rel.txt", URI::fromFSPath( "rel.txt" ).getFSPath() );
}

#if EE_PLATFORM == EE_PLATFORM_WIN
UTEST( URI, fromFSPathWindowsDrivesAndUNC ) {
	URI drive = URI::fromFSPath( "C:\\Users\\me\\file.txt" );
	EXPECT_STDSTREQ( "/c:/Users/me/file.txt", drive.getPath() );
	EXPECT_STDSTREQ( "C:\\Users\\me\\file.txt", drive.getFSPath() );
	EXPECT_STDSTREQ( "/c:/", URI::fromFSPath( "C:" ).getPath() );
	EXPECT_STDSTREQ( "/c:/", URI::fromFSPath( "C:\\Users" ).getParent().getPath() );

	URI unc = URI::fromFSPath( "\\\\server\\share\\dir\\f.txt" );
	EXPECT_STDSTREQ( "server", unc.getHost() );
	EXPECT_STDSTREQ( "/share/dir/f.txt", unc.getPath() );
	EXPECT_STDSTREQ( "\\\\server\\share\\dir\\f.txt", unc.getFSPath() );
}
#else
UTEST( URI, fromFSPathPosixRoot ) {
	URI root = URI::fromFSPath( "/" );
	EXPECT_STDSTREQ( "/", root.getPath() );
	EXPECT_STDSTREQ( "file:///", root.toString() );
	EXPECT_STDSTREQ( "/", root.getParent().getPath() );
	EXPECT_STDSTREQ( "/tmp/\xc3\xa1rbol", URI::fromFSPath( "/tmp/\xc3\xa1rbol" ).getFSPath() );
}
#endif
