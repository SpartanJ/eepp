#ifndef EE_NETWORKCIPADDRESS_HPP
#define EE_NETWORKCIPADDRESS_HPP

#include <eepp/network/base.hpp>
#include <eepp/system/ctime.hpp>
using namespace EE::System;

#include <istream>
#include <ostream>
#include <string>

namespace EE { namespace Network {

/** @brief Encapsulate an IPv4 network address */
class EE_API cIpAddress
{
	public:
		/** @brief Default constructor
		**  This constructor creates an empty (invalid) address */
		cIpAddress();

		/** @brief Construct the address from a string
		**  Here @a address can be either a decimal address
		**  (ex: "192.168.1.56") or a network name (ex: "localhost").
		**  @param address IP address or network name */
		cIpAddress(const std::string& address);

		/** @brief Construct the address from a string
		**  Here @a address can be either a decimal address
		**  (ex: "192.168.1.56") or a network name (ex: "localhost").
		**  This is equivalent to the constructor taking a std::string
		**  parameter, it is defined for convenience so that the
		**  implicit conversions from literal strings to cIpAddress work.
		**  @param address IP address or network name */
		cIpAddress(const char* address);

		/** @brief Construct the address from 4 bytes
		**  Calling cIpAddress(a, b, c, d) is equivalent to calling
		**  cIpAddress("a.b.c.d"), but safer as it doesn't have to
		**  parse a string to get the address components.
		**  @param byte0 First byte of the address
		**  @param byte1 Second byte of the address
		**  @param byte2 Third byte of the address
		**  @param byte3 Fourth byte of the address */
		cIpAddress(Uint8 byte0, Uint8 byte1, Uint8 byte2, Uint8 byte3);

		/** @brief Construct the address from a 32-bits integer
		**  This constructor uses the internal representation of
		**  the address directly. It should be used for optimization
		**  purposes, and only if you got that representation from
		**  cIpAddress::ToInteger().
		**  @param address 4 bytes of the address packed into a 32-bits integer
		**  @see ToInteger */
		explicit cIpAddress(Uint32 address);

		/** @brief Get a string representation of the address
		**  The returned string is the decimal representation of the
		**  IP address (like "192.168.1.56"), even if it was constructed
		**  from a host name.
		**  @return String representation of the address
		**  @see ToInteger */
		std::string ToString() const;

		/** @brief Get an integer representation of the address
		**  The returned number is the internal representation of the
		**  address, and should be used for optimization purposes only
		**  (like sending the address through a socket).
		**  The integer produced by this function can then be converted
		**  back to a cIpAddress with the proper constructor.
		**  @return 32-bits unsigned integer representation of the address
		**  @see ToString */
		Uint32 ToInteger() const;

		/** @brief Get the computer's local address
		**  The local address is the address of the computer from the
		**  LAN point of view, i.e. something like 192.168.1.56. It is
		**  meaningful only for communications over the local network.
		**  Unlike GetPublicAddress, this function is fast and may be
		**  used safely anywhere.
		**  @return Local IP address of the computer
		**  @see GetPublicAddress */
		static cIpAddress GetLocalAddress();

		/** @brief Get the computer's public address
		**  The public address is the address of the computer from the
		**  internet point of view, i.e. something like 89.54.1.169.
		**  It is necessary for communications over the world wide web.
		**  The only way to get a public address is to ask it to a
		**  distant website; as a consequence, this function depends on
		**  both your network connection and the server, and may be
		**  very slow. You should use it as few as possible. Because
		**  this function depends on the network connection and on a distant
		**  server, you may use a time limit if you don't want your program
		**  to be possibly stuck waiting in case there is a problem; this
		**  limit is deactivated by default.
		**  @param timeout Maximum time to wait
		**  @return Public IP address of the computer
		**  @see GetLocalAddress */
		static cIpAddress GetPublicAddress(cTime timeout = cTime::Zero);

		// Static member data
		static const cIpAddress None;	  ///< Value representing an empty/invalid address
		static const cIpAddress LocalHost; ///< The "localhost" address (for connecting a computer to itself locally)
		static const cIpAddress Broadcast; ///< The "broadcast" address (for sending UDP messages to everyone on a local network)
	private :
		// Member data
		Uint32 mAddress; ///< Address stored as an unsigned 32 bits integer
};

/** @brief Overload of == operator to compare two IP addresses
**  @param left  Left operand (a IP address)
**  @param right Right operand (a IP address)
**  @return True if both addresses are equal */
EE_API bool operator ==(const cIpAddress& left, const cIpAddress& right);

/** @brief Overload of != operator to compare two IP addresses
**  @param left  Left operand (a IP address)
**  @param right Right operand (a IP address)
**  @return True if both addresses are different */
EE_API bool operator !=(const cIpAddress& left, const cIpAddress& right);

/** @brief Overload of < operator to compare two IP addresses
**  @param left  Left operand (a IP address)
**  @param right Right operand (a IP address)
**  @return True if @a left is lesser than @a right */
EE_API bool operator <(const cIpAddress& left, const cIpAddress& right);

/** @brief Overload of > operator to compare two IP addresses
**  @param left  Left operand (a IP address)
**  @param right Right operand (a IP address)
**  @return True if @a left is greater than @a right */
EE_API bool operator >(const cIpAddress& left, const cIpAddress& right);

/** @brief Overload of <= operator to compare two IP addresses
**  @param left  Left operand (a IP address)
**  @param right Right operand (a IP address)
**  @return True if @a left is lesser or equal than @a right */
EE_API bool operator <=(const cIpAddress& left, const cIpAddress& right);

/** @brief Overload of >= operator to compare two IP addresses
**  @param left  Left operand (a IP address)
**  @param right Right operand (a IP address)
**  @return True if @a left is greater or equal than @a right */
EE_API bool operator >=(const cIpAddress& left, const cIpAddress& right);

/** @brief Overload of >> operator to extract an IP address from an input stream
**  @param stream  Input stream
**  @param address IP address to extract
**  @return Reference to the input stream */
EE_API std::istream& operator >>(std::istream& stream, cIpAddress& address);

/** @brief Overload of << operator to print an IP address to an output stream
**  @param stream  Output stream
**  @param address IP address to print
**  @return Reference to the output stream */
EE_API std::ostream& operator <<(std::ostream& stream, const cIpAddress& address);

}}

#endif // EE_NETWORKCIPADDRESS_HPP

/**
@class cIpAddress
@ingroup Network
cIpAddress is a utility class for manipulating network
addresses. It provides a set a implicit constructors and
conversion functions to easily build or transform an IP
address from/to various representations.

Usage example:
@code
cIpAddress a0;									 // an invalid address
cIpAddress a1 = cIpAddress::None;			   // an invalid address (same as a0)
cIpAddress a2("127.0.0.1");						// the local host address
cIpAddress a3 = cIpAddress::Broadcast;		  // the broadcast address
cIpAddress a4(192, 168, 1, 56);					// a local address
cIpAddress a5("my_computer");					  // a local address created from a network name
cIpAddress a6("89.54.1.169");					  // a distant address
cIpAddress a7("www.google.com");				   // a distant address created from a network name
cIpAddress a8 = cIpAddress::GetLocalAddress();  // my address on the local network
cIpAddress a9 = cIpAddress::GetPublicAddress(); // my address on the internet
@endcode
Note that cIpAddress currently doesn't support IPv6
nor other types of network addresses.
*/
