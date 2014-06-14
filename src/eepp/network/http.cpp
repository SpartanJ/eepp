#include <eepp/network/http.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <limits>

using namespace EE::Network::SSL;

namespace {
	// Convert a string to lower case
	std::string toLower(std::string str) {
		for (std::string::iterator i = str.begin(); i != str.end(); ++i)
			*i = static_cast<char>(std::tolower(*i));
		return str;
	}
}

namespace EE { namespace Network {

Http::Request::Request(const std::string& uri, Method method, const std::string& body, bool validateCertificate, bool validateHostname ) :
	mValidateCertificate( validateCertificate ),
	mValidateHostname( validateHostname )
{
	SetMethod(method);
	SetUri(uri);
	SetHttpVersion(1, 0);
	SetBody(body);
}

void Http::Request::SetField(const std::string& field, const std::string& value) {
	mFields[toLower(field)] = value;
}

void Http::Request::SetMethod(Http::Request::Method method) {
	mMethod = method;
}

void Http::Request::SetUri(const std::string& uri) {
	mUri = uri;

	// Make sure it starts with a '/'
	if (mUri.empty() || (mUri[0] != '/'))
		mUri.insert(0, "/");
}

void Http::Request::SetHttpVersion(unsigned int major, unsigned int minor) {
	mMajorVersion = major;
	mMinorVersion = minor;
}

void Http::Request::SetBody(const std::string& body) {
	mBody = body;
}

const std::string &Http::Request::GetUri() const {
	return mUri;
}

const bool& Http::Request::ValidateCertificate() const {
	return mValidateCertificate;
}

void Http::Request::ValidateCertificate(bool enable) {
	mValidateCertificate = enable;
}

const bool &Http::Request::ValidateHostname() const {
	return mValidateHostname;
}

void Http::Request::ValidateHostname(bool enable) {
	mValidateHostname = enable;
}

std::string Http::Request::Prepare() const {
	std::ostringstream out;

	// Convert the method to its string representation
	std::string method;
	switch (mMethod) {
		default :
		case Get:		method = "GET";  break;
		case Post:		method = "POST"; break;
		case Head:		method = "HEAD"; break;
		case Put:		method = "PUT"; break;
		case Delete:	method = "DELETE"; break;
	}

	// Write the first line containing the request type
	out << method << " " << mUri << " ";
	out << "HTTP/" << mMajorVersion << "." << mMinorVersion << "\r\n";

	// Write fields
	for (FieldTable::const_iterator i = mFields.begin(); i != mFields.end(); ++i) {
		out << i->first << ": " << i->second << "\r\n";
	}

	// Use an extra \r\n to separate the header from the body
	out << "\r\n";

	// Add the body
	out << mBody;

	return out.str();
}

bool Http::Request::HasField(const std::string& field) const {
	return mFields.find(toLower(field)) != mFields.end();
}

Http::Response::Response() :
	mStatus	  (ConnectionFailed),
	mMajorVersion(0),
	mMinorVersion(0)
{
}

const std::string& Http::Response::GetField(const std::string& field) const {
	FieldTable::const_iterator it = mFields.find(toLower(field));
	if (it != mFields.end()) {
		return it->second;
	} else {
		static const std::string empty = "";
		return empty;
	}
}

Http::Response::Status Http::Response::GetStatus() const {
	return mStatus;
}

unsigned int Http::Response::GetMajorHttpVersion() const {
	return mMajorVersion;
}

unsigned int Http::Response::GetMinorHttpVersion() const {
	return mMinorVersion;
}

const std::string& Http::Response::GetBody() const {
	return mBody;
}

void Http::Response::Parse(const std::string& data) {
	std::istringstream in(data);

	// Extract the HTTP version from the first line
	std::string version;

	if (in >> version) {
		std::locale loc;
		if ((version.size() >= 8) && (version[6] == '.') &&
			(toLower(version.substr(0, 5)) == "http/")   &&
			 std::isdigit(version[5],loc) && std::isdigit(version[7],loc)) {
			mMajorVersion = version[5] - '0';
			mMinorVersion = version[7] - '0';
		} else {
			// Invalid HTTP version
			mStatus = InvalidResponse;
			return;
		}
	}

	// Extract the status code from the first line
	int status;

	if (in >> status) {
		mStatus = static_cast<Status>(status);
	} else {
		// Invalid status code
		mStatus = InvalidResponse;
		return;
	}

	// Ignore the end of the first line
	in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Parse the other lines, which contain fields, one by one
	ParseFields(in);

	// Finally extract the body
	mBody.clear();

	// Determine whether the transfer is chunked
	if (toLower(GetField("transfer-encoding")) != "chunked") {
		// Not chunked - everything at once
		std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), std::back_inserter(mBody));
	} else {
		// Chunked - have to read chunk by chunk
		std::size_t length;

		// Read all chunks, identified by a chunk-size not being 0
		while (in >> std::hex >> length) {
			// Drop the rest of the line (chunk-extension)
			in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

			// Copy the actual content data
			std::istreambuf_iterator<char> it(in);
			for (std::size_t i = 0; i < length; i++)
				mBody.push_back(*it++);
		}

		// Drop the rest of the line (chunk-extension)
		in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Read all trailers (if present)
		ParseFields(in);
	}
}

void Http::Response::ParseFields(std::istream &in) {
	std::string line;
	while (std::getline(in, line) && (line.size() > 2)) {
		std::string::size_type pos = line.find(": ");

		if (pos != std::string::npos) {
			// Extract the field name and its value
			std::string field = line.substr(0, pos);
			std::string value = line.substr(pos + 2);

			// Remove any trailing \r
			if (!value.empty() && (*value.rbegin() == '\r'))
				value.erase(value.size() - 1);

			// Add the field
			mFields[toLower(field)] = value;
		}
	}
}

Http::Http() :
	mConnection( NULL ),
	mHost(),
	mPort(0),
	mIsSSL( false )
{
}

Http::Http(const std::string& host, unsigned short port, bool useSSL) :
	mConnection( NULL ),
	mIsSSL( false )
{
	SetHost(host, port, useSSL);
}

Http::~Http() {
	std::list<cAsyncRequest*>::iterator itt;

	// First we wait to finish any request pending
	for ( itt = mThreads.begin(); itt != mThreads.end(); itt++ ) {
		(*itt)->Wait();
	}

	for ( itt = mThreads.begin(); itt != mThreads.end(); itt++ ) {
		eeDelete( *itt );
	}

	// Then we destroy the last open connection
	TcpSocket * tcp = mConnection;

	eeSAFE_DELETE( tcp );
}

void Http::SetHost(const std::string& host, unsigned short port, bool useSSL) {
	// Check the protocol
	if (toLower(host.substr(0, 7)) == "http://") {
		// HTTP protocol
		mHostName = host.substr(7);
		mPort	 = (port != 0 ? port : 80);
	} else if (toLower(host.substr(0, 8)) == "https://") {
		// HTTPS protocol
		#ifdef EE_SSL_SUPPORT
		mIsSSL		= true;
		mHostName	= host.substr(8);
		mPort		= (port != 0 ? port : 443);
		#else
		mHostName	= "";
		mPort		= 0;
		#endif
	} else {
		// Undefined protocol - use HTTP, unless SSL is specified
		mHostName	= host;
		mPort		= (port != 0 ? port : 80);

		#ifdef EE_SSL_SUPPORT
		mPort		= useSSL ? (port != 0 ? port : 443) : mPort;
		mIsSSL		= useSSL;
		#endif
	}

	// Remove any trailing '/' from the host name
	if (!mHostName.empty() && (*mHostName.rbegin() == '/'))
		mHostName.erase(mHostName.size() - 1);

	mHost = IpAddress(mHostName);
}

Http::Response Http::SendRequest(const Http::Request& request, cTime timeout) {
	if ( NULL == mConnection ) {
		TcpSocket * Conn	= mIsSSL ? eeNew( SSLSocket, ( mHostName, request.ValidateCertificate(), request.ValidateHostname() ) ) : eeNew( TcpSocket, () );
		mConnection			= Conn;
	}

	// First make sure that the request is valid -- add missing mandatory fields
	Request toSend(request);

	if (!toSend.HasField("From")) {
		toSend.SetField("From", "user@eepp.com.ar");
	}

	if (!toSend.HasField("User-Agent")) {
		toSend.SetField("User-Agent", "eepp-network");
	}

	if (!toSend.HasField("Host")) {
		toSend.SetField("Host", mHostName);
	}

	if (!toSend.HasField("Content-Length")) {
		std::ostringstream out;
		out << toSend.mBody.size();
		toSend.SetField("Content-Length", out.str());
	}

	if ((toSend.mMethod == Request::Post) && !toSend.HasField("Content-Type")) {
		toSend.SetField("Content-Type", "application/x-www-form-urlencoded");
	}

	if ((toSend.mMajorVersion * 10 + toSend.mMinorVersion >= 11) && !toSend.HasField("Connection")) {
		toSend.SetField("Connection", "close");
	}

	// Prepare the response
	Response received;

	// Connect the socket to the host
	if (mConnection->Connect(mHost, mPort, timeout) == Socket::Done) {
		// Convert the request to string and send it through the connected socket
		std::string requestStr = toSend.Prepare();

		if (!requestStr.empty()) {
			// Send it through the socket
			if (mConnection->Send(requestStr.c_str(), requestStr.size()) == Socket::Done) {
				// Wait for the server's response
				std::string receivedStr;
				std::size_t size = 0;
				char buffer[1024];

				while (mConnection->Receive(buffer, sizeof(buffer), size) == Socket::Done) {
					receivedStr.append(buffer, buffer + size);
				}

				// Build the Response object from the received data
				received.Parse(receivedStr);
			}
		}

		// Close the connection
		mConnection->Disconnect();
	}

	return received;
}

Http::cAsyncRequest::cAsyncRequest(Http *http, AsyncResponseCallback cb, Http::Request request, cTime timeout) :
	mHttp( http ),
	mCb( cb ),
	mRequest( request ),
	mTimeout( timeout ),
	mRunning( true )
{
}

void Http::cAsyncRequest::Run() {
	Http::Response response = mHttp->SendRequest( mRequest, mTimeout );

	mCb( *mHttp, mRequest, response );

	// The Async Request destroys the socket used to create the request
	TcpSocket * tcp = mHttp->mConnection;
	eeSAFE_DELETE( tcp );
	mHttp->mConnection = NULL;

	mRunning = false;
}

void Http::RemoveOldThreads() {
	std::list<cAsyncRequest*> remove;

	std::list<cAsyncRequest*>::iterator it = mThreads.begin();

	for ( ; it != mThreads.end(); it++ ) {
		cAsyncRequest * ar = (*it);

		if ( !ar->mRunning ) {
			// We need to be sure, since the state is set in the thread, this will not block the thread anyway
			ar->Wait();

			eeDelete( ar );

			remove.push_back( ar );
		}
	}

	for ( it = remove.begin(); it != remove.end(); it++ ) {
		mThreads.remove( (*it) );
	}
}

void Http::SendAsyncRequest( AsyncResponseCallback cb, const Http::Request& request, cTime timeout ) {
	cAsyncRequest * thread = eeNew( cAsyncRequest, ( this, cb, request, timeout ) );

	thread->Launch();

	// Clean old threads
	cLock l( mThreadsMutex );

	RemoveOldThreads();

	mThreads.push_back( thread );
}

const IpAddress &Http::GetHost() const {
	return mHost;
}

const std::string &Http::GetHostName() const {
	return mHostName;
}

const unsigned short& Http::GetPort() const {
	return mPort;
}

}}
