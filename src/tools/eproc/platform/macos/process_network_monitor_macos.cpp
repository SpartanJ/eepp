#include "process_network_monitor_macos.hpp"

#include <charconv>
#include <csignal>
#include <eepp/system/log.hpp>
#include <eepp/system/monotonicclock.hpp>
#include <limits>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <util.h>

using namespace EE::System;

namespace eproc {

namespace {

bool parseUnsigned( std::string_view text, Uint64& value ) {
	const auto [end, error] = std::from_chars( text.data(), text.data() + text.size(), value );
	return error == std::errc() && end == text.data() + text.size();
}

} // namespace

bool ProcessNetworkMonitorMacOS::parseRow( std::string_view line, Int64& pid, Uint64& received,
										   Uint64& sent ) {
	if ( line.empty() || line.back() != ',' )
		return false;
	line.remove_suffix( 1 );
	const auto sentComma = line.rfind( ',' );
	if ( sentComma == std::string_view::npos ||
		 !parseUnsigned( line.substr( sentComma + 1 ), sent ) )
		return false;
	line.remove_suffix( line.size() - sentComma );
	const auto receivedComma = line.rfind( ',' );
	if ( receivedComma == std::string_view::npos ||
		 !parseUnsigned( line.substr( receivedComma + 1 ), received ) )
		return false;
	line = line.substr( 0, receivedComma );
	const auto dot = line.rfind( '.' );
	Uint64 parsedPid = 0;
	if ( dot == std::string_view::npos || !parseUnsigned( line.substr( dot + 1 ), parsedPid ) ||
		 parsedPid > static_cast<Uint64>( std::numeric_limits<pid_t>::max() ) )
		return false;
	pid = static_cast<Int64>( parsedPid );
	return pid > 0;
}

ProcessNetworkMonitorMacOS::ProcessNetworkMonitorMacOS() {
	pid_t child = forkpty( &mMaster, nullptr, nullptr, nullptr );
	if ( child == 0 ) {
		execl( "/usr/bin/nettop", "nettop", "-P", "-L", "0", "-s", "1", "-n", "-x", "-t",
			   "external", "-J", "bytes_in,bytes_out", nullptr );
		_exit( 127 );
	}
	if ( child < 0 ) {
		EE::System::Log::warning( "eproc: could not start macOS network monitor" );
		mMaster = -1;
		return;
	}
	mChildPid = child;
	mReader = std::thread( &ProcessNetworkMonitorMacOS::readLoop, this );
}

ProcessNetworkMonitorMacOS::~ProcessNetworkMonitorMacOS() {
	if ( mChildPid > 0 )
		kill( mChildPid, SIGTERM );
	if ( mReader.joinable() )
		mReader.join();
	if ( mMaster >= 0 )
		close( mMaster );
	if ( mChildPid > 0 )
		waitpid( mChildPid, nullptr, 0 );
}

void ProcessNetworkMonitorMacOS::applyRates( std::vector<ProcessInfo>& processes ) const {
	std::lock_guard<std::mutex> lock( mMutex );
	for ( auto& process : processes ) {
		const auto it = mRates.find( process.pid );
		if ( it != mRates.end() ) {
			process.netDownload = it->second.received;
			process.netUpload = it->second.sent;
		}
	}
}

void ProcessNetworkMonitorMacOS::publish( std::unordered_map<Int64, Counters>& current,
										  MonotonicClock::TimePoint sampledAt ) {
	mNextRates.clear();
	const auto elapsed = ( sampledAt - mPreviousSample ).asMicroseconds();
	if ( elapsed > 0 ) {
		mNextRates.reserve( current.size() );
		for ( const auto& [pid, counters] : current ) {
			const auto previous = mPrevious.find( pid );
			if ( previous == mPrevious.end() || counters.received < previous->second.received ||
				 counters.sent < previous->second.sent )
				continue;
			auto rate = [elapsed]( Uint64 delta ) -> Int64 {
				const long double value = static_cast<long double>( delta ) * 1'000'000 / elapsed;
				return value >= std::numeric_limits<Int64>::max()
						   ? std::numeric_limits<Int64>::max()
						   : static_cast<Int64>( value );
			};
			mNextRates.emplace( pid, Rates{ rate( counters.received - previous->second.received ),
											rate( counters.sent - previous->second.sent ) } );
		}
	}
	mPrevious.swap( current );
	mPreviousSample = sampledAt;
	std::lock_guard<std::mutex> lock( mMutex );
	mRates.swap( mNextRates );
}

void ProcessNetworkMonitorMacOS::readLoop() {
	std::string pending;
	std::unordered_map<Int64, Counters> current;
	bool inSample = false;
	char buffer[8192];
	for ( ;; ) {
		const ssize_t count = read( mMaster, buffer, sizeof( buffer ) );
		if ( count <= 0 )
			break;
		pending.append( buffer, static_cast<size_t>( count ) );
		if ( pending.size() > 1 << 20 ) {
			pending.clear();
			current.clear();
			inSample = false;
			continue;
		}
		size_t start = 0;
		size_t end = 0;
		while ( ( end = pending.find( '\n', start ) ) != std::string::npos ) {
			std::string_view line( pending.data() + start, end - start );
			if ( !line.empty() && line.back() == '\r' )
				line.remove_suffix( 1 );
			if ( line == ",bytes_in,bytes_out," ) {
				if ( inSample )
					publish( current, MonotonicClock::now() );
				current.clear();
				inSample = true;
			} else if ( line.starts_with( "nettop:" ) ) {
				EE::System::Log::warning( "eproc: %.*s", static_cast<int>( line.size() ),
										  line.data() );
			} else if ( inSample ) {
				Int64 pid = 0;
				Uint64 received = 0, sent = 0;
				if ( parseRow( line, pid, received, sent ) )
					current[pid] = { received, sent };
			}
			start = end + 1;
		}
		if ( start > 0 )
			pending.erase( 0, start );
	}
	std::lock_guard<std::mutex> lock( mMutex );
	mRates.clear();
}

} // namespace eproc
