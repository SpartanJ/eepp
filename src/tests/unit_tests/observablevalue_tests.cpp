#include "utest.h"
#include <eepp/core/computedvalue.hpp>
#include <eepp/core/observablevalue.hpp>
#include <eepp/core/observablevector.hpp>
#include <eepp/scene/actionmanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/models/observablelistmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/databinding/uibindinggroup.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/databinding/uicommand.hpp>
#include <eepp/ui/databinding/uiobservedelivery.hpp>
#include <eepp/ui/databinding/uiproperty.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/databinding/uivaluebinding.hpp>
#include <thread>

using namespace EE;
using namespace EE::UI;

UTEST( ObservableValue, notifiesUntilConnectionIsDestroyed ) {
	ObservableValue<int> value( 1 );
	int observed = 0;
	{
		auto connection = value.observe( [&]( const int& newValue ) { observed = newValue; } );
		value = 2;
		EXPECT_EQ( observed, 2 );
	}

	value = 3;
	EXPECT_EQ( observed, 2 );
}

UTEST( ObservableValue, connectionsFollowMovedValueAndExpireWithIt ) {
	typename ObservableValue<int>::Connection connection;
	int observed = 0;
	{
		ObservableValue<int> original( 1 );
		connection = original.observe( [&]( const int& newValue ) { observed = newValue; } );
		ObservableValue<int> moved( std::move( original ) );
		moved = 2;
		EXPECT_EQ( observed, 2 );
	}
	EXPECT_FALSE( static_cast<bool>( connection ) );
}

UTEST( ObservableValue, canBeDestroyedDuringNotification ) {
	auto value = std::make_unique<ObservableValue<int>>( 1 );
	typename ObservableValue<int>::Connection destroyingConnection;
	destroyingConnection = value->observe( [&]( const int& ) { value.reset(); } );
	int observed = 0;
	auto remainingConnection =
		value->observe( [&]( const int& newValue ) { observed = newValue; } );

	*value = 2;

	EXPECT_TRUE( value == nullptr );
	EXPECT_EQ( observed, 2 );
	EXPECT_FALSE( static_cast<bool>( destroyingConnection ) );
	EXPECT_FALSE( static_cast<bool>( remainingConnection ) );
}

UTEST( ObservableValue, notifiesObserversInRegistrationOrder ) {
	ObservableValue<int> value( 0 );
	std::vector<int> order;
	auto third = value.observe( [&]( const int& ) { order.emplace_back( 3 ); } );
	auto first = value.observe( [&]( const int& ) { order.emplace_back( 1 ); } );
	auto second = value.observe( [&]( const int& ) { order.emplace_back( 2 ); } );

	value = 1;

	EXPECT_EQ( order.size(), 3u );
	EXPECT_EQ( order[0], 3 );
	EXPECT_EQ( order[1], 1 );
	EXPECT_EQ( order[2], 2 );
}

UTEST( ObservableValue, queuesLatestReentrantValue ) {
	ObservableValue<int> value( 0 );
	std::vector<int> observed;
	auto updating = value.observe( [&]( const int& current ) {
		observed.emplace_back( current );
		if ( current == 1 ) {
			value = 2;
			value = 3;
		}
	} );
	auto recording =
		value.observe( [&]( const int& current ) { observed.emplace_back( current * 10 ); } );

	value = 1;

	ASSERT_EQ( observed.size(), 4u );
	EXPECT_EQ( observed[0], 1 );
	EXPECT_EQ( observed[1], 10 );
	EXPECT_EQ( observed[2], 3 );
	EXPECT_EQ( observed[3], 30 );
}

UTEST( ObservableValue, preservesSnapshotsWithoutCopyingCallbacks ) {
	ObservableValue<int> value( 0 );
	int firstCalls = 0;
	int disconnectedCalls = 0;
	int addedCalls = 0;
	typename ObservableValue<int>::Connection disconnected;
	typename ObservableValue<int>::Connection added;
	auto first = value.observe( [&]( const int& current ) {
		++firstCalls;
		if ( current == 1 ) {
			disconnected.disconnect();
			added = value.observe( [&]( const int& ) { ++addedCalls; } );
			value = 2;
		}
	} );
	disconnected = value.observe( [&]( const int& ) { ++disconnectedCalls; } );

	value = 1;

	EXPECT_EQ( firstCalls, 2 );
	EXPECT_EQ( disconnectedCalls, 1 );
	EXPECT_EQ( addedCalls, 1 );
	EXPECT_FALSE( static_cast<bool>( disconnected ) );
	EXPECT_TRUE( static_cast<bool>( added ) );
}

UTEST( ComputedValue, derivesExplicitDependenciesAndSuppressesEqualResults ) {
	ObservableValue<std::string> first( "Ada" );
	ObservableValue<std::string> last( "Lovelace" );
	auto fullName =
		computedValue( first, last, []( const std::string& first, const std::string& last ) {
			return first + " " + last;
		} );
	int notifications = 0;
	auto connection = fullName.observe( [&]( const std::string& ) { ++notifications; } );

	EXPECT_TRUE( fullName.get() == "Ada Lovelace" );
	first = "Grace";
	EXPECT_TRUE( fullName.get() == "Grace Lovelace" );
	EXPECT_EQ( notifications, 1 );
	first = "Grace";
	EXPECT_EQ( notifications, 1 );
}

UTEST( ComputedValue, supportsChainsAndSafeDestructionOrders ) {
	auto source = std::make_unique<ObservableValue<int>>( 2 );
	auto doubled = computedValue( *source, []( int value ) { return value * 2; } );
	auto label = computedValue( doubled, []( int value ) { return String::toString( value ); } );
	auto connection = label.observe( []( const std::string& ) {} );

	*source = 3;
	EXPECT_TRUE( label.get() == "6" );
	source.reset();
	EXPECT_TRUE( label.get() == "6" );
	EXPECT_TRUE( static_cast<bool>( connection ) );
}

UTEST( ComputedValue, bindsOneWayToWidget ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - ComputedValue Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UITextInput::New();
	ObservableValue<int> count( 2 );
	auto label = computedValue( count, []( int value ) { return String::toString( value * 2 ); } );
	auto binding =
		bindValue( label, widget, UIValueConverter<std::string>::converterString(), "text" );

	EXPECT_TRUE( widget->getText() == "4" );
	count = 3;
	EXPECT_TRUE( widget->getText() == "6" );
	eeDelete( widget );
	EXPECT_FALSE( static_cast<bool>( binding ) );
}

UTEST( UIProperty, participatesInComputedValuesAndCommands ) {
	UIProperty<int> count( 2 );
	auto doubled = computedValue( count, []( int value ) { return value * 2; } );
	EXPECT_EQ( doubled.get(), 4 );

	count = 3;
	EXPECT_EQ( doubled.get(), 6 );

	UIProperty<bool> enabled( true );
	int executions = 0;
	UICommand command( [&] { ++executions; }, enabled );
	EXPECT_TRUE( command.execute() );
	enabled = false;
	EXPECT_FALSE( command.execute() );
	EXPECT_EQ( executions, 1 );
}

UTEST( UIValueConverter, customInputConversionUsesDefaultOutputConversion ) {
	UIValueConverter<int> converter(
		[]( const CSS::PropertyDefinition*, const std::string& value ) -> UIValueResult<int> {
			int parsed = 0;
			return String::fromString( parsed, value ) ? UIValueResult<int>( parsed )
													   : UIValueResult<int>::error( 1 );
		} );

	auto converted = converter.fromValue( nullptr, 42 );
	ASSERT_TRUE( converted );
	EXPECT_TRUE( *converted.value == "42" );
}

UTEST( UIBindingGroup, aggregatesValidationDirtyAndReset ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIBindingGroup Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto nameInput = UITextInput::New();
	auto countInput = UITextInput::New();
	ObservableValue<std::string> name( "Ada" );
	int count = 2;
	UIBindingGroup form;
	form += bindValue( name, nameInput, UIValueConverter<std::string>::converterString(), "text",
					   Event::OnTextChanged );
	form += UIDataBind<int>::New( &count, countInput, UIValueConverter<int>::converterDefault(),
								  "text", Event::OnTextChanged );
	int changeNotifications = 0;
	form.onChange( [&] { ++changeNotifications; } );

	EXPECT_TRUE( form.isValid() );
	EXPECT_FALSE( form.isDirty() );
	auto widgets = form.widgets();
	ASSERT_EQ( widgets.size(), 2u );
	EXPECT_EQ( widgets[0], nameInput );
	EXPECT_EQ( widgets[1], countInput );
	name = "Grace";
	EXPECT_TRUE( form.isDirty() );
	form.reset();
	EXPECT_TRUE( name.get() == "Ada" );
	EXPECT_FALSE( form.isDirty() );
	countInput->setText( "invalid" );
	EXPECT_FALSE( form.isValid() );
	EXPECT_TRUE( form.firstInvalidWidget() == countInput );
	ASSERT_EQ( form.errors().size(), 1u );
	countInput->setEnabled( false );
	EXPECT_TRUE( form.isValid() );
	EXPECT_TRUE( form.errors().empty() );
	countInput->setEnabled( true );
	EXPECT_FALSE( form.isValid() );
	name = "Grace";
	const int notificationsBeforeAggregateUnchanged = changeNotifications;
	name = "Lovelace";
	EXPECT_EQ( notificationsBeforeAggregateUnchanged + 1, changeNotifications );
	form.clear();
	EXPECT_TRUE( form.isValid() );
	eeDelete( nameInput );
	eeDelete( countInput );
}

UTEST( UIBindingGroup, publishesAggregateStateForComputedConsumers ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIBindingGroup Aggregate Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto input = UITextInput::New();
	ObservableValue<std::string> value( "initial" );
	UIBindingGroup form;
	form += bindValue( value, input, UIValueConverter<std::string>::converterString(), "text",
					   Event::OnTextChanged );
	auto canSave = computedValue( form.validValue(), form.dirtyValue(),
								  []( bool valid, bool dirty ) { return valid && dirty; } );

	EXPECT_FALSE( canSave.get() );
	value = "changed";
	EXPECT_TRUE( canSave.get() );
	form.markClean();
	EXPECT_FALSE( canSave.get() );
	eeDelete( input );
}

UTEST( UIBindingGroup, tracksUIPropertyValidationDirtyStateAndLifetime ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIProperty Group Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto input = UITextInput::New();
	auto mirrorInput = UITextInput::New();
	UIValueConverter<int> validPort(
		[]( const CSS::PropertyDefinition*, const std::string& value ) -> UIValueResult<int> {
			int parsed = 0;
			return String::fromString( parsed, value ) && parsed >= 1 && parsed <= 65535
					   ? UIValueResult<int>( parsed )
					   : UIValueResult<int>::error( 1, "invalid port" );
		} );
	UIBindingGroup form;
	{
		UIProperty<int> port( 8080, UnorderedSet<UIWidget*>{ input, mirrorInput }, validPort );
		form += port;

		auto widgets = form.widgets();
		ASSERT_EQ( widgets.size(), 2u );
		EXPECT_TRUE( std::find( widgets.begin(), widgets.end(), input ) != widgets.end() );
		EXPECT_TRUE( std::find( widgets.begin(), widgets.end(), mirrorInput ) != widgets.end() );
		EXPECT_TRUE( form.isValid() );
		EXPECT_FALSE( form.isDirty() );

		port = 9000;
		EXPECT_TRUE( form.isDirty() );
		form.reset();
		EXPECT_EQ( port.get(), 8080 );
		EXPECT_FALSE( form.isDirty() );

		input->setText( "9001" );
		EXPECT_EQ( port.get(), 9001 );
		EXPECT_TRUE( form.isDirty() );

		input->setText( "invalid" );
		EXPECT_FALSE( form.isValid() );
		EXPECT_EQ( form.firstInvalidWidget(), input );
		input->setEnabled( false );
		EXPECT_TRUE( form.isValid() );
		input->setEnabled( true );
		EXPECT_FALSE( form.isValid() );
	}

	EXPECT_TRUE( form.isValid() );
	EXPECT_FALSE( form.isDirty() );
	EXPECT_TRUE( form.widgets().empty() );
	EXPECT_TRUE( form.validValue().get() );
	EXPECT_FALSE( form.dirtyValue().get() );
	eeDelete( input );
	eeDelete( mirrorInput );
}

UTEST( UICommand, synchronizesEnabledStateAndPreventsReentrantExecution ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UICommand Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UIWidget::New();
	int executions = 0;
	UICommand* commandPtr = nullptr;
	UICommand command( [&] {
		++executions;
		commandPtr->execute();
	} );
	commandPtr = &command;
	auto binding = bindCommand( command, *widget );
	EXPECT_TRUE( command.execute() );
	EXPECT_EQ( executions, 1 );
	command.enabled() = false;
	EXPECT_FALSE( widget->isEnabled() );
	EXPECT_FALSE( command.execute() );
	eeDelete( widget );
}

UTEST( UICommand, shortcutUsesSceneDispatchAndRestoresPreviousMapping ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UICommand Shortcut Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto shortcut = KeyBindings::Shortcut{ KEY_S, KeyMod::getDefaultModifier() };
	app.getUI()->getKeyBindings().addKeybind( shortcut, "previous-save" );
	int executions = 0;
	UICommand command( [&] { ++executions; } );
	{
		auto binding = bindCommand( command, *app.getUI(), shortcut );
		auto registered = app.getUI()->getKeyBindings().getCommandFromKeyBind( shortcut );
		EXPECT_TRUE( registered != "previous-save" );
		app.getUI()->executeKeyBindingCommand( registered );
		EXPECT_EQ( executions, 1 );
		command.enabled() = false;
		app.getUI()->executeKeyBindingCommand( registered );
		EXPECT_EQ( executions, 1 );
	}
	EXPECT_TRUE( app.getUI()->getKeyBindings().getCommandFromKeyBind( shortcut ) ==
				 "previous-save" );
}

UTEST( UICommand, followsComputedEnabledSource ) {
	ObservableValue<bool> valid( true );
	ObservableValue<bool> dirty( false );
	auto canSave =
		computedValue( valid, dirty, []( bool valid, bool dirty ) { return valid && dirty; } );
	int executions = 0;
	UICommand command( [&] { ++executions; }, canSave );

	EXPECT_FALSE( command.execute() );
	dirty = true;
	EXPECT_TRUE( command.execute() );
	EXPECT_EQ( executions, 1 );
	valid = false;
	EXPECT_FALSE( command.execute() );
}

UTEST( UICommand, compositeBindingOwnsCommandButtonAndShortcut ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UICommand Composite Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto button = UIWidget::New();
	button->setParent( app.getUI()->getRoot() );
	ObservableValue<bool> enabled( true );
	int executions = 0;
	auto shortcut = KeyBindings::Shortcut{ KEY_S, KeyMod::getDefaultModifier() };
	auto binding = bindCommand( [&] { ++executions; }, enabled, *button, *app.getUI(), shortcut );

	button->sendMouseEvent( Event::MouseClick, Vector2i::Zero, 0 );
	EXPECT_EQ( executions, 1 );
	auto registered = app.getUI()->getKeyBindings().getCommandFromKeyBind( shortcut );
	app.getUI()->executeKeyBindingCommand( registered );
	EXPECT_EQ( executions, 2 );
	enabled = false;
	EXPECT_FALSE( button->isEnabled() );
	app.getUI()->executeKeyBindingCommand( registered );
	EXPECT_EQ( executions, 2 );
}

UTEST( ObservableVector, emitsIncrementalChangesAndDrivesModel ) {
	ObservableVector<std::string> values( { "a", "b" } );
	auto model = Models::ObservableListModel<std::string>::create( values );
	std::vector<ObservableVector<std::string>::Change> changes;
	auto connection = values.observe( [&]( const auto& change ) { changes.push_back( change ); } );

	values.insert( 1, "x" );
	EXPECT_EQ( model->rowCount(), 3u );
	EXPECT_TRUE( model->data( model->index( 1 ) ).toString() == "x" );
	values.set( 1, "y" );
	values.move( 1, 2 );
	values.erase( 0 );
	EXPECT_EQ( values.size(), 2u );
	EXPECT_EQ( changes.size(), 8u );
}

UTEST( ObservableVector, preservesSnapshotsAcrossNestedNotifications ) {
	ObservableVector<int> values( { 1 } );
	int firstCalls = 0;
	int disconnectedCalls = 0;
	int addedCalls = 0;
	bool nested = false;
	typename ObservableVector<int>::Connection disconnected;
	typename ObservableVector<int>::Connection added;
	auto first = values.observe( [&]( const auto& change ) {
		++firstCalls;
		if ( !nested && change.type == ObservableVector<int>::ChangeType::Insert &&
			 change.phase == ObservableVector<int>::Phase::Before ) {
			nested = true;
			disconnected.disconnect();
			added = values.observe( [&]( const auto& ) { ++addedCalls; } );
			values.set( 0, 2 );
		}
	} );
	disconnected = values.observe( [&]( const auto& ) { ++disconnectedCalls; } );

	values.pushBack( 3 );

	EXPECT_EQ( firstCalls, 4 );
	EXPECT_EQ( disconnectedCalls, 1 );
	EXPECT_EQ( addedCalls, 3 );
	EXPECT_FALSE( static_cast<bool>( disconnected ) );
	EXPECT_TRUE( static_cast<bool>( added ) );
}

UTEST( ObservableListModel, formatsFiltersAndMapsSourceRows ) {
	ObservableVector<std::string> values( { "alpha", "beta", "alpine" } );
	auto model = Models::ObservableListModel<std::string>::create(
		values, []( const std::string& value, Models::ModelRole role ) {
			return role == Models::ModelRole::Display ? Models::Variant( "item: " + value )
													  : Models::Variant{};
		} );
	model->setFilter(
		[]( const std::string& value ) { return String::startsWith( value, "al" ); } );

	ASSERT_EQ( model->rowCount(), 2u );
	EXPECT_TRUE( model->data( model->index( 1 ) ).toString() == "item: alpine" );
	ASSERT_TRUE( model->sourceRow( model->index( 1 ) ) );
	EXPECT_EQ( *model->sourceRow( model->index( 1 ) ), 2u );
	values.pushBack( "albatross" );
	EXPECT_EQ( model->rowCount(), 3u );
	model->clearFilter();
	EXPECT_EQ( model->rowCount(), 4u );
}

UTEST( ObservableListModel, retainsSourceStorageAfterObservableVectorDestruction ) {
	std::shared_ptr<Models::ObservableListModel<std::string>> model;
	{
		ObservableVector<std::string> values( { "alpha", "beta" } );
		model = Models::ObservableListModel<std::string>::create( values );
		EXPECT_EQ( model->rowCount(), 2u );
	}

	ASSERT_EQ( model->rowCount(), 2u );
	EXPECT_TRUE( model->data( model->index( 0 ) ).toString() == "alpha" );
	EXPECT_TRUE( model->data( model->index( 1 ) ).toString() == "beta" );
	model.reset();
}

UTEST( UIThreadObservation, usesImmediateMainThreadFastPathAndExpiresSafely ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIThreadObservation Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UIWidget::New();
	ObservableValue<int> value( 1 );
	int delivered = 0;
	auto observation =
		observeOnUIThread( value, *app.getUI(), *widget,
						   [&]( UIWidget&, const int& current ) { delivered = current; } );
	value = 2;
	EXPECT_EQ( delivered, 2 );
	eeDelete( widget );
	EXPECT_FALSE( static_cast<bool>( observation ) );
}

UTEST( UIThreadObservation, callbackCanCloseEndpoint ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIThreadObservation Reentrancy Test",
						WindowStyle::Default, WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UIWidget::New();
	ObservableValue<int> value( 1 );
	auto observation =
		observeOnUIThread( value, *app.getUI(), *widget,
						   [&]( UIWidget& endpoint, const int& ) { eeDelete( &endpoint ); } );

	value = 2;
	EXPECT_FALSE( static_cast<bool>( observation ) );
}

UTEST( UIThreadObservation, workerDeliveryIsOrderedAndHonorsScopedLifetimes ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIThreadObservation Worker Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UIWidget::New();
	ObservableValue<int> value( 1 );
	std::vector<int> delivered;
	auto observation =
		observeOnUIThread( value, *app.getUI(), *widget, [&]( UIWidget&, const int& current ) {
			delivered.push_back( current );
		} );

	std::thread producer( [&] {
		value = 2;
		value = 3;
	} );
	producer.join();
	EXPECT_TRUE( delivered.empty() );
	app.getUI()->getActionManager()->update( Time::Zero );
	ASSERT_EQ( delivered.size(), 2u );
	EXPECT_EQ( delivered[0], 2 );
	EXPECT_EQ( delivered[1], 3 );

	std::thread queuedBeforeClose( [&] { value = 4; } );
	queuedBeforeClose.join();
	eeDelete( widget );
	app.getUI()->getActionManager()->update( Time::Zero );
	EXPECT_EQ( delivered.size(), 2u );
	EXPECT_FALSE( static_cast<bool>( observation ) );

	auto disconnectedWidget = UIWidget::New();
	ObservableValue<int> disconnectedValue( 1 );
	int deliveredAfterDisconnect = 0;
	auto disconnectedObservation =
		observeOnUIThread( disconnectedValue, *app.getUI(), *disconnectedWidget,
						   [&]( UIWidget&, const int& ) { ++deliveredAfterDisconnect; } );
	std::thread queuedBeforeDisconnect( [&] { disconnectedValue = 2; } );
	queuedBeforeDisconnect.join();
	disconnectedObservation.disconnect();
	app.getUI()->getActionManager()->update( Time::Zero );
	EXPECT_EQ( deliveredAfterDisconnect, 0 );
	eeDelete( disconnectedWidget );
}

UTEST( UIValueBinding, synchronizesBothDirectionsAndHandlesEndpointLifetimes ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIValueBinding Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UICheckBox::New();
	ObservableValue<bool> value( true );
	auto binding = bindValue( value, widget );

	EXPECT_TRUE( widget->isChecked() );
	value = false;
	EXPECT_FALSE( widget->isChecked() );
	widget->setChecked( true );
	EXPECT_TRUE( value.get() );

	{
		ObservableValue<bool> temporary( false );
		binding = bindValue( temporary, widget );
		EXPECT_TRUE( static_cast<bool>( binding ) );
	}
	EXPECT_FALSE( static_cast<bool>( binding ) );

	eeDelete( widget );
	EXPECT_FALSE( static_cast<bool>( binding ) );
}

UTEST( UIValueBinding, validatesWidgetProposalsButAcceptsAuthoritativeObservableValues ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIValueBinding Validation Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UITextInput::New();
	ObservableValue<std::string> value( "initial" );
	UIValueConverter<std::string> nonEmpty(
		[]( const CSS::PropertyDefinition*,
			const std::string& candidate ) -> UIValueResult<std::string> {
			return candidate.empty() ? UIValueResult<std::string>::error( 300, "empty value" )
									 : UIValueResult<std::string>::success( candidate );
		},
		UIValueConverter<std::string>::converterString().fromValue );
	auto binding = bindValue( value, widget, nonEmpty, "text", Event::OnTextChanged );

	widget->setText( "" );
	EXPECT_TRUE( value.get() == "initial" );
	EXPECT_FALSE( binding.isValid() );
	EXPECT_EQ( *binding.validationState()->code(), 300u );

	widget->setText( "valid" );
	EXPECT_TRUE( value.get() == "valid" );
	EXPECT_TRUE( binding.isValid() );

	value = "";
	EXPECT_TRUE( widget->getText().empty() );
	EXPECT_TRUE( binding.isValid() );

	eeDelete( widget );
	EXPECT_TRUE( binding.isValid() );
}

UTEST( UIValueBinding, convertsFormattedModelValues ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIValueBinding Converter Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UITextInput::New();
	ObservableValue<double> amount( 10.0 );
	UIValueConverter<double> euro(
		[]( const CSS::PropertyDefinition*, const std::string& text ) -> UIValueResult<double> {
			const std::string prefix( "€" );
			double value;
			if ( text.rfind( prefix, 0 ) != 0 ||
				 !String::fromString( value, text.substr( prefix.size() ) ) )
				return UIValueResult<double>::error( 400, "expected an EUR amount" );
			if ( value < 0 )
				return UIValueResult<double>::error( 401 );
			return value;
		},
		[]( const CSS::PropertyDefinition*, double value ) {
			return UIValueResult<std::string>( "€" + String::fromDouble( value ) );
		} );
	auto binding = bindValue( amount, widget, euro, "text", Event::OnTextChanged );

	EXPECT_TRUE( widget->getText() == "€10" );
	widget->setText( "€25" );
	EXPECT_EQ( amount.get(), 25.0 );
	EXPECT_TRUE( binding.isValid() );

	widget->setText( "USD 30" );
	EXPECT_EQ( amount.get(), 25.0 );
	EXPECT_EQ( *binding.validationState()->code(), 400u );

	widget->setText( "€-5" );
	EXPECT_EQ( amount.get(), 25.0 );
	EXPECT_EQ( *binding.validationState()->code(), 401u );

	amount = -5;
	EXPECT_TRUE( widget->getText() == "€-5" );
	EXPECT_TRUE( binding.isValid() );

	eeDelete( widget );
}
