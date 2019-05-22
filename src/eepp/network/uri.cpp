#include <eepp/network/uri.hpp>

namespace EE { namespace Network {

static void AppendHex(std::string& str, int value, int width) {
	eeASSERT (width > 0 && width < 64);

	char buffer[64];
	std::sprintf(buffer, "%0*X", width, value);
	str.append(buffer);
}

static std::string FormatHex(int value, int width) {
	std::string result;
	AppendHex(result, value, width);
	return result;
}

const std::string URI::RESERVED_PATH		= "?#";
const std::string URI::RESERVED_QUERY		= "#";
const std::string URI::RESERVED_FRAGMENT	= "";
const std::string URI::ILLEGAL				= "%<>{}|\\\"^`";

URI::URI():
	mPort(0)
{}

URI::URI(const std::string& uri):
	mPort(0)
{
	parse(uri);
}

URI::URI(const char* uri):
	mPort(0)
{
	parse(std::string(uri));
}

URI::URI(const std::string& scheme, const std::string& pathEtc):
	mScheme(scheme),
	mPort(0)
{
	String::toLowerInPlace(mScheme);
	mPort = getWellKnownPort();
	std::string::const_iterator beg = pathEtc.begin();
	std::string::const_iterator end = pathEtc.end();
	parsePathEtc(beg, end);
}

URI::URI(const std::string& scheme, const std::string& authority, const std::string& pathEtc):
	mScheme(scheme)
{
	String::toLowerInPlace(mScheme);
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
	beg = pathEtc.begin();
	end = pathEtc.end();
	parsePathEtc(beg, end);
}

URI::URI(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query):
	mScheme(scheme),
	mPath(path),
	mQuery(query)
{
	String::toLowerInPlace(mScheme);
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
}

URI::URI(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query, const std::string& fragment):
	mScheme(scheme),
	mPath(path),
	mQuery(query),
	mFragment(fragment)
{
	String::toLowerInPlace(mScheme);
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
}

URI::URI(const URI& uri):
	mScheme(uri.mScheme),
	mUserInfo(uri.mUserInfo),
	mHost(uri.mHost),
	mPort(uri.mPort),
	mPath(uri.mPath),
	mQuery(uri.mQuery),
	mFragment(uri.mFragment)
{
}

URI::URI(const URI& baseURI, const std::string& relativeURI):
	mScheme(baseURI.mScheme),
	mUserInfo(baseURI.mUserInfo),
	mHost(baseURI.mHost),
	mPort(baseURI.mPort),
	mPath(baseURI.mPath),
	mQuery(baseURI.mQuery),
	mFragment(baseURI.mFragment)
{
	resolve(relativeURI);
}

URI::~URI()
{}

URI& URI::operator = (const URI& uri) {
	if (&uri != this) {
		mScheme   = uri.mScheme;
		mUserInfo = uri.mUserInfo;
		mHost     = uri.mHost;
		mPort     = uri.mPort;
		mPath     = uri.mPath;
		mQuery    = uri.mQuery;
		mFragment = uri.mFragment;
	}
	return *this;
}

URI& URI::operator = (const std::string& uri) {
	clear();
	parse(uri);
	return *this;
}

URI& URI::operator = (const char* uri) {
	clear();
	parse(std::string(uri));
	return *this;
}

void URI::swap(URI& uri) {
	std::swap(mScheme, uri.mScheme);
	std::swap(mUserInfo, uri.mUserInfo);
	std::swap(mHost, uri.mHost);
	std::swap(mPort, uri.mPort);
	std::swap(mPath, uri.mPath);
	std::swap(mQuery, uri.mQuery);
	std::swap(mFragment, uri.mFragment);
}

void URI::clear() {
	mScheme.clear();
	mUserInfo.clear();
	mHost.clear();
	mPort = 0;
	mPath.clear();
	mQuery.clear();
	mFragment.clear();
}

std::string URI::toString() const {
	std::string uri;
	if (isRelative()) {
		encode(mPath, RESERVED_PATH, uri);
	} else {
		uri = mScheme;
		uri += ':';
		std::string auth = getAuthority();
		if (!auth.empty() || mScheme == "file") {
			uri.append("//");
			uri.append(auth);
		}
		if (!mPath.empty()) {
			if (!auth.empty() && mPath[0] != '/')
				uri += '/';
			encode(mPath, RESERVED_PATH, uri);
		} else if (!mQuery.empty() || !mFragment.empty()) {
			uri += '/';
		}
	}
	if (!mQuery.empty()) {
		uri += '?';
		uri.append(mQuery);
	}
	if (!mFragment.empty()) {
		uri += '#';
		encode(mFragment, RESERVED_FRAGMENT, uri);
	}
	return uri;
}

void URI::setScheme(const std::string& scheme) {
	mScheme = scheme;
	String::toLowerInPlace(mScheme);
	if (mPort == 0)
		mPort = getWellKnownPort();
}

void URI::setUserInfo(const std::string& userInfo) {
	mUserInfo.clear();
	decode(userInfo, mUserInfo);
}

void URI::getHost(const std::string& host) {
	mHost = host;
}

unsigned short URI::getPort() const {
	if (mPort == 0)
		return getWellKnownPort();
	else
		return mPort;
}

void URI::getPort(unsigned short port) {
	mPort = port;
}

std::string URI::getAuthority() const {
	std::string auth;
	if (!mUserInfo.empty()) {
		auth.append(mUserInfo);
		auth += '@';
	}
	if (mHost.find(':') != std::string::npos) {
		auth += '[';
		auth += mHost;
		auth += ']';
	}
	else auth.append(mHost);
	if (mPort && !isWellKnownPort()) {
		auth += ':';
		auth += String::toStr( mPort );
	}
	return auth;
}

void URI::setAuthority(const std::string& authority) {
	mUserInfo.clear();
	mHost.clear();
	mPort = 0;
	std::string::const_iterator beg = authority.begin();
	std::string::const_iterator end = authority.end();
	parseAuthority(beg, end);
}

std::string URI::getSchemeAndAuthority() {
	return getScheme() + getAuthority();
}

void URI::getPath(const std::string& path) {
	mPath.clear();
	decode(path, mPath);
}

void URI::setRawQuery(const std::string& query) {
	mQuery = query;
}

void URI::setQuery(const std::string& query) {
	mQuery.clear();
	encode(query, RESERVED_QUERY, mQuery);
}

std::string URI::getQuery() const {
	std::string query;
	decode(mQuery, query);
	return query;
}

void URI::getFragment(const std::string& fragment) {
	mFragment.clear();
	decode(fragment, mFragment);
}

void URI::setPathEtc(const std::string& pathEtc) {
	mPath.clear();
	mQuery.clear();
	mFragment.clear();
	std::string::const_iterator beg = pathEtc.begin();
	std::string::const_iterator end = pathEtc.end();
	parsePathEtc(beg, end);
}

std::string URI::getPathEtc() const {
	std::string pathEtc;
	encode(mPath, RESERVED_PATH, pathEtc);
	if (!mQuery.empty()) {
		pathEtc += '?';
		pathEtc += mQuery;
	}
	if (!mFragment.empty()) {
		pathEtc += '#';
		encode(mFragment, RESERVED_FRAGMENT, pathEtc);
	}
	return pathEtc;
}

std::string URI::getPathAndQuery() const {
	std::string pathAndQuery;
	encode(mPath, RESERVED_PATH, pathAndQuery);
	if (!mQuery.empty()) {
		pathAndQuery += '?';
		pathAndQuery += mQuery;
	}
	return pathAndQuery;
}

void URI::resolve(const std::string& relativeURI) {
	URI ParsedURI(relativeURI);
	resolve(ParsedURI);
}

void URI::resolve(const URI& relativeURI) {
	if (!relativeURI.mScheme.empty()) {
		mScheme   = relativeURI.mScheme;
		mUserInfo = relativeURI.mUserInfo;
		mHost     = relativeURI.mHost;
		mPort     = relativeURI.mPort;
		mPath     = relativeURI.mPath;
		mQuery    = relativeURI.mQuery;
		removeDotSegments();
	} else {
		if (!relativeURI.mHost.empty()) {
			mUserInfo = relativeURI.mUserInfo;
			mHost     = relativeURI.mHost;
			mPort     = relativeURI.mPort;
			mPath     = relativeURI.mPath;
			mQuery    = relativeURI.mQuery;
			removeDotSegments();
		} else {
			if (relativeURI.mPath.empty()) {
				if (!relativeURI.mQuery.empty())
					mQuery = relativeURI.mQuery;
			} else {
				if (relativeURI.mPath[0] == '/') {
					mPath = relativeURI.mPath;
					removeDotSegments();
				} else {
					mergePath(relativeURI.mPath);
				}
				mQuery = relativeURI.mQuery;
			}
		}
	}
	mFragment = relativeURI.mFragment;
}

bool URI::isRelative() const {
	return mScheme.empty();
}

bool URI::empty() const {
	return mScheme.empty() && mHost.empty() && mPath.empty() && mQuery.empty() && mFragment.empty();
}

bool URI::operator == (const URI& uri) const {
	return equals(uri);
}

bool URI::operator == (const std::string& uri) const {
	URI ParsedURI(uri);
	return equals(ParsedURI);
}

bool URI::operator != (const URI& uri) const {
	return !equals(uri);
}

bool URI::operator != (const std::string& uri) const {
	URI ParsedURI(uri);
	return !equals(ParsedURI);
}

bool URI::equals(const URI& uri) const {
	return mScheme   == uri.mScheme
		&& mUserInfo == uri.mUserInfo
		&& mHost     == uri.mHost
		&& getPort() == uri.getPort()
		&& mPath     == uri.mPath
		&& mQuery    == uri.mQuery
		&& mFragment == uri.mFragment;
}

void URI::normalize() {
	removeDotSegments(!isRelative());
}

void URI::removeDotSegments(bool removeLeading) {
	if (mPath.empty()) return;
	
	bool leadingSlash  = *(mPath.begin()) == '/';
	bool trailingSlash = *(mPath.rbegin()) == '/';
	std::vector<std::string> segments;
	std::vector<std::string> NormalizedSegments;
	getPathSegments(segments);
	for (std::vector<std::string>::const_iterator it = segments.begin(); it != segments.end(); ++it) {
		if (*it == "..") {
			if (!NormalizedSegments.empty()) {
				if (NormalizedSegments.back() == "..")
					NormalizedSegments.push_back(*it);
				else
					NormalizedSegments.pop_back();
			} else if (!removeLeading) {
				NormalizedSegments.push_back(*it);
			}
		} else if (*it != ".") {
			NormalizedSegments.push_back(*it);
		}
	}
	buildPath(NormalizedSegments, leadingSlash, trailingSlash);
}

void URI::getPathSegments(std::vector<std::string>& segments) {
	getPathSegments(mPath, segments);
}

std::string URI::getLastPathSegment() {
	std::vector<std::string> segments;
	getPathSegments( segments );

	if ( !segments.empty() ) {
		return segments[ segments.size() - 1 ];
	}

	return "";
}

void URI::getPathSegments(const std::string& path, std::vector<std::string>& segments) {
	std::string::const_iterator it  = path.begin();
	std::string::const_iterator end = path.end();
	std::string seg;
	while (it != end) {
		if (*it == '/') {
			if (!seg.empty()) {
				segments.push_back(seg);
				seg.clear();
			}
		}
		else seg += *it;
		++it;
	}
	if (!seg.empty())
		segments.push_back(seg);
}

void URI::encode(const std::string& str, const std::string& reserved, std::string& encodedStr) {
	for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
		char c = *it;
		if ((c >= 'a' && c <= 'z') || 
		    (c >= 'A' && c <= 'Z') || 
		    (c >= '0' && c <= '9') ||
		    c == '-' || c == '_' || 
			c == '.' || c == '~') {
			encodedStr += c;
		} else if (c <= 0x20 || c >= 0x7F || ILLEGAL.find(c) != std::string::npos || reserved.find(c) != std::string::npos) {
			encodedStr += '%';
			encodedStr += FormatHex((unsigned) (unsigned char) c, 2);
		}
		else encodedStr += c;
	}
}

void URI::decode(const std::string& str, std::string& decodedStr) {
	std::string::const_iterator it  = str.begin();
	std::string::const_iterator end = str.end();
	while (it != end) {
		char c = *it++;
		if (c == '%') {
			if (it == end) {
				return; //throw SyntaxException("URI encoding: no hex digit following percent sign", str);
			}

			char hi = *it++;
			if (it == end) {
				return; //throw SyntaxException("URI encoding: two hex digits must follow percent sign", str);
			}

			char lo = *it++;
			if (hi >= '0' && hi <= '9')
				c = hi - '0';
			else if (hi >= 'A' && hi <= 'F')
				c = hi - 'A' + 10;
			else if (hi >= 'a' && hi <= 'f')
				c = hi - 'a' + 10;
			else {
				return; //throw SyntaxException("URI encoding: not a hex digit");
			}
			c *= 16;
			if (lo >= '0' && lo <= '9')
				c += lo - '0';
			else if (lo >= 'A' && lo <= 'F')
				c += lo - 'A' + 10;
			else if (lo >= 'a' && lo <= 'f')
				c += lo - 'a' + 10;
			else {
				return; //throw SyntaxException("URI encoding: not a hex digit");
			}
		}
		decodedStr += c;
	}
}

bool URI::isWellKnownPort() const {
	return mPort == getWellKnownPort();
}

unsigned short URI::getWellKnownPort() const {
	if (mScheme == "ftp")
		return 21;
	else if (mScheme == "ssh")
		return 22;
	else if (mScheme == "telnet")
		return 23;
	else if (mScheme == "http")
		return 80;
	else if (mScheme == "nntp")
		return 119;
	else if (mScheme == "ldap")
		return 389;
	else if (mScheme == "https")
		return 443;
	else if (mScheme == "rtsp")
		return 554;
	else if (mScheme == "sip")
		return 5060;
	else if (mScheme == "sips")
		return 5061;
	else if (mScheme == "xmpp")
		return 5222;
	else
		return 0;
}

void URI::parse(const std::string& uri) {
	std::string::const_iterator it  = uri.begin();
	std::string::const_iterator end = uri.end();
	if (it == end) return;
	if (*it != '/' && *it != '.' && *it != '?' && *it != '#') {
		std::string scheme;
		while (it != end && *it != ':' && *it != '?' && *it != '#' && *it != '/') scheme += *it++;
		if (it != end && *it == ':') {
			++it;
			if (it == end) {
				return; //throw SyntaxException("URI scheme must be followed by authority or path", uri);
			}
			setScheme(scheme);
			if (*it == '/') {
				++it;
				if (it != end && *it == '/') {
					++it;
					parseAuthority(it, end);
				}
				else --it;
			}
			parsePathEtc(it, end);
		} else {
			it = uri.begin();
			parsePathEtc(it, end);
		}
	} else {
		parsePathEtc(it, end);
	}
}

void URI::parseAuthority(std::string::const_iterator& it, const std::string::const_iterator& end) {
	std::string userInfo;
	std::string part;
	while (it != end && *it != '/' && *it != '?' && *it != '#') {
		if (*it == '@') {
			userInfo = part;
			part.clear();
		}
		else part += *it;
		++it;
	}
	std::string::const_iterator pbeg = part.begin();
	std::string::const_iterator pend = part.end();
	parseHostAndPort(pbeg, pend);
	mUserInfo = userInfo;
}

void URI::parseHostAndPort(std::string::const_iterator& it, const std::string::const_iterator& end) {
	if (it == end) return;
	std::string host;
	if (*it == '[') {
		// IPv6 address
		++it;
		while (it != end && *it != ']') host += *it++;
		if (it == end) {
			return; //throw SyntaxException("unterminated IPv6 address");
		}
		++it;
	} else {
		while (it != end && *it != ':') host += *it++;
	}
	if (it != end && *it == ':') {
		++it;
		std::string port;
		while (it != end) port += *it++;
		if (!port.empty()) {
			int nport = 0;
			
			if ( String::fromString<int>( nport, port ) && nport > 0 && nport < 65536) {
				mPort = (unsigned short) nport;
			} else {
				return; //throw SyntaxException("bad or invalid port number", port);
			}
		} else {
			mPort = getWellKnownPort();
		}
	} else {
		mPort = getWellKnownPort();
	}
	mHost = host;
	String::toLowerInPlace(mHost);
}

void URI::parsePath(std::string::const_iterator& it, const std::string::const_iterator& end) {
	std::string path;
	while (it != end && *it != '?' && *it != '#') path += *it++;
	decode(path, mPath);
}

void URI::parsePathEtc(std::string::const_iterator& it, const std::string::const_iterator& end) {
	if (it == end) return;
	if (*it != '?' && *it != '#')
		parsePath(it, end);
	if (it != end && *it == '?') {
		++it;
		parseQuery(it, end);
	}
	if (it != end && *it == '#') {
		++it;
		parseFragment(it, end);
	}	
}

void URI::parseQuery(std::string::const_iterator& it, const std::string::const_iterator& end) {
	mQuery.clear();
	while (it != end && *it != '#') mQuery += *it++;
}

void URI::parseFragment(std::string::const_iterator& it, const std::string::const_iterator& end) {
	std::string fragment;
	while (it != end) fragment += *it++;
	decode(fragment, mFragment);
}

void URI::mergePath(const std::string& path) {
	std::vector<std::string> segments;
	std::vector<std::string> NormalizedSegments;
	bool addLeadingSlash = false;
	if (!mPath.empty()) {
		getPathSegments(segments);
		bool endsWithSlash = *(mPath.rbegin()) == '/';
		if (!endsWithSlash && !segments.empty())
			segments.pop_back();
		addLeadingSlash = mPath[0] == '/';
	}
	getPathSegments(path, segments);
	addLeadingSlash = addLeadingSlash || (!path.empty() && path[0] == '/');
	bool hasTrailingSlash = (!path.empty() && *(path.rbegin()) == '/');
	bool addTrailingSlash = false;
	for (std::vector<std::string>::const_iterator it = segments.begin(); it != segments.end(); ++it) {
		if (*it == "..") {
			addTrailingSlash = true;
			if (!NormalizedSegments.empty())
				NormalizedSegments.pop_back();
		} else if (*it != ".") {
			addTrailingSlash = false;
			NormalizedSegments.push_back(*it);
		}
		else addTrailingSlash = true;
	}
	buildPath(NormalizedSegments, addLeadingSlash, hasTrailingSlash || addTrailingSlash);
}

void URI::buildPath(const std::vector<std::string>& segments, bool leadingSlash, bool trailingSlash) {
	mPath.clear();
	bool first = true;
	for (std::vector<std::string>::const_iterator it = segments.begin(); it != segments.end(); ++it) {
		if (first) {
			first = false;
			if (leadingSlash)
				mPath += '/';
			else if (mScheme.empty() && (*it).find(':') != std::string::npos)
				mPath.append("./");
		}
		else mPath += '/';
		mPath.append(*it);
	}
	if (trailingSlash) 
		mPath += '/';
}

}}
