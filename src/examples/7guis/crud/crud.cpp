#include <eepp/ee.hpp>

struct Person {
	std::uint64_t id;
	std::string name;
	std::string surname;
};

class PeopleModel : public Model {
  public:
	PeopleModel( const std::vector<Person>& people, std::string filterStr = "" ) : mData( people ) {
		filter( String::toLower( filterStr ) );
	}
	size_t rowCount( const ModelIndex& ) const override { return mData.size(); }
	size_t columnCount( const ModelIndex& ) const override { return 1; }
	std::string columnName( const size_t& ) const override { return ""; }

	ModelIndex index( int row, int column,
					  const ModelIndex& parent = ModelIndex() ) const override {
		if ( row >= (int)rowCount( parent ) || column >= (int)columnCount( parent ) )
			return {};
		return Model::index( row, column, parent );
	}

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		if ( role == ModelRole::Display )
			return Variant(
				String::format( "%s, %s", getPerson( index ).surname, getPerson( index ).name ) );
		return {};
	}

	void filter( const std::string& filterStr ) {
		if ( filterStr.empty() )
			return;
		std::vector<Person> data;
		for ( auto& people : mData )
			if ( String::startsWith( String::toLower( people.surname ), filterStr ) )
				data.emplace_back( std::move( people ) );
		mData = std::move( data );
		invalidate( Model::UpdateFlag::DontInvalidateIndexes );
	}

	void setPeople( const std::vector<Person>& people ) {
		mData = people;
		invalidate( Model::UpdateFlag::DontInvalidateIndexes );
	}

	const Person& getPerson( const ModelIndex& index ) const { return mData[index.row()]; }

  protected:
	std::vector<Person> mData;
};

// Reference https://eugenkiss.github.io/7guis/tasks#crud
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
	std::vector<Person> people{
		{ 1, "Hans", "Emil" },
		{ 2, "Max", "Mustermann" },
		{ 3, "Roman", "Tisch" },
	};
	std::uint64_t nextId = people.back().id + 1;
	auto listView = vbox->find<UIListView>( "list" );
	auto filterView = vbox->find<UITextInput>( "filter" );
	auto nameView = vbox->find<UITextInput>( "name" );
	auto surnameView = vbox->find<UITextInput>( "surname" );
	auto createBut = vbox->find<UIPushButton>( "create" );
	auto updateBut = vbox->find<UIPushButton>( "update" );
	auto deleteBut = vbox->find<UIPushButton>( "delete" );
	auto model = std::make_shared<PeopleModel>( people );
	listView->setModel( model );
	const auto updateModel = [&people, filterView, &model]( bool updatePeople, bool updateFilter ) {
		if ( updatePeople || updateFilter )
			model->setPeople( people );
		if ( updateFilter )
			model->filter( filterView->getText().toUtf8() );
	};
	const auto updateButs = [createBut, updateBut, deleteBut, listView]() {
		createBut->setEnabled( !listView->getSelection().first().isValid() );
		updateBut->setEnabled( listView->getSelection().first().isValid() );
		deleteBut->setEnabled( listView->getSelection().first().isValid() );
	};
	const auto& clearInputs = [nameView, surnameView]() {
		nameView->setText( "" );
		surnameView->setText( "" );
	};
	const auto updateSelection = [&updateButs, listView, nameView, surnameView, &clearInputs]() {
		updateButs();
		if ( listView->getSelection().first().isValid() ) {
			auto selPerson = static_cast<PeopleModel*>( listView->getModel() )
								 ->getPerson( listView->getSelection().first() );
			nameView->setText( selPerson.name );
			surnameView->setText( selPerson.surname );
		} else {
			clearInputs();
		}
	};
	listView->on( Event::OnSelectionChanged, [&updateSelection]( auto ) { updateSelection(); } );
	filterView->on( Event::OnTextChanged,
					[&updateModel, listView, &clearInputs, &updateButs, &model]( auto ) {
						updateModel( true, true );
						updateButs();
						clearInputs();
						listView->getSelection().clear( model->rowCount( {} ) == 0 );
						if ( model->rowCount( {} ) > 0 )
							listView->setSelection( model->index( 0, 0 ) );
					} );
	createBut->onClick( [&]( auto ) {
		if ( nameView->getText().empty() || surnameView->getText().empty() ) {
			UIMessageBox::New( UIMessageBox::OK, "Complete name and surname" )->showWhenReady();
			return;
		}
		people.emplace_back(
			Person{ nextId++, nameView->getText().toUtf8(), surnameView->getText().toUtf8() } );
		clearInputs();
		updateModel( true, false );
		filterView->setText( "" );
	} );
	updateBut->onClick( [&]( auto ) {
		auto selPerson = static_cast<PeopleModel*>( listView->getModel() )
							 ->getPerson( listView->getSelection().first() );

		auto found =
			std::find_if( people.begin(), people.end(), [&selPerson]( const Person& person ) {
				return selPerson.id == person.id;
			} );

		if ( found != people.end() ) {
			found->name = nameView->getText().toUtf8();
			found->surname = surnameView->getText().toUtf8();
			clearInputs();
			updateModel( true, true );
		}
	} );
	deleteBut->onClick( [&]( auto ) {
		if ( !listView->getSelection().first().isValid() ) {
			UIMessageBox::New( UIMessageBox::OK, "Select a person from the list" )->showWhenReady();
			return;
		}
		auto selPerson = static_cast<PeopleModel*>( listView->getModel() )
							 ->getPerson( listView->getSelection().first() );

		auto found =
			std::find_if( people.begin(), people.end(), [&selPerson]( const Person& person ) {
				return selPerson.id == person.id;
			} );

		if ( found != people.end() ) {
			people.erase( found );
			clearInputs();
			updateModel( true, false );
			filterView->setText( "" );
		}
	} );
	updateButs();
	return app.run();
}
