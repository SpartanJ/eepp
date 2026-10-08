#ifndef EPROC_GPU_READER_NVIDIA_HPP
#define EPROC_GPU_READER_NVIDIA_HPP

#include <eepp/core/containers.hpp>
#include <vector>

using namespace EE;

namespace eproc {

/** Per-process GPU utilization and GPU memory of the NVIDIA devices in the system.
 *
 *  The NVIDIA Management Library is loaded at run time, so the tool builds and runs on machines
 *  without an NVIDIA driver: an unavailable reader simply reports no GPU information. */
class NvidiaGpuReader {
  public:
	NvidiaGpuReader();
	~NvidiaGpuReader();
	NvidiaGpuReader( const NvidiaGpuReader& ) = delete;
	NvidiaGpuReader& operator=( const NvidiaGpuReader& ) = delete;

	/** True when libnvidia-ml loaded and nvmlInit() succeeded. */
	bool isAvailable() const;

	/** Fills per-PID GPU utilization in percent (0..100) and per-PID GPU memory in KiB.
	 *  Entries are only present for processes actually using the GPU. */
	void query( UnorderedMap<long, int>& usagePercent, UnorderedMap<long, long>& memoryKiB );

  private:
	// Only the NVML declarations this reader uses are mirrored here, the rest of the API lives in
	// the driver's own nvml.h which is deliberately not vendored.
	using NvmlDevice = void*;
	using NvmlReturn = int;

	// nvmlProcessUtilizationSample_t: process id, sample timestamp in microseconds and the
	// per-engine utilization values in percent.
	struct ProcessUtilizationSample {
		unsigned int pid;
		unsigned long long timeStamp;
		unsigned int smUtil;
		unsigned int memUtil;
		unsigned int encUtil;
		unsigned int decUtil;
	};
	static_assert( sizeof( ProcessUtilizationSample ) == 32,
				   "must match nvmlProcessUtilizationSample_t" );

	// nvmlProcessInfo_v1_t: the structure filled by the version-less running-process queries, a
	// process id followed by the device memory that process uses, in bytes.
	struct RunningProcessInfo {
		unsigned int pid;
		unsigned long long usedGpuMemory;
	};
	static_assert( sizeof( RunningProcessInfo ) == 16, "must match nvmlProcessInfo_v1_t" );

	using NvmlInitFn = NvmlReturn ( * )();
	using NvmlShutdownFn = NvmlReturn ( * )();
	using NvmlDeviceGetCountFn = NvmlReturn ( * )( unsigned int* );
	using NvmlDeviceGetHandleByIndexFn = NvmlReturn ( * )( unsigned int, NvmlDevice* );
	using NvmlDeviceGetProcessUtilizationFn = NvmlReturn ( * )( NvmlDevice,
																ProcessUtilizationSample*,
																unsigned int*, unsigned long long );
	using NvmlDeviceGetRunningProcessesFn = NvmlReturn ( * )( NvmlDevice, unsigned int*,
															  RunningProcessInfo* );

	/** Adds the utilization of the processes sampled on one device, keeping the highest value seen
	 *  per process and refreshing newestTimestamp with the newest sample returned. */
	void collectUtilization( NvmlDevice device, UnorderedMap<long, int>& usagePercent,
							 unsigned long long& newestTimestamp );

	/** Adds the GPU memory of the processes running on one device, summed per process. */
	void collectMemory( NvmlDevice device, UnorderedMap<long, long>& memoryKiB );

	void collectRunningProcesses( NvmlDevice device, NvmlDeviceGetRunningProcessesFn entryPoint,
								  UnorderedMap<long, long>& memoryKiB );

	void* mLib{ nullptr };
	bool mInitialized{ false };
	unsigned long long mLastTimestamp{ 0 };
	// resolved function pointers
	NvmlInitFn mNvmlInit{ nullptr };
	NvmlShutdownFn mNvmlShutdown{ nullptr };
	NvmlDeviceGetCountFn mNvmlDeviceGetCount{ nullptr };
	NvmlDeviceGetHandleByIndexFn mNvmlDeviceGetHandleByIndex{ nullptr };
	NvmlDeviceGetProcessUtilizationFn mNvmlDeviceGetProcessUtilization{ nullptr };
	NvmlDeviceGetRunningProcessesFn mNvmlComputeProcesses{ nullptr };
	NvmlDeviceGetRunningProcessesFn mNvmlGraphicsProcesses{ nullptr };
	// Optional: drivers without MPS support do not export this entry point.
	NvmlDeviceGetRunningProcessesFn mNvmlMpsComputeProcesses{ nullptr };

	// Reused across queries, so polling does not allocate per device.
	std::vector<ProcessUtilizationSample> mSampleBuffer;
	std::vector<RunningProcessInfo> mProcessBuffer;
};

} // namespace eproc

#endif // EPROC_GPU_READER_NVIDIA_HPP
