#include <eepp/ee.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 720, 640, "eepp - Accessibility" } );
	auto scene = app.getUI();
	auto content = scene->loadLayoutFromString( R"xml(
		<vbox lw="mp" lh="mp" padding="16dp" gap="8dp">
			<TextView text="Accessibility settings" font-size="22dp" />
			<TextInput id="project-name" accessibility-label="Project name" text="eepp" lw="mp" />
			<TextEdit id="description" accessibility-label="Description" text="Accessible UI" lw="mp" lh="80dp" />
			<CheckBox id="autosave" text="Enable autosave" />
			<RadioButton id="light-theme" text="Light theme" />
			<RadioButton id="dark-theme" text="Dark theme" />
			<ComboBox id="language" accessibility-label="Language" lw="mp" />
			<Slider id="volume" accessibility-label="Volume" value="40" min-value="0" max-value="100" lw="mp" />
			<SpinBox id="retries" accessibility-label="Retry count" value="3" min-value="0" max-value="10" />
			<ProgressBar id="progress" accessibility-label="Operation progress" progress="35" lw="mp" />
			<TabWidget id="sections" lw="mp" lh="120dp">
				<vbox id="general-panel" lw="mp" lh="mp">
					<TextView text="General settings panel" />
				</vbox>
				<vbox id="advanced-panel" lw="mp" lh="mp">
					<TextView text="Advanced settings panel" />
				</vbox>
				<Tab text="General" owns="general-panel" />
				<Tab text="Advanced" owns="advanced-panel" />
			</TabWidget>
			<hbox lw="mp" lh="wc" gap="8dp">
				<PushButton id="save" text="Save settings" />
				<PushButton id="inspect" text="Open inspector" />
			</hbox>
			<TextView id="status" text="Ready" />
		</vbox>
	)xml" );

	auto comboBox = content->find<UIComboBox>( "language" );
	comboBox->getListBox()->addListBoxItems( { "English", "Spanish", "German" } );
	comboBox->setText( "English" );

	auto status = content->find<UITextView>( "status" );
	content->find<UIPushButton>( "save" )->onClick(
		[status]( auto ) { status->setText( "Settings saved" ); } );
	content->find<UIPushButton>( "inspect" )->onClick(
		[scene]( auto ) { EE::UI::Tools::UIWidgetInspector::create( scene ); } );

	return app.run();
}
