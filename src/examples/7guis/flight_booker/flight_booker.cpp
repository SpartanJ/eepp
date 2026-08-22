#include <eepp/ee.hpp>
#include <iomanip>
#include <sstream>

// Reference https://eugenkiss.github.io/7guis/tasks/#flight
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 440, 240, "eepp - 7GUIs - Flight Booker" } );
	UIWidget* vbox = app.getUI()->loadLayoutFromString( R"xml(
	<style>
	.error_input,
	.error_input:hover,
	.error_input:focus {
		color: var(--theme-error);
		border-color: var(--theme-error);
	}
	</style>
	<vbox layout_width="match_parent" layout_height="match_parent" padding="8dp">
		<DropDownList id="type" layout_width="match_parent" selectedIndex="0" marginBottom="4dp">
			<item>one-way flight</item>
			<item>return flight</item>
		</DropDownList>
		<TextInput id="date_from" layout_width="match_parent" marginBottom="4dp" hint="Date From" />
		<TextInput id="date_to" layout_width="match_parent" marginBottom="4dp" hint="Date To" />
		<PushButton id="book" layout_width="match_parent" text="Book" />
	</vbox>
	)xml" );
	auto flightTypeInput = vbox->find<UIDropDownList>( "type" );
	auto departureInput = vbox->find<UITextInput>( "date_from" );
	auto returnInput = vbox->find<UITextInput>( "date_to" );
	auto bookButton = vbox->find<UIPushButton>( "book" );

	const auto parseDate = []( const std::string& text ) -> std::optional<std::time_t> {
		if ( std::count( text.begin(), text.end(), '.' ) != 2 )
			return {};
		std::tm date = {};
		std::istringstream stream( text );
		stream >> std::get_time( &date, "%d.%m.%Y" );
		return stream.fail() ? std::optional<std::time_t>{}
							 : std::optional<std::time_t>{ std::mktime( &date ) };
	};
	const auto formatDate = []( std::time_t value ) {
		std::stringstream stream;
		stream << std::put_time( std::localtime( &value ), "%d.%m.%Y" );
		return stream.str();
	};
	using Date = std::optional<std::time_t>;
	auto dateConverter = UIValueConverter<Date>(
		[&]( const CSS::PropertyDefinition*, const std::string& text ) -> UIValueResult<Date> {
			auto value = parseDate( text );
			return value ? UIValueResult<Date>( value )
						 : UIValueResult<Date>::error( 1, "date must use DD.MM.YYYY" );
		},
		[&]( const CSS::PropertyDefinition*, const Date& value ) -> UIValueResult<std::string> {
			return value ? formatDate( *value ) : std::string{};
		} );

	std::time_t today = std::time( nullptr );
	UIProperty<std::string> flightType( "one-way flight", flightTypeInput );
	UIProperty<Date> departureDate( today, departureInput, dateConverter );
	UIProperty<Date> returnDate( today, returnInput, dateConverter );
	UIBindingGroup form;
	form += flightType;
	form += departureDate;
	form += returnDate;
	form.onChange( [&] {
		for ( auto widget : form.widgets() )
			widget->removeClass( "error_input" );
		for ( const auto& error : form.errors() )
			if ( error.widget )
				error.widget->addClass( "error_input" );
	} );

	auto isReturnFlight = computedValue(
		flightType, []( const std::string& type ) { return type == "return flight"; } );
	auto returnInputEnabled = bindValue( isReturnFlight, returnInput, "enabled" );

	auto canBook = computedValue(
		form.validValue(), isReturnFlight, departureDate, returnDate,
		[]( bool valid, bool returnFlight, const Date& departure, const Date& returning ) {
			return valid && departure &&
				   ( !returnFlight || ( returning && *returning >= *departure ) );
		} );
	auto bookEnabled = bindValue( canBook, bookButton, "enabled" );
	bookButton->setFocus()->onClick( [&]( const MouseEvent* ) {
		String message( String::format( "You just booked a %s on %s", flightType.get(),
										departureInput->getText().toUtf8() ) );
		UIMessageBox::New( UIMessageBox::OK, message )->showWhenReady();
	} );

	return app.run();
}
