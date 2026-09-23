#ifndef EPROC_GPU_READER_DRM_HPP
#define EPROC_GPU_READER_DRM_HPP

#include <chrono>
#include <eepp/core/containers.hpp>
#include <string>

using namespace EE;

namespace eproc {

/** Reads the standard DRM client counters from /proc/<pid>/fdinfo. Worker-thread only. */
class DrmGpuReader {
  public:
	void beginSample();
	void query( long pid, long long startTime, int& usagePercent, long& memoryKiB );

  private:
	struct ClientSample {
		unsigned long long engineNs{ 0 };
		std::chrono::steady_clock::time_point sampledAt{};
		unsigned int pass{ 0 };
	};

	UnorderedMap<std::string, ClientSample> mPrevious;
	std::chrono::steady_clock::time_point mSampleTime{};
	unsigned int mPass{ 0 };
	bool mAvailable{ false };
};

} // namespace eproc

#endif // EPROC_GPU_READER_DRM_HPP
