#include "performance_counters_windows.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <dxgi.h>
#include <eepp/system/sys.hpp>
#include <limits>
#include <pdhmsg.h>

namespace eproc {

namespace {

bool validCounter( DWORD status ) {
	return status == PDH_CSTATUS_VALID_DATA || status == PDH_CSTATUS_NEW_DATA;
}

bool decimal( std::wstring_view text, Uint64& value ) {
	if ( text.empty() )
		return false;
	value = 0;
	for ( wchar_t ch : text ) {
		if ( ch < L'0' || ch > L'9' ||
			 value > ( std::numeric_limits<Uint64>::max() - ( ch - L'0' ) ) / 10 )
			return false;
		value = value * 10 + ( ch - L'0' );
	}
	return true;
}

} // namespace

PerformanceCountersWindows::~PerformanceCountersWindows() {
	if ( mQuery )
		PdhCloseQuery( mQuery );
}

void PerformanceCountersWindows::initialize() {
	mInitialized = true;
	readAdapterNames();
	if ( PdhOpenQueryW( nullptr, 0, &mQuery ) != ERROR_SUCCESS )
		return;
	// English paths are translated by PDH, including on non-English Windows installations.
	const auto add = [this]( const wchar_t* path, PDH_HCOUNTER& counter ) {
		if ( PdhAddEnglishCounterW( mQuery, path, 0, &counter ) != ERROR_SUCCESS )
			counter = nullptr;
	};
	add( L"\\Processor Information(*)\\% Processor Time", mCpu );
	add( L"\\GPU Engine(*)\\Utilization Percentage", mGpu );
	add( L"\\GPU Process Memory(*)\\Dedicated Usage", mProcessDedicated );
	add( L"\\GPU Process Memory(*)\\Shared Usage", mProcessShared );
	add( L"\\GPU Adapter Memory(*)\\Dedicated Usage", mAdapterDedicated );
	add( L"\\GPU Adapter Memory(*)\\Shared Usage", mAdapterShared );
}

void PerformanceCountersWindows::readAdapterNames() {
	// Resolve DXGI dynamically, like the optional driver APIs on the other backends.
	void* library = EE::System::Sys::loadObject( "dxgi.dll" );
	if ( !library )
		return;
	using CreateFactory = HRESULT( WINAPI* )( REFIID, void** );
	auto create = reinterpret_cast<CreateFactory>(
		EE::System::Sys::loadFunction( library, "CreateDXGIFactory" ) );
	IDXGIFactory* factory = nullptr;
	if ( create &&
		 SUCCEEDED( create( __uuidof( IDXGIFactory ), reinterpret_cast<void**>( &factory ) ) ) ) {
		IDXGIAdapter* adapter = nullptr;
		for ( UINT i = 0; factory->EnumAdapters( i, &adapter ) == S_OK; ++i ) {
			DXGI_ADAPTER_DESC description{};
			if ( SUCCEEDED( adapter->GetDesc( &description ) ) && description.VendorId != 0x1414 ) {
				GpuInfo gpu;
				gpu.id = String::format( "luid_0x%08x_0x%08x_phys_0",
										 static_cast<unsigned>( description.AdapterLuid.HighPart ),
										 static_cast<unsigned>( description.AdapterLuid.LowPart ) );
				gpu.name = String::fromWide( description.Description ).toUtf8();
				mAdapters.push_back( std::move( gpu ) );
			}
			adapter->Release();
		}
		factory->Release();
	}
	EE::System::Sys::unloadObject( library );
}

std::string PerformanceCountersWindows::adapterId( std::wstring_view instance ) {
	if ( instance.substr( 0, 5 ) != L"luid_" )
		return {};
	const size_t engine = instance.find( L"_eng_" );
	instance = instance.substr( 0, engine );
	// Counter identities are ASCII; avoid locale-sensitive wide conversion.
	return std::string( instance.begin(), instance.end() );
}

Uint32 PerformanceCountersWindows::adapterIndex( SystemInfo& system, std::wstring_view instance ) {
	const std::string id = adapterId( instance );
	auto found = std::find_if( system.gpus.begin(), system.gpus.end(),
							   [&]( const GpuInfo& gpu ) { return gpu.id == id; } );
	if ( found == system.gpus.end() ) {
		system.gpus.emplace_back();
		found = std::prev( system.gpus.end() );
		found->id = id;
	}
	return static_cast<Uint32>( found - system.gpus.begin() );
}

PDH_FMT_COUNTERVALUE_ITEM_W* PerformanceCountersWindows::read( PDH_HCOUNTER counter, DWORD format,
															   DWORD& count ) {
	count = 0;
	if ( !counter )
		return nullptr;
	// Reuse the last allocation; retry if instances appeared between the size and data queries.
	for ( int attempt = 0; attempt < 3; ++attempt ) {
		DWORD bytes = static_cast<DWORD>( mBuffer.size() * sizeof( Uint64 ) );
		auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>( mBuffer.data() );
		const DWORD status = static_cast<DWORD>(
			PdhGetFormattedCounterArrayW( counter, format, &bytes, &count, items ) );
		if ( status == ERROR_SUCCESS )
			return items;
		if ( status != PDH_MORE_DATA || bytes == 0 )
			break;
		mBuffer.resize( ( static_cast<size_t>( bytes ) + sizeof( Uint64 ) - 1 ) /
						sizeof( Uint64 ) );
	}
	count = 0;
	return nullptr;
}

bool PerformanceCountersWindows::processInstance( std::wstring_view name, Int64& pid,
												  std::wstring_view& engine ) {
	if ( name.substr( 0, 4 ) != L"pid_" )
		return false;
	const size_t end = name.find( L'_', 4 );
	Uint64 value = 0;
	if ( end == std::wstring_view::npos || !decimal( name.substr( 4, end - 4 ), value ) ||
		 value == 0 || value > MAXDWORD || name.substr( end, 6 ) != L"_luid_" )
		return false;
	pid = static_cast<Int64>( value );
	engine = name.substr( end + 1 );
	return true;
}

bool PerformanceCountersWindows::coreInstance( std::wstring_view name, unsigned& index ) {
	const size_t comma = name.find( L',' );
	Uint64 group = 0, core = 0;
	if ( comma == std::wstring_view::npos || !decimal( name.substr( 0, comma ), group ) ||
		 !decimal( name.substr( comma + 1 ), core ) || group > USHRT_MAX || core >= 64 )
		return false;
	index = static_cast<unsigned>( group * 64 + core );
	return true;
}

void PerformanceCountersWindows::recordDevice( GpuSample& process, Uint32 adapter ) {
	if ( std::find( process.devices.begin(), process.devices.end(), adapter ) ==
		 process.devices.end() )
		process.devices.push_back( adapter );
}

void PerformanceCountersWindows::readGpuUsage( SystemInfo& system ) {
	DWORD count = 0;
	const auto* items = read( mGpu, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, count );
	for ( DWORD i = 0; i < count; ++i ) {
		const auto& value = items[i].FmtValue;
		Int64 pid = 0;
		std::wstring_view engine;
		if ( !items[i].szName || !validCounter( value.CStatus ) ||
			 !std::isfinite( value.doubleValue ) || value.doubleValue < 0 ||
			 !processInstance( items[i].szName, pid, engine ) )
			continue;
		const int usage = static_cast<int>( std::clamp( value.doubleValue, 0.0, 100.0 ) );
		auto& process = mProcesses[pid];
		// Like Task Manager, show the busiest engine rather than summing independent engines.
		process.usage = std::max( process.usage, usage );
		mEngines[engine] += value.doubleValue;
		const Uint32 adapter = adapterIndex( system, engine );
		recordDevice( process, adapter );
	}
	for ( const auto& [engine, usage] : mEngines ) {
		const int percent = static_cast<int>( std::clamp( usage, 0.0, 100.0 ) );
		system.gpuUsage = std::max( system.gpuUsage, percent );
		auto& gpu = system.gpus[adapterIndex( system, engine )];
		gpu.usage = std::max( gpu.usage, percent );
	}
	// Keys borrow the PDH buffer, which the next counter read will overwrite.
	mEngines.clear();
}

void PerformanceCountersWindows::readGpuMemory( PDH_HCOUNTER counter, bool perProcess,
												SystemInfo& system ) {
	DWORD count = 0;
	const auto* items = read( counter, PDH_FMT_LARGE, count );
	for ( DWORD i = 0; i < count; ++i ) {
		const auto& value = items[i].FmtValue;
		if ( !items[i].szName || !validCounter( value.CStatus ) || value.largeValue < 0 )
			continue;
		const Int64 kib = value.largeValue / 1024;
		if ( perProcess ) {
			Int64 pid = 0;
			std::wstring_view engine;
			if ( processInstance( items[i].szName, pid, engine ) ) {
				auto& process = mProcesses[pid];
				process.memory = std::max<Int64>( process.memory, 0 ) + kib;
				const Uint32 adapter = adapterIndex( system, engine );
				recordDevice( process, adapter );
			}
		} else if ( std::wstring_view( items[i].szName ).substr( 0, 5 ) == L"luid_" ) {
			// Adapter counters count shared allocations once. Summing process memory would
			// count allocations shared by multiple processes more than once.
			system.gpuMemory = std::max<Int64>( system.gpuMemory, 0 ) + kib;
			auto& gpu = system.gpus[adapterIndex( system, items[i].szName )];
			gpu.memory = std::max<Int64>( gpu.memory, 0 ) + kib;
		}
	}
}

void PerformanceCountersWindows::collect( std::vector<ProcessInfo>& processes,
										  SystemInfo& system ) {
	if ( !mInitialized )
		initialize();
	if ( !mQuery || PdhCollectQueryData( mQuery ) != ERROR_SUCCESS )
		return;

	mCores.clear();
	double totalCpuUsage = 0.0;
	bool validCores = true;
	DWORD count = 0;
	const auto* items = read( mCpu, PDH_FMT_DOUBLE, count );
	for ( DWORD i = 0; i < count; ++i ) {
		unsigned index = 0;
		if ( !items[i].szName || !coreInstance( items[i].szName, index ) )
			continue;
		const auto& value = items[i].FmtValue;
		const bool valid = validCounter( value.CStatus ) && std::isfinite( value.doubleValue );
		const float usage =
			valid ? static_cast<float>( std::clamp( value.doubleValue, 0.0, 100.0 ) ) : 0.f;
		validCores = validCores && valid;
		totalCpuUsage += usage;
		mCores.push_back( { index, usage } );
	}
	std::sort( mCores.begin(), mCores.end(),
			   []( const CoreSample& a, const CoreSample& b ) { return a.index < b.index; } );
	if ( !mCores.empty() ) {
		system.coreCpuUsage.reserve( mCores.size() );
		for ( const auto& core : mCores )
			system.coreCpuUsage.push_back( core.usage );
		system.cpuCount = static_cast<int>( mCores.size() );
		// GetSystemTimes covers only the calling processor group on machines with >64 CPUs.
		// The per-core counters include all groups and provide the corresponding overall load.
		if ( validCores )
			system.cpuUsage = static_cast<float>( totalCpuUsage / mCores.size() );
	}

	system.gpus = mAdapters;
	mProcesses.clear();
	readGpuUsage( system );
	readGpuMemory( mProcessDedicated, true, system );
	readGpuMemory( mProcessShared, true, system );
	readGpuMemory( mAdapterDedicated, false, system );
	readGpuMemory( mAdapterShared, false, system );
	for ( auto& process : processes ) {
		auto it = mProcesses.find( process.pid );
		if ( it != mProcesses.end() ) {
			process.gpuDevices = std::move( it->second.devices );
			std::sort( process.gpuDevices.begin(), process.gpuDevices.end() );
			process.gpuUsage = it->second.usage;
			process.gpuMemory = it->second.memory;
		}
	}
}

} // namespace eproc
