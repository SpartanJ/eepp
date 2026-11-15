#ifndef EPROC_GPU_READER_DRM_HPP
#define EPROC_GPU_READER_DRM_HPP

#include "../../process_collector.hpp"
#include <eepp/core/containers.hpp>
#include <eepp/system/monotonicclock.hpp>
#include <string>

using namespace EE;

namespace eproc {

using namespace EE::System;

/** Reads the standard DRM client counters from /proc/<pid>/fdinfo. Worker-thread only. */
class DrmGpuReader {
  public:
	void beginSample( std::vector<GpuInfo>& gpus );
	void query( long pid, long long startTime, int& usagePercent, long& memoryKiB,
				std::vector<GpuInfo>& gpus, SmallVector<Uint32, 2>& devices );

  private:
	/** NVIDIA counters come from NVML. Unknown drivers still require the fdinfo scan. */
	static bool hasSupportedDevice( const char* drmClassPath );

	struct Device {
		std::string id;
		std::string name;
	};

	/** Model lookup is cached; polling only reads the small sysfs counter files. */
	static std::string deviceName( const std::string& path );

	void collectDevices( std::vector<GpuInfo>& gpus );

	struct ClientSample {
		unsigned long long engineNs{ 0 };
		MonotonicClock::TimePoint sampledAt{};
		unsigned int pass{ 0 };
	};

	UnorderedMap<std::string, ClientSample> mPrevious;
	std::vector<Device> mDevices;
	UnorderedSet<std::string> mSampleClients;
	MonotonicClock::TimePoint mSampleTime{};
	unsigned int mPass{ 0 };
	bool mAvailable{ false };
};

} // namespace eproc

#endif // EPROC_GPU_READER_DRM_HPP
