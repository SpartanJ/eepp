#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include "archiveinput.hpp"
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <limits>
#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

namespace EE { namespace VFS { namespace detail {

ArchiveInput::ArchiveInput( const std::string& path ) : mPath( path ) {
	if ( !openFile() )
		return;
#if EE_PLATFORM == EE_PLATFORM_WIN
	int result = _fseeki64( mFile, 0, SEEK_END );
#else
	int result = ::fseeko( mFile, 0, SEEK_END );
#endif
	mSize = result == 0 ? tell() : -1;
	if ( mSize < 0 || seek( 0 ) < 0 ) {
		std::fclose( mFile );
		mFile = nullptr;
	}
}

bool ArchiveInput::openFile() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	HANDLE handle =
		CreateFileW( String::fromUtf8( mPath ).toWideString().c_str(), GENERIC_READ,
					 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
					 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr );
	if ( handle == INVALID_HANDLE_VALUE )
		return false;
	int fd = _open_osfhandle( reinterpret_cast<std::intptr_t>( handle ), _O_RDONLY | _O_BINARY );
	if ( fd < 0 ) {
		CloseHandle( handle );
		return false;
	}
	mFile = _fdopen( fd, "rb" );
	if ( !mFile )
		_close( fd );
#else
	mFile = System::FileSystem::fopenUtf8( mPath, "rb" );
#endif
	return mFile != nullptr;
}

bool ArchiveInput::suspend() {
	if ( !mFile )
		return false;
	mPosition = tell();
	int result = std::fclose( mFile );
	mFile = nullptr;
	return result == 0 && mPosition >= 0;
}

bool ArchiveInput::resume() {
	if ( mFile )
		return true;
	if ( !openFile() )
		return false;
	if ( seek( mPosition ) < 0 ) {
		std::fclose( mFile );
		mFile = nullptr;
		return false;
	}
	return true;
}

ArchiveInput::~ArchiveInput() {
	if ( mFile )
		std::fclose( mFile );
}

std::intptr_t ArchiveInput::read( char* data, size_t size ) {
	const size_t got = std::fread( data, 1, size, mFile );
	return got == 0 && std::ferror( mFile ) ? -1 : static_cast<std::intptr_t>( got );
}

Int64 ArchiveInput::seek( Int64 position ) {
	if ( position < 0 || !mFile )
		return -1;
#if EE_PLATFORM == EE_PLATFORM_WIN
	return _fseeki64( mFile, position, SEEK_SET ) == 0 ? position : -1;
#else
	if ( static_cast<Uint64>( position ) >
		 static_cast<Uint64>( std::numeric_limits<off_t>::max() ) )
		return -1;
	return ::fseeko( mFile, static_cast<off_t>( position ), SEEK_SET ) == 0 ? position : -1;
#endif
}

Int64 ArchiveInput::tell() const {
	if ( !mFile )
		return -1;
#if EE_PLATFORM == EE_PLATFORM_WIN
	return _ftelli64( mFile );
#else
	return ::ftello( mFile );
#endif
}

}}} // namespace EE::VFS::detail
