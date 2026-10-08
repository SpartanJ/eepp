#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int argc, char** argv ) {
	UIApplication app( { 1020, 720, "eepp - Date and Time Pickers" } );
	if ( !app.getWindow() || !app.getWindow()->isOpen() )
		return EXIT_FAILURE;
	bool usLocale = false;
	bool narrow = false;
	std::string fontSize;
	auto* ui = app.getUI();
	for ( int i = 1; i < argc; ++i ) {
		const std::string_view option( argv[i] );
		if ( option == "--us" )
			usLocale = true;
		else if ( option == "--light" )
			ui->setColorSchemePreference( ColorSchemePreference::Light );
		else if ( option == "--large-font" )
			fontSize = "24dp";
		else if ( option == "--small-font" )
			fontSize = "10dp";
		else if ( option == "--narrow" )
			narrow = true;
	}
	ui->loadLayoutFromString( R"xml(
		<vbox id="form" layout_width="match_parent" layout_height="match_parent" padding="16dp">
			<style>
				#form { background-color: var(--back); }
				#form > textview { margin-top: 8dp; margin-bottom: 4dp; }
				datepicker, timepicker, datetimepicker { layout-width: 320dp; layout-height: wrap_content; }
			</style>
			<textview text="Date (type digits, paste ISO, or open the calendar)" />
			<datepicker id="date" value="2026-09-28" />
			<textview text="Empty date" />
			<datepicker id="empty" />
			<textview text="Time, in fifteen minute steps" />
			<timepicker id="time" value="20:30:00" minute-step="15" hour-cycle="24" />
			<textview text="Local date and time (calendar selection preserves time)" />
			<datetimepicker id="datetime" value="2026-09-28T20:14:00" />
			<textview text="Constrained date" />
			<datepicker id="bounded" min-date="2026-09-10" max-date="2026-10-20" value="2026-09-28" />
			<textview text="Read-only and disabled" />
			<hbox layout_width="match_parent" layout_height="wrap_content">
				<datepicker id="readonly" value="2026-09-28" allow-editing="false" />
				<datepicker id="disabled" value="2026-09-28" enabled="false" margin-left="8dp" />
			</hbox>
			<textview id="value" text="Select or edit a value" />
		</vbox>
	)xml" );
	if ( !fontSize.empty() )
		ui->combineStyleSheet( "* { font-size: " + fontSize + "; }" );
	if ( narrow )
		ui->combineStyleSheet( "datepicker, timepicker, datetimepicker { layout-width: 160dp; }" );
	EventConnectionList connections;
	const auto locale = DateTimeLocale::fromLocaleName( usLocale ? "en_US" : "es_AR" );
	for ( const char* tag : { "datepicker", "timepicker", "datetimepicker" } ) {
		for ( auto* widget : ui->getRoot()->querySelectorAll( tag ) ) {
			auto* edit = widget->asType<UIDateTimeEdit>();
			edit->setLocale( locale );
			connections += edit->connect( Event::OnValueChange, [ui, edit]( const Event* ) {
				ui->find<UITextView>( "value" )->setText( edit->getSerializedValue() );
			} );
		}
	}
	connections += ui->connect( Event::KeyUp, [ui]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 )
			UIWidgetInspector::create( ui );
	} );
	return app.run();
}
