#include <eepp/ee.hpp>

// A settings form is a useful fit for the data-handling helpers: several independently validated
// fields share dirty/reset/save behavior, and Save is reachable from both a button and a shortcut.
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 520, 410, "eepp - UI Data Handling" } );
	auto root = app.getUI()->loadLayoutFromString( R"xml(
	<style>
		.field-error,
		.field-error:hover,
		.field-error:focus {
			color: var(--theme-error);
			border-color: var(--theme-error);
		}
		.validation-message { color: var(--theme-error); }
	</style>
	<vbox layout_width="match_parent" layout_height="match_parent" padding="12dp">
		<TextView text="Deployment settings" font-size="18dp" margin-bottom="12dp" />
		<hbox margin-bottom="8dp">
			<TextView text="Project" min-width="90dp" layout-gravity="center_vertical" />
			<TextInput id="project" layout-width="0dp" layout-weight="1" />
		</hbox>
		<hbox margin-bottom="8dp">
			<TextView text="Host" min-width="90dp" layout-gravity="center_vertical" />
			<TextInput id="host" layout-width="0dp" layout-weight="1" />
		</hbox>
		<hbox margin-bottom="12dp">
			<TextView text="Port" min-width="90dp" layout-gravity="center_vertical" />
			<TextInput id="port" layout-width="0dp" layout-weight="1" />
		</hbox>
		<CheckBox id="automatic" text="Deploy automatically" margin-bottom="12dp" />
		<TextView id="validation-message" class="validation-message" min-height="20dp"
			margin-bottom="8dp" />
		<TextView id="summary" margin-bottom="12dp" />
		<hbox>
			<PushButton id="save" text="Save (Ctrl+S)" margin-right="8dp" />
			<PushButton id="reset" text="Reset" />
			<TextView id="status" margin-left="12dp" layout-gravity="center_vertical" />
		</hbox>
	</vbox>
	)xml" );
	if ( !app.getWindow()->isOpen() )
		return EXIT_FAILURE;

	auto projectInput = root->find<UITextInput>( "project" );
	auto hostInput = root->find<UITextInput>( "host" );
	auto portInput = root->find<UITextInput>( "port" );
	auto automaticInput = root->find<UICheckBox>( "automatic" );
	auto validationMessage = root->find<UITextView>( "validation-message" );
	auto saveButton = root->find<UIPushButton>( "save" );
	auto resetButton = root->find<UIPushButton>( "reset" );
	auto status = root->find<UITextView>( "status" );

	ObservableValue<std::string> project( "eepp" );
	ObservableValue<std::string> host( "localhost" );
	ObservableValue<int> port( 8080 );
	ObservableValue<bool> automatic( false );
	auto requiredText = UIValueConverter<std::string>(
		[]( const CSS::PropertyDefinition*,
			const std::string& value ) -> UIValueResult<std::string> {
			return value.empty() ? UIValueResult<std::string>::error( 100, "value is required" )
								 : UIValueResult<std::string>( value );
		} );
	auto validPort = UIValueConverter<int>(
		[]( const CSS::PropertyDefinition*, const std::string& value ) -> UIValueResult<int> {
			int parsed = 0;
			if ( !String::fromString( parsed, value ) || parsed < 1 || parsed > 65535 )
				return UIValueResult<int>::error( 101, "port must be between 1 and 65535" );
			return parsed;
		} );

	UIBindingGroup form;
	form += bindValue( project, projectInput, requiredText );
	form += bindValue( host, hostInput, requiredText );
	form += bindValue( port, portInput, validPort );
	form += bindValue( automatic, automaticInput );
	form.onChange( [&] {
		for ( auto widget : form.widgets() )
			widget->removeClass( "field-error" );
		std::string message;
		for ( const auto& error : form.errors() ) {
			if ( error.widget )
				error.widget->addClass( "field-error" );
			if ( !message.empty() )
				message += '\n';
			if ( error.validation && error.validation->code == 100 )
				message +=
					error.widget == projectInput ? "Project is required." : "Host is required.";
			else if ( error.validation && error.validation->code == 101 )
				message += "Port must be between 1 and 65535.";
			else
				message += "Invalid value.";
		}
		validationMessage->setText( message );
	} );

	auto summary = computedValue(
		project, host, port, automatic,
		[]( const std::string& project, const std::string& host, int port, bool automatic ) {
			return project + " deploys to " + host + ":" + String::toString( port ) +
				   ( automatic ? " automatically" : " manually" );
		} );
	auto summaryBinding = bindValue( summary, root->find<UIWidget>( "summary" ) );
	auto canSave = computedValue( form.validValue(), form.dirtyValue(),
								  []( bool valid, bool dirty ) { return valid && dirty; } );
	auto saveCommand = bindCommand(
		[&] {
			form.markClean();
			status->setText( "Saved" );
		},
		canSave, *saveButton, *app.getUI(), { KEY_S, KeyMod::getDefaultModifier() } );
	resetButton->onClick( [&]( const MouseEvent* ) {
		form.reset();
		status->setText( "Reset to last save" );
	} );

	return app.run();
}
