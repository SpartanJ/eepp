#include "gpu_reader_drm.hpp"
#include <eepp/system/monotonicclock.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>

using namespace EE::System;

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

bool DrmGpuReader::hasSupportedDevice( const char* drmClassPath ) {
	DIR* devices = opendir( drmClassPath );
	if ( !devices )
		return true;

	bool foundDevice = false;
	bool supported = false;
	struct dirent* entry;
	while ( ( entry = readdir( devices ) ) != nullptr ) {
		const char* number = nullptr;
		if ( strncmp( entry->d_name, "card", 4 ) == 0 )
			number = entry->d_name + 4;
		else if ( strncmp( entry->d_name, "renderD", 7 ) == 0 )
			number = entry->d_name + 7;
		if ( !number || *number < '0' || *number > '9' )
			continue;
		while ( *number >= '0' && *number <= '9' )
			++number;
		if ( *number != '\0' )
			continue; // Connector entries such as card0-DP-1 are not devices.

		foundDevice = true;
		char path[PATH_MAX];
		char driver[PATH_MAX];
		const int length =
			snprintf( path, sizeof( path ), "%s/%s/device/driver", drmClassPath, entry->d_name );
		const ssize_t driverLength = length > 0 && static_cast<size_t>( length ) < sizeof( path )
										 ? readlink( path, driver, sizeof( driver ) - 1 )
										 : -1;
		if ( driverLength <= 0 || static_cast<size_t>( driverLength ) >= sizeof( driver ) - 1 ) {
			supported = true;
			break;
		}
		driver[driverLength] = '\0';
		const char* name = strrchr( driver, '/' );
		if ( strcmp( name ? name + 1 : driver, "nvidia" ) != 0 ) {
			supported = true;
			break;
		}
	}
	closedir( devices );
	// Missing/unknown sysfs information must not suppress a potentially supported device.
	return supported || !foundDevice;
}

std::string DrmGpuReader::deviceName( const std::string& path ) {
	auto hex = [&]( const char* leaf ) {
		unsigned value = 0;
		if ( FILE* file = fopen( ( path + '/' + leaf ).c_str(), "r" ) ) {
			if ( fscanf( file, "%x", &value ) != 1 )
				value = 0;
			fclose( file );
		}
		return value;
	};
	const unsigned vendor = hex( "vendor" ), device = hex( "device" ), revision = hex( "revision" );
	char line[1024];
	if ( vendor == 0x1002 ) {
		if ( FILE* file = fopen( "/usr/share/libdrm/amdgpu.ids", "r" ) ) {
			while ( fgets( line, sizeof( line ), file ) ) {
				unsigned id, rev;
				int offset = 0;
				if ( sscanf( line, "%x, %x, %n", &id, &rev, &offset ) == 2 && offset > 0 &&
					 id == device && rev == revision ) {
					fclose( file );
					line[strcspn( line, "\r\n" )] = '\0';
					return line + offset;
				}
			}
			fclose( file );
		}
	}
	// The PCI database also covers integrated devices absent from amdgpu.ids.
	for ( const char* database : { "/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids" } ) {
		FILE* file = fopen( database, "r" );
		if ( !file )
			continue;
		bool matchingVendor = false;
		while ( fgets( line, sizeof( line ), file ) ) {
			unsigned id;
			int offset = 0;
			if ( line[0] != '\t' && sscanf( line, "%x %n", &id, &offset ) == 1 ) {
				matchingVendor = id == vendor;
			} else if ( matchingVendor && line[0] == '\t' && line[1] != '\t' &&
						sscanf( line + 1, "%x %n", &id, &offset ) == 1 && id == device &&
						offset > 0 ) {
				fclose( file );
				line[strcspn( line, "\r\n" )] = '\0';
				std::string name = line + 1 + offset;
				if ( vendor == 0x1002 ) {
					const size_t start = name.find( '[' ), end = name.find( ']' );
					if ( start != std::string::npos && end != std::string::npos && end > start )
						return "AMD " + name.substr( start + 1, end - start - 1 );
				}
				return name;
			}
		}
		fclose( file );
	}
	return {};
}

void DrmGpuReader::collectDevices( std::vector<GpuInfo>& gpus ) {
	gpus.clear();
	DIR* dir = opendir( "/sys/class/drm" );
	if ( !dir )
		return;
	struct dirent* entry;
	while ( ( entry = readdir( dir ) ) != nullptr ) {
		unsigned card;
		int end = 0;
		if ( sscanf( entry->d_name, "card%u%n", &card, &end ) != 1 || end == 0 ||
			 entry->d_name[end] )
			continue;
		char devicePath[PATH_MAX];
		char link[PATH_MAX];
		const int length =
			snprintf( link, sizeof( link ), "/sys/class/drm/%s/device", entry->d_name );
		if ( length <= 0 || static_cast<size_t>( length ) >= sizeof( link ) ||
			 !realpath( link, devicePath ) )
			continue;
		const std::string id = strrchr( devicePath, '/' ) + 1;
		auto cached = std::find_if( mDevices.begin(), mDevices.end(),
									[&]( const Device& device ) { return device.id == id; } );
		if ( cached == mDevices.end() ) {
			mDevices.push_back( { id, deviceName( devicePath ) } );
			cached = std::prev( mDevices.end() );
		}
		GpuInfo gpu;
		gpu.id = id;
		gpu.name = cached->name;
		auto counter = [&]( const char* leaf ) -> Int64 {
			unsigned long long value;
			char path[PATH_MAX];
			const int length = snprintf( path, sizeof( path ), "%s/%s", link, leaf );
			if ( length <= 0 || static_cast<size_t>( length ) >= sizeof( path ) )
				return -1;
			FILE* file = fopen( path, "r" );
			if ( !file )
				return -1;
			const bool valid = fscanf( file, "%llu", &value ) == 1;
			fclose( file );
			return valid && value <= static_cast<unsigned long long>( INT64_MAX )
					   ? static_cast<Int64>( value )
					   : -1;
		};
		const Int64 busy = counter( "gpu_busy_percent" );
		gpu.usage = busy >= 0 ? static_cast<int>( std::min<Int64>( busy, 100 ) ) : -1;
		const Int64 used = counter( "mem_info_vram_used" );
		const Int64 total = counter( "mem_info_vram_total" );
		gpu.memory = used >= 0 ? used / 1024 : -1;
		gpu.totalMemory = total >= 0 ? total / 1024 : -1;
		gpu.usageTracked = gpu.usage < 0;
		gpu.memoryTracked = gpu.memory < 0;
		gpus.push_back( std::move( gpu ) );
	}
	closedir( dir );
	std::sort( gpus.begin(), gpus.end(),
			   []( const GpuInfo& a, const GpuInfo& b ) { return a.id < b.id; } );
}

void DrmGpuReader::beginSample( std::vector<GpuInfo>& gpus ) {
	collectDevices( gpus );
	mSampleClients.clear();
	mSampleTime = MonotonicClock::now();
	// /dev/dri also exists with the proprietary NVIDIA driver, whose fdinfo is ignored
	// by readClient(). Avoid walking every process's descriptors when only that driver exists.
	// Recheck each sample for hotplug; mixed-GPU systems retain the ordinary DRM path.
	mAvailable = access( "/dev/dri", F_OK ) == 0 && hasSupportedDevice( "/sys/class/drm" );
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

void DrmGpuReader::query( long pid, long long startTime, int& usagePercent, long& memoryKiB,
						  std::vector<GpuInfo>& gpus, SmallVector<Uint32, 2>& devices ) {
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
		auto gpu = std::find_if( gpus.begin(), gpus.end(),
								 [&]( const GpuInfo& gpu ) { return gpu.id == client.device; } );
		if ( gpu != gpus.end() ) {
			const auto index = static_cast<Uint32>( gpu - gpus.begin() );
			if ( std::find( devices.begin(), devices.end(), index ) == devices.end() )
				devices.push_back( index );
		}
		// Descriptors can be shared across processes; count a DRM client once per adapter.
		const bool firstClient =
			gpu != gpus.end() && ( gpu->usageTracked || gpu->memoryTracked ) &&
			mSampleClients.emplace( client.driver + ':' + client.device + ':' + client.id ).second;
		if ( client.hasMemory ) {
			if ( firstClient && gpu != gpus.end() && gpu->memoryTracked )
				gpu->memory = std::max<Int64>( gpu->memory, 0 ) + client.memoryBytes / 1024;
			memoryBytes += client.memoryBytes;
			hasMemory = true;
		}
		if ( client.hasEngine ) {
			hasEngine = true;
			auto& previous = mPrevious[key];
			if ( previous.pass != 0 && previous.pass + 1 == mPass &&
				 client.engineNs >= previous.engineNs ) {
				const Time elapsed = mSampleTime - previous.sampledAt;
				if ( elapsed > Time::Zero ) {
					const double clientUsage =
						std::min( 100.0, ( client.engineNs - previous.engineNs ) * 100.0 /
											 ( elapsed.asSeconds() * 1e9 ) );
					usage += clientUsage;
					if ( firstClient && gpu != gpus.end() && gpu->usageTracked )
						gpu->usage = std::min( 100, std::max( gpu->usage, 0 ) +
														static_cast<int>( clientUsage ) );
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
