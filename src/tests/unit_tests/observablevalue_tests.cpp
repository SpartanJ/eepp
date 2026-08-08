#include "utest.h"
#include <eepp/core/observablevalue.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
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
