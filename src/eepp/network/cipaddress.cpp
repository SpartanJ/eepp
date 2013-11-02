#include <eepp/network/cipaddress.hpp>
#include <eepp/network/chttp.hpp>
#include <eepp/network/platform/platformimpl.hpp>
#include <cstring>

namespace {
	EE::Uint32 Resolve(const std::string& address) {
		if (address == "255.255.255.255") {
			// The broadcast address needs to be handled explicitely,
			// because it is also the value returned by inet_addr on error
			return INADDR_BROADCAST;
		} else {
			// Try to convert the address as a byte representation ("xxx.xxx.xxx.xxx")
			EE::Uint32 ip = inet_addr(address.c_str());
			if (ip != INADDR_NONE)
				return ip;

			// Not a valid address, try to convert it as a host name
			addrinfo hints;
			std::memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			addrinfo* result = NULL;

			if (getaddrinfo(address.c_str(), NULL, &hints, &result) == 0) {
				if (result) {
					ip = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr.s_addr;
					freeaddrinfo(result);
					return ip;
				}
			}

			// Not a valid address nor a host name
			return 0;
		}
	}
}

namespace EE { namespace Network {

const cIpAddress cIpAddress::None;
const cIpAddress cIpAddress::LocalHost(127, 0, 0, 1);
const cIpAddress cIpAddress::Broadcast(255, 255, 255, 255);

cIpAddress::cIpAddress() :
	mAddress(0)
{
	// We're using 0 (INADDR_ANY) instead of INADDR_NONE to represent the invalid address,
	// because the latter is also the broadcast address (255.255.255.255); it's ok because
	// eepp doesn't publicly use INADDR_ANY (it is always used implicitely)
}

cIpAddress::cIpAddress(const std::string& address) :
	mAddress(Resolve(address))
{
}

cIpAddress::cIpAddress(const char* address) :
	mAddress(Resolve(address))
{
}

cIpAddress::cIpAddress(Uint8 byte0, Uint8 byte1, Uint8 byte2, Uint8 byte3) :
	mAddress(htonl((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3))
{
}

cIpAddress::cIpAddress(Uint32 address) :
	mAddress(htonl(address))
{
}

std::string cIpAddress::ToString() const {
	in_addr address;
	address.s_addr = mAddress;

	return inet_ntoa(address);
}


Uint32 cIpAddress::ToInteger() const {
	return ntohl(mAddress);
}

cIpAddress cIpAddress::GetLocalAddress() {
	// The method here is to connect a UDP socket to anyone (here to localhost),
	// and get the local socket address with the getsockname function.
	// UDP connection will not send anything to the network, so this function won't cause any overhead.

	cIpAddress localAddress;

	// Create the socket
	SocketHandle sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == Private::cSocketImpl::InvalidSocket())
		return localAddress;

	// Connect the socket to localhost on any port
	sockaddr_in address = Private::cSocketImpl::CreateAddress(ntohl(INADDR_LOOPBACK), 9);
	if (connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
		Private::cSocketImpl::Close(sock);
		return localAddress;
	}

	// Get the local address of the socket connection
	Private::cSocketImpl::AddrLength size = sizeof(address);
	if (getsockname(sock, reinterpret_cast<sockaddr*>(&address), &size) == -1) {
		Private::cSocketImpl::Close(sock);
		return localAddress;
	}

	// Close the socket
	Private::cSocketImpl::Close(sock);

	// Finally build the IP address
	localAddress = cIpAddress(ntohl(address.sin_addr.s_addr));

	return localAddress;
}

cIpAddress cIpAddress::GetPublicAddress(cTime timeout) {
	// The trick here is more complicated, because the only way
	// to get our public IP address is to get it from a distant computer.
	// Here we get the web page from http://www.sfml-dev.org/ip-provider.php
	// and parse the result to extract our IP address
	// (not very hard: the web page contains only our IP address).
	cHttp server("www.sfml-dev.org");
	cHttp::Request request("/ip-provider.php", cHttp::Request::Get);
	cHttp::Response page = server.SendRequest(request, timeout);
	if (page.GetStatus() == cHttp::Response::Ok)
		return cIpAddress(page.GetBody());

	// Something failed: return an invalid address
	return cIpAddress();
}

bool operator ==(const cIpAddress& left, const cIpAddress& right) {
	return left.ToInteger() == right.ToInteger();
}

bool operator !=(const cIpAddress& left, const cIpAddress& right) {
	return !(left == right);
}

bool operator <(const cIpAddress& left, const cIpAddress& right) {
	return left.ToInteger() < right.ToInteger();
}

bool operator >(const cIpAddress& left, const cIpAddress& right) {
	return right < left;
}

bool operator <=(const cIpAddress& left, const cIpAddress& right) {
	return !(right < left);
}

bool operator >=(const cIpAddress& left, const cIpAddress& right) {
	return !(left < right);
}

std::istream& operator >>(std::istream& stream, cIpAddress& address) {
	std::string str;
	stream >> str;
	address = cIpAddress(str);

	return stream;
}

std::ostream& operator <<(std::ostream& stream, const cIpAddress& address) {
	return stream << address.ToString();
}

}}
