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

	// Adapter queries are optional; older drivers can still provide process accounting.
	mNvmlDeviceGetName = resolveSymbol<NvmlDeviceGetStringFn>( mLib, "nvmlDeviceGetName" );
	mNvmlDeviceGetUUID = resolveSymbol<NvmlDeviceGetStringFn>( mLib, "nvmlDeviceGetUUID" );
	mNvmlDeviceGetHandleByPciBusId =
		resolveSymbol<NvmlDeviceGetHandleByPciBusIdFn>( mLib, "nvmlDeviceGetHandleByPciBusId_v2" );
	mNvmlDeviceGetUtilizationRates =
		resolveSymbol<NvmlDeviceGetUtilizationRatesFn>( mLib, "nvmlDeviceGetUtilizationRates" );
	mNvmlDeviceGetMemoryInfo =
		resolveSymbol<NvmlDeviceGetMemoryInfoFn>( mLib, "nvmlDeviceGetMemoryInfo" );

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
							 UnorderedMap<long, long>& memoryKiB, std::vector<GpuInfo>& gpus ) {
	usagePercent.clear();
	memoryKiB.clear();
	mProcessDevices.clear();

	if ( !mInitialized )
		return;

	// The device count is read on every query, GPUs can come and go while the tool runs.
	unsigned int deviceCount = 0;
	if ( mNvmlDeviceGetCount( &deviceCount ) != NvmlSuccess )
		return;

	for ( unsigned int index = 0; index < deviceCount; index++ ) {
		NvmlDevice device = nullptr;
		// A device that cannot be queried is skipped, the remaining ones are still reported.
		if ( mNvmlDeviceGetHandleByIndex( index, &device ) != NvmlSuccess || !device )
			continue;

		// Match sysfs by PCI address, so NVML and DRM never create two cards for one GPU.
		auto found = std::find_if( gpus.begin(), gpus.end(), [&]( const GpuInfo& gpu ) {
			NvmlDevice candidate = nullptr;
			return mNvmlDeviceGetHandleByPciBusId &&
				   mNvmlDeviceGetHandleByPciBusId( gpu.id.c_str(), &candidate ) == NvmlSuccess &&
				   candidate == device;
		} );
		if ( found == gpus.end() ) {
			char uuid[96]{};
			if ( !mNvmlDeviceGetUUID ||
				 mNvmlDeviceGetUUID( device, uuid, sizeof( uuid ) ) != NvmlSuccess ) {
				// Optional identity queries must not suppress existing process accounting.
				mDeviceIndex = UINT32_MAX;
				unsigned long long newestTimestamp = mLastTimestamp;
				collectUtilization( device, usagePercent, newestTimestamp );
				collectMemory( device, memoryKiB );
				mLastTimestamp = newestTimestamp;
				continue;
			}
			gpus.emplace_back();
			found = std::prev( gpus.end() );
			found->id = uuid;
		}
		char name[128]{};
		if ( mNvmlDeviceGetName &&
			 mNvmlDeviceGetName( device, name, sizeof( name ) ) == NvmlSuccess )
			found->name = name;
		Utilization utilization{};
		if ( mNvmlDeviceGetUtilizationRates &&
			 mNvmlDeviceGetUtilizationRates( device, &utilization ) == NvmlSuccess ) {
			found->usage = static_cast<int>( std::min( utilization.gpu, 100u ) );
			found->usageTracked = false;
		}
		MemoryInfo memory{};
		if ( mNvmlDeviceGetMemoryInfo &&
			 mNvmlDeviceGetMemoryInfo( device, &memory ) == NvmlSuccess ) {
			found->memory = memory.used / BytesPerKiB;
			found->totalMemory = memory.total / BytesPerKiB;
			found->memoryTracked = false;
		}
		mLastTimestamp = mLastTimestamps[found->id];
		unsigned long long newestTimestamp = mLastTimestamp;
		mDeviceIndex = static_cast<Uint32>( found - gpus.begin() );
		collectUtilization( device, usagePercent, newestTimestamp );
		collectMemory( device, memoryKiB );
		// Only samples newer than this adapter's timestamp are requested next time.
		mLastTimestamps[found->id] = newestTimestamp;
	}
}

const SmallVector<Uint32, 2>* NvidiaGpuReader::devicesForProcess( long pid ) const {
	auto found = mProcessDevices.find( pid );
	return found != mProcessDevices.end() ? &found->second : nullptr;
}

void NvidiaGpuReader::recordDevice( long pid ) {
	if ( mDeviceIndex == UINT32_MAX )
		return;
	auto& devices = mProcessDevices[pid];
	if ( devices.empty() || devices.back() != mDeviceIndex )
		devices.push_back( mDeviceIndex );
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
		recordDevice( pid );
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
		recordDevice( static_cast<long>( process.pid ) );
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
