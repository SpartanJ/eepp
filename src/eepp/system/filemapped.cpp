#include <eepp/system/filemapped.hpp>

#include <cerrno>
#include <cstring>
#include <limits>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace EE { namespace System {

#if EE_PLATFORM == EE_PLATFORM_WIN

static std::string getPlatformError( const char* prefix ) {
	const DWORD err = GetLastError();
	return std::string( prefix ) + ": " + std::to_string( static_cast<unsigned long>( err ) );
}

#else

static std::string getPlatformError( const char* prefix ) {
	return std::string( prefix ) + ": " + std::strerror( errno );
}

#endif

FileMapped::FileMapped() = default;

FileMapped::FileMapped( const std::string& path, Mode mode ) {
	open( path, mode );
}

FileMapped::~FileMapped() {
	close();
}

FileMapped::FileMapped( FileMapped&& other ) noexcept {
	moveFrom( std::move( other ) );
}

FileMapped& FileMapped::operator=( FileMapped&& other ) noexcept {
	if ( this != &other ) {
		close();
		moveFrom( std::move( other ) );
	}
	return *this;
}

void FileMapped::moveFrom( FileMapped&& other ) noexcept {
	mAddress = other.mAddress;
	mSize = other.mSize;
	mMode = other.mMode;
	mOpen = other.mOpen;
	mLastError = std::move( other.mLastError );

#if EE_PLATFORM == EE_PLATFORM_WIN
	mFileHandle = other.mFileHandle;
	mMapHandle = other.mMapHandle;

	other.mFileHandle = nullptr;
	other.mMapHandle = nullptr;
#else
	mFd = other.mFd;
	other.mFd = -1;
#endif

	other.mAddress = nullptr;
	other.mSize = 0;
	other.mMode = Mode::Read;
	other.mOpen = false;
	other.mLastError.clear();
}

bool FileMapped::open( const std::string& path, Mode mode ) {
	close();
	clearLastError();

	mMode = mode;
	mOpen = false;

#if EE_PLATFORM == EE_PLATFORM_WIN
	const DWORD desiredAccess =
		mode == Mode::ReadWrite ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;

	const DWORD shareMode = FILE_SHARE_READ | ( mode == Mode::Read ? FILE_SHARE_WRITE : 0 );

	HANDLE file = CreateFileA( path.c_str(), desiredAccess, shareMode, nullptr, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, nullptr );

	if ( file == INVALID_HANDLE_VALUE ) {
		setLastError( getPlatformError( "CreateFileA failed" ) );
		return false;
	}

	LARGE_INTEGER fileSize;
	if ( !GetFileSizeEx( file, &fileSize ) ) {
		setLastError( getPlatformError( "GetFileSizeEx failed" ) );
		CloseHandle( file );
		return false;
	}

	if ( fileSize.QuadPart < 0 ||
		 static_cast<unsigned long long>( fileSize.QuadPart ) >
			 static_cast<unsigned long long>( std::numeric_limits<size_t>::max() ) ) {
		setLastError( "File is too large to map into this process address space" );
		CloseHandle( file );
		return false;
	}

	mFileHandle = file;
	mSize = static_cast<size_t>( fileSize.QuadPart );

	if ( mSize == 0 ) {
		mOpen = true;
		return true;
	}

	const DWORD protect = mode == Mode::ReadWrite ? PAGE_READWRITE : PAGE_READONLY;

	HANDLE mapping = CreateFileMappingA( file, nullptr, protect, 0, 0, nullptr );

	if ( !mapping ) {
		setLastError( getPlatformError( "CreateFileMappingA failed" ) );
		close();
		return false;
	}

	mMapHandle = mapping;

	const DWORD mapAccess = mode == Mode::ReadWrite ? FILE_MAP_WRITE : FILE_MAP_READ;

	void* address = MapViewOfFile( mapping, mapAccess, 0, 0, 0 );

	if ( !address ) {
		setLastError( getPlatformError( "MapViewOfFile failed" ) );
		close();
		return false;
	}

	mAddress = address;
	mOpen = true;
	return true;

#else

	const int flags = mode == Mode::ReadWrite ? O_RDWR : O_RDONLY;

	int fd = ::open( path.c_str(), flags );
	if ( fd == -1 ) {
		setLastError( getPlatformError( "open failed" ) );
		return false;
	}

	struct stat st;
	if ( fstat( fd, &st ) == -1 ) {
		setLastError( getPlatformError( "fstat failed" ) );
		::close( fd );
		return false;
	}

	if ( !S_ISREG( st.st_mode ) ) {
		setLastError( "Path is not a regular file" );
		::close( fd );
		return false;
	}

	if ( st.st_size < 0 ||
		 static_cast<unsigned long long>( st.st_size ) >
			 static_cast<unsigned long long>( std::numeric_limits<size_t>::max() ) ) {
		setLastError( "File is too large to map into this process address space" );
		::close( fd );
		return false;
	}

	mFd = fd;
	mSize = static_cast<size_t>( st.st_size );

	if ( mSize == 0 ) {
		mOpen = true;
		return true;
	}

	const int prot = mode == Mode::ReadWrite ? PROT_READ | PROT_WRITE : PROT_READ;
	const int mapFlags = mode == Mode::ReadWrite ? MAP_SHARED : MAP_PRIVATE;

	void* address = mmap( nullptr, mSize, prot, mapFlags, fd, 0 );
	if ( address == MAP_FAILED ) {
		setLastError( getPlatformError( "mmap failed" ) );
		mAddress = nullptr;
		close();
		return false;
	}

	mAddress = address;
	mOpen = true;
	return true;

#endif
}

void FileMapped::close() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( mAddress ) {
		UnmapViewOfFile( mAddress );
		mAddress = nullptr;
	}

	if ( mMapHandle ) {
		CloseHandle( static_cast<HANDLE>( mMapHandle ) );
		mMapHandle = nullptr;
	}

	if ( mFileHandle ) {
		CloseHandle( static_cast<HANDLE>( mFileHandle ) );
		mFileHandle = nullptr;
	}
#else
	if ( mAddress ) {
		munmap( mAddress, mSize );
		mAddress = nullptr;
	}

	if ( mFd != -1 ) {
		::close( mFd );
		mFd = -1;
	}
#endif

	mSize = 0;
	mMode = Mode::Read;
	mOpen = false;
}

bool FileMapped::flush() {
	clearLastError();

	if ( !mOpen ) {
		setLastError( "File is not open" );
		return false;
	}

	if ( mMode != Mode::ReadWrite ) {
		setLastError( "Cannot flush read-only mapped file" );
		return false;
	}

	if ( mSize == 0 || !mAddress )
		return true;

#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( !FlushViewOfFile( mAddress, mSize ) ) {
		setLastError( getPlatformError( "FlushViewOfFile failed" ) );
		return false;
	}

	if ( mFileHandle && !FlushFileBuffers( static_cast<HANDLE>( mFileHandle ) ) ) {
		setLastError( getPlatformError( "FlushFileBuffers failed" ) );
		return false;
	}
#else
	if ( msync( mAddress, mSize, MS_SYNC ) == -1 ) {
		setLastError( getPlatformError( "msync failed" ) );
		return false;
	}
#endif

	return true;
}

}} // namespace EE::System
