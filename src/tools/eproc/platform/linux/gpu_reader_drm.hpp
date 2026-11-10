#ifndef EPROC_GPU_READER_DRM_HPP
#define EPROC_GPU_READER_DRM_HPP

#include <eepp/core/containers.hpp>
#include <eepp/system/monotonicclock.hpp>
#include <string>

using namespace EE;

namespace eproc {

using namespace EE::System;

/** Reads the standard DRM client counters from /proc/<pid>/fdinfo. Worker-thread only. */
class DrmGpuReader {
  public:
	void beginSample();
	void query( long pid, long long startTime, int& usagePercent, long& memoryKiB );

  private:
	/** NVIDIA counters come from NVML. Unknown drivers still require the fdinfo scan. */
	static bool hasSupportedDevice( const char* drmClassPath );

	struct ClientSample {
		unsigned long long engineNs{ 0 };
		MonotonicClock::TimePoint sampledAt{};
		unsigned int pass{ 0 };
	};

	UnorderedMap<std::string, ClientSample> mPrevious;
	MonotonicClock::TimePoint mSampleTime{};
	unsigned int mPass{ 0 };
	bool mAvailable{ false };
};

} // namespace eproc

#endif // EPROC_GPU_READER_DRM_HPP
