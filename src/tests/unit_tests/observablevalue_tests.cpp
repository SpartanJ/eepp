#include "utest.h"
#include <eepp/core/observablevalue.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uivaluebinding.hpp>

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
