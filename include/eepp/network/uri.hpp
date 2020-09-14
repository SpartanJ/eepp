#ifndef EE_NETWORK_URI_HPP
#define EE_NETWORK_URI_HPP

#include <eepp/core.hpp>
#include <vector>

namespace EE { namespace Network {

/** @class EE::Network::URI
* A Uniform Resource Identifier, as specified in RFC 3986.
*
* The URI class provides methods for building URIs from their
* parts, as well as for splitting URIs into their parts.
* Furthermore, the class provides methods for resolving
* relative URIs against base URIs.
*
* The class automatically performs a few normalizations on
* all URIs and URI parts passed to it:
* scheme identifiers are converted to lower case.
* percent-encoded characters are decoded
* optionally, dot segments are removed from paths (see Normalize())
* Examples of URI structure:

@code
  foo://username:password@example.com:8042/over/there/index.dtb?type=animal&name=narwhal#nose
  \_/   \_______________/ \_________/ \__/            \___/ \_/ \______________________/ \__/
   |           |               |       |                |    |            |                |
   |       userinfo         hostname  port              |    |          query          fragment
   |    \________________________________/\_____________|____|/ \__/        \__/
   |                    |                          |    |    |    |          |
   |                    |                          |    |    |    |          |
scheme              authority                    path   |    |    interpretable as keys
 name   \_______________________________________________|____|/       \____/     \_____/
   |                         |                          |    |          |           |
   |                 hierarchical part                  |    |    interpretable as values
   |                                                    |    |
   |            path               interpretable as filename |
   |   ___________|____________                              |
  / \ /                        \                             |
  urn:example:animal:ferret:nose               interpretable as extension

				path
		 _________|________
 scheme /                  \
  name  userinfo  hostname       query
  _|__   ___|__   ____|____   _____|_____
 /    \ /      \ /         \ /           \
 mailto:username@example.com?subject=Topic
@endcode

This class is based on the Poco library URI class.
*/
class EE_API URI {
  public:
	/** Creates an empty URI. */
	URI();

	/** Parses an URI from the given string. */
	URI( const std::string& uri );

	/** Parses an URI from the given string. */
	URI( const char* uri );

	/** Creates an URI from its parts. */
	URI( const std::string& scheme, const std::string& pathEtc );

	/** Creates an URI from its parts. */
	URI( const std::string& scheme, const std::string& authority, const std::string& pathEtc );

	/** Creates an URI from its parts. */
	URI( const std::string& scheme, const std::string& authority, const std::string& path,
		 const std::string& query );

	/** Creates an URI from its parts. */
	URI( const std::string& scheme, const std::string& authority, const std::string& path,
		 const std::string& query, const std::string& fragment );

	/** Copy constructor. Creates an URI from another one. */
	URI( const URI& uri );

	/** Creates an URI from a base URI and a relative URI, according to the algorithm in section 5.2
	 * of RFC 3986. */
	URI( const URI& baseURI, const std::string& relativeURI );

	/** Destroys the URI. */
	~URI();

	/** Assignment operator. */
	URI& operator=( const URI& uri );

	/** Parses and assigns an URI from the given string. */
	URI& operator=( const std::string& uri );

	/** Parses and assigns an URI from the given string. */
	URI& operator=( const char* uri );

	/** Swaps the URI with another one. */
	void swap( URI& uri );

	/** Clears all parts of the URI. */
	void clear();

	/** @returns a string representation of the URI.
	 * Characters in the path, query and fragment parts will be percent-encoded as necessary. */
	std::string toString() const;

	/** @returns the scheme part of the URI. */
	const std::string& getScheme() const;

	/** Sets the scheme part of the URI. The given scheme is converted to lower-case.
	 * A list of registered URI schemes can be found at
	 * <http://www.iana.org/assignments/uri-schemes>. */
	void setScheme( const std::string& scheme );

	/** @returns the user-info part of the URI. */
	const std::string& getUserInfo() const;

	/** Sets the user-info part of the URI. */
	void setUserInfo( const std::string& userInfo );

	/** @returns the host part of the URI. */
	const std::string& getHost() const;

	/** Sets the host part of the URI. */
	void setHost( const std::string& host );

	/** @returns the port number part of the URI.
	 * If no port number (0) has been specified, the
	 * well-known port number (e.g., 80 for http) for
	 * the given scheme is returned if it is known.
	 * Otherwise, 0 is returned. */
	unsigned short getPort() const;

	/** Sets the port number part of the URI. */
	void getPort( unsigned short port );

	/** @returns the authority part (userInfo, host and port) of the URI.
	 * If the port number is a well-known port
	 * number for the given scheme (e.g., 80 for http), it
	 * is not included in the authority. */
	std::string getAuthority() const;

	/** Parses the given authority part for the URI and sets
	 * the user-info, host, port components accordingly. */
	void setAuthority( const std::string& authority );

	/** @returns The scheme and authority of the URI. */
	std::string getSchemeAndAuthority() const;

	/** @returns The path part of the URI. */
	const std::string& getPath() const;

	/** Sets the path part of the URI. */
	void getPath( const std::string& path );

	/** @returns the query part of the URI. */
	std::string getQuery() const;

	/** Sets the query part of the URI. */
	void setQuery( const std::string& query );

	/** @returns the unencoded query part of the URI. */
	const std::string& getRawQuery() const;

	/** Sets the query part of the URI. */
	void setRawQuery( const std::string& query );

	/** @returns the fragment part of the URI. */
	const std::string& getFragment() const;

	/** Sets the fragment part of the URI. */
	void getFragment( const std::string& fragment );

	/** Sets the path, query and fragment parts of the URI. */
	void setPathEtc( const std::string& pathEtc );

	/** @returns the path, query and fragment parts of the URI. */
	std::string getPathEtc() const;

	/** @returns the path and query parts of the URI. */
	std::string getPathAndQuery() const;

	/** Resolves the given relative URI against the base URI.
	 * See section 5.2 of RFC 3986 for the algorithm used. */
	void resolve( const std::string& relativeURI );

	/** Resolves the given relative URI against the base URI.
	 * See section 5.2 of RFC 3986 for the algorithm used. */
	void resolve( const URI& relativeURI );

	/** @returns true if the URI is a relative reference, false otherwise.
	 * A relative reference does not contain a scheme identifier.
	 * Relative references are usually resolved against an absolute
	 * base reference. */
	bool isRelative() const;

	/** @returns true if the URI is empty, false otherwise. */
	bool empty() const;

	/** @returns true if both URIs are identical, false otherwise.
	 * Two URIs are identical if their scheme, authority,
	 * path, query and fragment part are identical. */
	bool operator==( const URI& uri ) const;

	/** Parses the given URI and returns true if both URIs are identical, false otherwise. */
	bool operator==( const std::string& uri ) const;

	/** @returns true if both URIs are identical, false otherwise. */
	bool operator!=( const URI& uri ) const;

	/** Parses the given URI and returns true if both URIs are identical, false otherwise. */
	bool operator!=( const std::string& uri ) const;

	/** Normalizes the URI by removing all but leading . and .. segments from the path.
	 * If the first path segment in a relative path contains a colon (:),
	 * such as in a Windows path containing a drive letter, a dot segment (./)
	 * is prepended in accordance with section 3.3 of RFC 3986. */
	void normalize();

	/** Places the single path segments (delimited by slashes) into the given vector. */
	void getPathSegments( std::vector<std::string>& segments );

	/** @return The last path segment if any */
	std::string getLastPathSegment();

	/** URI-encodes the given string by escaping reserved and non-ASCII
	 * characters. The encoded string is appended to encodedStr. */
	static void encode( const std::string& str, const std::string& reserved,
						std::string& encodedStr );

	/** URI-decodes the given string by replacing percent-encoded
	 * characters with the actual character. The decoded string
	 * is appended to decodedStr. */
	static void decode( const std::string& str, std::string& decodedStr );

	/** URI encodes the string.  */
	static std::string encode( const std::string& str );

	/** URI decodes the string. */
	static std::string decode( const std::string& str );

  protected:
	/** @returns true if both uri's are equivalent. */
	bool equals( const URI& uri ) const;

	/** @returns true if the URI's port number is a well-known one
	 * (for example, 80, if the scheme is http). */
	bool isWellKnownPort() const;

	/** @returns the well-known port number for the URI's scheme,
	 * or 0 if the port number is not known. */
	unsigned short getWellKnownPort() const;

	/** Parses and assigns an URI from the given string. Throws a
	 * SyntaxException if the uri is not valid. */
	void parse( const std::string& uri );

	/** Parses and sets the user-info, host and port from the given data. */
	void parseAuthority( std::string::const_iterator& it, const std::string::const_iterator& end );

	/** Parses and sets the host and port from the given data. */
	void parseHostAndPort( std::string::const_iterator& it,
						   const std::string::const_iterator& end );

	/** Parses and sets the path from the given data. */
	void parsePath( std::string::const_iterator& it, const std::string::const_iterator& end );

	/** Parses and sets the path, query and fragment from the given data. */
	void parsePathEtc( std::string::const_iterator& it, const std::string::const_iterator& end );

	/** Parses and sets the query from the given data. */
	void parseQuery( std::string::const_iterator& it, const std::string::const_iterator& end );

	/** Parses and sets the fragment from the given data. */
	void parseFragment( std::string::const_iterator& it, const std::string::const_iterator& end );

	/** Appends a path to the URI's path. */
	void mergePath( const std::string& path );

	/** Removes all dot segments from the path. */
	void removeDotSegments( bool removeLeading = true );

	/** Places the single path segments (delimited by slashes) into the given vector. */
	static void getPathSegments( const std::string& path, std::vector<std::string>& segments );

	/** Builds the path from the given segments. */
	void buildPath( const std::vector<std::string>& segments, bool leadingSlash,
					bool trailingSlash );

	static const std::string RESERVED_PATH;
	static const std::string RESERVED_QUERY;
	static const std::string RESERVED_FRAGMENT;
	static const std::string ILLEGAL;

  private:
	std::string mScheme;
	std::string mUserInfo;
	std::string mHost;
	unsigned short mPort;
	std::string mPath;
	std::string mQuery;
	std::string mFragment;
};

inline const std::string& URI::getScheme() const {
	return mScheme;
}

inline const std::string& URI::getUserInfo() const {
	return mUserInfo;
}

inline const std::string& URI::getHost() const {
	return mHost;
}

inline const std::string& URI::getPath() const {
	return mPath;
}

inline const std::string& URI::getRawQuery() const {
	return mQuery;
}

inline const std::string& URI::getFragment() const {
	return mFragment;
}

inline void swap( URI& u1, URI& u2 ) {
	u1.swap( u2 );
}

}} // namespace EE::Network

#endif
