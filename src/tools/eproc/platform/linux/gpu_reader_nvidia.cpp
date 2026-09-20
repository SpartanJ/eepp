#include "gpu_reader_nvidia.hpp"

#include <eepp/system/sys.hpp>

#include <algorithm>

using namespace EE::System;

namespace eproc {

namespace {

// Return codes and sentinels mirrored from nvml.h.
constexpr int NvmlSuccess = 0;
constexpr int NvmlErrorInsufficientSize = 7;
constexpr unsigned long long NvmlValueNotAvailable = ~0ULL;

// GPU memory is reported in bytes, the readers of this tool work in KiB.
constexpr unsigned long long BytesPerKiB = 1024;

// Bounds the retries of the two call queries, in case a device reports more entries than the
// previous call reserved room for.
constexpr int MaxQueryAttempts = 4;

template <typename Fn> Fn resolveSymbol( void* lib, const char* name ) {
	return reinterpret_cast<Fn>( Sys::loadFunction( lib, name ) );
}

} // namespace

NvidiaGpuReader::NvidiaGpuReader() {
	mLib = Sys::loadObject( "libnvidia-ml.so.1" );
	if ( !mLib )
		mLib = Sys::loadObject( "libnvidia-ml.so" );

	// No driver installed: stay unavailable, the tool keeps working without GPU information.
	if ( !mLib )
		return;

	mNvmlInit = resolveSymbol<NvmlInitFn>( mLib, "nvmlInit" );
	mNvmlShutdown = resolveSymbol<NvmlShutdownFn>( mLib, "nvmlShutdown" );
	mNvmlDeviceGetCount = resolveSymbol<NvmlDeviceGetCountFn>( mLib, "nvmlDeviceGetCount" );
	mNvmlDeviceGetHandleByIndex =
		resolveSymbol<NvmlDeviceGetHandleByIndexFn>( mLib, "nvmlDeviceGetHandleByIndex" );
	mNvmlDeviceGetProcessUtilization =
		resolveSymbol<NvmlDeviceGetProcessUtilizationFn>( mLib, "nvmlDeviceGetProcessUtilization" );
	mNvmlComputeProcesses = resolveSymbol<NvmlDeviceGetRunningProcessesFn>(
		mLib, "nvmlDeviceGetComputeRunningProcesses" );
	mNvmlGraphicsProcesses = resolveSymbol<NvmlDeviceGetRunningProcessesFn>(
		mLib, "nvmlDeviceGetGraphicsRunningProcesses" );
	// Optional entry point, drivers that do not support MPS do not export it.
	mNvmlMpsComputeProcesses = resolveSymbol<NvmlDeviceGetRunningProcessesFn>(
		mLib, "nvmlDeviceGetMPSComputeRunningProcesses" );

	if ( !mNvmlInit || !mNvmlShutdown || !mNvmlDeviceGetCount || !mNvmlDeviceGetHandleByIndex ||
		 !mNvmlDeviceGetProcessUtilization || !mNvmlComputeProcesses || !mNvmlGraphicsProcesses )
		return;

	if ( mNvmlInit() != NvmlSuccess )
		return;

	mInitialized = true;
}

NvidiaGpuReader::~NvidiaGpuReader() {
	if ( mInitialized && mNvmlShutdown )
		mNvmlShutdown();

	if ( mLib )
		Sys::unloadObject( mLib );
}

bool NvidiaGpuReader::isAvailable() const {
	return mInitialized;
}

void NvidiaGpuReader::query( UnorderedMap<long, int>& usagePercent,
							 UnorderedMap<long, long>& memoryKiB ) {
	usagePercent.clear();
	memoryKiB.clear();

	if ( !mInitialized )
		return;

	// The device count is read on every query, GPUs can come and go while the tool runs.
	unsigned int deviceCount = 0;
	if ( mNvmlDeviceGetCount( &deviceCount ) != NvmlSuccess )
		return;

	unsigned long long newestTimestamp = mLastTimestamp;
	for ( unsigned int index = 0; index < deviceCount; index++ ) {
		NvmlDevice device = nullptr;
		// A device that cannot be queried is skipped, the remaining ones are still reported.
		if ( mNvmlDeviceGetHandleByIndex( index, &device ) != NvmlSuccess || !device )
			continue;

		collectUtilization( device, usagePercent, newestTimestamp );
		collectMemory( device, memoryKiB );
	}

	// Only samples newer than this timestamp are requested on the next query.
	mLastTimestamp = newestTimestamp;
}

void NvidiaGpuReader::collectUtilization( NvmlDevice device, UnorderedMap<long, int>& usagePercent,
										  unsigned long long& newestTimestamp ) {
	// The first call only asks for the number of samples the device has since mLastTimestamp.
	unsigned int count = 0;
	if ( mNvmlDeviceGetProcessUtilization( device, nullptr, &count, mLastTimestamp ) !=
			 NvmlErrorInsufficientSize ||
		 count == 0 )
		return;

	int result = NvmlErrorInsufficientSize;
	unsigned int filled = 0;
	for ( int attempt = 0; attempt < MaxQueryAttempts; attempt++ ) {
		mSampleBuffer.resize( count );
		filled = count;
		result = mNvmlDeviceGetProcessUtilization( device, mSampleBuffer.data(), &filled,
												   mLastTimestamp );
		if ( result != NvmlErrorInsufficientSize )
			break;

		// The device produced even more samples in the meantime: retry with the reported size.
		if ( filled <= count )
			return;

		count = filled;
	}

	if ( result != NvmlSuccess )
		return;

	const unsigned int samples =
		std::min<unsigned int>( filled, static_cast<unsigned int>( mSampleBuffer.size() ) );
	for ( unsigned int i = 0; i < samples; i++ ) {
		const ProcessUtilizationSample& sample = mSampleBuffer[i];
		if ( sample.timeStamp > newestTimestamp )
			newestTimestamp = sample.timeStamp;

		const long pid = static_cast<long>( sample.pid );
		const int utilization = static_cast<int>( sample.smUtil );
		auto found = usagePercent.find( pid );
		if ( found == usagePercent.end() )
			usagePercent[pid] = utilization;
		else if ( utilization > found->second )
			found->second = utilization;
	}
}

void NvidiaGpuReader::collectMemory( NvmlDevice device, UnorderedMap<long, long>& memoryKiB ) {
	collectRunningProcesses( device, mNvmlComputeProcesses, memoryKiB );
	collectRunningProcesses( device, mNvmlGraphicsProcesses, memoryKiB );
	// MPS compute processes are only reported by their own query, and the entry point is missing on
	// drivers without MPS support.
	if ( mNvmlMpsComputeProcesses )
		collectRunningProcesses( device, mNvmlMpsComputeProcesses, memoryKiB );
}

void NvidiaGpuReader::collectRunningProcesses( NvmlDevice device,
											   NvmlDeviceGetRunningProcessesFn entryPoint,
											   UnorderedMap<long, long>& memoryKiB ) {
	if ( !entryPoint )
		return;

	// The first call only asks for the number of processes the device reports.
	unsigned int count = 0;
	if ( entryPoint( device, &count, nullptr ) != NvmlErrorInsufficientSize || count == 0 )
		return;

	int result = NvmlErrorInsufficientSize;
	unsigned int filled = 0;
	for ( int attempt = 0; attempt < MaxQueryAttempts; attempt++ ) {
		mProcessBuffer.resize( count );
		filled = count;
		result = entryPoint( device, &filled, mProcessBuffer.data() );
		if ( result != NvmlErrorInsufficientSize )
			break;

		// More processes appeared in the meantime: retry with the reported size.
		if ( filled <= count )
			return;

		count = filled;
	}

	if ( result != NvmlSuccess )
		return;

	const unsigned int processes =
		std::min<unsigned int>( filled, static_cast<unsigned int>( mProcessBuffer.size() ) );
	for ( unsigned int i = 0; i < processes; i++ ) {
		const RunningProcessInfo& process = mProcessBuffer[i];
		// Windows and some of the older drivers do not report the used memory of a process.
		if ( process.usedGpuMemory == NvmlValueNotAvailable )
			continue;

		// A process can be listed by more than one query (compute, graphics and MPS) and by more
		// than one device; the reported usage is accumulated because that sum is the process's
		// overall GPU memory, which is what nvtop reports.
		//
		// Do NOT "de-duplicate" this with a per-query maximum: a process that appears in both the
		// compute and graphics lists genuinely uses the memory reported by each, and taking the
		// maximum would halve the figure for exactly those processes. ksysguard6 accumulates the
		// same way.
		memoryKiB[static_cast<long>( process.pid )] +=
			static_cast<long>( process.usedGpuMemory / BytesPerKiB );
	}
}

} // namespace eproc
