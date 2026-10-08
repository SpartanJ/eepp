#include "gpu_reader_drm.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>

namespace eproc {

namespace {

struct DrmClient {
	std::string driver;
	std::string device;
	std::string id;
	unsigned long long engineNs{ 0 };
	unsigned long long memoryBytes{ 0 };
	bool hasEngine{ false };
	bool hasMemory{ false };
};

bool readClient( const char* path, DrmClient& client ) {
	FILE* file = fopen( path, "r" );
	if ( !file )
		return false;
	char line[512];
	while ( fgets( line, sizeof( line ), file ) ) {
		if ( strncmp( line, "drm-driver:", 11 ) == 0 ) {
			char value[128];
			if ( sscanf( line + 11, "%127s", value ) == 1 )
				client.driver = value;
		} else if ( strncmp( line, "drm-pdev:", 9 ) == 0 ) {
			char value[128];
			if ( sscanf( line + 9, "%127s", value ) == 1 )
				client.device = value;
		} else if ( strncmp( line, "drm-client-id:", 14 ) == 0 ) {
			char value[128];
			if ( sscanf( line + 14, "%127s", value ) == 1 )
				client.id = value;
		} else if ( strncmp( line, "drm-engine-", 11 ) == 0 &&
					strncmp( line, "drm-engine-capacity-", 20 ) != 0 ) {
			const char* colon = strchr( line, ':' );
			unsigned long long value;
			char unit[8];
			if ( colon && sscanf( colon + 1, "%llu %7s", &value, unit ) == 2 &&
				 strcmp( unit, "ns" ) == 0 ) {
				client.engineNs += value;
				client.hasEngine = true;
			}
		} else if ( strncmp( line, "drm-memory-", 11 ) == 0 ) {
			const char* colon = strchr( line, ':' );
			unsigned long long value;
			char unit[8] = {};
			if ( colon && sscanf( colon + 1, "%llu %7s", &value, unit ) >= 1 ) {
				if ( strcmp( unit, "KiB" ) == 0 )
					value *= 1024;
				else if ( strcmp( unit, "MiB" ) == 0 )
					value *= 1024 * 1024;
				else if ( unit[0] != '\0' )
					continue;
				client.memoryBytes += value;
				client.hasMemory = true;
			}
		}
	}
	fclose( file );
	return !client.driver.empty() && !client.id.empty() && client.driver != "nvidia";
}

} // namespace

void DrmGpuReader::beginSample() {
	mSampleTime = std::chrono::steady_clock::now();
	mAvailable = access( "/dev/dri", F_OK ) == 0;
	if ( ++mPass == 0 )
		++mPass;
	// Entries of exited processes are retained briefly to avoid rebuilding the map each pass.
	if ( ( mPass % 30 ) == 0 ) {
		for ( auto it = mPrevious.begin(); it != mPrevious.end(); ) {
			if ( it->second.pass + 1 < mPass )
				it = mPrevious.erase( it );
			else
				++it;
		}
	}
}

void DrmGpuReader::query( long pid, long long startTime, int& usagePercent, long& memoryKiB ) {
	if ( !mAvailable )
		return;
	char dirPath[64];
	if ( snprintf( dirPath, sizeof( dirPath ), "/proc/%ld/fdinfo", pid ) <= 0 )
		return;
	DIR* dir = opendir( dirPath );
	if ( !dir )
		return;

	UnorderedSet<std::string> seenClients;
	double usage = 0;
	unsigned long long memoryBytes = 0;
	bool hasEngine = false;
	bool hasDelta = false;
	bool hasMemory = false;
	struct dirent* entry;
	while ( ( entry = readdir( dir ) ) != nullptr ) {
		if ( entry->d_name[0] < '0' || entry->d_name[0] > '9' )
			continue;
		char fdPath[96];
		if ( snprintf( fdPath, sizeof( fdPath ), "/proc/%ld/fd/%s", pid, entry->d_name ) <= 0 )
			continue;
		char target[PATH_MAX];
		const ssize_t targetLength = readlink( fdPath, target, sizeof( target ) - 1 );
		if ( targetLength < 0 )
			continue;
		target[targetLength] = '\0';
		if ( strncmp( target, "/dev/dri/", 9 ) != 0 )
			continue;
		char infoPath[96];
		if ( snprintf( infoPath, sizeof( infoPath ), "%s/%s", dirPath, entry->d_name ) <= 0 )
			continue;
		DrmClient client;
		if ( !readClient( infoPath, client ) )
			continue;
		std::string key = std::to_string( pid ) + ':' + std::to_string( startTime ) + ':' +
						  client.driver + ':' + client.device + ':' + client.id;
		if ( !seenClients.emplace( key ).second )
			continue;
		if ( client.hasMemory ) {
			memoryBytes += client.memoryBytes;
			hasMemory = true;
		}
		if ( client.hasEngine ) {
			hasEngine = true;
			auto& previous = mPrevious[key];
			if ( previous.pass != 0 && previous.pass + 1 == mPass &&
				 client.engineNs >= previous.engineNs ) {
				const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
					mSampleTime - previous.sampledAt );
				if ( elapsed.count() > 0 ) {
					usage += std::min( 100.0, ( client.engineNs - previous.engineNs ) * 100.0 /
												  elapsed.count() );
					hasDelta = true;
				}
			}
			// Some drivers briefly report a lower value; retain the high-water mark until the
			// counter catches up, as the DRM usage-stat specification requires.
			previous.engineNs = std::max( previous.engineNs, client.engineNs );
			previous.sampledAt = mSampleTime;
			previous.pass = mPass;
		}
	}
	closedir( dir );
	if ( hasMemory )
		memoryKiB = static_cast<long>( memoryBytes / 1024 );
	// Engine counters are cumulative; the first sample has no meaningful rate.
	if ( hasEngine && hasDelta )
		usagePercent = static_cast<int>( std::min( 100.0, usage ) );
}

} // namespace eproc
