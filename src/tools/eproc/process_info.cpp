#include "process_info.hpp"

#include <cstdio>

namespace eproc {

static std::string formatScaledIEC( double amount, const char* const* units, size_t unitCount ) {
	for ( size_t i = 0; i + 1 < unitCount; ++i ) {
		if ( amount < 1024.0 ) {
			char buf[32];
			snprintf( buf, sizeof( buf ), "%.1f %s", amount, units[i] );
			return buf;
		}
		amount /= 1024.0;
	}

	char buf[32];
	snprintf( buf, sizeof( buf ), "%.1f %s", amount, units[unitCount - 1] );
	return buf;
}

std::string formatBytesPerSecond( long bytes ) {
	if ( bytes <= 0 )
		return {};

	static const char* const units[] = { "K/s", "M/s", "G/s", "T/s" };
	if ( bytes < 1024 )
		return std::to_string( bytes ) + " B/s";
	return formatScaledIEC( static_cast<double>( bytes ) / 1024.0, units, 4 );
}

std::string formatBytes( long long bytes ) {
	if ( bytes < 0 )
		return {};

	if ( bytes < 1024 )
		return std::to_string( bytes ) + " B";

	static const char* const units[] = { "KiB", "MiB", "GiB", "TiB", "PiB" };
	return formatScaledIEC( static_cast<double>( bytes ) / 1024.0, units, 5 );
}

// Mirrors ksysguard6's ProcessModel::formatByteSize(): the unit is picked with a 0.9 hysteresis
// threshold so a value never reads as "1.0" of a unit it barely exceeds, kilobytes stay integral,
// and larger units keep one decimal. That is what produces "0 K", "853.8 M" and "4.2 G".
std::string formatKiB( long kib ) {
	if ( kib < 0 )
		return {};

	static const double KiB = 1024.0;
	static const char* const units[] = { "M", "G", "T", "P" };

	if ( static_cast<double>( kib ) < KiB * 0.9 )
		return std::to_string( kib ) + " K";

	double amount = kib;
	for ( size_t i = 0; i + 1 < 4; ++i ) {
		amount /= KiB;
		if ( amount < KiB * 0.9 ) {
			char buf[32];
			snprintf( buf, sizeof( buf ), "%.1f %s", amount, units[i] );
			return buf;
		}
	}

	char buf[32];
	snprintf( buf, sizeof( buf ), "%.1f %s", amount / KiB, units[3] );
	return buf;
}

// IEC form used by the status bar: "3.2 MiB", "62.7 GiB". Kilobyte amounts stay integral,
// matching how the original renders small memory totals.
std::string formatKiBIEC( long kib ) {
	if ( kib < 0 )
		return {};

	if ( kib < 1024 )
		return std::to_string( kib ) + " KiB";

	static const char* const units[] = { "MiB", "GiB", "TiB", "PiB" };
	return formatScaledIEC( static_cast<double>( kib ) / 1024.0, units, 4 );
}

std::string ProcessInfo::formatMemory() const {
	return formatKiB( getMemoryForSort() );
}

std::string ProcessInfo::formatSharedMem() const {
	return formatKiB( sharedMem );
}

std::string ProcessInfo::formatCpu() const {
	int total = userUsage + sysUsage;
	if ( total <= 0 )
		return {};
	return std::to_string( total ) + "%";
}

std::string ProcessInfo::formatGpuUsage() const {
	if ( gpuUsage < 0 )
		return {};
	return std::to_string( gpuUsage ) + "%";
}

std::string ProcessInfo::formatGpuMemory() const {
	if ( gpuMemory < 0 )
		return {};
	return formatKiBIEC( gpuMemory );
}

std::string ProcessInfo::formatDownload() const {
	return formatBytesPerSecond( netDownload );
}

std::string ProcessInfo::formatUpload() const {
	return formatBytesPerSecond( netUpload );
}

std::string ProcessInfo::formatCpuTime( long ticksPerSecond ) const {
	const long long totalTicks = static_cast<long long>( userTime ) + sysTime;
	if ( totalTicks < 0 || ticksPerSecond <= 0 )
		return {};

	const long long totalSeconds = totalTicks / ticksPerSecond;
	char buffer[32];
	snprintf( buffer, sizeof( buffer ), "%lld:%02lld", totalSeconds / 60, totalSeconds % 60 );
	return buffer;
}

std::string ProcessInfo::formatRelativeStartTime( double uptimeSeconds,
												  long ticksPerSecond ) const {
	if ( startTime <= 0 || uptimeSeconds < 0 || ticksPerSecond <= 0 )
		return {};

	const double age = uptimeSeconds - static_cast<double>( startTime ) / ticksPerSecond;
	if ( age < 0 )
		return {};

	const long long totalSeconds = static_cast<long long>( age );
	const long long hours = totalSeconds / 3600;
	const long long minutes = ( totalSeconds / 60 ) % 60;
	const long long seconds = totalSeconds % 60;
	char buffer[48];
	snprintf( buffer, sizeof( buffer ), "%lld:%02lld:%02lld", hours, minutes, seconds );
	return buffer;
}

} // namespace eproc
