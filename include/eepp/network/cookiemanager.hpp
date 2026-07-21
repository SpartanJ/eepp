#ifndef EE_NETWORK_COOKIEMANAGER_HPP
#define EE_NETWORK_COOKIEMANAGER_HPP

#include <eepp/config.hpp>
#include <eepp/network/http.hpp>

#include <map>
#include <string>

namespace EE { namespace Network {

class EE_API CookieManager {
  public:
	CookieManager();

	/** Store Set-Cookie headers from an HTTP response for the given domain. */
	void storeCookies( const std::string& domain, const Http::Response& response );

	/** Store cookies from a raw Set-Cookie header string. */
	void storeCookiesFromHeader( const std::string& domain, const std::string& setCookieHeader );

	/** Build the Cookie header string for outgoing requests to the given domain. */
	std::string getCookieHeader( const std::string& domain ) const;

	/** Remove all stored cookies. */
	void clear();

	/** @return The number of cookie entries across all domains. */
	size_t size() const;

	/** @return true if no cookies are stored. */
	bool empty() const;

	/** @return true if the domain has cookies */
	bool hasCookie( const std::string& domain ) const;

  protected:
	mutable Mutex mMutex;
	UnorderedMap<std::string, std::map<std::string, std::string>> mCookies;

	void parseSetCookie( const std::string& domain, const std::string& setCookieHeader );
};

}} // namespace EE::Network

#endif
