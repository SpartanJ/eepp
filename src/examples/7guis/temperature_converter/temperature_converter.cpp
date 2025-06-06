#include <eepp/ee.hpp>

// Reference https://eugenkiss.github.io/7guis/tasks#temp
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 490, 64, "eepp - 7GUIs - Temperature Converter" } );
	UIWidget* hbox = app.getUI()->loadLayoutFromString( R"xml(
	<hbox layout_width="match_parent" layout_height="wrap_content" padding="8dp" gravity="center">
		<TextInput id="celsius_input" layout_width="96dp" layout_height="wrap_content" numeric="true" allow-float="true" />
		<TextView text="Celsius = " layout_height="match_parent" padding="0dp 4dp 0dp 4dp" enabled="false" />
		<TextInput id="fahrenheit_input" layout_width="96dp" layout_height="wrap_content" numeric="true" allow-float="true" />
		<TextView text="Fahrenheit" layout_height="match_parent" padding="0dp 4dp 0dp 4dp" enabled="false" />
	</hbox>
	)xml" );
	UIProperty<double> celsius( 0, hbox->find( "celsius_input" )->setFocus()->asType<UIWidget>() );
	UIProperty<double> fahrenheit( 32, hbox->find<UITextInput>( "fahrenheit_input" ) );
	celsius.changed( [&fahrenheit]( auto c ) { fahrenheit = c * 9 / 5 + 32; } );
	fahrenheit.changed( [&celsius]( auto f ) { celsius = ( f - 32 ) * 5 / 9; } );
	return app.run();
}
