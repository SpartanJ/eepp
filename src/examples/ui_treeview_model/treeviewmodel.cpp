#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 800, 600, "eepp - StringMapModel Example" } );

	std::map<std::string, std::vector<std::string>> data = {
		{ "Category 1", { "Item 1.1", "Item 1.2", "Item 1.3" } },
		{ "Category 2", { "Item 2.1", "Item 2.2", "Something else" } },
		{ "Fruits", { "Apple", "Banana", "Orange", "Grape" } },
		{ "Programming Languages", { "C++", "Lua", "Python", "Rust" } } };

	auto model = StringMapModel<std::string>::create( data );

	UIWidget* vBox = app.getUI()->loadLayoutFromString( R"xml(
		<vbox layout_width="match_parent" layout_height="match_parent">
			<TextInput id="filter_input" layout_width="match_parent" layout_height="wrap_content" hint="Filter: " />
			<TreeView id="tree_view" layout_width="match_parent" layout_height="0dp" layout_weight="1" />
		</vbox>
	)xml" );

	auto treeView = vBox->find<UITreeView>( "tree_view" );
	auto filterInput = vBox->find<UITextInput>( "filter_input" );

	treeView->setHeadersVisible( false );
	treeView->setAutoExpandOnSingleColumn( true );
	treeView->setModel( model );
	treeView->expandAll();

	filterInput->on( Event::OnTextChanged, [model, filterInput]( const Event* ) {
		model->filter( filterInput->getText().toUtf8() );
	} );
	filterInput->setFocus();

	return app.run();
}
