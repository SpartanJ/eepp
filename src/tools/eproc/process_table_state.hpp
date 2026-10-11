#ifndef EPROC_PROCESS_TABLE_STATE_HPP
#define EPROC_PROCESS_TABLE_STATE_HPP

#include "process_model.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <nlohmann/json.hpp>

namespace eproc::ProcessTableState {

inline constexpr int kProcessTableStateVersion = 6;

constexpr std::array<size_t, 12> kOptionalProcessColumns = { {
	ProcessModel::ColTotalMemory,
	ProcessModel::ColVirtualSize,
	ProcessModel::ColCpuTime,
	ProcessModel::ColNiceness,
	ProcessModel::ColRelativeStartTime,
	ProcessModel::ColTty,
	ProcessModel::ColIoRead,
	ProcessModel::ColIoWrite,
	ProcessModel::ColThreads,
	ProcessModel::ColMemoryPercent,
	ProcessModel::ColFamilyMemory,
	ProcessModel::ColFamilyMemoryPercent,
} };

#if EE_PLATFORM == EE_PLATFORM_WIN
constexpr std::array<size_t, 8> kUnavailableProcessColumns = { {
	ProcessModel::ColSharedMem,
	ProcessModel::ColGpuUsage,
	ProcessModel::ColGpuMemory,
	ProcessModel::ColDownload,
	ProcessModel::ColUpload,
	ProcessModel::ColVirtualSize,
	ProcessModel::ColNiceness,
	ProcessModel::ColTty,
} };
#elif EE_PLATFORM == EE_PLATFORM_MACOS
constexpr std::array<size_t, 6> kUnavailableProcessColumns = { {
	ProcessModel::ColSharedMem,
	ProcessModel::ColGpuUsage,
	ProcessModel::ColGpuMemory,
	ProcessModel::ColDownload,
	ProcessModel::ColUpload,
	ProcessModel::ColTty,
} };
#elif EE_PLATFORM == EE_PLATFORM_BSD
constexpr std::array<size_t, 8> kUnavailableProcessColumns = { {
	ProcessModel::ColSharedMem,
	ProcessModel::ColGpuUsage,
	ProcessModel::ColGpuMemory,
	ProcessModel::ColDownload,
	ProcessModel::ColUpload,
	ProcessModel::ColTty,
	ProcessModel::ColIoRead,
	ProcessModel::ColIoWrite,
} };
#else
constexpr std::array<size_t, 0> kUnavailableProcessColumns{};
#endif

bool isProcessColumnSupported( size_t column );

/** Parses the current saved state format. Invalid/unsupported state returns null. */
nlohmann::json parse( const std::string& savedState );

/** Predicts the restored active view's Family Memory visibility without constructing widgets. */
bool showsFamilyMemory( const nlohmann::json& state, bool treeMode );

/** Shared width validation; visibility comes from either defaults or the actual view. */
template <typename IsHidden>
bool isValidWidths( const nlohmann::json& widths, size_t columnCount, IsHidden isHidden ) {
	if ( !widths.is_object() || !widths.contains( "mode" ) || !widths["mode"].is_string() ||
		 !widths.contains( "widths" ) || !widths["widths"].is_array() ||
		 widths["widths"].size() != columnCount )
		return false;

	const auto& mode = widths["mode"].get_ref<const std::string&>();
	if ( mode != "pixels" && mode != "percentage" )
		return false;

	bool hasVisibleWidth = false;
	for ( size_t column = 0; column < columnCount; ++column ) {
		const auto& width = widths["widths"][column];
		const double value = width.is_number() ? width.get<double>() : 0;
		if ( !width.is_number() || !std::isfinite( value ) || value < 0 )
			return false;
		if ( mode == "pixels" && !isHidden( column ) && value <= 1.0 )
			return false;
		if ( !isHidden( column ) && value > 0 )
			hasVisibleWidth = true;
	}

	return hasVisibleWidth;
}

} // namespace eproc::ProcessTableState

#endif
