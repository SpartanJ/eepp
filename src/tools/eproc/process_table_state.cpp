#include "process_table_state.hpp"
#include <iterator>

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
	if ( state["version"] == 6 ) {
		// GPU Device was appended to preserve the indexes of all saved columns.
		auto migrate = []( nlohmann::json& view ) {
			if ( !view.is_object() )
				return;
			if ( view.contains( "widths" ) && view["widths"].is_object() &&
				 view["widths"].contains( "widths" ) && view["widths"]["widths"].is_array() &&
				 view["widths"]["widths"].size() == ProcessModel::ColCount - 1 ) {
				// Percentage widths are normalized by the view; reserve a modest share.
				const bool percentage =
					view["widths"].contains( "mode" ) && view["widths"]["mode"] == "percentage";
				view["widths"]["widths"].push_back( percentage ? 5 : 100 );
			}
			if ( view.contains( "column_order" ) && view["column_order"].is_array() &&
				 view["column_order"].size() == ProcessModel::ColCount - 1 ) {
				auto& order = view["column_order"];
				auto afterMemory = std::find( order.begin(), order.end(),
											  nlohmann::json( ProcessModel::ColGpuMemory ) );
				order.insert( afterMemory != order.end() ? std::next( afterMemory ) : order.end(),
							  ProcessModel::ColGpuDevice );
			}
		};
		migrate( state );
		if ( state.contains( "tree" ) )
			migrate( state["tree"] );
		state["version"] = kProcessTableStateVersion;
	}
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
