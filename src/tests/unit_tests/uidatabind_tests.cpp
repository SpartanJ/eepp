#include "utest.h"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiproperty.hpp>
#include <eepp/ui/uitextinput.hpp>

using namespace EE;
using namespace EE::UI;

UTEST( UIValueValidation, supportsCodesAndOptionalDiagnostics ) {
	auto success = UIValueValidationResult::success();
	EXPECT_TRUE( static_cast<bool>( success ) );
	EXPECT_FALSE( success.code.has_value() );
	EXPECT_FALSE( success.debugMessage.has_value() );

	auto coded = UIValueValidationResult::error( 42 );
	EXPECT_FALSE( static_cast<bool>( coded ) );
	EXPECT_TRUE( coded.code.has_value() );
	EXPECT_EQ( *coded.code, 42u );
	EXPECT_FALSE( coded.debugMessage.has_value() );

	auto diagnosed = UIValueValidationResult::error( 43, "technical detail" );
	EXPECT_EQ( *diagnosed.code, 43u );
	EXPECT_TRUE( *diagnosed.debugMessage == "technical detail" );
	auto diagnosticOnly = UIValueValidationResult::error( std::string( "only diagnostic" ) );
	EXPECT_FALSE( diagnosticOnly.code.has_value() );
	EXPECT_TRUE( *diagnosticOnly.debugMessage == "only diagnostic" );
}

UTEST( UIValueValidation, observesOnlyDistinctResultChanges ) {
	UIValueValidationState state;
	int notifications = 0;
	auto connection = state.observe( [&]( const UIValueValidationResult& ) { ++notifications; } );

	state.set( UIValueValidationResult::error( 7 ) );
	state.set( UIValueValidationResult::error( 7 ) );
	state.set( UIValueValidationResult::error( 7, "detail" ) );
	state.clear();

	EXPECT_EQ( notifications, 3 );
	EXPECT_TRUE( state.isValid() );
}

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
	auto value = converter.toValue( nullptr, "widget value" );
	EXPECT_TRUE( value );
	EXPECT_TRUE( *value.value == "widget value" );
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

UTEST( UIDataBind, validatesWidgetProposalsButAcceptsAuthoritativeModelValues ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIDataBind Validation Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	std::string value( "initial" );
	auto firstWidget = UITextInput::New();
	auto secondWidget = UITextInput::New();
	UIValueConverter<std::string> converter(
		[]( const CSS::PropertyDefinition*,
			const std::string& candidate ) -> UIValueResult<std::string> {
			return candidate == "invalid" ? UIValueResult<std::string>::error( 100 )
										  : UIValueResult<std::string>::success( candidate );
		},
		UIValueConverter<std::string>::converterString().fromValue );
	UIDataBind<std::string> binding( &value, UnorderedSet<UIWidget*>{ firstWidget, secondWidget },
									 converter, "text", Event::OnTextChanged );

	firstWidget->setText( "invalid" );
	EXPECT_TRUE( value == "initial" );
	EXPECT_TRUE( firstWidget->getText() == "invalid" );
	EXPECT_TRUE( secondWidget->getText() == "initial" );
	EXPECT_FALSE( binding.isValid() );
	EXPECT_EQ( *binding.validationState().code(), 100u );

	auto result = binding.set( std::string( "invalid" ) );
	EXPECT_TRUE( static_cast<bool>( result ) );
	EXPECT_TRUE( value == "invalid" );
	EXPECT_TRUE( binding.isValid() );

	EXPECT_TRUE( binding.set( std::string( "initial" ) ) );
	firstWidget->setText( "initial" );
	firstWidget->setText( "invalid" );
	eeDelete( firstWidget );
	firstWidget = nullptr;
	EXPECT_TRUE( binding.isValid() );

	firstWidget = UITextInput::New();
	binding.bind( firstWidget );
	firstWidget->setText( "valid" );
	EXPECT_TRUE( value == "valid" );
	EXPECT_TRUE( secondWidget->getText() == "valid" );
	EXPECT_TRUE( binding.isValid() );

	eeDelete( firstWidget );
	eeDelete( secondWidget );
}

UTEST( UIDataBind, defaultNumericConversionFailurePreservesModel ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIDataBind Parse Validation Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UITextInput::New();
	int value = 5;
	UIDataBind<int> binding( &value, widget, UIDataBind<int>::converterDefault(), "text",
							 Event::OnTextChanged );

	widget->setText( "not a number" );
	EXPECT_EQ( value, 5 );
	EXPECT_FALSE( binding.isValid() );
	EXPECT_EQ( *binding.validationState().code(),
			   static_cast<Uint32>( UIValueValidationError::ConversionFailed ) );

	eeDelete( widget );
}

UTEST( UIDataBind, failedModelToWidgetConversionDoesNotApplyPartialText ) {
	UIApplication app(
		WindowSettings( 320, 240, "eepp - UIDataBind Conversion Test", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto widget = UITextInput::New();
	int value = 1;
	UIValueConverter<int> converter(
		[]( const CSS::PropertyDefinition*, const std::string& ) {
			return UIValueResult<int>::success( 1 );
		},
		[]( const CSS::PropertyDefinition*, const int& converted ) {
			return converted == 2 ? UIValueResult<std::string>::error( 200 )
								  : UIValueResult<std::string>::success( "partial" );
		} );
	UIDataBind<int> binding( &value, widget, converter, "text", Event::OnTextChanged );
	EXPECT_TRUE( widget->getText() == "partial" );

	auto result = binding.set( 2 );
	EXPECT_FALSE( static_cast<bool>( result ) );
	EXPECT_TRUE( widget->getText() == "partial" );
	EXPECT_EQ( value, 2 );
	EXPECT_FALSE( binding.set( 2 ) );
	EXPECT_FALSE( binding.isValid() );

	eeDelete( widget );
}
