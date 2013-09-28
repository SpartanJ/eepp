#include <eepp/network/chttp.hpp>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <sstream>

namespace {
	// Convert a string to lower case
	std::string toLower(std::string str) {
		for (std::string::iterator i = str.begin(); i != str.end(); ++i)
			*i = static_cast<char>(std::tolower(*i));
		return str;
	}
}

namespace EE { namespace Network {

cHttp::Request::Request(const std::string& uri, Method method, const std::string& body) {
	SetMethod(method);
	SetUri(uri);
	SetHttpVersion(1, 0);
	SetBody(body);
}

void cHttp::Request::SetField(const std::string& field, const std::string& value) {
	mFields[toLower(field)] = value;
}

void cHttp::Request::SetMethod(cHttp::Request::Method method) {
	mMethod = method;
}

void cHttp::Request::SetUri(const std::string& uri) {
	mUri = uri;

	// Make sure it starts with a '/'
	if (mUri.empty() || (mUri[0] != '/'))
		mUri.insert(0, "/");
}

void cHttp::Request::SetHttpVersion(unsigned int major, unsigned int minor) {
	mMajorVersion = major;
	mMinorVersion = minor;
}

void cHttp::Request::SetBody(const std::string& body) {
	mBody = body;
}

std::string cHttp::Request::Prepare() const {
	std::ostringstream out;

	// Convert the method to its string representation
	std::string method;
	switch (mMethod) {
		default :
		case Get :  method = "GET";  break;
		case Post : method = "POST"; break;
		case Head : method = "HEAD"; break;
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

bool cHttp::Request::HasField(const std::string& field) const {
	return mFields.find(toLower(field)) != mFields.end();
}

cHttp::Response::Response() :
	mStatus	  (ConnectionFailed),
	mMajorVersion(0),
	mMinorVersion(0)
{
}

const std::string& cHttp::Response::GetField(const std::string& field) const {
	FieldTable::const_iterator it = mFields.find(toLower(field));
	if (it != mFields.end()) {
		return it->second;
	} else {
		static const std::string empty = "";
		return empty;
	}
}

cHttp::Response::Status cHttp::Response::GetStatus() const {
	return mStatus;
}

unsigned int cHttp::Response::GetMajorHttpVersion() const {
	return mMajorVersion;
}

unsigned int cHttp::Response::GetMinorHttpVersion() const {
	return mMinorVersion;
}

const std::string& cHttp::Response::GetBody() const {
	return mBody;
}

void cHttp::Response::Parse(const std::string& data) {
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
	in.ignore(10000, '\n');

	// Parse the other lines, which contain fields, one by one
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

	// Finally extract the body
	mBody.clear();
	std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), std::back_inserter(mBody));
}

cHttp::cHttp() :
	mHost(),
	mPort(0)
{
}

cHttp::cHttp(const std::string& host, unsigned short port) {
	SetHost(host, port);
}

void cHttp::SetHost(const std::string& host, unsigned short port) {
	// Check the protocol
	if (toLower(host.substr(0, 7)) == "http://") {
		// HTTP protocol
		mHostName = host.substr(7);
		mPort	 = (port != 0 ? port : 80);
	} else if (toLower(host.substr(0, 8)) == "https://") {
		// HTTPS protocol -- unsupported (requires encryption and certificates and stuff...)
		//err() << "HTTPS protocol is not supported by cHttp" << std::endl;
		mHostName = "";
		mPort	 = 0;
	} else {
		// Undefined protocol - use HTTP
		mHostName = host;
		mPort	 = (port != 0 ? port : 80);
	}

	// Remove any trailing '/' from the host name
	if (!mHostName.empty() && (*mHostName.rbegin() == '/'))
		mHostName.erase(mHostName.size() - 1);

	mHost = cIpAddress(mHostName);
}

cHttp::Response cHttp::SendRequest(const cHttp::Request& request, cTime timeout) {
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
	if (mConnection.Connect(mHost, mPort, timeout) == cSocket::Done) {
		// Convert the request to string and send it through the connected socket
		std::string requestStr = toSend.Prepare();

		if (!requestStr.empty()) {
			// Send it through the socket
			if (mConnection.Send(requestStr.c_str(), requestStr.size()) == cSocket::Done) {
				// Wait for the server's response
				std::string receivedStr;
				std::size_t size = 0;
				char buffer[1024];

				while (mConnection.Receive(buffer, sizeof(buffer), size) == cSocket::Done) {
					receivedStr.append(buffer, buffer + size);
				}

				// Build the Response object from the received data
				received.Parse(receivedStr);
			}
		}

		// Close the connection
		mConnection.Disconnect();
	}

	return received;
}

}}
