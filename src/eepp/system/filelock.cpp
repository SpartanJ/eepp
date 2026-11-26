#include <eepp/system/filelock.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/core/string.hpp>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include <cerrno>
#include <fcntl.h>
#if EE_PLATFORM != EE_PLATFORM_SOLARIS
#include <sys/file.h>
#endif
#include <unistd.h>
#endif

namespace EE { namespace System {

FileLock::~FileLock() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( mHandle ) {
		OVERLAPPED overlap{};
		UnlockFileEx( mHandle, 0, 1, 0, &overlap );
		CloseHandle( mHandle );
	}
#elif defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( mHandle != -1 )
		close( mHandle );
#endif
}

FileLock::Result FileLock::tryLock( const std::string& path ) {
	if ( path.empty() || path.find( '\0' ) != std::string::npos )
		return Result::Error;

#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( mHandle )
		return Result::Error;
	HANDLE handle = CreateFileW( String::fromUtf8( path ).toWideString().c_str(),
								 GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
								 nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( handle == INVALID_HANDLE_VALUE )
		return Result::Error;
	OVERLAPPED overlap{};
	if ( LockFileEx( handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0,
					 &overlap ) ) {
		mHandle = handle;
		return Result::Acquired;
	}
	const auto error = GetLastError();
	CloseHandle( handle );
	return error == ERROR_LOCK_VIOLATION ? Result::Busy : Result::Error;
#elif defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	( EE_PLATFORM != EE_PLATFORM_SOLARIS || defined( F_OFD_SETLK ) )
	if ( mHandle != -1 )
		return Result::Error;
	int handle;
	do {
		handle = open( path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600 );
	} while ( handle == -1 && errno == EINTR );
	if ( handle == -1 )
		return Result::Error;
	int result;
#if EE_PLATFORM == EE_PLATFORM_SOLARIS
	struct flock lock{};
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	do {
		result = fcntl( handle, F_OFD_SETLK, &lock );
	} while ( result == -1 && errno == EINTR );
#else
	do {
		result = flock( handle, LOCK_EX | LOCK_NB );
	} while ( result == -1 && errno == EINTR );
#endif
	if ( result == 0 ) {
		mHandle = handle;
		return Result::Acquired;
	}
	const auto error = errno;
	close( handle );
	const bool busy = error == EACCES || error == EAGAIN || error == EWOULDBLOCK;
	return busy ? Result::Busy : Result::Error;
#else
	return Result::Error;
#endif
}

}} // namespace EE::System
