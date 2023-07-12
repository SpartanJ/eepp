#include <bitset>
#include <eepp/core/debug.hpp>
#include <eepp/core/memorymanager.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX
#define SUBPROCESS_USE_POSIX_SPAWN
#endif
#include <thirdparty/subprocess/subprocess.h>
#include <vector>

#ifdef EE_PLATFORM_POSIX
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include <windows.h>
#endif

namespace EE { namespace System {

#define PROCESS_PTR ( static_cast<struct subprocess_s*>( mProcess ) )

Process::Process() {}

Process::Process( const std::string& command, const Uint32& options,
				  const std::unordered_map<std::string, std::string>& environment,
				  const std::string& workingDirectory, const size_t& bufferSize ) :
	mBufferSize( bufferSize ) {
	create( command, options, environment, workingDirectory );
}

Process::~Process() {
	mShuttingDown = true;
	if ( mProcess && isAlive() ) {
		subprocess_init_shutdown( PROCESS_PTR );
		kill();
	}
	if ( mStdOutThread.joinable() )
		mStdOutThread.join();
	if ( mStdErrThread.joinable() )
		mStdErrThread.join();
	eeFree( mProcess );
}

bool Process::create( const std::string& command, const Uint32& options,
					  const std::unordered_map<std::string, std::string>& environment,
					  const std::string& workingDirectory ) {
	if ( mProcess )
		return false;
	std::vector<std::string> cmdArr = String::split( command, " ", "", "\"", true );
	std::vector<const char*> strings;
	mProcess = eeMalloc( sizeof( subprocess_s ) );
	memset( mProcess, 0, sizeof( subprocess_s ) );
	if ( !environment.empty() ) {
		std::string rcommand;
		if ( FileSystem::fileExists( command ) ) {
			rcommand = command;
		} else {
			rcommand = Sys::which( command );
			if ( rcommand.empty() )
				return false;
		}
		strings.push_back( rcommand.c_str() );
		std::vector<std::string> envArr;
		std::vector<const char*> envStrings;
		for ( const auto& pair : environment ) {
			envArr.push_back( String::format( "%s=%s", pair.first.c_str(), pair.second.c_str() ) );
			envStrings.push_back( envArr[envArr.size() - 1].c_str() );
		}
		envStrings.push_back( NULL );

		auto ret = 0 == subprocess_create_ex( strings.data(), options, envStrings.data(),
											  !workingDirectory.empty() ? workingDirectory.c_str()
																		: nullptr,
											  PROCESS_PTR );
		return ret;
	}
	for ( size_t i = 0; i < cmdArr.size(); ++i )
		strings.push_back( cmdArr[i].c_str() );
	strings.push_back( NULL );
	auto ret =
		0 == subprocess_create_ex( strings.data(), options, nullptr,
								   !workingDirectory.empty() ? workingDirectory.c_str() : nullptr,
								   PROCESS_PTR );
	return ret;
}

bool Process::create( const std::string& command, const std::string& args, const Uint32& options,
					  const std::unordered_map<std::string, std::string>& environment,
					  const std::string& workingDirectory ) {
	if ( mProcess )
		return false;
	std::vector<std::string> cmdArr = String::split( args, " ", "", "\"", true );
	std::vector<const char*> strings;
	mProcess = eeMalloc( sizeof( subprocess_s ) );
	memset( mProcess, 0, sizeof( subprocess_s ) );
	if ( !environment.empty() ) {
		std::string rcommand;
		if ( FileSystem::fileExists( command ) ) {
			rcommand = command;
		} else {
			rcommand = Sys::which( command );
			if ( rcommand.empty() )
				return false;
		}
		strings.push_back( rcommand.c_str() );
		for ( size_t i = 0; i < cmdArr.size(); ++i )
			strings.push_back( cmdArr[i].c_str() );
		strings.push_back( NULL );

		std::vector<std::string> envArr;
		std::vector<const char*> envStrings;
		for ( const auto& pair : environment ) {
			envArr.push_back( String::format( "%s=%s", pair.first.c_str(), pair.second.c_str() ) );
			envStrings.push_back( envArr[envArr.size() - 1].c_str() );
		}
		envStrings.push_back( NULL );

		auto ret = 0 == subprocess_create_ex( strings.data(), options, envStrings.data(),
											  !workingDirectory.empty() ? workingDirectory.c_str()
																		: nullptr,
											  PROCESS_PTR );
		return ret;
	}

	strings.push_back( command.c_str() );
	for ( size_t i = 0; i < cmdArr.size(); ++i )
		strings.push_back( cmdArr[i].c_str() );
	strings.push_back( NULL );

	auto ret =
		0 == subprocess_create_ex( strings.data(), options, nullptr,
								   !workingDirectory.empty() ? workingDirectory.c_str() : nullptr,
								   PROCESS_PTR );
	return ret;
}

size_t Process::readAllStdOut( std::string& buffer ) {
	size_t bytesRead = 0;
	size_t totalBytesRead = 0;
	const size_t chunkSize = mBufferSize;
	totalBytesRead = bytesRead = readStdOut( (char* const)buffer.c_str(), buffer.size() );
	while ( bytesRead != 0 && isAlive() && !mShuttingDown ) {
		bytesRead = readStdOut( (char* const)buffer.c_str() + bytesRead, chunkSize );
		if ( bytesRead ) {
			totalBytesRead += bytesRead;
			buffer.resize( totalBytesRead + chunkSize );
		}
	}
	return totalBytesRead;
}

size_t Process::readStdOut( std::string& buffer ) {
	return readStdOut( (char* const)buffer.c_str(), buffer.size() );
}

size_t Process::readStdOut( char* const buffer, const size_t& size ) {
	eeASSERT( mProcess != nullptr );
	return subprocess_read_stdout( PROCESS_PTR, buffer, size );
}

size_t Process::readAllStdErr( std::string& buffer ) {
	size_t bytesRead = 0;
	size_t totalBytesRead = 0;
	const size_t chunkSize = 4096;
	totalBytesRead = bytesRead = readStdErr( (char* const)buffer.c_str(), buffer.size() );
	while ( bytesRead != 0 && isAlive() && !mShuttingDown ) {
		bytesRead = readStdErr( (char* const)buffer.c_str() + bytesRead, chunkSize );
		if ( bytesRead ) {
			totalBytesRead += bytesRead;
			buffer.resize( totalBytesRead + chunkSize );
		}
	}
	return totalBytesRead;
}

size_t Process::readStdErr( std::string& buffer ) {
	return readStdErr( (char* const)buffer.c_str(), buffer.size() );
}

size_t Process::readStdErr( char* const buffer, const size_t& size ) {
	eeASSERT( mProcess != nullptr );
	return subprocess_read_stderr( PROCESS_PTR, buffer, size );
}

size_t Process::write( const char* buffer, const size_t& size ) {
	eeASSERT( mProcess != nullptr );
	if ( mShuttingDown )
		return 0;
	Lock l( mStdInMutex );
	return subprocess_write_stdin( PROCESS_PTR, (char* const)buffer, size );
}

size_t Process::write( const std::string& buffer ) {
	return write( buffer.c_str(), buffer.size() );
}

bool Process::join( int* const returnCodeOut ) {
	eeASSERT( mProcess != nullptr );
	return 0 == subprocess_join( PROCESS_PTR, returnCodeOut );
}

bool Process::kill() {
	eeASSERT( mProcess != nullptr );
	if ( PROCESS_PTR->alive ) {
		destroy();
		return 0 == subprocess_terminate( PROCESS_PTR );
	}
	return false;
}

bool Process::destroy() {
	eeASSERT( mProcess != nullptr );
	return 0 == subprocess_destroy( PROCESS_PTR );
}

bool Process::isAlive() {
	eeASSERT( mProcess != nullptr );
	return 0 != subprocess_alive( PROCESS_PTR );
}

FILE* Process::getStdIn() const {
	eeASSERT( mProcess != nullptr );
	return subprocess_stdin( PROCESS_PTR );
}

FILE* Process::getStdOut() const {
	eeASSERT( mProcess != nullptr );
	return subprocess_stdout( PROCESS_PTR );
}

FILE* Process::getStdErr() const {
	eeASSERT( mProcess != nullptr );
	return subprocess_stderr( PROCESS_PTR );
}

void Process::startShutdown() {
	mShuttingDown = true;
}

const bool& Process::isShuttingDown() {
	return mShuttingDown;
}

void Process::startAsyncRead( ReadFn readStdOut, ReadFn readStdErr ) {
	eeASSERT( mProcess != nullptr );
	mReadStdOutFn = readStdOut;
	mReadStdErrFn = readStdErr;
#if EE_PLATFORM == EE_PLATFORM_WIN
	void* stdOutFd =
		SUBPROCESS_PTR_CAST( void*, _get_osfhandle( _fileno( PROCESS_PTR->stdout_file ) ) );
	void* stdErrFd =
		SUBPROCESS_PTR_CAST( void*, _get_osfhandle( _fileno( PROCESS_PTR->stderr_file ) ) );
	if ( stdOutFd ) {
		mStdOutThread = std::thread( [this, stdOutFd]() {
			unsigned n;
			std::string buffer;
			buffer.resize( mBufferSize );
			while ( !mShuttingDown ) {
				n = subprocess_read_stdout( PROCESS_PTR, static_cast<char* const>( &buffer[0] ),
											mBufferSize );
				if ( n == 0 )
					break;
				if ( n < static_cast<long>( mBufferSize - 1 ) )
					buffer[n] = '\0';
				if ( !mShuttingDown )
					mReadStdOutFn( buffer.c_str(), static_cast<size_t>( n ) );
			}
		} );
	}
	if ( stdErrFd && stdErrFd != stdOutFd ) {
		mStdErrThread = std::thread( [this, stdErrFd]() {
			unsigned n;
			std::string buffer;
			buffer.resize( mBufferSize );
			while ( !mShuttingDown ) {
				n = subprocess_read_stderr( PROCESS_PTR, static_cast<char* const>( &buffer[0] ),
											mBufferSize );
				if ( n == 0 )
					break;
				if ( n < static_cast<long>( mBufferSize - 1 ) )
					buffer[n] = '\0';

				if ( !mShuttingDown )
					mReadStdErrFn( buffer.c_str(), static_cast<size_t>( n ) );
			}
		} );
	}
#elif defined( EE_PLATFORM_POSIX )
	mStdOutThread = std::thread( [this] {
		auto stdOutFd = fileno( PROCESS_PTR->stdout_file );
		auto stdErrFd = PROCESS_PTR->stderr_file ? fileno( PROCESS_PTR->stderr_file ) : 0;
		std::vector<pollfd> pollfds;
		std::bitset<2> fdIsStdOut;
		if ( stdOutFd ) {
			fdIsStdOut.set( pollfds.size() );
			pollfds.emplace_back();
			pollfds.back().fd =
				fcntl( stdOutFd, F_SETFL, fcntl( stdOutFd, F_GETFL ) | O_NONBLOCK ) == 0 ? stdOutFd
																						 : -1;
			pollfds.back().events = POLLIN;
		}
		if ( stdErrFd && stdOutFd != stdErrFd ) {
			pollfds.emplace_back();
			pollfds.back().fd =
				fcntl( stdErrFd, F_SETFL, fcntl( stdErrFd, F_GETFL ) | O_NONBLOCK ) == 0 ? stdErrFd
																						 : -1;
			pollfds.back().events = POLLIN;
		}
		std::string buffer;
		buffer.resize( mBufferSize );
		bool anyOpen = !pollfds.empty();
		while ( anyOpen && !mShuttingDown && errno != EINTR ) {
			int res = poll( pollfds.data(), static_cast<nfds_t>( pollfds.size() ), 100 );
			if ( res > 0 ) {
				anyOpen = false;
				for ( size_t i = 0; i < pollfds.size(); ++i ) {
					if ( pollfds[i].fd >= 0 ) {
						if ( pollfds[i].revents & POLLIN ) {
							const ssize_t n = read( pollfds[i].fd, &buffer[0], mBufferSize );
							if ( n > 0 ) {
								if ( n < static_cast<long>( mBufferSize - 1 ) )
									buffer[n] = '\0';
								if ( fdIsStdOut[i] )
									mReadStdOutFn( buffer.c_str(), static_cast<size_t>( n ) );
								else
									mReadStdErrFn( buffer.c_str(), static_cast<size_t>( n ) );
							} else if ( n == 0 || ( n < 0 && errno != EINTR && errno != EAGAIN &&
													errno != EWOULDBLOCK ) ) {
								pollfds[i].fd = -1;
								continue;
							}
						}
						if ( pollfds[i].revents & ( POLLERR | POLLHUP | POLLNVAL ) ) {
							pollfds[i].fd = -1;
							continue;
						}
						anyOpen = true;
					}
				}
			}
		}
	} );
#endif
}

}} // namespace EE::System
