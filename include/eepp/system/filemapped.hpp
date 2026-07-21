#ifndef EE_SYSTEM_FILEMAPPED_HPP
#define EE_SYSTEM_FILEMAPPED_HPP

#include <eepp/config.hpp>
#include <string>
#include <string_view>

namespace EE { namespace System {

class EE_API FileMapped {
  public:
	enum class Mode { Read, ReadWrite };

	FileMapped();
	explicit FileMapped( const std::string& path, Mode mode = Mode::Read );
	~FileMapped();

	FileMapped( const FileMapped& ) = delete;
	FileMapped& operator=( const FileMapped& ) = delete;

	FileMapped( FileMapped&& other ) noexcept;
	FileMapped& operator=( FileMapped&& other ) noexcept;

	bool open( const std::string& path, Mode mode = Mode::Read );
	void close();

	bool flush();

	bool isOpen() const { return mOpen; }
	bool isMapped() const { return mAddress != nullptr; }
	bool isReadWrite() const { return mMode == Mode::ReadWrite; }

	const Uint8* data() const { return static_cast<const Uint8*>( mAddress ); }
	Uint8* data() { return static_cast<Uint8*>( mAddress ); }

	const char* cdata() const { return static_cast<const char*>( mAddress ); }
	char* cdata() { return static_cast<char*>( mAddress ); }

	std::string_view view() const {
		return mAddress ? std::string_view{ cdata(), mSize } : std::string_view{};
	}

	size_t size() const { return mSize; }

	bool empty() const { return mSize == 0; }

	const std::string& getLastError() const { return mLastError; }

  protected:
	void moveFrom( FileMapped&& other ) noexcept;

	void clearLastError() { mLastError.clear(); }
	void setLastError( const std::string& error ) { mLastError = error; }

	void* mAddress{ nullptr };
	size_t mSize{ 0 };
	Mode mMode{ Mode::Read };
	bool mOpen{ false };
	std::string mLastError;

#if EE_PLATFORM == EE_PLATFORM_WIN
	void* mFileHandle{ nullptr };
	void* mMapHandle{ nullptr };
#else
	int mFd{ -1 };
#endif
};

}} // namespace EE::System

#endif
