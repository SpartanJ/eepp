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
	UITextInput* celsiusInput = hbox->find<UITextInput>( "celsius_input" );
	UITextInput* fahrenheitInput = hbox->find<UITextInput>( "fahrenheit_input" );
	const auto f2c = []( double f ) { return ( f - 32 ) * 5 / 9; };
	const auto c2f = []( double c ) { return c * 9 / 5 + 32; };
	bool converting = false;
	const auto convert = [&]( bool fromCelsius ) {
		if ( converting ) // Only process input value change and skip value change from setText
			return;
		BoolScopedOp op( converting, true );
		auto sourceInput = fromCelsius ? celsiusInput : fahrenheitInput;
		double val;
		if ( !String::fromString( val, sourceInput->getText() ) )
			return;
		auto str( String::fromDouble( fromCelsius ? c2f( val ) : f2c( val ) ) );
		if ( fromCelsius )
			fahrenheitInput->setText( str );
		else
			celsiusInput->setText( str );
	};
	celsiusInput->setFocus()->on( Event::OnValueChange, [&convert]( auto ) { convert( true ); } );
	fahrenheitInput->on( Event::OnValueChange, [&convert]( auto ) { convert( false ); } );
	return app.run();
}
