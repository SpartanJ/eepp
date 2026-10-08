#include <eepp/ee.hpp>

struct Person {
	std::uint64_t id;
	std::string name;
	std::string surname;
	bool operator==( const Person& other ) const = default;
};

// Reference https://eugenkiss.github.io/7guis/tasks/#crud
EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 640, 480, "eepp - 7GUIs - CRUD" } );
	UIWidget* vbox = app.getUI()->loadLayoutFromString( R"xml(
	<vbox layout_width="match_parent" layout_height="match_parent" padding="8dp">
		<hbox layout_width="match_parent" layout_height="wrap_content" marginBottom="8dp">
			<TextView text="Filter prefix: " layout_gravity="center" />
			<TextInput id="filter" layout_width="100dp" />
		</hbox>
		<hbox layout_width="match_parent" layout_height="fixed" layout_weight="1">
			<ListView id="list" layout_height="match_parent" layout_width="fixed" layout_weight="0.5" />
			<vbox layout_width="fixed" layout_weight="0.5" layout_height="wrap_content" marginLeft="8dp">
				<hbox marginBottom="8dp">
					<TextView text="Name: " layout_gravity="center" minWidth="60dp" />
					<TextInput id="name" layout_width="100dp" />
				</hbox>
				<hbox>
					<TextView text="Surname: " layout_gravity="center" minWidth="60dp" />
					<TextInput id="surname" layout_width="100dp" />
				</hbox>
			</vbox>
		</hbox>
		<hbox layout_width="match_parent" layout_height="wrap_content" marginTop="8dp">
			<PushButton id="create" text="Create" marginRight="8dp" />
			<PushButton id="update" text="Update" marginRight="8dp" />
			<PushButton id="delete" text="Delete" />
		</hbox>
	</vbox>
	)xml" );
	auto listView = vbox->find<UIListView>( "list" );
	auto filterInput = vbox->find<UITextInput>( "filter" );
	auto nameInput = vbox->find<UITextInput>( "name" );
	auto surnameInput = vbox->find<UITextInput>( "surname" );
	auto createButton = vbox->find<UIPushButton>( "create" );
	auto updateButton = vbox->find<UIPushButton>( "update" );
	auto deleteButton = vbox->find<UIPushButton>( "delete" );

	ObservableVector<Person> people( {
		{ 1, "Hans", "Emil" },
		{ 2, "Max", "Mustermann" },
		{ 3, "Roman", "Tisch" },
	} );
	std::uint64_t nextId = people[people.size() - 1].id + 1;
	auto model =
		ObservableListModel<Person>::create( people, []( const Person& person, ModelRole role ) {
			return role == ModelRole::Display
					   ? Variant( String::format( "%s, %s", person.surname, person.name ) )
					   : Variant{};
		} );
	listView->setModel( model );

	UIProperty<std::string> filter( filterInput );
	UIProperty<std::string> name( nameInput );
	UIProperty<std::string> surname( surnameInput );
	const auto clearInputs = [&] {
		name = "";
		surname = "";
	};

	ObservableValue<bool> hasSelection( false );
	auto updateButtonEnabled = bindReadOnlyValue( hasSelection, updateButton, "enabled" );
	auto deleteButtonEnabled = bindReadOnlyValue( hasSelection, deleteButton, "enabled" );
	listView->on( Event::OnSelectionChanged, [&]( const Event* ) {
		auto selected = listView->getSelection().first();
		hasSelection = selected.isValid();
		if ( const Person* person = model->at( selected ) ) {
			name = person->name;
			surname = person->surname;
		} else {
			clearInputs();
		}
	} );

	auto filterConnection = filter.observe( [&]( const std::string& prefix ) {
		if ( prefix.empty() ) {
			model->clearFilter();
		} else {
			model->setFilter( [prefix]( const Person& person ) {
				return String::istartsWith( person.surname, prefix );
			} );
		}
		if ( model->rowCount() > 0 )
			listView->setSelection( model->index( 0 ) );
	} );

	createButton->onClick( [&]( const MouseEvent* ) {
		if ( name.get().empty() || surname.get().empty() ) {
			UIMessageBox::New( UIMessageBox::OK, "Complete name and surname" )->showWhenReady();
			return;
		}
		people.pushBack( { nextId++, name.get(), surname.get() } );
		clearInputs();
		filter = "";
	} );
	updateButton->onClick( [&]( const MouseEvent* ) {
		auto row = model->sourceRow( listView->getSelection().first() );
		if ( row )
			people.set( *row, { people[*row].id, name.get(), surname.get() } );
		clearInputs();
	} );
	deleteButton->onClick( [&]( const MouseEvent* ) {
		auto row = model->sourceRow( listView->getSelection().first() );
		if ( row )
			people.erase( *row );
		clearInputs();
		filter = "";
	} );

	return app.run();
}
