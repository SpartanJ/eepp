#include "../../tools/eproc/process_table_state.hpp"
#include "utest.hpp"

using namespace eproc;
using namespace eproc::ProcessTableState;

namespace {

nlohmann::json tableState() {
	nlohmann::json state;
	state["version"] = kProcessTableStateVersion;
	state["widths"]["mode"] = "pixels";
	state["widths"]["widths"] = std::vector<int>( ProcessModel::ColCount, 100 );
	state["hidden_columns"] = kOptionalProcessColumns;
	return state;
}

void showColumn( nlohmann::json& state, size_t column ) {
	auto& hidden = state["hidden_columns"];
	hidden.erase( std::remove( hidden.begin(), hidden.end(), nlohmann::json( column ) ),
				  hidden.end() );
}

} // namespace

UTEST( EProcProcessTableState, DefaultAndInvalidStatesDoNotRequestPss ) {
	for ( const auto& saved : { "", "{", "null", "[]", "{}", "{\"version\":999}",
								"{\"version\":18446744073709551615}" } ) {
		EXPECT_FALSE( showsFamilyMemory( parse( saved ), false ) );
		EXPECT_FALSE( showsFamilyMemory( parse( saved ), true ) );
	}
	auto state = tableState();
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
	state.erase( "hidden_columns" );
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
	state["hidden_columns"] = "invalid";
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
}

UTEST( EProcProcessTableState, EitherFamilyColumnRequestsPss ) {
	auto state = tableState();
	showColumn( state, ProcessModel::ColFamilyMemory );
	EXPECT_TRUE( showsFamilyMemory( parse( state.dump() ), false ) );
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), true ) );
	state = tableState();
	showColumn( state, ProcessModel::ColFamilyMemoryPercent );
	EXPECT_TRUE( showsFamilyMemory( parse( state.dump() ), false ) );
	showColumn( state, ProcessModel::ColFamilyMemory );
	EXPECT_TRUE( showsFamilyMemory( parse( state.dump() ), false ) );
	state["hidden_columns"] =
		nlohmann::json::array( { -1, 999, "invalid", 18446744073709551615ull } );
	EXPECT_TRUE( showsFamilyMemory( parse( state.dump() ), false ) );
}

UTEST( EProcProcessTableState, UsesOnlyTheActiveViewsColumns ) {
	auto state = tableState();
	state["tree"] = tableState();
	showColumn( state["tree"], ProcessModel::ColFamilyMemoryPercent );
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
	EXPECT_TRUE( showsFamilyMemory( parse( state.dump() ), true ) );
	state["tree"] = tableState();
	showColumn( state, ProcessModel::ColFamilyMemory );
	EXPECT_TRUE( showsFamilyMemory( parse( state.dump() ), false ) );
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), true ) );
}

UTEST( EProcProcessTableState, RejectedWidthsKeepDefaultVisibility ) {
	auto state = tableState();
	showColumn( state, ProcessModel::ColFamilyMemory );
	state["widths"]["widths"][ProcessModel::ColName] = 0;
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
	state["widths"]["mode"] = "percentage";
	EXPECT_TRUE( showsFamilyMemory( parse( state.dump() ), false ) );
	state["widths"]["widths"][0] = -1;
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
	state["widths"]["widths"] = nlohmann::json::array();
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
	state.erase( "widths" );
	EXPECT_FALSE( showsFamilyMemory( parse( state.dump() ), false ) );
}

#if EE_PLATFORM == EE_PLATFORM_MACOS
UTEST( EProcProcessTableState, MacOSExposesNetworkButHidesUnavailableGpuColumns ) {
	EXPECT_TRUE( isProcessColumnSupported( ProcessModel::ColDownload ) );
	EXPECT_TRUE( isProcessColumnSupported( ProcessModel::ColUpload ) );
	EXPECT_FALSE( isProcessColumnSupported( ProcessModel::ColGpuUsage ) );
	EXPECT_FALSE( isProcessColumnSupported( ProcessModel::ColGpuMemory ) );
}
#endif

UTEST( EProcProcessTableState, MigratesGpuDeviceWithoutDiscardingSavedViews ) {
	auto state = tableState();
	state["version"] = 6;
	state["widths"]["widths"].erase( state["widths"]["widths"].end() - 1 );
	std::vector<size_t> order;
	for ( size_t i = 0; i < ProcessModel::ColCount - 1; ++i )
		order.push_back( i );
	state["column_order"] = order;
	state["widths"]["widths"][ProcessModel::ColName] = 321;
	showColumn( state, ProcessModel::ColFamilyMemory );
	state["tree"] = state;
	state["tree"]["widths"]["mode"] = "percentage";
	const auto migrated = parse( state.dump() );
	ASSERT_TRUE( migrated.is_object() );
	EXPECT_EQ( migrated["version"].get<int>(), kProcessTableStateVersion );
	for ( const auto* view : { &migrated, &migrated["tree"] } ) {
		EXPECT_EQ( ( *view )["widths"]["widths"].size(), ProcessModel::ColCount );
		EXPECT_EQ( ( *view )["widths"]["widths"][ProcessModel::ColName].get<int>(), 321 );
		const auto& columns = ( *view )["column_order"];
		EXPECT_EQ( columns.size(), ProcessModel::ColCount );
		EXPECT_EQ( columns[ProcessModel::ColGpuMemory + 1].get<size_t>(),
				   ProcessModel::ColGpuDevice );
	}
	EXPECT_EQ( migrated["widths"]["widths"][ProcessModel::ColGpuDevice].get<int>(), 100 );
	EXPECT_EQ( migrated["tree"]["widths"]["widths"][ProcessModel::ColGpuDevice].get<int>(), 5 );
	EXPECT_TRUE( showsFamilyMemory( migrated, false ) );
	EXPECT_TRUE( showsFamilyMemory( migrated, true ) );
}
