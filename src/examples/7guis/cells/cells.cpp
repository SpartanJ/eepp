#include "spreadsheet.hpp"
#include <eepp/ee.hpp>

// Referece https://eugenkiss.github.io/7guis/tasks/#cells
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1024, 768, "eepp - 7GUIs - Cells" } );
	UIWidget* rlay = app.getUI()->loadLayoutFromString( R"xml(
	<style>
		#sheet tableview::cell {
			border-type: inside;
			border-right: 1dprd solid var(--button-border);
			border-bottom: 1dprd solid var(--button-border);
		}
	</style>
	<RelativeLayout layout_width="match_parent" layout_height="match_parent">
		<TableView id="sheet" layout_width="match_parent" layout_height="match_parent" />
	</RelativeLayout>
	)xml" );
	auto table = rlay->find<UITableView>( "sheet" );
	auto model = std::make_shared<Spreadsheet>();
	table->setModel( model );
	table->setColumnsWidth( PixelDensity::dpToPx( 80 ) );
	table->setRowHeaderWidth( PixelDensity::dpToPx( 32 ) );
	table->setSelectionType( UITableView::SelectionType::Cell );
	table->setEditable( true );
	table->setEditTriggers( UIAbstractView::EditTrigger::DoubleClicked |
							UIAbstractTableView::EditTrigger::EditKeyPressed );
	table->setEditShortcuts( { { KEY_F2 }, { KEY_RETURN }, { KEY_KP_ENTER } } );
	table->onCreateEditingDelegate = [model]( const ModelIndex& ) -> ModelEditingDelegate* {
		auto ret = StringModelEditingDelegate::New();
		ret->setPullDataFrom( ModelRole::Custom );
		return ret;
	};
	return app.run();
}
