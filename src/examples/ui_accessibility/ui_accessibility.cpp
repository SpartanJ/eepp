#include <eepp/ee.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 1280, 720, "eepp - Accessibility" } );
	auto scene = app.getUI();
	auto content = scene->loadLayoutFromString( R"xml(
		<vbox lw="mp" lh="mp" padding="16dp" padding="8dp">
			<TextView text="Accessibility settings" font-size="22dp" />
			<TextInput id="project-name" aria-label="Project name" text="eepp" lw="mp" />
			<TextEdit id="description" aria-label="Description" aria-description="Multiline editor. Press Mod Tab to move to the next control." text="Accessible UI" lw="mp" lh="80dp" />
			<CheckBox id="autosave" aria-label="Enable autosave" text="Enable autosave" />
			<RadioButton id="light-theme" aria-label="Light Theme" text="Light theme" />
			<RadioButton id="dark-theme" aria-label="Dark Theme" text="Dark theme" />
			<ComboBox id="language" aria-label="Language" lw="mp" />
			<Slider id="volume" aria-label="Volume" value="40" min-value="0" max-value="100" lw="mp" orientation="horizontal" />
			<SpinBox id="retries" aria-label="Retry count" value="3" min-value="0" max-value="10" lw="mp" />
			<ProgressBar id="progress" aria-label="Operation progress" progress="35" lw="mp" />
			<TabWidget id="sections" lw="mp" lh="40dp">
				<vbox id="general-panel" lw="mp" lh="mp">
					<TextView text="General settings panel" />
				</vbox>
				<vbox id="advanced-panel" lw="mp" lh="mp">
					<TextView text="Advanced settings panel" />
				</vbox>
				<Tab text="General" owns="general-panel" />
				<Tab text="Advanced" owns="advanced-panel" />
			</TabWidget>
			<hbox lw="mp" lh="40dp" margin-top="8dp">
				<ListView id="items" aria-label="Colors" lw="50%" lw="0" lw8="0.5" lh="80dp" />
				<TableView id="projects" aria-label="Projects" lw="0" lw8="0.5" lh="80dp" />
			</hbox>
			<hbox lw="mp" lh="wc" margin-top="8dp">
				<PushButton id="save" text="Save settings" />
				<PushButton id="inspect" text="Open inspector" />
			</hbox>
			<TextView id="status" text="Ready" />
		</vbox>
	)xml" );
	content->setAccessibilityRole( AccessibilityRole::Window );
	content->setAccessibilityLabel( "Accessibility settings" );

	auto comboBox = content->find<UIComboBox>( "language" );
	comboBox->getListBox()->addListBoxItems( { "English", "Spanish", "German" } );
	comboBox->setText( "English" );
	auto list = content->find<UIListView>( "items" );
	list->setModel( Models::ItemListOwnerModel<std::string>::create( { "Red", "Green", "Blue" } ) );
	auto table = content->find<UITableView>( "projects" );
	auto tableModel = Models::ItemPairListOwnerModel<std::string, std::string>::create(
		{ { "eepp", "Active" }, { "ecode", "Ready" } } );
	tableModel->setColumnName( 0, "Project" );
	tableModel->setColumnName( 1, "Status" );
	table->setModel( tableModel );

	auto status = content->find<UITextView>( "status" );
	content->find<UIPushButton>( "save" )->onClick(
		[status]( auto ) { status->setText( "Settings saved" ); } );
	content->find<UIPushButton>( "inspect" )->onClick( [scene]( auto ) {
		EE::UI::Tools::UIWidgetInspector::create( scene );
	} );
	content->find( "project-name" )->setFocus();
	return app.run();
}
