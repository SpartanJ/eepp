#include "utest.h"

#include <eepp/network/cookiemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::Network;

static Network::Http::Response fakeResponse( const std::string& setCookieValue ) {
	Network::Http::Request::FieldTable fields;
	fields["set-cookie"] = setCookieValue;
	Network::Http::Response::Status status{};
	return Network::Http::Response::createFakeResponse( fields, status, "" );
}

UTEST( CookieManager, storeAndRetrieve ) {
	CookieManager cm;
	auto response = fakeResponse( "session=abc123; Path=/; HttpOnly" );
	cm.storeCookies( "example.com", response );

	EXPECT_EQ( cm.size(), 1u );
	EXPECT_TRUE( !cm.empty() );
	EXPECT_TRUE( cm.getCookieHeader( "example.com" ) == "session=abc123" );
}

UTEST( CookieManager, domainIsolation ) {
	CookieManager cm;
	cm.storeCookies( "site-a.com", fakeResponse( "a=1" ) );
	cm.storeCookies( "site-b.com", fakeResponse( "b=2" ) );

	EXPECT_EQ( cm.size(), 2u );
	EXPECT_TRUE( cm.getCookieHeader( "site-a.com" ) == "a=1" );
	EXPECT_TRUE( cm.getCookieHeader( "site-b.com" ) == "b=2" );
	EXPECT_TRUE( cm.getCookieHeader( "other.com" ).empty() );
}

UTEST( CookieManager, multipleCookiesPerDomain ) {
	CookieManager cm;
	cm.storeCookies( "example.com", fakeResponse( "a=1; Path=/" ) );
	cm.storeCookies( "example.com", fakeResponse( "b=2" ) );

	EXPECT_EQ( cm.size(), 2u );
	std::string header = cm.getCookieHeader( "example.com" );
	EXPECT_TRUE( header.find( "a=1" ) != std::string::npos );
	EXPECT_TRUE( header.find( "b=2" ) != std::string::npos );
}

UTEST( CookieManager, ignoresAttributes ) {
	CookieManager cm;
	cm.storeCookies( "example.com",
					 fakeResponse( "token=xyz; Path=/app; HttpOnly; Secure; SameSite=Lax" ) );

	EXPECT_TRUE( cm.getCookieHeader( "example.com" ) == "token=xyz" );
}

UTEST( CookieManager, clearsAll ) {
	CookieManager cm;
	cm.storeCookies( "a.com", fakeResponse( "a=1" ) );
	cm.storeCookies( "b.com", fakeResponse( "b=2" ) );

	EXPECT_EQ( cm.size(), 2u );
	cm.clear();
	EXPECT_TRUE( cm.empty() );
	EXPECT_EQ( cm.size(), 0u );
}

UTEST( CookieManager, emptyCookieRejected ) {
	CookieManager cm;
	cm.storeCookies( "example.com", fakeResponse( "=empty" ) );
	EXPECT_TRUE( cm.empty() );
}

UTEST( CookieManager, storeFromRawHeader ) {
	CookieManager cm;
	cm.storeCookiesFromHeader( "example.com", "key=value; Path=/" );
	EXPECT_TRUE( cm.getCookieHeader( "example.com" ) == "key=value" );
}

UTEST( CookieManager, clearDomain ) {
	CookieManager cm;
	cm.storeCookiesFromHeader( "one.example", "a=1" );
	cm.storeCookiesFromHeader( "two.example", "b=2" );
	cm.clearDomain( "one.example" );
	EXPECT_FALSE( cm.hasCookie( "one.example" ) );
	EXPECT_TRUE( cm.hasCookie( "two.example" ) );
}

UTEST( CookieManager, persistence ) {
	const std::string path = Sys::getTempPath() + "eepp_cookies_test.txt";
	CookieManager saved;
	saved.storeCookiesFromHeader( "one.example", "name=value with spaces" );
	saved.storeCookiesFromHeader( "two.example", "other=\"quoted\"" );
	ASSERT_TRUE( saved.saveToFile( path ) );
	CookieManager loaded;
	ASSERT_TRUE( loaded.loadFromFile( path ) );
	EXPECT_TRUE( loaded.getCookieHeader( "one.example" ) == "name=value with spaces" );
	EXPECT_TRUE( loaded.getCookieHeader( "two.example" ) == "other=\"quoted\"" );
	saved.storeCookiesFromHeader( "one.example", "name=replaced" );
	ASSERT_TRUE( saved.saveToFile( path ) );
	ASSERT_TRUE( loaded.loadFromFile( path ) );
	EXPECT_TRUE( loaded.getCookieHeader( "one.example" ) == "name=replaced" );
	EXPECT_FALSE( FileSystem::fileExists( path + ".tmp" ) );
	FileSystem::fileRemove( path );
}
