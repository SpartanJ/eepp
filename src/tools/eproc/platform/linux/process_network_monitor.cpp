#include "process_network_monitor.hpp"

#include <eepp/system/log.hpp>

#include <arpa/inet.h>
#include <dirent.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <pcap/pcap.h>
#include <pcap/sll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

using namespace EE::System;

namespace eproc {

namespace {

constexpr Uint8 kIPv4 = 4;
constexpr Uint8 kIPv6 = 6;
constexpr size_t kProcPathCapacity = 64;
constexpr size_t kFdTargetCapacity = 128;

#ifndef DLT_LINUX_SLL2
constexpr int kLinuxSll2 = 276;
#else
constexpr int kLinuxSll2 = DLT_LINUX_SLL2;
#endif

inline Uint16 readBigEndian16( const Uint8* data ) {
	return static_cast<Uint16>( data[0] << 8 | data[1] );
}

inline Uint32 readBigEndian32( const Uint8* data ) {
	return static_cast<Uint32>( data[0] ) << 24 | static_cast<Uint32>( data[1] ) << 16 |
		   static_cast<Uint32>( data[2] ) << 8 | static_cast<Uint32>( data[3] );
}

inline bool isNumericName( const char* name, long& value ) {
	if ( !name || !*name )
		return false;

	char* end = nullptr;
	value = strtol( name, &end, 10 );
	return value > 0 && end != name && *end == '\0';
}

inline int hexDigit( char value ) {
	if ( value >= '0' && value <= '9' )
		return value - '0';
	if ( value >= 'a' && value <= 'f' )
		return value - 'a' + 10;
	if ( value >= 'A' && value <= 'F' )
		return value - 'A' + 10;
	return -1;
}

inline bool parseHexByte( std::string_view value, size_t offset, Uint8& result ) {
	if ( offset + 2 > value.size() )
		return false;

	const int high = hexDigit( value[offset] );
	const int low = hexDigit( value[offset + 1] );
	if ( high < 0 || low < 0 )
		return false;

	result = static_cast<Uint8>( high * 16 + low );
	return true;
}

inline bool nextToken( const char*& cursor, const char* end, std::string_view& token ) {
	while ( cursor < end &&
			( *cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n' ) )
		++cursor;
	if ( cursor >= end )
		return false;

	const char* begin = cursor;
	while ( cursor < end && *cursor != ' ' && *cursor != '\t' && *cursor != '\r' &&
			*cursor != '\n' )
		++cursor;

	token = std::string_view( begin, static_cast<size_t>( cursor - begin ) );
	return true;
}

} // namespace

std::size_t
ProcessNetworkMonitor::EndpointHash::operator()( const Endpoint& endpoint ) const noexcept {
	std::size_t hash = endpoint.family * 131u + endpoint.port;
	for ( Uint8 byte : endpoint.address )
		hash = hash * 16777619u ^ byte;
	return hash;
}

ProcessNetworkMonitor::ProcessNetworkMonitor() : mLastSnapshot( std::chrono::steady_clock::now() ) {
	mCaptureThread = std::thread( &ProcessNetworkMonitor::captureLoop, this );
}

ProcessNetworkMonitor::~ProcessNetworkMonitor() {
	mRunning.store( false );
	if ( mCaptureThread.joinable() )
		mCaptureThread.join();
}

bool ProcessNetworkMonitor::parseUnsigned( std::string_view token, Uint64& value, int base ) {
	if ( token.empty() )
		return false;

	value = 0;
	auto result = std::from_chars( token.data(), token.data() + token.size(), value, base );
	return result.ec == std::errc() && result.ptr == token.data() + token.size();
}

bool ProcessNetworkMonitor::parseProcEndpoint( std::string_view token, Endpoint& endpoint ) {
	const size_t separator = token.rfind( ':' );
	if ( separator == std::string_view::npos )
		return false;

	const std::string_view address = token.substr( 0, separator );
	Uint64 port = 0;
	if ( !parseUnsigned( token.substr( separator + 1 ), port, 16 ) || port > 0xffff )
		return false;

	Endpoint parsed;
	parsed.port = static_cast<Uint16>( port );
	if ( address.size() == 8 ) {
		parsed.family = kIPv4;
		for ( size_t i = 0; i < 4; ++i ) {
			if ( !parseHexByte( address, ( 3 - i ) * 2, parsed.address[i] ) )
				return false;
		}
	} else if ( address.size() == 32 ) {
		parsed.family = kIPv6;
		for ( size_t word = 0; word < 4; ++word ) {
			for ( size_t byte = 0; byte < 4; ++byte ) {
				if ( !parseHexByte( address, word * 8 + ( 3 - byte ) * 2,
									parsed.address[word * 4 + byte] ) )
					return false;
			}
		}
	} else {
		return false;
	}

	bool isMapped = parsed.family == kIPv6;
	for ( size_t i = 0; isMapped && i < 10; ++i )
		isMapped = parsed.address[i] == 0;
	isMapped = isMapped && parsed.address[10] == 0xff && parsed.address[11] == 0xff;
	if ( isMapped ) {
		Endpoint mapped;
		mapped.family = kIPv4;
		mapped.port = parsed.port;
		std::copy_n( parsed.address.begin() + 12, 4, mapped.address.begin() );
		parsed = mapped;
	}

	endpoint = parsed;
	return true;
}

bool ProcessNetworkMonitor::parseSocketLine( std::string_view line, Endpoint& endpoint,
											 Uint64& inode ) {
	const char* cursor = line.data();
	const char* end = cursor + line.size();
	std::string_view token;
	std::string_view localAddress;
	std::string_view inodeToken;

	for ( int index = 0; index <= 9; ++index ) {
		if ( !nextToken( cursor, end, token ) )
			return false;
		if ( index == 1 )
			localAddress = token;
		else if ( index == 9 )
			inodeToken = token;
	}

	return parseProcEndpoint( localAddress, endpoint ) && parseUnsigned( inodeToken, inode, 10 );
}

void ProcessNetworkMonitor::collectSocketOwners( UnorderedMap<Uint64, long>& owners ) {
	DIR* procDirectory = opendir( "/proc" );
	if ( !procDirectory )
		return;

	char fdPath[kProcPathCapacity];
	char target[kFdTargetCapacity];
	while ( dirent* processEntry = readdir( procDirectory ) ) {
		long pid = 0;
		if ( !isNumericName( processEntry->d_name, pid ) )
			continue;

		const int pathLength = snprintf( fdPath, sizeof( fdPath ), "/proc/%ld/fd", pid );
		if ( pathLength <= 0 || static_cast<size_t>( pathLength ) >= sizeof( fdPath ) )
			continue;

		DIR* fdDirectory = opendir( fdPath );
		if ( !fdDirectory )
			continue;

		while ( dirent* fdEntry = readdir( fdDirectory ) ) {
			if ( fdEntry->d_name[0] == '.' &&
				 ( fdEntry->d_name[1] == '\0' ||
				   ( fdEntry->d_name[1] == '.' && fdEntry->d_name[2] == '\0' ) ) )
				continue;

			ssize_t length =
				readlinkat( dirfd( fdDirectory ), fdEntry->d_name, target, sizeof( target ) - 1 );
			if ( length <= 9 )
				continue;
			target[length] = '\0';

			std::string_view link( target, static_cast<size_t>( length ) );
			if ( link.compare( 0, 8, "socket:[" ) != 0 || link.back() != ']' )
				continue;

			Uint64 inode = 0;
			if ( parseUnsigned( link.substr( 8, link.size() - 9 ), inode, 10 ) )
				owners.insert_or_assign( inode, pid );
		}

		closedir( fdDirectory );
	}

	closedir( procDirectory );
}

void ProcessNetworkMonitor::collectSocketTable( const char* path, Uint8 family,
												const UnorderedMap<Uint64, long>& owners,
												EndpointMap& endpoints ) {
	FILE* file = fopen( path, "r" );
	if ( !file )
		return;

	char line[512];
	while ( fgets( line, sizeof( line ), file ) ) {
		Endpoint endpoint;
		Uint64 inode = 0;
		if ( !parseSocketLine( line, endpoint, inode ) ||
			 ( endpoint.family != family && !( family == kIPv6 && endpoint.family == kIPv4 ) ) )
			continue;

		auto owner = owners.find( inode );
		if ( owner != owners.end() )
			endpoints.insert_or_assign( endpoint, owner->second );
	}

	fclose( file );
}

void ProcessNetworkMonitor::refreshMapping() {
	UnorderedMap<Uint64, long> owners;
	collectSocketOwners( owners );

	EndpointMap endpoints;
	collectSocketTable( "/proc/net/tcp", kIPv4, owners, endpoints );
	collectSocketTable( "/proc/net/tcp6", kIPv6, owners, endpoints );
	collectSocketTable( "/proc/net/udp", kIPv4, owners, endpoints );
	collectSocketTable( "/proc/net/udp6", kIPv6, owners, endpoints );

	std::lock_guard<std::mutex> lock( mMutex );
	mEndpoints = std::move( endpoints );
}

long ProcessNetworkMonitor::findPid( const Endpoint& endpoint ) const {
	auto exact = mEndpoints.find( endpoint );
	if ( exact != mEndpoints.end() )
		return exact->second;

	Endpoint wildcard = endpoint;
	wildcard.address.fill( 0 );
	auto anyAddress = mEndpoints.find( wildcard );
	return anyAddress != mEndpoints.end() ? anyAddress->second : 0;
}

bool ProcessNetworkMonitor::parsePacket( int dataLink, const Uint8* data, size_t length,
										 Endpoint& source, Endpoint& destination ) {
	size_t networkOffset = 0;
	Uint16 etherType = 0;

	switch ( dataLink ) {
		case DLT_EN10MB:
			if ( length < 14 )
				return false;
			networkOffset = 14;
			etherType = readBigEndian16( data + 12 );
			while ( etherType == ETHERTYPE_VLAN || etherType == 0x88a8 || etherType == 0x9100 ) {
				if ( length < networkOffset + 4 )
					return false;
				etherType = readBigEndian16( data + networkOffset + 2 );
				networkOffset += 4;
			}
			break;
		case DLT_LINUX_SLL:
			if ( length < 16 )
				return false;
			networkOffset = 16;
			etherType = readBigEndian16( data + 14 );
			break;
		case kLinuxSll2:
			if ( length < 20 )
				return false;
			networkOffset = 20;
			etherType = readBigEndian16( data );
			break;
		case DLT_RAW:
			networkOffset = 0;
			break;
		case DLT_NULL: {
			if ( length < 4 )
				return false;
			Uint32 linkFamily = 0;
			std::memcpy( &linkFamily, data, sizeof( linkFamily ) );
			if ( linkFamily != AF_INET && linkFamily != AF_INET6 )
				linkFamily = ntohl( linkFamily );
			if ( linkFamily == AF_INET )
				etherType = ETHERTYPE_IP;
			else if ( linkFamily == AF_INET6 )
				etherType = ETHERTYPE_IPV6;
			else
				return false;
			networkOffset = 4;
			break;
		}
		case DLT_LOOP: {
			if ( length < 4 )
				return false;
			const Uint32 linkFamily = readBigEndian32( data );
			if ( linkFamily == AF_INET )
				etherType = ETHERTYPE_IP;
			else if ( linkFamily == AF_INET6 )
				etherType = ETHERTYPE_IPV6;
			else
				return false;
			networkOffset = 4;
			break;
		}
		default:
			return false;
	}

	Uint8 transportProtocol = 0;
	size_t transportOffset = 0;
	if ( etherType == ETHERTYPE_IP ) {
		if ( length < networkOffset + 20 )
			return false;
		const Uint8 versionAndLength = data[networkOffset];
		if ( ( versionAndLength >> 4 ) != 4 )
			return false;
		const size_t headerLength = static_cast<size_t>( versionAndLength & 0x0f ) * 4;
		if ( headerLength < 20 || length < networkOffset + headerLength )
			return false;
		if ( ( readBigEndian16( data + networkOffset + 6 ) & 0x1fff ) != 0 )
			return false;

		source = Endpoint{};
		destination = Endpoint{};
		source.family = destination.family = kIPv4;
		std::copy_n( data + networkOffset + 12, 4, source.address.begin() );
		std::copy_n( data + networkOffset + 16, 4, destination.address.begin() );
		transportProtocol = data[networkOffset + 9];
		transportOffset = networkOffset + headerLength;
	} else if ( etherType == ETHERTYPE_IPV6 ) {
		if ( length < networkOffset + 40 )
			return false;
		if ( ( data[networkOffset] >> 4 ) != 6 )
			return false;

		source = Endpoint{};
		destination = Endpoint{};
		source.family = destination.family = kIPv6;
		std::copy_n( data + networkOffset + 8, 16, source.address.begin() );
		std::copy_n( data + networkOffset + 24, 16, destination.address.begin() );

		transportProtocol = data[networkOffset + 6];
		transportOffset = networkOffset + 40;
		while ( transportProtocol != IPPROTO_TCP && transportProtocol != IPPROTO_UDP ) {
			size_t extensionLength = 0;
			if ( transportProtocol == IPPROTO_HOPOPTS || transportProtocol == IPPROTO_ROUTING ||
				 transportProtocol == IPPROTO_DSTOPTS ) {
				if ( length < transportOffset + 2 )
					return false;
				extensionLength = static_cast<size_t>( data[transportOffset + 1] + 1 ) * 8;
			} else if ( transportProtocol == IPPROTO_FRAGMENT ) {
				if ( length < transportOffset + 8 ||
					 ( readBigEndian16( data + transportOffset + 2 ) & 0xfff8 ) != 0 )
					return false;
				extensionLength = 8;
			} else if ( transportProtocol == IPPROTO_AH ) {
				if ( length < transportOffset + 2 )
					return false;
				extensionLength = static_cast<size_t>( data[transportOffset + 1] + 2 ) * 4;
			} else {
				return false;
			}

			if ( length < transportOffset + extensionLength )
				return false;
			transportProtocol = data[transportOffset];
			transportOffset += extensionLength;
		}
	} else {
		return false;
	}

	if ( ( transportProtocol != IPPROTO_TCP && transportProtocol != IPPROTO_UDP ) ||
		 length < transportOffset + 4 )
		return false;

	source.port = readBigEndian16( data + transportOffset );
	destination.port = readBigEndian16( data + transportOffset + 2 );
	return source.port != 0 && destination.port != 0;
}

void ProcessNetworkMonitor::processPacket( int dataLink, const Uint8* data, size_t length,
										   size_t wireLength ) {
	Endpoint source;
	Endpoint destination;
	if ( !parsePacket( dataLink, data, length, source, destination ) )
		return;

	std::lock_guard<std::mutex> lock( mMutex );
	const long uploadPid = findPid( source );
	const long downloadPid = findPid( destination );
	const long pid = uploadPid > 0 ? uploadPid : downloadPid;
	if ( pid <= 0 )
		return;

	Traffic& traffic = mTraffic[pid];
	if ( uploadPid > 0 )
		traffic.upload += wireLength;
	else
		traffic.download += wireLength;
}

long ProcessNetworkMonitor::rateFor( Uint64 bytes, std::chrono::steady_clock::duration elapsed ) {
	const long double seconds = std::chrono::duration<long double>( elapsed ).count();
	if ( seconds <= 0.0L )
		return 0;

	const long double rate = static_cast<long double>( bytes ) / seconds;
	if ( rate >= static_cast<long double>( std::numeric_limits<long>::max() ) )
		return std::numeric_limits<long>::max();
	return static_cast<long>( rate );
}

void ProcessNetworkMonitor::applyRates( std::vector<ProcessInfo>& processes ) {
	const auto now = std::chrono::steady_clock::now();
	std::lock_guard<std::mutex> lock( mMutex );

	const bool available = mAvailable.load();
	const auto elapsed = now - mLastSnapshot;
	for ( ProcessInfo& process : processes ) {
		if ( !available ) {
			process.netDownload = -1;
			process.netUpload = -1;
			continue;
		}

		auto traffic = mTraffic.find( process.pid );
		if ( traffic == mTraffic.end() ) {
			process.netDownload = 0;
			process.netUpload = 0;
		} else {
			process.netDownload = rateFor( traffic->second.download, elapsed );
			process.netUpload = rateFor( traffic->second.upload, elapsed );
		}
	}

	mTraffic.clear();
	mLastSnapshot = now;
}

void ProcessNetworkMonitor::captureLoop() {
	char errorBuffer[PCAP_ERRBUF_SIZE] = {};
	pcap_t* capture = pcap_create( nullptr, errorBuffer );
	if ( !capture ) {
		Log::warning( "eproc: could not start per-process network capture: %s", errorBuffer );
		return;
	}

	pcap_set_snaplen( capture, 256 );
	pcap_set_promisc( capture, 0 );
	pcap_set_timeout( capture, 250 );
	int result = pcap_activate( capture );
	if ( result < 0 ) {
		Log::warning( "eproc: per-process network capture is unavailable: %s",
					  pcap_geterr( capture ) );
		pcap_close( capture );
		return;
	}

	bpf_program filter;
	const int compileResult =
		pcap_compile( capture, &filter, "tcp or udp", 1, PCAP_NETMASK_UNKNOWN );
	if ( compileResult < 0 ) {
		Log::warning( "eproc: could not install the per-process network capture filter: %s",
					  pcap_geterr( capture ) );
		pcap_close( capture );
		return;
	}
	if ( pcap_setfilter( capture, &filter ) < 0 ) {
		Log::warning( "eproc: could not install the per-process network capture filter: %s",
					  pcap_geterr( capture ) );
		pcap_freecode( &filter );
		pcap_close( capture );
		return;
	}
	pcap_freecode( &filter );

	mDataLink = pcap_datalink( capture );
	if ( mDataLink < 0 ) {
		pcap_close( capture );
		return;
	}

	mAvailable.store( true );
	while ( mRunning.load() ) {
		pcap_pkthdr* header = nullptr;
		const u_char* data = nullptr;
		result = pcap_next_ex( capture, &header, &data );
		if ( result == 1 )
			processPacket( mDataLink, data, header->caplen, header->len );
		else if ( result == PCAP_ERROR_BREAK || result == PCAP_ERROR )
			break;
	}

	mAvailable.store( false );
	pcap_close( capture );
}

} // namespace eproc
