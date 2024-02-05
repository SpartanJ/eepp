#include <eepp/ee.hpp>
#include <iomanip>

// Reference https://eugenkiss.github.io/7guis/tasks#flight
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 440, 240, "eepp - 7GUIs - Flight Booker" } );
	UIWidget* hbox = app.getUI()->loadLayoutFromString( R"xml(
	<style>
	.error_input {
		color: var(--theme-error);
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
	auto ddlType = hbox->find<UIDropDownList>( "type" );
	auto dateFrom = hbox->find<UITextInput>( "date_from" );
	auto dateTo = hbox->find<UITextInput>( "date_to" );
	auto bookBut = hbox->find<UIPushButton>( "book" );
	const auto covertDate = []( const String& dateStr ) -> std::optional<std::time_t> {
		if ( std::count( dateStr.begin(), dateStr.end(), '.' ) != 2 )
			return {};
		std::tm time = {};
		std::istringstream ss( dateStr );
		ss >> std::get_time( &time, "%d.%m.%Y" );
		if ( ss.fail() )
			return {};
		return std::mktime( &time );
	};
	const auto getCurrentDate = []() {
		std::time_t now = std::time( nullptr );
		std::tm* ltm = std::localtime( &now );
		std::stringstream ss;
		ss << std::put_time( ltm, "%d.%m.%Y" );
		return ss.str();
	};
	const auto updateDateInput = [&]( UITextInput* input,
									  const std::optional<std::time_t>& date ) -> bool {
		if ( !input->isEnabled() || date ) {
			input->removeClass( "error_input" );
			return true;
		} else
			input->addClass( "error_input" );
		return false;
	};
	const auto update = [&]() {
		std::optional<std::time_t> fromDate = covertDate( dateFrom->getText() );
		std::optional<std::time_t> toDate = covertDate( dateTo->getText() );
		dateTo->setEnabled( ddlType->getListBox()->getItemSelectedIndex() != 0 );
		bookBut->setEnabled( updateDateInput( dateFrom, fromDate ) &&
							 updateDateInput( dateTo, toDate ) &&
							 ( ddlType->getListBox()->getItemSelectedIndex() == 0 ||
							   ( fromDate && toDate && *fromDate < *toDate ) ) );
	};
	ddlType->on( Event::OnItemSelected, [&update]( auto ) { update(); } );
	dateFrom->setText( getCurrentDate() )->on( Event::OnValueChange, [&update]( auto ) {
		update();
	} );
	dateTo->on( Event::OnValueChange, [&update]( auto ) { update(); } );
	bookBut->setFocus()->onClick( [&]( auto ) {
		String msg( String::format( "You just booked a %s on %s",
									ddlType->getListBox()->getItemSelectedText().toUtf8(),
									dateFrom->getText().toUtf8() ) );
		UIMessageBox::New( UIMessageBox::OK, msg )->showWhenReady();
	} );
	update();
	return app.run();
}
