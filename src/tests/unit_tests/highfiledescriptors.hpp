#pragma once

#include <eepp/config.hpp>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <unistd.h>
#include <vector>

/** Occupies every descriptor below FD_SETSIZE, so the next descriptor opened by the process is
 * beyond the range select() can handle. Common default soft RLIMIT_NOFILE values (1024 on most
 * Linux distributions, 256 on macOS) leave no room above FD_SETSIZE, so the soft limit is raised
 * up to the hard limit when needed and restored on destruction. */
class HighFileDescriptorReservation {
  public:
	HighFileDescriptorReservation() {
		// Room for the reserved descriptors plus the sockets created by the test.
		const rlim_t required = FD_SETSIZE + 64;

		if ( getrlimit( RLIMIT_NOFILE, &mOriginalLimit ) != 0 )
			return;

		if ( mOriginalLimit.rlim_cur != RLIM_INFINITY && mOriginalLimit.rlim_cur < required ) {
			if ( mOriginalLimit.rlim_max != RLIM_INFINITY && mOriginalLimit.rlim_max < required )
				return;

			struct rlimit raised = mOriginalLimit;
			raised.rlim_cur = required;
			if ( setrlimit( RLIMIT_NOFILE, &raised ) != 0 )
				return;
			mRestoreLimit = true;
		}

		mFds.reserve( FD_SETSIZE );
		while ( mFds.empty() || mFds.back() < FD_SETSIZE ) {
			int fd = ::open( "/dev/null", O_RDONLY );
			if ( fd < 0 )
				return;
			mFds.push_back( fd );
		}
		mReady = true;
	}

	~HighFileDescriptorReservation() {
		for ( int fd : mFds )
			::close( fd );

		if ( mRestoreLimit )
			setrlimit( RLIMIT_NOFILE, &mOriginalLimit );
	}

	HighFileDescriptorReservation( const HighFileDescriptorReservation& ) = delete;

	HighFileDescriptorReservation& operator=( const HighFileDescriptorReservation& ) = delete;

	/** @return True when the next opened descriptor is guaranteed to be >= FD_SETSIZE. */
	bool isReady() const { return mReady; }

  private:
	std::vector<int> mFds;
	struct rlimit mOriginalLimit{};
	bool mRestoreLimit{ false };
	bool mReady{ false };
};
#endif
