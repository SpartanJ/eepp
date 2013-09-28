#include <eepp/network/cpacket.hpp>
#include <eepp/network/platform/platformimpl.hpp>
#include <cstring>

#ifndef EE_NO_WIDECHAR
#include <cwchar>
#endif

namespace EE { namespace Network {

cPacket::cPacket() :
	mReadPos(0),
	mIsValid(true)
{
}

cPacket::~cPacket()
{
}

void cPacket::Append(const void* data, std::size_t sizeInBytes) {
	if (data && (sizeInBytes > 0)) {
		std::size_t start = mData.size();
		mData.resize(start + sizeInBytes);
		std::memcpy(&mData[start], data, sizeInBytes);
	}
}

void cPacket::Clear() {
	mData.clear();
	mReadPos = 0;
	mIsValid = true;
}

const void* cPacket::GetData() const {
	return !mData.empty() ? &mData[0] : NULL;
}

std::size_t cPacket::GetDataSize() const {
	return mData.size();
}

bool cPacket::EndOfcPacket() const {
	return mReadPos >= mData.size();
}

cPacket::operator BoolType() const {
	return mIsValid ? &cPacket::CheckSize : NULL;
}

cPacket& cPacket::operator >>(bool& data) {
	Uint8 value;
	if (*this >> value)
		data = (value != 0);

	return *this;
}

cPacket& cPacket::operator >>(Int8& data) {
	if (CheckSize(sizeof(data))) {
		data = *reinterpret_cast<const Int8*>(&mData[mReadPos]);
		mReadPos += sizeof(data);
	}

	return *this;
}

cPacket& cPacket::operator >>(Uint8& data) {
	if (CheckSize(sizeof(data))) {
		data = *reinterpret_cast<const Uint8*>(&mData[mReadPos]);
		mReadPos += sizeof(data);
	}

	return *this;
}

cPacket& cPacket::operator >>(Int16& data) {
	if (CheckSize(sizeof(data))) {
		data = ntohs(*reinterpret_cast<const Int16*>(&mData[mReadPos]));
		mReadPos += sizeof(data);
	}

	return *this;
}

cPacket& cPacket::operator >>(Uint16& data) {
	if (CheckSize(sizeof(data))) {
		data = ntohs(*reinterpret_cast<const Uint16*>(&mData[mReadPos]));
		mReadPos += sizeof(data);
	}

	return *this;
}


cPacket& cPacket::operator >>(Int32& data) {
	if (CheckSize(sizeof(data))) {
		data = ntohl(*reinterpret_cast<const Int32*>(&mData[mReadPos]));
		mReadPos += sizeof(data);
	}

	return *this;
}

cPacket& cPacket::operator >>(Uint32& data) {
	if (CheckSize(sizeof(data))) {
		data = ntohl(*reinterpret_cast<const Uint32*>(&mData[mReadPos]));
		mReadPos += sizeof(data);
	}

	return *this;
}

cPacket& cPacket::operator >>(float& data) {
	if (CheckSize(sizeof(data))) {
		data = *reinterpret_cast<const float*>(&mData[mReadPos]);
		mReadPos += sizeof(data);
	}

	return *this;
}

cPacket& cPacket::operator >>(double& data) {
	if (CheckSize(sizeof(data))) {
		data = *reinterpret_cast<const double*>(&mData[mReadPos]);
		mReadPos += sizeof(data);
	}

	return *this;
}

cPacket& cPacket::operator >>(char* data) {
	// First extract string length
	Uint32 length = 0;
	*this >> length;

	if ((length > 0) && CheckSize(length)) {
		// Then extract characters
		std::memcpy(data, &mData[mReadPos], length);
		data[length] = '\0';

		// Update reading position
		mReadPos += length;
	}

	return *this;
}

cPacket& cPacket::operator >>(std::string& data) {
	// First extract string length
	Uint32 length = 0;
	*this >> length;

	data.clear();
	if ((length > 0) && CheckSize(length))
	{
		// Then extract characters
		data.assign(&mData[mReadPos], length);

		// Update reading position
		mReadPos += length;
	}

	return *this;
}

#ifndef EE_NO_WIDECHAR
cPacket& cPacket::operator >>(wchar_t* data) {
	// First extract string length
	Uint32 length = 0;
	*this >> length;

	if ((length > 0) && CheckSize(length * sizeof(Uint32))) {
		// Then extract characters
		for (Uint32 i = 0; i < length; ++i) {
			Uint32 character = 0;
			*this >> character;
			data[i] = static_cast<wchar_t>(character);
		}

		data[length] = L'\0';
	}

	return *this;
}

cPacket& cPacket::operator >>(std::wstring& data) {
	// First extract string length
	Uint32 length = 0;
	*this >> length;

	data.clear();
	if ((length > 0) && CheckSize(length * sizeof(Uint32))) {
		// Then extract characters
		for (Uint32 i = 0; i < length; ++i) {
			Uint32 character = 0;
			*this >> character;
			data += static_cast<wchar_t>(character);
		}
	}

	return *this;
}
#endif

cPacket& cPacket::operator >>(String& data) {
	// First extract the string length
	Uint32 length = 0;
	*this >> length;

	data.clear();
	if ((length > 0) && CheckSize(length * sizeof(Uint32))) {
		// Then extract characters
		for (Uint32 i = 0; i < length; ++i) {
			Uint32 character = 0;
			*this >> character;
			data += character;
		}
	}

	return *this;
}

cPacket& cPacket::operator <<(bool data) {
	*this << static_cast<Uint8>(data);
	return *this;
}

cPacket& cPacket::operator <<(Int8 data) {
	Append(&data, sizeof(data));
	return *this;
}

cPacket& cPacket::operator <<(Uint8 data) {
	Append(&data, sizeof(data));
	return *this;
}

cPacket& cPacket::operator <<(Int16 data) {
	Int16 toWrite = htons(data);
	Append(&toWrite, sizeof(toWrite));
	return *this;
}

cPacket& cPacket::operator <<(Uint16 data) {
	Uint16 toWrite = htons(data);
	Append(&toWrite, sizeof(toWrite));
	return *this;
}

cPacket& cPacket::operator <<(Int32 data) {
	Int32 toWrite = htonl(data);
	Append(&toWrite, sizeof(toWrite));
	return *this;
}

cPacket& cPacket::operator <<(Uint32 data) {
	Uint32 toWrite = htonl(data);
	Append(&toWrite, sizeof(toWrite));
	return *this;
}

cPacket& cPacket::operator <<(float data) {
	Append(&data, sizeof(data));
	return *this;
}

cPacket& cPacket::operator <<(double data) {
	Append(&data, sizeof(data));
	return *this;
}

cPacket& cPacket::operator <<(const char* data) {
	// First insert string length
	Uint32 length = std::strlen(data);
	*this << length;

	// Then insert characters
	Append(data, length * sizeof(char));

	return *this;
}

cPacket& cPacket::operator <<(const std::string& data) {
	// First insert string length
	Uint32 length = static_cast<Uint32>(data.size());
	*this << length;

	// Then insert characters
	if (length > 0)
		Append(data.c_str(), length * sizeof(std::string::value_type));

	return *this;
}

#ifndef EE_NO_WIDECHAR
cPacket& cPacket::operator <<(const wchar_t* data) {
	// First insert string length
	Uint32 length = std::wcslen(data);
	*this << length;

	// Then insert characters
	for (const wchar_t* c = data; *c != L'\0'; ++c)
		*this << static_cast<Uint32>(*c);

	return *this;
}

cPacket& cPacket::operator <<(const std::wstring& data) {
	// First insert string length
	Uint32 length = static_cast<Uint32>(data.size());
	*this << length;

	// Then insert characters
	if (length > 0) {
		for (std::wstring::const_iterator c = data.begin(); c != data.end(); ++c)
			*this << static_cast<Uint32>(*c);
	}

	return *this;
}
#endif

cPacket& cPacket::operator <<(const String& data) {
	// First insert the string length
	Uint32 length = static_cast<Uint32>(data.size());
	*this << length;

	// Then insert characters
	if (length > 0) {
		for (String::ConstIterator c = data.begin(); c != data.end(); ++c)
			*this << *c;
	}

	return *this;
}

bool cPacket::CheckSize(std::size_t size) {
	mIsValid = mIsValid && (mReadPos + size <= mData.size());
	return mIsValid;
}

const void* cPacket::OnSend(std::size_t& size) {
	size = GetDataSize();
	return GetData();
}

void cPacket::OnReceive(const void* data, std::size_t size) {
	Append(data, size);
}

}}

