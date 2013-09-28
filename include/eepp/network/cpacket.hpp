#ifndef EE_NETWORKCPACKET_HPP
#define EE_NETWORKCPACKET_HPP

#include <eepp/network/base.hpp>
#include <string>
#include <vector>

namespace EE { namespace Network {

class cTcpSocket;
class cUdpSocket;

/** @brief Utility class to build blocks of data to transfer over the network */
class EE_API cPacket {
	// A bool-like type that cannot be converted to integer or pointer types
	typedef bool (cPacket::*BoolType)(std::size_t);
	public:

	/** @brief Default constructor
	**  Creates an empty packet. */
	cPacket();

	/** @brief Virtual destructor */
	virtual ~cPacket();

	/** @brief Append data to the end of the packet
	**  @param data		Pointer to the sequence of bytes to append
	**  @param sizeInBytes Number of bytes to append
	**  @see Clear */
	void Append(const void* data, std::size_t sizeInBytes);

	/** @brief Clear the packet
	**  After calling Clear, the packet is empty.
	**  @see Append */
	void Clear();

	/** @brief Get a pointer to the data contained in the packet
	**  Warning: the returned pointer may become invalid after
	**  you append data to the packet, therefore it should never
	**  be stored.
	**  The return pointer is NULL if the packet is empty.
	**  @return Pointer to the data
	**  @see GetDataSize */
	const void* GetData() const;

	/** @brief Get the size of the data contained in the packet
	**  This function returns the number of bytes pointed to by
	**  what GetData returns.
	**  @return Data size, in bytes
	**  @see GetData */
	std::size_t GetDataSize() const;

	/** @brief Tell if the reading position has reached the
	///		end of the packet
	**  This function is useful to know if there is some data
	**  left to be read, without actually reading it.
	**  @return True if all data was read, false otherwise
	**  @see operator bool */
	bool EndOfcPacket() const;

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
	**  @see EndOfcPacket */
	operator BoolType() const;

	/**  Overloads of operator >> to read data from the packet */
	cPacket& operator >>(bool&		 data);
	cPacket& operator >>(Int8&		 data);
	cPacket& operator >>(Uint8&		data);
	cPacket& operator >>(Int16&		data);
	cPacket& operator >>(Uint16&	   data);
	cPacket& operator >>(Int32&		data);
	cPacket& operator >>(Uint32&	   data);
	cPacket& operator >>(float&		data);
	cPacket& operator >>(double&	   data);
	cPacket& operator >>(char*		 data);
	cPacket& operator >>(std::string&  data);
	#ifndef EE_NO_WIDECHAR
	cPacket& operator >>(wchar_t*	  data);
	cPacket& operator >>(std::wstring& data);
	#endif
	cPacket& operator >>(String&	   data);

	/**  Overloads of operator << to write data into the packet */
	cPacket& operator <<(bool				data);
	cPacket& operator <<(Int8				data);
	cPacket& operator <<(Uint8			   data);
	cPacket& operator <<(Int16			   data);
	cPacket& operator <<(Uint16			  data);
	cPacket& operator <<(Int32			   data);
	cPacket& operator <<(Uint32			  data);
	cPacket& operator <<(float			   data);
	cPacket& operator <<(double			  data);
	cPacket& operator <<(const char*		 data);
	cPacket& operator <<(const std::string&  data);
	#ifndef EE_NO_WIDECHAR
	cPacket& operator <<(const wchar_t*	  data);
	cPacket& operator <<(const std::wstring& data);
	#endif
	cPacket& operator <<(const String&	   data);
protected:
	friend class cTcpSocket;
	friend class cUdpSocket;

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
	virtual const void* OnSend(std::size_t& size);

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
	virtual void OnReceive(const void* data, std::size_t size);
private:
	/**  Disallow comparisons between packets */
	bool operator ==(const cPacket& right) const;
	bool operator !=(const cPacket& right) const;

	/** @brief Check if the packet can extract a given number of bytes
	**  This function updates accordingly the state of the packet.
	**  @param size Size to check
	**  @return True if @a size bytes can be read from the packet */
	bool CheckSize(std::size_t size);

	// Member data
	std::vector<char>	mData;	///< Data stored in the packet
	std::size_t			mReadPos; ///< Current reading position in the packet
	bool				mIsValid; ///< Reading state of the packet
};

}}

#endif // EE_NETWORKCPACKET_HPP

/**
@class cPacket
@ingroup Network

cPackets provide a safe and easy way to serialize data,
in order to send it over the network using sockets
(cTcpSocket, cUdpSocket).

cPackets solve 2 fundamental problems that arise when
transfering data over the network:
@li data is interpreted correctly according to the endianness
@li the bounds of the packet are preserved (one send == one receive)

The cPacket class provides both input and output modes.
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
cPacket packet;
packet << x << s << d;

// Send it over the network (socket is a valid cTcpSocket)
socket.Send(packet);

-----------------------------------------------------------------

// Receive the packet at the other end
cPacket packet;
socket.Receive(packet);

// Extract the variables contained in the packet
Uint32 x;
std::string s;
double d;
if (packet >> x >> s >> d)
{
	 // Data extracted successfully...
}
@endcode

cPackets have built-in operator >> and << overloads for
standard types:
@li bool
@li fixed-size integer types (Int8/16/32, Uint8/16/32)
@li floating point numbers (float, double)
@li string types (char*, wchar_t*, std::string, std::wstring, String)

Like standard streams, it is also possible to define your own
overloads of operators >> and << in order to handle your
custom types.

@code
struct MyStruct
{
	 float	   number;
	 Int8	integer;
	 std::string str;
};

cPacket& operator <<(cPacket& packet, const MyStruct& m)
{
	 return packet << m.number << m.integer << m.str;
}

cPacket& operator >>(cPacket& packet, MyStruct& m)
{
	 return packet >> m.number >> m.integer >> m.str;
}
@endcode

cPackets also provide an extra feature that allows to apply
custom transformations to the data before it is sent,
and after it is received. This is typically used to
handle automatic compression or encryption of the data.
This is achieved by inheriting from cPacket, and overriding
the OnSend and OnReceive functions.

Here is an example:
@code
class ZipcPacket : public cPacket
{
	 virtual const void* OnSend(std::size_t& size)
	 {
		 const void* srcData = GetData();
		 std::size_t srcSize = GetDataSize();

		 return MySuperZipFunction(srcData, srcSize, &size);
	 }

	 virtual void OnReceive(const void* data, std::size_t size)
	 {
		 std::size_t dstSize;
		 const void* dstData = MySuperUnzipFunction(data, size, &dstSize);

		 append(dstData, dstSize);
	 }
};

// Use like regular packets:
ZipcPacket packet;
packet << x << s << d;
...
@endcode

@see cTcpSocket, cUdpSocket
*/
