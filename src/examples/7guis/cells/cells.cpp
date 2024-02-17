#include "spreadsheet.hpp"
#include <eepp/ee.hpp>

// Referece https://eugenkiss.github.io/7guis/tasks/#cells
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1024, 768, "eepp - 7GUIs - Cells" } );
	UIWidget* rlay = app.getUI()->loadLayoutFromString( R"xml(
	<style>
		.font_theme_normal { color: var(--font); }
		.font_theme_success { color: var(--theme-success); }
		.font_theme_warning { color: var(--theme-warning); }
		.font_theme_error { color: var(--theme-error); }
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
	table->setSelection( model->index( 0, 0 ) );
	table->setFocus();
	table->onCreateEditingDelegate = [table,
									  model]( const ModelIndex& index ) -> ModelEditingDelegate* {
		auto ret = StringModelEditingDelegate::New();
		ret->setPullDataFrom( ModelRole::Custom );
		ret->onValueSet = [table, model, index] {
			if ( index.row() + 1 < static_cast<Int64>( model->rowCount() ) )
				table->setSelection( model->index( index.row() + 1, index.column() ) );
		};
		return ret;
	};
	return app.run();
}
