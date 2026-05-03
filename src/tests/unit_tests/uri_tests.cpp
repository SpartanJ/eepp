#include "utest.hpp"
#include <eepp/network/uri.hpp>

using namespace EE;
using namespace EE::Network;

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
