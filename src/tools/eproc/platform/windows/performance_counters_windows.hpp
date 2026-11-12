#ifndef EPROC_PERFORMANCE_COUNTERS_WINDOWS_HPP
#define EPROC_PERFORMANCE_COUNTERS_WINDOWS_HPP

#include "../../process_collector.hpp"
#include <eepp/core/containers.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <pdh.h>
#include <string_view>

namespace eproc {

/** Optional WDDM GPU and per-logical-processor counters, sampled on the collection worker. */
class PerformanceCountersWindows {
  public:
	~PerformanceCountersWindows();

	PerformanceCountersWindows() = default;

	PerformanceCountersWindows( const PerformanceCountersWindows& ) = delete;

	PerformanceCountersWindows& operator=( const PerformanceCountersWindows& ) = delete;

	void collect( std::vector<ProcessInfo>& processes, SystemInfo& system );

  private:
	friend struct PerformanceCountersWindowsTestAccess;

	struct GpuSample {
		Int64 memory{ -1 };
		int usage{ -1 };
	};

	struct CoreSample {
		unsigned index;
		float usage;
	};

	void initialize();

	PDH_FMT_COUNTERVALUE_ITEM_W* read( PDH_HCOUNTER counter, DWORD format, DWORD& count );

	void readGpuUsage( SystemInfo& system );

	void readGpuMemory( PDH_HCOUNTER counter, bool perProcess, SystemInfo& system );

	static bool processInstance( std::wstring_view name, Int64& pid, std::wstring_view& engine );

	static bool coreInstance( std::wstring_view name, unsigned& index );

	PDH_HQUERY mQuery{ nullptr };
	PDH_HCOUNTER mCpu{ nullptr };
	PDH_HCOUNTER mGpu{ nullptr };
	PDH_HCOUNTER mProcessDedicated{ nullptr };
	PDH_HCOUNTER mProcessShared{ nullptr };
	PDH_HCOUNTER mAdapterDedicated{ nullptr };
	PDH_HCOUNTER mAdapterShared{ nullptr };
	// PDH owns variable-length instance names in this aligned, reusable result buffer.
	std::vector<Uint64> mBuffer;
	std::vector<CoreSample> mCores;
	UnorderedMap<Int64, GpuSample> mProcesses;
	UnorderedMap<std::wstring_view, double> mEngines;
	bool mInitialized{ false };
};

} // namespace eproc

#endif
