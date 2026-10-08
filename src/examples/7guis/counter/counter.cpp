#include <eepp/ee.hpp>

// Reference https://eugenkiss.github.io/7guis/tasks#counter
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 380, 64, "eepp - 7GUIs - Counter" } );
	UIWidget* hbox = app.getUI()->loadLayoutFromString( R"xml(
	<hbox layout_width="match_parent" layout_height="match_parent" padding="8dp">
		<TextView id="count_view" layout_width="0" layout_weight="0.5" layout_height="match_parent" text="0" />
		<PushButton id="count_click" layout_width="0" layout_weight="0.5" layout_height="match_parent" text="Count" />
	</hbox>
	)xml" );
	UIProperty<int> count( 0, hbox->find<UITextView>( "count_view" ) );
	hbox->find( "count_click" )->onClick( [&count]( auto ) { ++count; } );
	return app.run();
}
