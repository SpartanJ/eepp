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
