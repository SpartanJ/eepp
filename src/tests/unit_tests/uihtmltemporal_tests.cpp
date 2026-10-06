#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicalendar.hpp>
#include <eepp/ui/uidatepicker.hpp>
#include <eepp/ui/uidatetimepicker.hpp>
#include <eepp/ui/uihtmlform.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitimepicker.hpp>
#include <eepp/ui/uiwebview.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;

namespace {

UIApplication temporalApp() {
	return UIApplication(
		WindowSettings{ 800,
						650,
						"HTML temporal input tests",
						WindowStyle::Default,
						WindowBackend::Default,
						32,
						{},
						1,
						false,
						true },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
}

} // namespace

UTEST( UIHTMLTemporal, NativeMappingAndStrictValueSanitization ) {
	auto app = temporalApp();
	struct Case {
		const char* type;
		const char* value;
		const char* normalized;
	};
	const Case cases[] = {
		{ "date", "2024-02-29", "2024-02-29" },
		{ "date", "0001-01-01", "0001-01-01" },
		{ "date", "00001-01-01", "00001-01-01" },
		{ "date", "12026-09-28", "12026-09-28" },
		{ "date", "2025-02-29", "" },
		{ "date", "0000-01-01", "" },
		{ "date", "28/09/2026", "" },
		{ "date", "2026-9-28", "" },
		{ "date", " 2026-09-28", "" },
		{ "time", "20:14", "20:14" },
		{ "time", "20:14:00", "20:14:00" },
		{ "time", "20:14:32.120", "20:14:32.120" },
		{ "time", "20:14:32.1", "20:14:32.1" },
		{ "time", "24:00", "" },
		{ "time", "8:14 PM", "" },
		{ "time", "20:14:32.1234", "" },
		{ "datetime-local", "2026-09-28 20:14:00.000", "2026-09-28T20:14" },
		{ "datetime-local", "2026-09-28T20:14:32.007", "2026-09-28T20:14:32.007" },
		{ "datetime-local", "2026-09-28T20:14Z", "" },
		{ "datetime-local", "2026-09-28T20:14+01:00", "" } };
	for ( const auto& test : cases ) {
		auto* input = UIHTMLInput::New();
		input->setParent( app.getUI()->getRoot() );
		input->setInputType( test.type );
		const Uint32 type = std::string_view( test.type ) == "date"	  ? UI_TYPE_DATEPICKER
							: std::string_view( test.type ) == "time" ? UI_TYPE_TIMEPICKER
																	  : UI_TYPE_DATETIMEPICKER;
		EXPECT_TRUE( input->getChildWidget()->isType( type ) );
		input->applyProperty( StyleSheetProperty( "value", test.value, false ) );
		EXPECT_TRUE( input->getFormValue().toUtf8() == test.normalized );
		auto* edit = static_cast<UIDateTimeEdit*>( input->getChildWidget() );
		edit->setLocale( DateTimeLocale::fromLocaleName( "en_US" ) );
		EXPECT_TRUE( input->getFormValue().toUtf8() == test.normalized );
		edit->setLocale( DateTimeLocale::fromLocaleName( "es_AR" ) );
		EXPECT_TRUE( input->getFormValue().toUtf8() == test.normalized );
		input->close();
		app.getUI()->update( Time::Zero );
	}
}

UTEST( UIHTMLTemporal, RangeRequiredAndStepValidation ) {
	auto app = temporalApp();
	auto* input = UIHTMLInput::New();
	input->setParent( app.getUI()->getRoot() );
	input->setInputType( "date" );
	input->applyProperty( StyleSheetProperty( "required", "required" ) );
	EXPECT_TRUE( input->getValidity().valueMissing );
	EXPECT_FALSE( input->checkValidity() );
	input->applyProperty( StyleSheetProperty( "min", "2026-09-01" ) );
	input->applyProperty( StyleSheetProperty( "max", "2026-09-30" ) );
	input->applyProperty( StyleSheetProperty( "step", "2" ) );
	input->applyProperty( StyleSheetProperty( "value", "2026-09-02" ) );
	EXPECT_TRUE( input->getValidity().stepMismatch );
	input->applyProperty( StyleSheetProperty( "value", "2026-09-03" ) );
	EXPECT_TRUE( input->checkValidity() );
	EXPECT_EQ( static_cast<UIDateTimeEdit*>( input->getChildWidget() )->getDayStep(), 2u );
	input->applyProperty( StyleSheetProperty( "value", "2026-10-01" ) );
	EXPECT_TRUE( input->getValidity().rangeOverflow );
	EXPECT_TRUE( input->getFormValue() == "2026-10-01" ); // Validation never clamps the DOM value.
	input->applyProperty( StyleSheetProperty( "min", "malformed" ) );
	input->applyProperty( StyleSheetProperty( "max", "malformed" ) );
	input->applyProperty( StyleSheetProperty( "step", "any" ) );
	EXPECT_TRUE( input->checkValidity() );
	input->setInputType( "time" );
	input->applyProperty( StyleSheetProperty( "min", "22:00" ) );
	input->applyProperty( StyleSheetProperty( "max", "02:00" ) );
	input->applyProperty( StyleSheetProperty( "value", "23:30" ) );
	EXPECT_TRUE( input->checkValidity() );
	input->applyProperty( StyleSheetProperty( "value", "01:30" ) );
	EXPECT_TRUE( input->checkValidity() );
	input->applyProperty( StyleSheetProperty( "value", "12:00" ) );
	EXPECT_TRUE( input->getValidity().rangeUnderflow );
	EXPECT_TRUE( input->getValidity().rangeOverflow );
	input->applyProperty( StyleSheetProperty( "min", "00:00:00.005" ) );
	input->applyProperty( StyleSheetProperty( "max", "23:59:59.999" ) );
	input->applyProperty( StyleSheetProperty( "step", "0.01" ) );
	input->applyProperty( StyleSheetProperty( "value", "00:00:00.015" ) );
	EXPECT_TRUE( input->checkValidity() );
	input->applyProperty( StyleSheetProperty( "value", "00:00:00.016" ) );
	EXPECT_TRUE( input->getValidity().stepMismatch );
	input->applyProperty(
		StyleSheetProperty( "readonly", "false" ) ); // HTML boolean: presence is true.
	EXPECT_FALSE( input->willValidate() );
	EXPECT_TRUE( input->checkValidity() );
	EXPECT_FALSE( static_cast<UIDateTimeEdit*>( input->getChildWidget() )->isEditingAllowed() );
	input->close();
	app.getUI()->update( Time::Zero );
}

UTEST( UIHTMLTemporal, ReadOnlySurvivesInputTypeChanges ) {
	auto app = temporalApp();
	auto* input = UIHTMLInput::New();
	input->setParent( app.getUI()->getRoot() );
	input->applyProperty( StyleSheetProperty( "readonly", "readonly" ) );
	for ( const char* type : { "password", "date", "time", "datetime-local", "text" } ) {
		input->setInputType( type );
		ASSERT_TRUE( input->getChildWidget()->isType( UI_TYPE_TEXTINPUT ) );
		EXPECT_FALSE( input->getChildWidget()->asType<UITextInput>()->isEditingAllowed() );
		app.getUI()->update( Time::Zero );
	}
}

UTEST( UIHTMLTemporal, DefaultStepsAndValueAttributeStepBase ) {
	auto app = temporalApp();
	auto* input = UIHTMLInput::New();
	input->setParent( app.getUI()->getRoot() );
	input->setInputType( "time" );
	auto* edit = static_cast<UIDateTimeEdit*>( input->getChildWidget() );
	edit->setTime( { 12, 0, 1 } );
	EXPECT_TRUE( input->getValidity().stepMismatch );
	input->applyProperty( StyleSheetProperty( "value", "12:00:01" ) );
	EXPECT_TRUE( input->checkValidity() );
	edit->setTime( { 12, 1, 1 } );
	EXPECT_TRUE( input->checkValidity() );
	edit->setTime( { 12, 1, 2 } );
	EXPECT_TRUE( input->getValidity().stepMismatch );
	input->applyProperty( StyleSheetProperty( "step", "900" ) );
	EXPECT_EQ( edit->getMinuteStep(), 15u );
	edit->setTime( { 12, 15, 1 } );
	EXPECT_TRUE( input->checkValidity() );
	input->setInputType( "datetime-local" );
	input->applyProperty( StyleSheetProperty( "min", "2026-09-28T23:50" ) );
	input->applyProperty( StyleSheetProperty( "step", "1200" ) );
	input->applyProperty( StyleSheetProperty( "value", "2026-09-29T00:10" ) );
	EXPECT_TRUE( input->checkValidity() );
	input->applyProperty( StyleSheetProperty( "value", "2026-09-29T00:11" ) );
	EXPECT_TRUE( input->getValidity().stepMismatch );
	input->close();
	app.getUI()->update( Time::Zero );
}

UTEST( UIHTMLTemporal, FormSerializationAndInteractiveValidation ) {
	auto app = temporalApp();
	auto* ui = app.getUI();
	ui->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<form id="form" action="https://example.test/submit" method="POST">
			<input id="date" name="date" type="date" required>
			<input name="time" value="20:14:00" type="time">
			<input name="datetime" type="datetime-local" value="2026-09-28 20:14:32.120">
			<input name="empty" type="date">
			<input name="disabled" type="date" value="2026-09-28" disabled>
		</form>)html" ) );
	ui->update( Time::Zero );
	auto* form = ui->getRoot()->find( "form" )->asType<UIHTMLForm>();
	ASSERT_TRUE( form );
	int requests = 0;
	std::string body;
	ui->setNavigationInterceptorCb( [&]( const NavigationRequest& request ) {
		++requests;
		body = request.body;
		return true;
	} );
	EXPECT_FALSE( form->requestSubmit() );
	EXPECT_EQ( requests, 0 );
	auto* input = ui->getRoot()->find( "date" )->asType<UIHTMLInput>();
	input->applyProperty( StyleSheetProperty( "value", "2026-09-28" ) );
	EXPECT_TRUE( form->requestSubmit() );
	EXPECT_EQ( requests, 1 );
	EXPECT_TRUE( body ==
				 "date=2026-09-28&time=20%3A14%3A00&datetime=2026-09-28T20%3A14%3A32.12&empty=" );
	input->applyProperty( StyleSheetProperty( "value", "invalid" ) );
	form->submit(); // The direct HTML submit API bypasses constraint validation.
	EXPECT_EQ( requests, 2 );
}

UTEST( UIHTMLTemporal, WebViewPopupRespectsPrivateDefaultsAndVisibleViewport ) {
	auto app = temporalApp();
	auto* web = UIWebView::New();
	web->setParent( app.getUI()->getRoot() );
	web->setPosition( 30, 20 );
	web->setSize( 600, 550 );
	auto* document = web->getDocumentSceneNode();
	ASSERT_TRUE( document );
	document->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<html><head><style>
		:root { --font: #ff0000; --list-back: #00ff00; }
		calendar, calendar::day, datepicker::button, datetimepicker::time-down { font-size: 80px; }
		</style></head><body>
		<input id="date" type="date" value="2026-09-28" style="font-size: 14px; width: 200px;">
		<div style="height: 1200px;"></div>
		</body></html>)html" ),
									web->getDocumentContainer() );
	web->refreshDocumentLayout();
	app.getUI()->update( Time::Zero );
	document->update( Time::Zero );
	auto* input = document->getRoot()->find( "date" )->asType<UIHTMLInput>();
	ASSERT_TRUE( input );
	auto* picker = static_cast<UIDatePicker*>( input->getChildWidget() );
	picker->showCalendar();
	document->update( Time::Zero );
	auto* calendar = picker->getCalendar();
	ASSERT_TRUE( calendar );
	EXPECT_TRUE( calendar->getUISceneNode() == document );
	EXPECT_TRUE( calendar->getParent() == document->getRoot() );
	EXPECT_TRUE( document->getVisibleWorldBounds().contains( calendar->getScreenRect() ) );
	EXPECT_LT( static_cast<UIPushButton*>( calendar->querySelector( "calendar::day" ) )
				   ->getTextView()
				   ->getFontSize(),
			   80u );
	EXPECT_LT( picker->getCalendarButton()->getTextView()->getFontSize(), 80u );
	ASSERT_TRUE( picker->getCalendarButton()->hasForeground() );
	ASSERT_TRUE( picker->getCalendarButton()->getForeground()->getLayer( 0 ) );
	EXPECT_TRUE( picker->getCalendarButton()->getForeground()->getLayer( 0 )->getDrawable() !=
				 nullptr );
	EXPECT_NE( calendar->getBackgroundColor().a, 0 );
	EXPECT_FALSE( calendar->getBackgroundColor() == Color::fromString( "#00ff00" ) );
	EXPECT_FALSE( picker->getCalendarButton()->getForegroundTint( 0 ) == Color::Red );
	const auto before = calendar->getScreenRect().getPosition();
	web->setPosition( 50, 40 );
	picker->getScreenRect();
	EXPECT_NEAR( calendar->getScreenRect().Left - before.x, 20.f, 0.1f );
	EXPECT_NEAR( calendar->getScreenRect().Top - before.y, 20.f, 0.1f );
}

UTEST( UIHTMLTemporal, AnonymousCheckboxRetainsUserAgentDefaults ) {
	for ( int engine = 0; engine < 2; ++engine ) {
		auto app = temporalApp();
		auto* ui = app.getUI();
		ui->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<html><head><style>checkbox::active, checkbox::inactive { width: 80px; }</style></head>
		<body><input id="checkbox" type="checkbox" checked></body></html>)html" ) );
		ui->update( Time::Zero );
		auto* input = ui->getRoot()->find( "checkbox" )->asType<UIHTMLInput>();
		ASSERT_TRUE( input );
		auto* button = input->getChildWidget()->querySelector( "checkbox::active" );
		ASSERT_TRUE( button );
		EXPECT_NEAR( button->getSize().getWidth(), 12.f, 0.1f );
		EXPECT_TRUE( button->hasForeground() );
	}
}

UTEST( UIHTMLTemporal, DateTimePopupKeepsPrivateDefaultsForLateTimeButtons ) {
	auto app = temporalApp();
	auto* web = UIWebView::New();
	web->setParent( app.getUI()->getRoot() );
	web->setSize( 700, 600 );
	auto* document = web->getDocumentSceneNode();
	document->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<html><head><style>
		:root { --font: #ff0000; --list-back: #00ff00; }
		datetimepicker::time-up, datetimepicker::time-down { width: 80px; foreground-tint: red; }
		input { background-color: #123456; }
		</style></head><body>
		<input id="datetime" type="datetime-local" value="2026-09-28T20:14"
			style="width: 200px;">
		</body></html>)html" ),
									web->getDocumentContainer() );
	web->refreshDocumentLayout();
	app.getUI()->update( Time::Zero );
	document->update( Time::Zero );
	auto* input = document->getRoot()->find( "datetime" )->asType<UIHTMLInput>();
	ASSERT_TRUE( input );
	EXPECT_TRUE( input->getBackgroundColor() == Color::fromString( "#123456" ) );
	auto* picker = static_cast<UIDateTimePicker*>( input->getChildWidget() );
	picker->showCalendar();
	document->update( Time::Zero );
	auto* time = picker->getPopupTimePicker();
	ASSERT_TRUE( time );
	EXPECT_TRUE( time->getParent()->getParent() == document->getRoot() );
	EXPECT_TRUE( time->getParent()->asType<UIWidget>()->getFlags() & UI_IGNORE_GLOBAL_CSS );
	picker->setShowSeconds( true );
	picker->setShowMilliseconds( true );
	document->update( Time::Zero );
	for ( const char* selector :
		  { "datetimepicker::time-up.hour", "datetimepicker::time-down.minute",
			"datetimepicker::time-up.second", "datetimepicker::time-down.millisecond" } ) {
		auto* button = time->querySelector( selector );
		ASSERT_TRUE( button );
		EXPECT_TRUE( button->getFlags() & UI_IGNORE_GLOBAL_CSS );
		EXPECT_LT( button->getSize().getWidth(), 80.f );
		EXPECT_FALSE( button->getForegroundTint( 0 ) == Color::Red );
		ASSERT_TRUE( button->hasForeground() );
		ASSERT_TRUE( button->getForeground()->getLayer( 0 ) );
		EXPECT_TRUE( button->getForeground()->getLayer( 0 )->getDrawable() );
	}
	// Replacing a control disconnects its scoped popup-attachment listener.
	input->setInputType( "text" );
	document->update( Time::Zero );
	input->setInputType( "date" );
	auto* date = static_cast<UIDatePicker*>( input->getChildWidget() );
	date->showCalendar();
	document->update( Time::Zero );
	EXPECT_TRUE( date->getCalendar()->getFlags() & UI_IGNORE_GLOBAL_CSS );
}

UTEST( UIHTMLTemporal, BareControlsHaveIntrinsicSizeAndLightDefaultsWithoutNativeTheme ) {
	auto app = temporalApp();
	auto* web = UIWebView::New();
	web->setParent( app.getUI()->getRoot() );
	web->setSize( 700, 600 );
	auto* document = web->getDocumentSceneNode();
	document->getUIThemeManager()->setDefaultTheme( UIThemePtr{} );
	document->loadLayoutFromString( HTMLFormatter::HTMLtoXML( R"html(
		<label>Start date:</label>
		<input id="filled" type="date" value="2018-07-22" min="2018-01-01" max="2018-12-31">
		<input id="empty" type="date">
		<input id="time" type="time" value="20:14">
		<input id="datetime" type="datetime-local" value="2018-07-22T20:14">
		<input id="emptydatetime" type="datetime-local">
		)html" ),
									web->getDocumentContainer() );
	web->refreshDocumentLayout();
	app.getUI()->update( Time::Zero );
	document->update( Time::Zero );
	for ( const char* id : { "filled", "empty", "time", "datetime", "emptydatetime" } ) {
		auto* input = document->getRoot()->find( id )->asType<UIHTMLInput>();
		ASSERT_TRUE( input );
		auto* edit = static_cast<UIDateTimeEdit*>( input->getChildWidget() );
		EXPECT_GT( input->getPixelsSize().getWidth(), 0.f );
		EXPECT_GT( input->getPixelsSize().getHeight(), 0.f );
		EXPECT_GE( edit->getPixelsSize().getWidth(),
				   edit->getTextWidth() + edit->getPixelsPadding().getWidth() );
		EXPECT_TRUE( input->getBackgroundColor() == Color::White );
		EXPECT_TRUE( edit->getFontColor() == Color::Black );
		if ( edit->isType( UI_TYPE_DATEPICKER ) ) {
			auto* picker = static_cast<UIDatePicker*>( edit );
			auto* button = picker->getCalendarButton();
			EXPECT_GT( button->getPixelsSize().getWidth(), 0.f );
			ASSERT_TRUE( button->hasForeground() );
			ASSERT_TRUE( button->getForeground()->getLayer( 0 ) );
			EXPECT_TRUE( button->getForeground()->getLayer( 0 )->getDrawable() );
			picker->showCalendar();
			document->update( Time::Zero );
			EXPECT_TRUE( document->getVisibleWorldBounds().contains(
				picker->getCalendar()->getScreenRect() ) );
			auto* title =
				picker->getCalendar()->querySelector( "calendar::title" )->asType<UIPushButton>();
			EXPECT_TRUE( title->getTextView()->getFontColor() == Color::Black );
			picker->hideCalendar();
		}
	}
}
