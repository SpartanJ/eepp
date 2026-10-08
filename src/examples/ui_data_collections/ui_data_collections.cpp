#include <eepp/ee.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

// ObservableVector is intended for live collections. The list model remains attached while row
// operations are forwarded incrementally, so unaffected selection and persistent indexes survive.
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 480, 360, "eepp - Observable Collection" } );
	auto root = app.getUI()->loadLayoutFromString( R"xml(
	<vbox layout-width="match_parent" layout-height="match_parent" padding="12dp">
		<TextView text="Build queue" font-size="18dp" margin-bottom="8dp" />
		<ListView id="tasks" layout-width="match_parent" layout-height="0dp" layout-weight="1" />
		<hbox margin-top="8dp">
			<TextInput id="task" layout-width="0dp" layout-weight="1" hint="New task" />
			<PushButton id="add" text="Add" margin-left="8dp" />
			<PushButton id="complete" text="Mark complete" margin-left="8dp" />
			<PushButton id="remove" text="Remove" margin-left="8dp" />
		</hbox>
	</vbox>
	)xml" );
	if ( !app.getWindow()->isOpen() )
		return EXIT_FAILURE;

	auto list = root->find<UIListView>( "tasks" );
	auto input = root->find<UITextInput>( "task" );
	ObservableVector<std::string> tasks( { "Build eepp", "Run unit tests", "Package release" } );
	list->setModel( ObservableListModel<std::string>::create( tasks ) );

	root->find( "add" )->onClick( [&]( const MouseEvent* ) {
		auto text = input->getText().toUtf8();
		if ( !text.empty() ) {
			tasks.pushBack( std::move( text ) );
			input->setText( "" )->setFocus();
		}
	} );
	root->find( "complete" )->onClick( [&]( const MouseEvent* ) {
		auto selected = list->getSelection().first();
		if ( selected.isValid() )
			tasks.set( selected.row(), "✅ " + tasks[selected.row()] );
	} );
	root->find( "remove" )->onClick( [&]( const MouseEvent* ) {
		auto selected = list->getSelection().first();
		if ( selected.isValid() )
			tasks.erase( selected.row() );
	} );

	return app.run();
}
