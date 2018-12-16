#include <eepp/network/ipaddress.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/platform/platformimpl.hpp>
#include <cstring>
#include <utility>

namespace EE { namespace Network {

const IpAddress IpAddress::None;
const IpAddress IpAddress::Any(0, 0, 0, 0);
const IpAddress IpAddress::LocalHost(127, 0, 0, 1);
const IpAddress IpAddress::Broadcast(255, 255, 255, 255);

IpAddress::IpAddress() :
	mAddress(0),
	mValid(false)
{
}

IpAddress::IpAddress(const std::string& address) :
	mAddress(0),
	mValid(false)
{
	resolve(address);
}

IpAddress::IpAddress(const char* address) :
	mAddress(0),
	mValid(false)
{
	resolve(address);
}

IpAddress::IpAddress(Uint8 byte0, Uint8 byte1, Uint8 byte2, Uint8 byte3) :
	mAddress(htonl((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3)),
	mValid(true)
{
}

IpAddress::IpAddress(Uint32 address) :
	mAddress(htonl(address)),
	mValid(true)
{
}

std::string IpAddress::toString() const {
	in_addr address;
	address.s_addr = mAddress;

	return inet_ntoa(address);
}


Uint32 IpAddress::toInteger() const {
	return ntohl(mAddress);
}

IpAddress IpAddress::getLocalAddress() {
	// The method here is to connect a UDP socket to anyone (here to localhost),
	// and get the local socket address with the getsockname function.
	// UDP connection will not send anything to the network, so this function won't cause any overhead.

	IpAddress localAddress;

	// Create the socket
	SocketHandle sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == Private::SocketImpl::invalidSocket())
		return localAddress;

	// Connect the socket to localhost on any port
	sockaddr_in address = Private::SocketImpl::createAddress(ntohl(INADDR_LOOPBACK), 9);
	if (connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
		Private::SocketImpl::close(sock);
		return localAddress;
	}

	// Get the local address of the socket connection
	Private::SocketImpl::AddrLength size = sizeof(address);
	if (getsockname(sock, reinterpret_cast<sockaddr*>(&address), &size) == -1) {
		Private::SocketImpl::close(sock);
		return localAddress;
	}

	// Close the socket
	Private::SocketImpl::close(sock);

	// Finally build the IP address
	localAddress = IpAddress(ntohl(address.sin_addr.s_addr));

	return localAddress;
}

IpAddress IpAddress::getPublicAddress(Time timeout) {
	// The trick here is more complicated, because the only way
	// to get our public IP address is to get it from a distant computer.
	// Here we get the web page from http://www.sfml-dev.org/ip-provider.php
	// and parse the result to extract our IP address
	// (not very hard: the web page contains only our IP address).
	Http server("ip.ensoft-dev.com");
	Http::Request request("/", Http::Request::Get);
	Http::Response page = server.sendRequest(request, timeout);
	if (page.getStatus() == Http::Response::Ok)
		return IpAddress(page.getBody());

	// Something failed: return an invalid address
	return IpAddress();
}

void IpAddress::resolve(const std::string& address) {
	mAddress = 0;
	mValid = false;

	if (address == "255.255.255.255")
	{
		// The broadcast address needs to be handled explicitly,
		// because it is also the value returned by inet_addr on error
		mAddress = INADDR_BROADCAST;
		mValid = true;
	}
	else if (address == "0.0.0.0")
	{
		mAddress = INADDR_ANY;
		mValid = true;
	}
	else
	{
		// Try to convert the address as a byte representation ("xxx.xxx.xxx.xxx")
		Uint32 ip = inet_addr(address.c_str());
		if (ip != INADDR_NONE)
		{
			mAddress = ip;
			mValid = true;
		}
		else
		{
			// Not a valid address, try to convert it as a host name
			addrinfo hints;
			std::memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			addrinfo* result = NULL;
			int res = getaddrinfo(address.c_str(), NULL, &hints, &result);
			if (res == 0)
			{
				if (result)
				{
					ip = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr.s_addr;
					freeaddrinfo(result);
					mAddress = ip;
					mValid = true;
				}
			}
			else
			{
				eePRINTL( "getaddrinfo on \"%s\": %s", address.c_str(), gai_strerror(res) );
			}
		}
	}
}

bool operator ==(const IpAddress& left, const IpAddress& right) {
	return !(left < right) && !(right < left);
}

bool operator !=(const IpAddress& left, const IpAddress& right) {
	return !(left == right);
}

bool operator <(const IpAddress& left, const IpAddress& right) {
	return std::make_pair(left.mValid, left.mAddress) < std::make_pair(right.mValid, right.mAddress);
}

bool operator >(const IpAddress& left, const IpAddress& right) {
	return right < left;
}

bool operator <=(const IpAddress& left, const IpAddress& right) {
	return !(right < left);
}

bool operator >=(const IpAddress& left, const IpAddress& right) {
	return !(left < right);
}

std::istream& operator >>(std::istream& stream, IpAddress& address) {
	std::string str;
	stream >> str;
	address = IpAddress(str);

	return stream;
}

std::ostream& operator <<(std::ostream& stream, const IpAddress& address) {
	return stream << address.toString();
}

}}
