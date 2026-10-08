#include <eepp/network/cookiemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <fstream>
#include <iomanip>

namespace EE { namespace Network {

CookieManager::CookieManager() {}

void CookieManager::storeCookies( const std::string& domain, const Http::Response& response ) {
	std::string setCookie = response.getField( "set-cookie" );
	if ( !setCookie.empty() )
		parseSetCookie( domain, setCookie );
}

void CookieManager::storeCookiesFromHeader( const std::string& domain,
											const std::string& setCookieHeader ) {
	if ( !setCookieHeader.empty() )
		parseSetCookie( domain, setCookieHeader );
}

std::string CookieManager::getCookieHeader( const std::string& domain ) const {
	Lock l( mMutex );
	auto it = mCookies.find( domain );
	if ( it == mCookies.end() || it->second.empty() )
		return "";

	std::string header;
	for ( const auto& pair : it->second ) {
		if ( !header.empty() )
			header += "; ";
		header += pair.first + "=" + pair.second;
	}
	return header;
}

void CookieManager::clear() {
	Lock l( mMutex );
	mCookies.clear();
}

void CookieManager::clearDomain( const std::string& domain ) {
	Lock l( mMutex );
	mCookies.erase( domain );
}

bool CookieManager::loadFromFile( const std::string& path ) {
	std::ifstream file( path, std::ios::binary );
	if ( !file )
		return false;
	std::string version;
	if ( !( file >> version ) || version != "eepp-cookies-1" )
		return false;
	UnorderedMap<std::string, std::map<std::string, std::string>> cookies;
	std::string domain, name, value;
	while ( file >> std::quoted( domain ) ) {
		if ( !( file >> std::quoted( name ) >> std::quoted( value ) ) || domain.empty() ||
			 name.empty() )
			return false;
		cookies[domain][name] = value;
	}
	if ( !file.eof() )
		return false;
	Lock l( mMutex );
	mCookies = std::move( cookies );
	return true;
}

bool CookieManager::saveToFile( const std::string& path ) const {
	const std::string temporaryPath = path + ".tmp";
	{
		std::ofstream file( temporaryPath, std::ios::binary | std::ios::trunc );
		if ( !file )
			return false;
		file << "eepp-cookies-1\n";
		{
			Lock l( mMutex );
			for ( const auto& domain : mCookies ) {
				for ( const auto& cookie : domain.second )
					file << std::quoted( domain.first ) << ' ' << std::quoted( cookie.first ) << ' '
						 << std::quoted( cookie.second ) << '\n';
			}
		}
		file.flush();
		if ( !file ) {
			FileSystem::fileRemove( temporaryPath );
			return false;
		}
	}
	if ( !FileSystem::fileMove( temporaryPath, path ) ) {
		FileSystem::fileRemove( temporaryPath );
		return false;
	}
	return true;
}

size_t CookieManager::size() const {
	Lock l( mMutex );
	size_t total = 0;
	for ( const auto& domainCookies : mCookies )
		total += domainCookies.second.size();
	return total;
}

bool CookieManager::empty() const {
	Lock l( mMutex );
	return mCookies.empty();
}

void CookieManager::parseSetCookie( const std::string& domain,
									const std::string& setCookieHeader ) {
	Lock l( mMutex );
	size_t end = setCookieHeader.find( ';' );
	std::string_view cookiePair( end != std::string::npos
									 ? std::string_view( setCookieHeader ).substr( 0, end )
									 : std::string_view( setCookieHeader ) );

	size_t eq = cookiePair.find( '=' );
	if ( eq == std::string::npos )
		return;

	std::string name( cookiePair.substr( 0, eq ) );
	std::string value( cookiePair.substr( eq + 1 ) );

	if ( name.empty() )
		return;

	mCookies[domain][String::trim( name )] = String::trim( value );
}

bool CookieManager::hasCookie( const std::string& domain ) const {
	Lock l( mMutex );
	return mCookies.find( domain ) != mCookies.end();
}

}} // namespace EE::Network
