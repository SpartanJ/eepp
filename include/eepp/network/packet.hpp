#ifndef EE_NETWORKCPACKET_HPP
#define EE_NETWORKCPACKET_HPP

#include <eepp/core.hpp>
#include <string>
#include <vector>

namespace EE { namespace Network {

class TcpSocket;
class UdpSocket;

/** @brief Utility class to build blocks of data to transfer over the network */
class EE_API Packet {
	// A bool-like type that cannot be converted to integer or pointer types
	typedef bool ( Packet::*BoolType )( std::size_t );

  public:
	/** @brief Default constructor
	**  Creates an empty packet. */
	Packet();

	/** @brief Virtual destructor */
	virtual ~Packet();

	/** @brief Append data to the end of the packet
	**  @param data		Pointer to the sequence of bytes to append
	**  @param sizeInBytes Number of bytes to append
	**  @see Clear */
	void append( const void* data, std::size_t sizeInBytes );

	/** @brief Clear the packet
	**  After calling Clear, the packet is empty.
	**  @see Append */
	void clear();

	/** @brief Get a pointer to the data contained in the packet
	**  Warning: the returned pointer may become invalid after
	**  you append data to the packet, therefore it should never
	**  be stored.
	**  The return pointer is NULL if the packet is empty.
	**  @return Pointer to the data
	**  @see GetDataSize */
	const void* getData() const;

	/** @brief Get the size of the data contained in the packet
	**  This function returns the number of bytes pointed to by
	**  what GetData returns.
	**  @return Data size, in bytes
	**  @see GetData */
	std::size_t getDataSize() const;

	/** @brief Tell if the reading position has reached the
	///		end of the packet
	**  This function is useful to know if there is some data
	**  left to be read, without actually reading it.
	**  @return True if all data was read, false otherwise
	**  @see operator bool */
	bool endOfPacket() const;

	/** @brief Test the validity of the packet, for reading
	**  This operator allows to test the packet as a boolean
	**  variable, to check if a reading operation was successful.
	**  A packet will be in an invalid state if it has no more
	**  data to read.
	**  This behaviour is the same as standard C++ streams.
	**  Usage example:
	**  @code
	**  float x;
	**  packet >> x;
	**  if (packet)
	**  {
	///	// ok, x was extracted successfully
	**  }
	///
	**  // -- or --
	///
	**  float x;
	**  if (packet >> x)
	**  {
	///	// ok, x was extracted successfully
	**  }
	**  @endcode
	///
	**  Don't focus on the return type, it's equivalent to bool but
	**  it disallows unwanted implicit conversions to integer or
	**  pointer types.
	///
	**  @return True if last data extraction from packet was successful
	///
	**  @see EndOfPacket */
	operator BoolType() const;

	/**  Overloads of operator >> to read data from the packet */
	Packet& operator>>( bool& data );
	Packet& operator>>( Int8& data );
	Packet& operator>>( Uint8& data );
	Packet& operator>>( Int16& data );
	Packet& operator>>( Uint16& data );
	Packet& operator>>( Int32& data );
	Packet& operator>>( Uint32& data );
	Packet& operator>>( float& data );
	Packet& operator>>( double& data );
	Packet& operator>>( char* data );
	Packet& operator>>( std::string& data );
#ifndef EE_NO_WIDECHAR
	Packet& operator>>( wchar_t* data );
	Packet& operator>>( std::wstring& data );
#endif
	Packet& operator>>( String& data );

	/**  Overloads of operator << to write data into the packet */
	Packet& operator<<( bool data );
	Packet& operator<<( Int8 data );
	Packet& operator<<( Uint8 data );
	Packet& operator<<( Int16 data );
	Packet& operator<<( Uint16 data );
	Packet& operator<<( Int32 data );
	Packet& operator<<( Uint32 data );
	Packet& operator<<( float data );
	Packet& operator<<( double data );
	Packet& operator<<( const char* data );
	Packet& operator<<( const std::string& data );
#ifndef EE_NO_WIDECHAR
	Packet& operator<<( const wchar_t* data );
	Packet& operator<<( const std::wstring& data );
#endif
	Packet& operator<<( const String& data );

  protected:
	friend class TcpSocket;
	friend class UdpSocket;

	/** @brief Called before the packet is sent over the network
	**  This function can be defined by derived classes to
	**  transform the data before it is sent; this can be
	**  used for compression, encryption, etc.
	**  The function must return a pointer to the modified data,
	**  as well as the number of bytes pointed.
	**  The default implementation provides the packet's data
	**  without transforming it.
	**  @param size Variable to fill with the size of data to send
	**  @return Pointer to the array of bytes to send
	**  @see OnReceive */
	virtual const void* onSend( std::size_t& size );

	/** @brief Called after the packet is received over the network
	**  This function can be defined by derived classes to
	**  transform the data after it is received; this can be
	**  used for uncompression, decryption, etc.
	**  The function receives a pointer to the received data,
	**  and must fill the packet with the transformed bytes.
	**  The default implementation fills the packet directly
	**  without transforming the data.
	**  @param data Pointer to the received bytes
	**  @param size Number of bytes
	**  @see OnSend */
	virtual void onReceive( const void* data, std::size_t size );

  private:
	/**  Disallow comparisons between packets */
	bool operator==( const Packet& right ) const;
	bool operator!=( const Packet& right ) const;

	/** @brief Check if the packet can extract a given number of bytes
	**  This function updates accordingly the state of the packet.
	**  @param size Size to check
	**  @return True if @a size bytes can be read from the packet */
	bool checkSize( std::size_t size );

	// Member data
	std::vector<char> mData; ///< Data stored in the packet
	std::size_t mReadPos;	 ///< Current reading position in the packet
	std::size_t mSendPos;	 ///< Current send position in the packet (for handling partial sends)
	bool mIsValid;			 ///< Reading state of the packet
};

}} // namespace EE::Network

#endif // EE_NETWORKCPACKET_HPP

/**
@class EE::Network::Packet

Packets provide a safe and easy way to serialize data,
in order to send it over the network using sockets
(TcpSocket, UdpSocket).

Packets solve 2 fundamental problems that arise when
transfering data over the network:
@li data is interpreted correctly according to the endianness
@li the bounds of the packet are preserved (one send == one receive)

The Packet class provides both input and output modes.
It is designed to follow the behaviour of standard C++ streams,
using operators >> and << to extract and insert data.

It is recommended to use only fixed-size types (like Int32, etc.),
to avoid possible differences between the sender and the receiver.
Indeed, the native C++ types may have different sizes on two platforms
and your data may be corrupted if that happens.

Usage example:
@code
Uint32 x = 24;
std::string s = "hello";
double d = 5.89;

// Group the variables to send into a packet
Packet packet;
packet << x << s << d;

// Send it over the network (socket is a valid TcpSocket)
socket.send(packet);

-----------------------------------------------------------------

// Receive the packet at the other end
Packet packet;
socket.receive(packet);

// Extract the variables contained in the packet
Uint32 x;
std::string s;
double d;
if (packet >> x >> s >> d) {
	 // Data extracted successfully...
}
@endcode

Packets have built-in operator >> and << overloads for
standard types:
@li bool
@li fixed-size integer types (Int8/16/32, Uint8/16/32)
@li floating point numbers (float, double)
@li string types (char*, wchar_t*, std::string, std::wstring, String)

Like standard streams, it is also possible to define your own
overloads of operators >> and << in order to handle your
custom types.

@code
struct MyStruct {
	 float	   number;
	 Int8	integer;
	 std::string str;
};

Packet& operator <<(Packet& packet, const MyStruct& m) {
	 return packet << m.number << m.integer << m.str;
}

Packet& operator >>(Packet& packet, MyStruct& m) {
	 return packet >> m.number >> m.integer >> m.str;
}
@endcode

Packets also provide an extra feature that allows to apply
custom transformations to the data before it is sent,
and after it is received. This is typically used to
handle automatic compression or encryption of the data.
This is achieved by inheriting from Packet, and overriding
the onSend and onReceive functions.

Here is an example:
@code
class ZipPacket : public Packet {
	 virtual const void* onSend(std::size_t& size) {
		 const void* srcData = getData();
		 std::size_t srcSize = getDataSize();

		 return MySuperZipFunction(srcData, srcSize, &size);
	 }

	 virtual void onReceive(const void* data, std::size_t size) {
		 std::size_t dstSize;
		 const void* dstData = MySuperUnzipFunction(data, size, &dstSize);

		 append(dstData, dstSize);
	 }
};

// Use like regular packets:
ZipPacket packet;
packet << x << s << d;
...
@endcode

@see EE::Network::TcpSocket, EE::Network::UdpSocket
*/
