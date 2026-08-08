#include "utest.h"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiproperty.hpp>

using namespace EE;
using namespace EE::UI;

UTEST( UIProperty, defaultConstructionOwnsUsableValue ) {
	UIProperty<int> property;
	EXPECT_EQ( property.value(), 0 );

	property = 42;
	EXPECT_EQ( property.value(), 42 );
}

UTEST( UIProperty, rvalueAssignmentReportsStoredValue ) {
	UIProperty<std::string> property;
	std::string reportedValue;
	property.changed( [&]( const std::string& value ) { reportedValue = value; } );

	property = std::string( "updated" );
	EXPECT_TRUE( property.value() == "updated" );
	EXPECT_TRUE( reportedValue == "updated" );
}

UTEST( UIProperty, numericMutationOperatorsPropagateAndReturnExpectedValues ) {
	UIProperty<int> property( 2 );
	int reportedValue = 0;
	property.changed( [&]( const int& value ) { reportedValue = value; } );

	++property;
	EXPECT_EQ( property.value(), 3 );
	EXPECT_EQ( reportedValue, 3 );
	EXPECT_EQ( property++, 3 );
	EXPECT_EQ( property.value(), 4 );
	property += 3;
	EXPECT_EQ( property.value(), 7 );
	property -= 2;
	EXPECT_EQ( property.value(), 5 );
	property *= 4;
	EXPECT_EQ( property.value(), 20 );
	property /= 5;
	EXPECT_EQ( property.value(), 4 );
	--property;
	EXPECT_EQ( property.value(), 3 );
	EXPECT_EQ( property--, 3 );
	EXPECT_EQ( property.value(), 2 );
}

UTEST( UIProperty, stringConcatenationPropagatesForStandardAndEEStrings ) {
	UIProperty<std::string> standardString( std::string( "hello" ) );
	std::string reportedStandardString;
	standardString.changed( [&]( const std::string& value ) { reportedStandardString = value; } );
	standardString += std::string( " world" );
	EXPECT_TRUE( standardString.value() == "hello world" );
	EXPECT_TRUE( reportedStandardString == "hello world" );
	EXPECT_TRUE( standardString + std::string( "!" ) == "hello world!" );

	UIProperty<String> eeString( String( "hello" ) );
	String reportedEEString;
	eeString.changed( [&]( const String& value ) { reportedEEString = value; } );
	eeString += String( " eepp" );
	EXPECT_TRUE( eeString.value() == String( "hello eepp" ) );
	EXPECT_TRUE( reportedEEString == String( "hello eepp" ) );
	EXPECT_TRUE( eeString + String( "!" ) == String( "hello eepp!" ) );
}

UTEST( UIDataBind, defaultStringConverterReadsWidgetValue ) {
	auto converter = UIDataBind<std::string>::converterDefault();
	std::string value;
	EXPECT_TRUE( converter.toValue( nullptr, value, "widget value" ) );
	EXPECT_TRUE( value == "widget value" );
}

UTEST( UIDataBind, lateBoundWidgetReceivesValueAndCanDieFirst ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIDataBind Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	bool value = false;
	UIDataBind<bool> binding( &value, UnorderedSet<UIWidget*>{},
							  UIDataBind<bool>::converterBool() );
	auto widget = UICheckBox::New();
	widget->setChecked( true );

	binding.bind( widget );
	EXPECT_FALSE( widget->isChecked() );
	EXPECT_EQ( binding.getWidgets().size(), 1u );

	eeDelete( widget );
	EXPECT_TRUE( binding.getWidgets().empty() );
}

UTEST( UIDataBind, supportsMultipleWidgetsAndDisconnectsWhenBindingDiesFirst ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIDataBind Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	bool value = true;
	auto firstWidget = UICheckBox::New();
	auto secondWidget = UICheckBox::New();
	{
		UIDataBind<bool> binding( &value, UnorderedSet<UIWidget*>{ firstWidget, secondWidget },
								  UIDataBind<bool>::converterBool() );
		EXPECT_TRUE( firstWidget->isChecked() );
		EXPECT_TRUE( secondWidget->isChecked() );
		EXPECT_EQ( binding.getWidgets().size(), 2u );

		binding.set( false );
		EXPECT_FALSE( firstWidget->isChecked() );
		EXPECT_FALSE( secondWidget->isChecked() );
	}

	firstWidget->setChecked( true );
	secondWidget->setChecked( true );
	EXPECT_FALSE( value );
	eeDelete( firstWidget );
	eeDelete( secondWidget );
}
