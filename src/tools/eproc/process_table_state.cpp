#include "process_table_state.hpp"

namespace eproc::ProcessTableState {

namespace {

bool defaultColumnHidden( size_t column ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( column == ProcessModel::ColTotalMemory )
		return false;
#endif
	return !isProcessColumnSupported( column ) ||
		   std::find( kOptionalProcessColumns.begin(), kOptionalProcessColumns.end(), column ) !=
			   kOptionalProcessColumns.end();
}

} // namespace

bool isProcessColumnSupported( size_t column ) {
	return std::find( kUnavailableProcessColumns.begin(), kUnavailableProcessColumns.end(),
					  column ) == kUnavailableProcessColumns.end();
}

nlohmann::json parse( const std::string& savedState ) {
	if ( savedState.empty() )
		return nullptr;
	auto state = nlohmann::json::parse( savedState, nullptr, false, true );
	if ( !state.is_object() || !state.contains( "version" ) ||
		 !state["version"].is_number_integer() )
		return nullptr;
	if ( state["version"] != kProcessTableStateVersion )
		return nullptr;
	return state;
}

bool showsFamilyMemory( const nlohmann::json& state, bool treeMode ) {
	// Restoration rejects the whole state when root widths are invalid for the default table.
	if ( !state.is_object() || !state.contains( "widths" ) ||
		 !isValidWidths( state["widths"], ProcessModel::ColCount, defaultColumnHidden ) )
		return false;
	// The tree has independent defaults when no valid tree state was saved.
	if ( treeMode && ( !state.contains( "tree" ) || !state["tree"].is_object() ) )
		return false;
	const auto& columns = treeMode ? state["tree"] : state;
	if ( !columns.contains( "hidden_columns" ) || !columns["hidden_columns"].is_array() )
		return false;
	bool memoryHidden = false;
	bool percentHidden = false;
	for ( const auto& column : columns["hidden_columns"] ) {
		if ( !column.is_number_integer() )
			continue;
		memoryHidden |= column == ProcessModel::ColFamilyMemory;
		percentHidden |= column == ProcessModel::ColFamilyMemoryPercent;
	}
	return !memoryHidden || !percentHidden;
}

} // namespace eproc::ProcessTableState
