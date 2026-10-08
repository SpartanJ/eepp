#include "utest.h"
#include <eepp/system/datetime.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/databinding/uiproperty.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicalendar.hpp>
#include <eepp/ui/uidatepicker.hpp>
#include <eepp/ui/uidatetimeedit.hpp>
#include <eepp/ui/uidatetimepicker.hpp>
#include <eepp/ui/uigridlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitimepicker.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/input.hpp>
#include <limits>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace {

class TestableDateTimeEdit : public UIDateTimeEdit {
  public:
	static TestableDateTimeEdit* New( DateTimeEditMode mode ) {
		return eeNew( TestableDateTimeEdit, ( mode ) );
	}

	bool typeDigit( char digit ) { return processDigit( digit ); }

	bool typeAmPm( char value ) { return processAmPm( value ); }

	void clickAt( const Vector2f& localPosition ) {
		Vector2f worldPosition = localPosition;
		nodeToWorld( worldPosition );
		onMouseClick( worldPosition.asInt(), EE_BUTTON_LMASK );
	}

	Vector2f sectionClickPosition( Int32 column ) {
		return PixelDensity::pxToDp( mTextCache.findCharacterPos( column ) + mRealAlignOffset +
									 Vector2f( mPaddingPx.Left, mPaddingPx.Top ) );
	}

	Uint32 key( Keycode code, Uint32 mod = 0 ) {
		return onKeyDown( KeyEvent( this, Event::KeyDown, code, SCANCODE_UNKNOWN, 0, mod ) );
	}

  protected:
	explicit TestableDateTimeEdit( DateTimeEditMode mode ) : UIDateTimeEdit( mode ) {}
};

class TestableCalendar : public UICalendar {
  public:
	static TestableCalendar* New() { return eeNew( TestableCalendar, () ); }

	Uint32 key( Keycode keyCode, Uint32 mod = 0 ) {
		KeyEvent event( this, Event::KeyDown, keyCode, SCANCODE_UNKNOWN, 0, mod );

		return onKeyDown( event );
	}

  protected:
	TestableCalendar() : UICalendar() {}
};

UIApplication testApp() {
	// Keep app teardown within each test, before process-static text caches are destroyed.
	return UIApplication(
		WindowSettings{ 640, 480, "eepp - date time edit tests" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
}

} // namespace

UTEST( UIDateTimeEdit, DateValueFormattingAndSerialization ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	EXPECT_TRUE( edit->getText().toUtf8() == "28/09/2026" );
	EXPECT_TRUE( edit->getSerializedValue() == "2026-09-28" );
	EXPECT_TRUE( edit->hasValue() );

	eeDelete( edit );
}

UTEST( UIDateTimeEdit, MillisecondSteppingCarriesAndRespectsYearLimits ) {
	auto app = testApp();
	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Time );
	edit->setShowMilliseconds( true );
	edit->setTime( TimeOfDay{ 23, 59, 59, 999 } );
	edit->setActiveSection( DateTimeSection::Millisecond );
	EXPECT_TRUE( edit->stepActiveSection( 1 ) );
	EXPECT_TRUE( edit->getTime() == ( TimeOfDay{ 0, 0, 0, 0 } ) );
	EXPECT_TRUE( edit->stepActiveSection( -1 ) );
	EXPECT_TRUE( edit->getTime() == ( TimeOfDay{ 23, 59, 59, 999 } ) );
	edit->setEditMode( DateTimeEditMode::DateTime );
	edit->setDateTime( LocalDateTime{ { 2026, 9, 28 }, { 23, 59, 59, 999 } } );
	edit->setActiveSection( DateTimeSection::Millisecond );
	EXPECT_TRUE( edit->stepActiveSection( 1 ) );
	EXPECT_TRUE( edit->getDateTime() == ( LocalDateTime{ { 2026, 9, 29 }, {} } ) );
	const LocalDateTime maximum{ { std::numeric_limits<Int32>::max(), 12, 31 },
								 { 23, 59, 59, 999 } };
	edit->setDateTime( maximum );
	EXPECT_FALSE( edit->stepActiveSection( 1 ) );
	EXPECT_TRUE( edit->getDateTime() == maximum );
	const LocalDateTime minimum{ { std::numeric_limits<Int32>::min(), 1, 1 }, {} };
	edit->setDateTime( minimum );
	EXPECT_FALSE( edit->stepActiveSection( -1 ) );
	EXPECT_TRUE( edit->getDateTime() == minimum );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, EmptyStateAndClear ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	EXPECT_FALSE( edit->hasValue() );
	EXPECT_TRUE( edit->getText().empty() );
	EXPECT_TRUE( edit->getHint().toUtf8() == "dd/mm/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->clear();
	EXPECT_FALSE( edit->hasValue() );
	EXPECT_TRUE( edit->getSerializedValue().empty() );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, DateSectionSteppingUsesCalendarArithmetic ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 1, 31 } );
	edit->setActiveSection( DateTimeSection::Month );
	EXPECT_TRUE( edit->stepActiveSection( 1 ) );
	ASSERT_TRUE( edit->getDate().has_value() );
	EXPECT_TRUE( *edit->getDate() == ( CalendarDate{ 2026, 2, 28 } ) );
	edit->setDate( CalendarDate{ 2024, 2, 29 } );
	edit->setActiveSection( DateTimeSection::Year );
	EXPECT_TRUE( edit->stepActiveSection( 1 ) );
	EXPECT_TRUE( *edit->getDate() == ( CalendarDate{ 2025, 2, 28 } ) );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, DateTimeHourSteppingCarriesDate ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::DateTime );
	edit->setDateFormat( "yyyy-MM-dd" );
	edit->setTimeFormat( "HH:mm" );
	edit->setDateTime( LocalDateTime{ CalendarDate{ 2026, 9, 28 }, TimeOfDay{ 23, 30, 0, 0 } } );
	edit->setActiveSection( DateTimeSection::Hour );
	EXPECT_TRUE( edit->stepActiveSection( 1 ) );
	ASSERT_TRUE( edit->getDateTime().has_value() );
	EXPECT_TRUE( *edit->getDateTime() ==
				 ( LocalDateTime{ CalendarDate{ 2026, 9, 29 }, TimeOfDay{ 0, 30, 0, 0 } } ) );

	eeDelete( edit );
}

UTEST( UIDateTimeEdit, ConstraintsClampAllProgrammaticMutations ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "yyyy-MM-dd" );
	edit->setMinDate( CalendarDate{ 2026, 9, 10 } );
	edit->setMaxDate( CalendarDate{ 2026, 9, 20 } );
	edit->setDate( CalendarDate{ 2026, 9, 1 } );
	ASSERT_TRUE( edit->getDate().has_value() );
	EXPECT_TRUE( *edit->getDate() == ( CalendarDate{ 2026, 9, 10 } ) );
	edit->setDate( CalendarDate{ 2026, 9, 30 } );
	EXPECT_TRUE( *edit->getDate() == ( CalendarDate{ 2026, 9, 20 } ) );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, TolerantSetTextAndInvalidTextPreservesValue ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->setText( "2027-01-31" );
	ASSERT_TRUE( edit->getDate().has_value() );
	EXPECT_TRUE( *edit->getDate() == ( CalendarDate{ 2027, 1, 31 } ) );
	edit->setText( "31/02/2027" );
	EXPECT_TRUE( *edit->getDate() == ( CalendarDate{ 2027, 1, 31 } ) );
	EXPECT_TRUE( edit->getText().toUtf8() == "31/01/2027" );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, NumericEntryCommitsAndAdvances ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->setActiveSection( DateTimeSection::Month );
	EXPECT_TRUE( edit->typeDigit( '1' ) );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Month );
	EXPECT_TRUE( edit->typeDigit( '2' ) );
	ASSERT_TRUE( edit->getDate().has_value() );
	EXPECT_TRUE( edit->getDate()->month == 12 );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Year );
	edit->setActiveSection( DateTimeSection::Day );
	EXPECT_TRUE( edit->typeDigit( '4' ) );
	EXPECT_TRUE( edit->getDate()->day == 4 );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Month );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, TwelveHourEntryKeepsAmPmAndAmPmTypingToggles ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Time );
	edit->setHourCycle( HourCycle::H12 );
	edit->setTimeFormat( "hh:mm a" );
	edit->setTime( TimeOfDay{ 20, 14, 0, 0 } );
	edit->setActiveSection( DateTimeSection::Hour );
	EXPECT_TRUE( edit->typeDigit( '9' ) );
	ASSERT_TRUE( edit->getTime().has_value() );
	EXPECT_TRUE( edit->getTime()->hour == 21 );
	edit->setActiveSection( DateTimeSection::AmPm );
	EXPECT_TRUE( edit->typeAmPm( 'a' ) );
	EXPECT_TRUE( edit->getTime()->hour == 9 );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, ValueChangeOnlyEmitsForCanonicalChanges ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	int changes = 0;
	auto valueConnection =
		edit->connect( Event::OnValueChange, [&changes]( const Event* ) { ++changes; } );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->setDateFormat( "yyyy-MM-dd" );
	EXPECT_EQ( changes, 1 );
	edit->setDate( CalendarDate{ 2026, 9, 29 } );
	EXPECT_EQ( changes, 2 );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, TimeStepsAndWheelDefault ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Time );
	EXPECT_FALSE( edit->isWheelEditingEnabled() );
	edit->setTimeFormat( "HH:mm:ss" );
	edit->setMinuteStep( 15 );
	edit->setTime( TimeOfDay{ 23, 50, 0, 0 } );
	edit->setActiveSection( DateTimeSection::Minute );
	EXPECT_TRUE( edit->stepActiveSection( 1 ) );
	ASSERT_TRUE( edit->getTime().has_value() );
	EXPECT_TRUE( *edit->getTime() == ( TimeOfDay{ 0, 5, 0, 0 } ) );
	eeDelete( edit );
}

UTEST( UICalendar, FixedGridAndAdjacentMonthDates ) {
	auto app = testApp();

	auto* calendar = TestableCalendar::New();
	calendar->setDisplayedMonth( CalendarDate{ 2026, 9, 1 } );
	EXPECT_EQ( calendar->getDayCellCount(), static_cast<size_t>( 42 ) );
	// Locale may be Sunday- or Monday-first, but September 1st must occur in the first week.
	const CalendarDate first = calendar->getFirstVisibleDate();
	EXPECT_TRUE( first <= ( CalendarDate{ 2026, 9, 1 } ) );
	EXPECT_TRUE( first.addDays( 6 ) >= ( CalendarDate{ 2026, 9, 1 } ) );
	EXPECT_TRUE( calendar->getDayCellDate( 41 ) == first.addDays( 41 ) );
	eeDelete( calendar );
}

UTEST( UICalendar, MonthNavigationDoesNotChangeSelection ) {
	auto app = testApp();

	auto* calendar = TestableCalendar::New();
	calendar->setSelectedDate( CalendarDate{ 2026, 9, 28 } );
	int changes = 0;
	auto valueConnection =
		calendar->connect( Event::OnValueChange, [&changes]( const Event* ) { ++changes; } );
	calendar->next();
	EXPECT_TRUE( calendar->getDisplayedMonth() == ( CalendarDate{ 2026, 10, 1 } ) );
	ASSERT_TRUE( calendar->getSelectedDate().has_value() );
	EXPECT_TRUE( *calendar->getSelectedDate() == ( CalendarDate{ 2026, 9, 28 } ) );
	EXPECT_EQ( changes, 0 );
	calendar->previous();
	EXPECT_TRUE( calendar->getDisplayedMonth() == ( CalendarDate{ 2026, 9, 1 } ) );

	eeDelete( calendar );
}

UTEST( UICalendar, ConstraintsClampSelectionAndKeyboardNavigation ) {
	auto app = testApp();

	auto* calendar = TestableCalendar::New();
	calendar->setMinDate( CalendarDate{ 2026, 9, 10 } );
	calendar->setMaxDate( CalendarDate{ 2026, 9, 20 } );
	calendar->setSelectedDate( CalendarDate{ 2026, 9, 1 } );
	ASSERT_TRUE( calendar->getSelectedDate().has_value() );
	EXPECT_TRUE( *calendar->getSelectedDate() == ( CalendarDate{ 2026, 9, 10 } ) );
	calendar->setFocusedDate( CalendarDate{ 2026, 9, 20 } );
	EXPECT_EQ( calendar->key( KEY_RIGHT ), static_cast<Uint32>( 1 ) );
	EXPECT_TRUE( calendar->getFocusedDate() == ( CalendarDate{ 2026, 9, 20 } ) );
	calendar->setFocusedDate( CalendarDate{ 2026, 9, 10 } );
	EXPECT_EQ( calendar->key( KEY_LEFT ), static_cast<Uint32>( 1 ) );
	EXPECT_TRUE( calendar->getFocusedDate() == ( CalendarDate{ 2026, 9, 10 } ) );

	eeDelete( calendar );
}

UTEST( UICalendar, KeyboardMovementAndSelection ) {
	auto app = testApp();

	auto* calendar = TestableCalendar::New();
	calendar->setFocusedDate( CalendarDate{ 2026, 9, 28 } );
	calendar->clearSelection();
	EXPECT_EQ( calendar->key( KEY_RIGHT ), static_cast<Uint32>( 1 ) );
	EXPECT_TRUE( calendar->getFocusedDate() == ( CalendarDate{ 2026, 9, 29 } ) );
	EXPECT_EQ( calendar->key( KEY_DOWN ), static_cast<Uint32>( 1 ) );
	EXPECT_TRUE( calendar->getFocusedDate() == ( CalendarDate{ 2026, 10, 6 } ) );
	EXPECT_FALSE( calendar->getSelectedDate().has_value() );
	EXPECT_EQ( calendar->key( KEY_RETURN ), static_cast<Uint32>( 1 ) );
	ASSERT_TRUE( calendar->getSelectedDate().has_value() );
	EXPECT_TRUE( *calendar->getSelectedDate() == ( CalendarDate{ 2026, 10, 6 } ) );
	eeDelete( calendar );
}

UTEST( UICalendar, MonthAndYearViewsNavigateDisplayOnly ) {
	auto app = testApp();

	auto* calendar = TestableCalendar::New();
	calendar->setSelectedDate( CalendarDate{ 2026, 9, 28 } );
	calendar->setView( CalendarView::Months );
	calendar->next();
	EXPECT_TRUE( calendar->getDisplayedMonth() == ( CalendarDate{ 2027, 9, 1 } ) );
	ASSERT_TRUE( calendar->getSelectedDate().has_value() );
	EXPECT_TRUE( *calendar->getSelectedDate() == ( CalendarDate{ 2026, 9, 28 } ) );
	calendar->setView( CalendarView::Years );
	calendar->next();
	EXPECT_TRUE( calendar->getView() == CalendarView::Years );
	EXPECT_TRUE( *calendar->getSelectedDate() == ( CalendarDate{ 2026, 9, 28 } ) );
	eeDelete( calendar );
}

UTEST( UICalendar, ValueChangeOnlyOnExplicitSelection ) {
	auto app = testApp();

	auto* calendar = TestableCalendar::New();
	int changes = 0;
	auto valueConnection =
		calendar->connect( Event::OnValueChange, [&changes]( const Event* ) { ++changes; } );
	calendar->setDisplayedMonth( CalendarDate{ 2026, 9, 1 } );
	calendar->setFocusedDate( CalendarDate{ 2026, 9, 28 } );
	calendar->next();
	calendar->previous();
	EXPECT_EQ( changes, 0 );
	calendar->setFocusedDate( CalendarDate{ 2026, 9, 28 } );
	EXPECT_TRUE( calendar->selectFocusedDate() );
	EXPECT_EQ( changes, 1 );
	EXPECT_TRUE( calendar->selectFocusedDate() );
	EXPECT_EQ( changes, 1 );
	eeDelete( calendar );
}

UTEST( UIDateTimeEdit, KeyboardNavigationPendingDigitsAndCancel ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->setActiveSection( DateTimeSection::Month );
	edit->typeDigit( '1' );
	EXPECT_TRUE( edit->getText().toUtf8() == "28/1_/2026" );
	EXPECT_TRUE( edit->getDate()->month == 9 );
	edit->key( KEY_ESCAPE );
	EXPECT_TRUE( edit->getText().toUtf8() == "28/09/2026" );
	edit->typeDigit( '1' );
	edit->key( KEY_RIGHT );
	EXPECT_TRUE( edit->getDate()->month == 1 );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Year );
	edit->key( KEY_HOME );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Day );
	edit->key( KEY_LEFT );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Day );
	edit->key( KEY_END );
	edit->key( KEY_RIGHT );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Year );
	edit->key( KEY_DELETE );
	EXPECT_FALSE( edit->hasValue() );

	eeDelete( edit );
}

UTEST( UIDateTimeEdit, PendingDigitTimeoutCommitsWithoutSleeping ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->setActiveSection( DateTimeSection::Month );
	edit->setPendingDigitTimeout( Time::Zero );
	edit->typeDigit( '1' );
	edit->scheduledUpdate( Time::Zero );
	EXPECT_TRUE( edit->getDate()->month == 1 );
	EXPECT_TRUE( edit->getText().toUtf8() == "28/01/2026" );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, PasteReplacesWholeValueFromSelectedSection ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	edit->setActiveSection( DateTimeSection::Month );

	const std::string previous = app.getWindow()->getClipboard()->getText();
	app.getWindow()->getClipboard()->setText( "2027-01-31" );
	edit->getDocument().execute( "paste" );
	EXPECT_TRUE( edit->getSerializedValue() == "2027-01-31" );

	app.getWindow()->getClipboard()->setText( "31/02/2027" );
	edit->getDocument().execute( "paste" );
	EXPECT_TRUE( edit->getSerializedValue() == "2027-01-31" );
	app.getWindow()->getClipboard()->setText( previous );

	eeDelete( edit );
}

UTEST( UIDateTimeEdit, ReadOnlyKeysAndWheelDoNotMutate ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Time );
	edit->setTime( TimeOfDay{ 12, 30 } );
	edit->setActiveSection( DateTimeSection::Hour );
	edit->setAllowEditing( false );
	edit->key( KEY_UP );
	edit->key( KEY_DELETE );
	EXPECT_FALSE( edit->stepActiveSection( 1 ) );
	EXPECT_TRUE( edit->getTime() == std::optional<TimeOfDay>( TimeOfDay{ 12, 30 } ) );

	eeDelete( edit );
}

UTEST( UIDateTimeEdit, LocaleAndFormatValidationAndPrecision ) {
	auto app = testApp();

	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setLocale( DateTimeLocale::fromLocaleName( "es_AR" ) );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	EXPECT_TRUE( edit->getText().toUtf8() == "28/09/2026" );
	edit->setLocale( DateTimeLocale::fromLocaleName( "en_US" ) );
	EXPECT_TRUE( edit->getText().toUtf8() == "09/28/2026" );
	edit->setDateFormat( "HH:mm" );
	EXPECT_TRUE( edit->getText().toUtf8() == "09/28/2026" );
	edit->setDateFormat( "d/M/yyyy" );
	edit->setActiveSection( DateTimeSection::Month );
	edit->typeDigit( '1' );
	edit->typeDigit( '2' );
	EXPECT_TRUE( edit->getDate()->month == 12 );
	edit->setEditMode( DateTimeEditMode::Time );
	edit->setTime( TimeOfDay{ 20, 14, 32, 125 } );
	edit->setTimeFormat( "HH:mm" );
	EXPECT_TRUE( edit->getSerializedValue() == "20:14:32.125" );
	edit->setTimeFormat( "dd/MM/yyyy" );
	EXPECT_TRUE( edit->getTimeFormat() == "HH:mm" );
	eeDelete( edit );
}

UTEST( UIDateTimeEdit, ContradictoryBoundsResolveAndEmptyPropertyRemovesConstraint ) {
	auto app = testApp();

	auto* edit = UIDateTimeEdit::New();
	edit->setMinDate( CalendarDate{ 2026, 9, 20 } );
	edit->setMaxDate( CalendarDate{ 2026, 9, 10 } );
	EXPECT_TRUE( edit->getMinDate() == edit->getMaxDate() );
	edit->setDate( CalendarDate{ 2026, 9, 1 } );
	EXPECT_TRUE( edit->getDate()->day == 10 );
	EXPECT_TRUE( edit->applyProperty( StyleSheetProperty( "min-date", "" ) ) );
	EXPECT_FALSE( edit->getMinDate() );
	edit->setAllowEmpty( false );
	edit->getDocument().setSelection(
		{ { 0, 0 }, { 0, static_cast<Int64>( edit->getText().size() ) } } );
	edit->getDocument().deleteSelection();
	EXPECT_TRUE( edit->hasValue() );
	EXPECT_FALSE( edit->getText().empty() );
	eeDelete( edit );
}

UTEST( UICalendar, LocaleAlignmentAndBoundChangeEvents ) {
	auto app = testApp();

	auto* calendar = UICalendar::New();
	calendar->setFirstDayOfWeek( 1 )->setDisplayedMonth( CalendarDate{ 2026, 9, 1 } );
	EXPECT_TRUE( calendar->getFirstVisibleDate() == ( CalendarDate{ 2026, 8, 31 } ) );
	calendar->setFirstDayOfWeek( 0 );
	EXPECT_TRUE( calendar->getFirstVisibleDate() == ( CalendarDate{ 2026, 8, 30 } ) );
	calendar->setSelectedDate( CalendarDate{ 2026, 9, 28 } );
	int changes = 0;
	auto valueConnection =
		calendar->connect( Event::OnValueChange, [&changes]( const Event* ) { ++changes; } );
	calendar->setMaxDate( CalendarDate{ 2026, 9, 20 } );
	EXPECT_TRUE( calendar->getSelectedDate()->day == 20 );
	EXPECT_EQ( changes, 1 );
	int selections = 0;
	auto selectionConnection =
		calendar->connect( Event::OnItemSelected, [&selections]( const Event* ) { ++selections; } );
	calendar->selectFocusedDate();
	EXPECT_EQ( changes, 1 );
	EXPECT_EQ( selections, 1 );

	eeDelete( calendar );
}

UTEST( UIDatePicker, EmptyOpeningAndEscapePreserveValue ) {
	auto app = testApp();

	auto* picker = UIDatePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	picker->setSize( 200, 30 );
	EXPECT_TRUE( picker->getCalendar() == nullptr );
	picker->showCalendar();
	ASSERT_TRUE( picker->getCalendar() );
	EXPECT_TRUE( picker->isCalendarVisible() );
	EXPECT_FALSE( picker->hasValue() );
	const auto today = CalendarDate::today();
	EXPECT_TRUE( picker->getCalendar()->getDisplayedMonth() ==
				 ( CalendarDate{ today.year, today.month, 1 } ) );
	picker->getCalendar()->forceKeyDown(
		KeyEvent( picker->getCalendar(), Event::KeyDown, KEY_ESCAPE, SCANCODE_UNKNOWN, 0, 0 ) );
	EXPECT_FALSE( picker->isCalendarVisible() );
	EXPECT_FALSE( picker->hasValue() );
	EXPECT_TRUE( picker->hasFocus() );
	eeDelete( picker );
}

UTEST( UIDatePicker, SelectionClosesAndReSelectingSameDateAlsoCloses ) {
	auto app = testApp();

	auto* picker = UIDatePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setDate( CalendarDate{ 2026, 9, 28 } );
	int changes = 0;
	auto valueConnection =
		picker->connect( Event::OnValueChange, [&changes]( const Event* ) { ++changes; } );
	picker->showCalendar();
	EXPECT_TRUE( picker->getCalendar()->getDisplayedMonth() == ( CalendarDate{ 2026, 9, 1 } ) );
	picker->getCalendar()->setFocusedDate( CalendarDate{ 2026, 10, 6 } )->selectFocusedDate();
	EXPECT_TRUE( picker->getSerializedValue() == "2026-10-06" );
	EXPECT_FALSE( picker->isCalendarVisible() );
	EXPECT_TRUE( picker->hasFocus() );
	EXPECT_EQ( changes, 1 );
	picker->showCalendar();
	picker->getCalendar()->selectFocusedDate();
	EXPECT_FALSE( picker->isCalendarVisible() );
	EXPECT_EQ( changes, 1 );

	eeDelete( picker );
}

UTEST( UIDatePicker, PopupPlacementFocusLossAndLifetime ) {
	auto app = testApp();

	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* picker = UIDatePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	picker->setSize( 200, 30 );
	picker->setPosition( 430, 440 );
	picker->showCalendar();
	auto* calendar = picker->getCalendar();
	EXPECT_TRUE( calendar->getScreenRect().Bottom <= picker->getScreenRect().Top );
	EXPECT_LE( calendar->getScreenRect().Right, app.getUI()->getWorldBounds().Right );
	EXPECT_TRUE( calendar->getParent() == picker->getWindowContainer() );
	auto* other = UITextInput::New();
	other->setParent( app.getUI()->getRoot() );
	other->setFocus();
	EXPECT_FALSE( picker->isCalendarVisible() );
	EXPECT_FALSE( calendar->isEnabled() );
	picker->applyProperty( StyleSheetProperty( "popup-to-root", "true" ) );
	picker->showCalendar();
	EXPECT_TRUE( calendar->getParent() == app.getUI()->getRoot() );
	picker->setEnabled( false );
	EXPECT_FALSE( picker->isCalendarVisible() );
	picker->setEnabled( true );
	picker->showCalendar();
	calendar->close();
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_TRUE( picker->getCalendar() == nullptr );
	EXPECT_FALSE( picker->isCalendarVisible() );

	eeDelete( picker );
	eeDelete( other );
}

UTEST( UIDatePicker, OwnerAndPopupCanCloseInTheSameUpdate ) {
	auto app = testApp();
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	for ( int closingPart = 0; closingPart < 4; ++closingPart ) {
		auto* picker = closingPart ? UIDateTimePicker::New() : UIDatePicker::New();
		picker->setParent( app.getUI()->getRoot() );
		picker->setSize( 200, 30 );
		picker->showCalendar();
		Node* popup = picker->getCalendar();
		if ( closingPart == 1 )
			popup =
				static_cast<UIDateTimePicker*>( picker )->getPopupTimePicker()->getParentWidget();
		else if ( closingPart == 3 )
			popup = static_cast<UIDateTimePicker*>( picker )->getPopupTimePicker();
		const size_t count = app.getUI()->getRoot()->getChildCount() - 2;
		popup->close();
		picker->showCalendar(); // A queued popup cannot be reused before its deletion completes.
		picker->close();
		app.getUI()->update( Time::Zero );
		app.getUI()->update( Time::Zero );
		EXPECT_EQ( app.getUI()->getRoot()->getChildCount(), count );
	}
}

UTEST( UIDatePicker, ButtonPaddingDoesNotGrowWhenOtherPaddingChanges ) {
	auto app = testApp();

	auto* picker = UIDatePicker::New();
	picker->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	picker->setSize( 200, 30 );
	picker->setPaddingRight( 5 );

	const auto right = picker->getPadding().Right;
	picker->setPaddingTop( 7 );
	picker->setPaddingBottom( 8 );
	picker->setPaddingLeft( 9 );
	EXPECT_EQ( picker->getPadding().Right, right );
	picker->setSize( 200, 40 );
	EXPECT_EQ( picker->getPadding().Right, right + 10 );
	EXPECT_EQ( picker->getCalendarButton()->getPosition().x, 160.f );
	EXPECT_TRUE( picker->getCalendarButton()->getSize() == Sizef( 40, 40 ) );

	eeDelete( picker );
}

UTEST( UIDatePicker, ReadOnlyCalendarIconTracksEditingPermission ) {
	for ( int theme = 0; theme < 3; ++theme ) {
		auto app = testApp();
		auto* ui = app.getUI();
		ui->setColorSchemePreference( theme == 1 ? ColorSchemePreference::Light
												 : ColorSchemePreference::Dark );
		std::string css;
		ASSERT_TRUE( FileSystem::fileGet( Sys::getProcessPath() + "../assets/ui/" +
											  ( theme == 2 ? "uitheme.css" : "breeze.css" ),
										  css ) );
		ui->setStyleSheet( css );
		auto* root = ui->loadLayoutFromString( R"xml(
			<vbox>
				<datepicker id="date" value="2026-09-28" allow-editing="false" />
				<datetimepicker id="datetime" value="2026-09-28T12:30:00" allow-editing="false" />
			</vbox>)xml" );
		ASSERT_TRUE( root );
		ui->update( Time::Zero );
		const Color muted = Color::fromString( theme == 2 ? "#969696" : "#727679" );
		for ( const auto* id : { "#date", "#datetime" } ) {
			auto* picker = root->querySelector( id )->asType<UIDatePicker>();
			auto* button = picker->getCalendarButton();
			const auto textColor = picker->getFontColor();
			EXPECT_TRUE( button->getForegroundTint( 0 ) == muted );
			EXPECT_EQ( button->getCursor(), Cursor::Arrow );
			button->pushState( UIState::StateHover );
			ui->update( Milliseconds( 250 ) );
			EXPECT_TRUE( button->getForegroundTint( 0 ) == muted );
			EXPECT_EQ( button->getBackgroundColor().a, 0 );
			picker->setAllowEditing( true );
			ui->update( Milliseconds( 250 ) );
			EXPECT_FALSE( button->getForegroundTint( 0 ) == muted );
			EXPECT_EQ( button->getCursor(), Cursor::Arrow );
			picker->setAllowEditing( false );
			ui->update( Milliseconds( 250 ) );
			EXPECT_TRUE( button->getForegroundTint( 0 ) == muted );
			EXPECT_EQ( button->getBackgroundColor().a, 0 );
			EXPECT_TRUE( picker->getFontColor() == textColor );
			EXPECT_TRUE( picker->isEnabled() );
		}
	}
}

UTEST( UIDateTimePicker, ChildButtonsKeepArrowCursorThroughMouseRouting ) {
	for ( int theme = 0; theme < 3; ++theme ) {
		auto app = testApp();
		auto* ui = app.getUI();
		ui->getUIThemeManager()->setDefaultEffectsEnabled( false );
		ui->setColorSchemePreference( theme == 1 ? ColorSchemePreference::Light
												 : ColorSchemePreference::Dark );
		std::string css;
		ASSERT_TRUE( FileSystem::fileGet( Sys::getProcessPath() + "../assets/ui/" +
											  ( theme == 2 ? "uitheme.css" : "breeze.css" ),
										  css ) );
		ui->setStyleSheet( css );
		auto* picker = UIDateTimePicker::New();
		picker->setParent( ui->getRoot() );
		picker->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		picker->setSize( 240, 30 );
		picker->setPosition( 10, 10 );
		picker->setDateTime( LocalDateTime{ { 2026, 9, 28 }, { 20, 14 } } );
		ui->update( Time::Zero );
		auto* dispatcher = ui->getEventDispatcher();
		auto* cursors = app.getWindow()->getCursorManager();
		const auto hover = [&]( UIWidget* widget, Cursor::SysType cursor ) {
			const Vector2f point = widget->getScreenRect().getCenter();
			dispatcher->getInput()->setMousePos(
				app.getWindow()->mapCoordsToPixel( point, app.getWindow()->getDefaultView() ) );
			dispatcher->update( Time::Zero );
			EXPECT_TRUE( dispatcher->getMouseOverNode() == widget );
			// Check the OS cursor, not just the widget's stored CSS property.
			EXPECT_EQ( cursors->getCurrentSysCursor(), cursor );
			ui->update( Time::Zero );
			EXPECT_EQ( cursors->getCurrentSysCursor(), cursor );
		};
		hover( picker, Cursor::SysIBeam );
		hover( picker->getCalendarButton(), Cursor::SysArrow );
		hover( picker, Cursor::SysIBeam );
		picker->setAllowEditing( false );
		ui->update( Time::Zero );
		hover( picker->getCalendarButton(), Cursor::SysArrow );
		picker->setAllowEditing( true );
		ui->update( Time::Zero );
		picker->showCalendar();
		ui->update( Time::Zero );
		auto* time = picker->getPopupTimePicker();
		ASSERT_TRUE( time );
		for ( const auto* selector : { "Calendar::previous", "Calendar::next" } ) {
			auto* button = picker->getCalendar()->querySelector( selector );
			ASSERT_TRUE( button );
			hover( button, Cursor::SysArrow );
		}
		for ( const auto* selector :
			  { "DateTimePicker::time-up.minute", "DateTimePicker::time-down.minute" } ) {
			auto* button = time->querySelector( selector );
			ASSERT_TRUE( button );
			hover( time, Cursor::SysIBeam );
			hover( button, Cursor::SysArrow );
			hover( time, Cursor::SysIBeam );
		}
		picker->hideCalendar();
		hover( picker, Cursor::SysIBeam );
		hover( picker->getCalendarButton(), Cursor::SysArrow );
		eeDelete( picker );
	}
}

UTEST( UIDateTimePicker, CalendarPreservesTimeAndEmptySelectionUsesMidnight ) {
	auto app = testApp();

	auto* picker = UIDateTimePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setDateTime(
		LocalDateTime{ CalendarDate{ 2026, 9, 28 }, TimeOfDay{ 20, 14, 32, 125 } } );
	picker->showCalendar();
	picker->getCalendar()->setFocusedDate( CalendarDate{ 2027, 1, 31 } )->selectFocusedDate();
	ASSERT_TRUE( picker->getDateTime() );
	EXPECT_TRUE( picker->getDateTime()->time == ( TimeOfDay{ 20, 14, 32, 125 } ) );
	EXPECT_TRUE( picker->getSerializedValue() == "2027-01-31T20:14:32.125" );
	picker->clear();
	picker->showCalendar();
	picker->getCalendar()->setFocusedDate( CalendarDate{ 2027, 2, 1 } )->selectFocusedDate();
	EXPECT_TRUE( picker->getDateTime()->time == TimeOfDay{} );
	picker->setMinDateTime( LocalDateTime{ CalendarDate{ 2027, 2, 1 }, TimeOfDay{ 9, 30 } } );
	picker->clear();
	picker->showCalendar();
	picker->getCalendar()->setFocusedDate( CalendarDate{ 2027, 2, 1 } )->selectFocusedDate();
	EXPECT_TRUE( picker->getDateTime()->time == ( TimeOfDay{ 9, 30 } ) );

	eeDelete( picker );
}

UTEST( UIDateTimePicker, SecondsCanBeShownWithoutMilliseconds ) {
	auto app = testApp();
	auto* picker = UIDateTimePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setLocale( DateTimeLocale::fromLocaleName( "es_AR" ) );
	const LocalDateTime value{ { 2026, 9, 28 }, { 20, 14, 32, 125 } };
	picker->setDateTime( value );
	EXPECT_FALSE( picker->getShowSeconds() );
	EXPECT_FALSE( picker->getShowMilliseconds() );
	EXPECT_TRUE( picker->getText().toUtf8() == "28/09/2026 20:14" );
	picker->showCalendar();
	auto* time = picker->getPopupTimePicker();
	ASSERT_TRUE( time );
	EXPECT_TRUE( time->getText().toUtf8() == "20:14" );
	picker->applyProperty( StyleSheetProperty( "show-seconds", "true" ) );
	EXPECT_TRUE( picker->getShowSeconds() );
	EXPECT_FALSE( picker->getShowMilliseconds() );
	EXPECT_TRUE( picker->getText().toUtf8() == "28/09/2026 20:14:32" );
	EXPECT_TRUE( time->getText().toUtf8() == "20:14:32" );
	auto* seconds = time->querySelector( "DateTimePicker::time-up.second" );
	ASSERT_TRUE( seconds && seconds->isVisible() );
	EXPECT_FALSE( time->querySelector( "DateTimePicker::time-up.millisecond" ) );
	picker->setShowMilliseconds( true );
	EXPECT_TRUE( time->getText().toUtf8() == "20:14:32.125" );
	auto* milliseconds = time->querySelector( "DateTimePicker::time-up.millisecond" );
	ASSERT_TRUE( milliseconds && milliseconds->isVisible() );
	picker->setShowMilliseconds( false );
	EXPECT_TRUE( picker->getShowSeconds() );
	EXPECT_TRUE( time->getText().toUtf8() == "20:14:32" );
	EXPECT_TRUE( seconds->isVisible() );
	EXPECT_FALSE( milliseconds->isVisible() );
	picker->setShowSeconds( false );
	EXPECT_TRUE( time->getText().toUtf8() == "20:14" );
	EXPECT_FALSE( seconds->isVisible() );
	EXPECT_TRUE( picker->getDateTime() == value );
	EXPECT_TRUE( time->getTime() == value.time );
	eeDelete( picker );
}

UTEST( UIDateTimePicker, PopupTimeSteppersPreserveDateAndPrecision ) {
	auto app = testApp();
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* picker = UIDateTimePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setShowSeconds( true );
	picker->setShowMilliseconds( true );
	picker->setMinuteStep( 15 );
	picker->setDateTime( LocalDateTime{ { 2026, 9, 28 }, { 20, 14, 32, 125 } } );
	int changes = 0;
	auto valueConnection =
		picker->connect( Event::OnValueChange, [&changes]( const Event* ) { ++changes; } );
	EXPECT_TRUE( picker->getPopupTimePicker() == nullptr );
	picker->showCalendar();
	app.getUI()->update( Time::Zero );
	auto* time = picker->getPopupTimePicker();
	ASSERT_TRUE( time );
	EXPECT_EQ( changes, 0 );
	EXPECT_TRUE( time->getTime() == ( TimeOfDay{ 20, 14, 32, 125 } ) );
	EXPECT_TRUE( picker->getCalendar()->isEnabled() );
	EXPECT_TRUE( picker->getCalendar()->isVisible() );
	EXPECT_TRUE( time->getParent() == picker->getCalendar()->getParent() );
	EXPECT_GE( time->getScreenRect().Top, picker->getCalendar()->getScreenRect().Bottom );
	auto* minuteUp = time->querySelector( "datetimepicker::time-up.minute" );
	ASSERT_TRUE( minuteUp && minuteUp->isVisible() );
	EXPECT_EQ( minuteUp->getCursor(), Cursor::Arrow );
	app.getUI()->getEventDispatcher()->sendMouseClick(
		minuteUp, minuteUp->getScreenRect().getCenter().asInt(), EE_BUTTON_LMASK );
	EXPECT_TRUE( picker->getDateTime() ==
				 ( LocalDateTime{ { 2026, 9, 28 }, { 20, 29, 32, 125 } } ) );
	EXPECT_EQ( changes, 1 );
	EXPECT_TRUE( picker->isCalendarVisible() );
	picker->getCalendar()->setFocusedDate( CalendarDate{ 2026, 10, 6 } )->selectFocusedDate();
	EXPECT_TRUE( picker->getDateTime() ==
				 ( LocalDateTime{ { 2026, 10, 6 }, { 20, 29, 32, 125 } } ) );
	EXPECT_TRUE( picker->isCalendarVisible() );
	time->setActiveSection( DateTimeSection::Hour );
	time->setFocus();
	app.getUI()->getEventDispatcher()->sendKeyDown( KEY_UP, SCANCODE_UNKNOWN, 0, 0 );
	EXPECT_TRUE( picker->getDateTime()->time == ( TimeOfDay{ 21, 29, 32, 125 } ) );
	app.getUI()->getEventDispatcher()->sendKeyDown( KEY_RETURN, SCANCODE_UNKNOWN, 0, 0 );
	EXPECT_FALSE( picker->isCalendarVisible() );
	EXPECT_TRUE( picker->hasFocus() );
	picker->showCalendar();
	EXPECT_TRUE( picker->getPopupTimePicker() == time );
	EXPECT_TRUE( time->getTime() == picker->getDateTime()->time );
	eeDelete( picker );
}

UTEST( UIDateTimePicker, EmptyPopupAndBoundaryTimeConstraints ) {
	auto app = testApp();
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* picker = UIDateTimePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setMinDateTime( LocalDateTime{ { 2026, 9, 28 }, { 9, 30 } } );
	picker->setMaxDateTime( LocalDateTime{ { 2026, 9, 28 }, { 17, 0 } } );
	int changes = 0;
	auto valueConnection =
		picker->connect( Event::OnValueChange, [&changes]( const Event* ) { ++changes; } );
	picker->showCalendar();
	auto* time = picker->getPopupTimePicker();
	ASSERT_TRUE( time );
	EXPECT_FALSE( picker->hasValue() );
	EXPECT_EQ( changes, 0 );
	EXPECT_TRUE( time->getMinTime() == ( TimeOfDay{ 9, 30 } ) );
	EXPECT_TRUE( time->getMaxTime() == ( TimeOfDay{ 17, 0 } ) );
	time->setActiveSection( DateTimeSection::Minute );
	time->stepActiveSection( 1 );
	EXPECT_TRUE( picker->getDateTime() == ( LocalDateTime{ { 2026, 9, 28 }, { 9, 31 } } ) );
	EXPECT_EQ( changes, 1 );
	time->setTime( TimeOfDay{ 23, 0 } );
	EXPECT_TRUE( picker->getDateTime()->time == ( TimeOfDay{ 17, 0 } ) );
	picker->setMaxDateTime( LocalDateTime{ { 2026, 9, 29 }, { 18, 0 } } );
	picker->getCalendar()->setFocusedDate( CalendarDate{ 2026, 9, 29 } )->selectFocusedDate();
	EXPECT_FALSE( time->getMinTime() );
	EXPECT_TRUE( time->getMaxTime() == ( TimeOfDay{ 18, 0 } ) );
	picker->clear();
	EXPECT_FALSE( picker->hasValue() );
	EXPECT_FALSE( picker->getCalendar()->getSelectedDate() );
	picker->hideCalendar();
	EXPECT_FALSE( picker->hasValue() );
	eeDelete( picker );
}

UTEST( UIDateTimePicker, FirstOpeningFitsStyledTimeFooter ) {
	for ( Float density : { 1.f, 1.5f, 2.f } ) {
		for ( int theme = 0; theme < 3; ++theme ) {
			UIApplication app(
				WindowSettings{ 1020, 720, "eepp - date time popup layout tests" },
				UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
										 density ) );
			auto* ui = app.getUI();
			ui->getUIThemeManager()->setDefaultEffectsEnabled( false );
			ui->setColorSchemePreference( theme == 1 ? ColorSchemePreference::Light
													 : ColorSchemePreference::Dark );
			std::string css;
			ASSERT_TRUE( FileSystem::fileGet( Sys::getProcessPath() + "../assets/ui/" +
												  ( theme == 2 ? "uitheme.css" : "breeze.css" ),
											  css ) );
			ui->setStyleSheet( css );
			auto* picker = UIDateTimePicker::New();
			picker->setParent( ui->getRoot() );
			picker->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
			picker->setSize( 320, 30 );
			picker->setPosition( 16, PixelDensity::pxToDp( 298.f ) );
			picker->setShowMilliseconds( true );
			picker->setDateTime( LocalDateTime{ { 2026, 9, 28 }, { 20, 14, 32, 125 } } );
			ui->update( Time::Zero );
			picker->showCalendar();
			ui->update( Time::Zero );
			auto* time = picker->getPopupTimePicker();
			ASSERT_TRUE( time );
			auto* popup = time->getParent();
			const auto verifyBounds = [&]() {
				EXPECT_LE( popup->getScreenRect().Bottom, ui->getWorldBounds().Bottom );
				EXPECT_GE( popup->getScreenRect().Top, ui->getWorldBounds().Top );
				EXPECT_LE( time->getScreenRect().Bottom, popup->getScreenRect().Bottom );
				EXPECT_EQ( time->getScreenRect().Top,
						   picker->getCalendar()->getScreenRect().Bottom );
				EXPECT_GE( time->getSize().getHeight(), 72.f );
				for ( auto* arrow : time->querySelectorAll( "DateTimePicker::time-down" ) ) {
					if ( arrow->isVisible() )
						EXPECT_LE( arrow->getScreenRect().Bottom, popup->getScreenRect().Bottom );
				}
			};
			verifyBounds();
			// Changing style metrics must resize and realign without stepping a time section.
			time->applyProperty( StyleSheetProperty( "font-size", "24dp" ) );
			time->setPaddingTop( 30 );
			time->setPaddingBottom( 30 );
			verifyBounds();
			eeDelete( picker );
		}
	}
}

UTEST( UIDateTimePicker, PopupFormatFocusPlacementAndLifetime ) {
	auto app = testApp();
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* picker = UIDateTimePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	picker->setSize( 200, 30 );
	picker->setPosition( 430, 440 );
	picker->setLocale( DateTimeLocale::fromLocaleName( "en_US" ) );
	picker->setDateTime( LocalDateTime{ { 2026, 9, 28 }, { 20, 14 } } );
	picker->showCalendar();
	app.getUI()->update( Time::Zero );
	auto* time = picker->getPopupTimePicker();
	ASSERT_TRUE( time );
	EXPECT_TRUE( time->getText().toUtf8() == "08:14 PM" );
	auto* popup = time->getParent();
	EXPECT_LE( popup->getScreenRect().Bottom, picker->getScreenRect().Top );
	EXPECT_LE( popup->getScreenRect().Right, app.getUI()->getWorldBounds().Right );
	auto* amPm = time->querySelector( "datetimepicker::time-up.am-pm" );
	ASSERT_TRUE( amPm && amPm->isVisible() );
	time->setFocus();
	EXPECT_TRUE( picker->isCalendarVisible() );
	const auto value = picker->getDateTime();
	time->setActiveSection( DateTimeSection::Minute );
	app.getUI()->getEventDispatcher()->sendTextInput( '1', 0 );
	app.getUI()->getEventDispatcher()->sendKeyDown( KEY_ESCAPE, SCANCODE_UNKNOWN, 0, 0 );
	EXPECT_FALSE( picker->isCalendarVisible() );
	EXPECT_TRUE( picker->getDateTime() == value );
	picker->showCalendar();
	picker->getCalendar()->close();
	app.getUI()->update( Time::Zero );
	EXPECT_TRUE( picker->getCalendar() == nullptr );
	EXPECT_FALSE( picker->isCalendarVisible() );
	picker->showCalendar();
	EXPECT_TRUE( picker->getCalendar()->getParent() == picker->getPopupTimePicker()->getParent() );
	picker->getPopupTimePicker()->close();
	app.getUI()->update( Time::Zero );
	EXPECT_TRUE( picker->getPopupTimePicker() == nullptr );
	picker->showCalendar();
	ASSERT_TRUE( picker->getPopupTimePicker() );
	auto* other = UITextInput::New();
	other->setParent( app.getUI()->getRoot() );
	picker->getPopupTimePicker()->setFocus();
	other->setFocus();
	EXPECT_FALSE( picker->isCalendarVisible() );
	picker->showCalendar();
	picker->setEnabled( false );
	EXPECT_FALSE( picker->isCalendarVisible() );
	eeDelete( picker );
	eeDelete( other );
}

UTEST( DateTimeWidgets, NativeXmlRegistrationAndTypedValueProperties ) {
	auto app = testApp();

	auto* root = app.getUI()->loadLayoutFromString( R"xml(
	<vbox>
		<datepicker id="date" value="1990-06-17" />
		<timepicker id="time" value="20:30:00" minute-step="15" />
		<datetimepicker id="datetime" value="2026-09-28T20:30:00" />
		<datetimeedit id="edit" mode="time" value="12:00:00" />
		<calendar id="calendar" first-day-of-week="1" value="2026-09-28" />
	</vbox>)xml" );
	ASSERT_TRUE( root );
	auto* date = root->querySelector( "#date" );

	auto* time = root->querySelector( "#time" );
	auto* datetime = root->querySelector( "#datetime" );
	ASSERT_TRUE( date && time && datetime );
	EXPECT_TRUE( date->isType( UI_TYPE_DATEPICKER ) );
	EXPECT_TRUE( date->isType( UI_TYPE_DATETIMEEDIT ) );
	EXPECT_TRUE( date->isType( UI_TYPE_TEXTINPUT ) );
	EXPECT_TRUE( time->isType( UI_TYPE_TIMEPICKER ) );
	EXPECT_TRUE( datetime->isType( UI_TYPE_DATETIMEPICKER ) );

	const auto* value = StyleSheetSpecification::instance()->getProperty( "value" );
	EXPECT_TRUE( date->getPropertyString( value ) == "1990-06-17" );
	EXPECT_TRUE( time->getPropertyString( value ) == "20:30:00" );
	EXPECT_TRUE( datetime->getPropertyString( value ) == "2026-09-28T20:30:00" );
	EXPECT_TRUE( root->querySelector( "#edit" )->getPropertyString( value ) == "12:00:00" );
	eeDelete( root );
}

UTEST( DateTimeWidgets, BindingUsesIsoAndPreservesHiddenPrecision ) {
	auto app = testApp();
	auto* picker = UIDateTimePicker::New();
	picker->setLocale( DateTimeLocale::fromLocaleName( "es_AR" ) );
	{
		UIProperty<std::string> value( std::string( "2026-09-28T20:14:32.125" ), picker );
		EXPECT_TRUE( picker->getSerializedValue() == "2026-09-28T20:14:32.125" );
		picker->setDateTime(
			LocalDateTime{ CalendarDate{ 2027, 1, 31 }, TimeOfDay{ 9, 15, 12, 250 } } );
		EXPECT_TRUE( value.value() == "2027-01-31T09:15:12.250" );
		picker->setDateFormat( "MM/dd/yyyy" );
		EXPECT_TRUE( value.value() == "2027-01-31T09:15:12.250" );
		value = std::string( "2028-02-29T23:59:59.999" );
		EXPECT_TRUE( picker->getDateTime()->date == ( CalendarDate{ 2028, 2, 29 } ) );
		picker->clear();
		EXPECT_TRUE( value.value().empty() );
	}
	eeDelete( picker );
}

UTEST( UICalendar, InvalidBoundsPreserveConstraintsAndYearPagesStayRepresentable ) {
	auto app = testApp();
	auto* calendar = UICalendar::New();
	calendar->setMinDate( CalendarDate{ 2026, 1, 1 } );
	EXPECT_FALSE( calendar->applyProperty( StyleSheetProperty( "min-date", "2026-02-31" ) ) );
	EXPECT_TRUE( calendar->getMinDate() == ( CalendarDate{ 2026, 1, 1 } ) );
	calendar->setMinDate( std::nullopt );
	for ( const Int32 year :
		  { std::numeric_limits<Int32>::min(), std::numeric_limits<Int32>::max() } ) {
		calendar->setDisplayedMonth( CalendarDate{ year, 6, 1 } );
		calendar->setView( CalendarView::Years );
		if ( year == std::numeric_limits<Int32>::min() )
			calendar->previous()->previous();
		else
			calendar->next()->next();
		EXPECT_TRUE( !calendar->querySelectorAll( "calendar::day:disabled" ).empty() );
		calendar->setView( CalendarView::Days );
	}
	eeDelete( calendar );
}

UTEST( UIDateTimeEdit, ModeChangePreservesNonemptyPolicy ) {
	auto app = testApp();
	auto* edit = UIDateTimeEdit::New();
	edit->setAllowEmpty( false );
	edit->setEditMode( DateTimeEditMode::Time );
	EXPECT_TRUE( edit->getTime().has_value() );
	edit->setEditMode( DateTimeEditMode::DateTime );
	EXPECT_TRUE( edit->getDateTime().has_value() );
	eeDelete( edit );
}

UTEST( UICalendar, DefaultSizeTracksFontMetricsAndFixedSizeRemainsExplicit ) {
	auto app = testApp();
	auto* calendar = UICalendar::New();
	calendar->setParent( app.getUI()->getRoot() );
	app.getUI()->update( Milliseconds( 16 ) );
	const auto initialSize = calendar->getSize();
	for ( const char* tag :
		  { "calendar::day::text", "calendar::title::text", "calendar::today::text" } ) {
		for ( auto* text : calendar->querySelectorAll( tag ) )
			text->asType<UITextView>()->setFontSize( PixelDensity::dpToPx( 24 ) );
	}
	for ( auto* text : calendar->querySelectorAll( "calendar::weekday" ) )
		text->asType<UITextView>()->setFontSize( PixelDensity::dpToPx( 24 ) );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_GT( calendar->getSize().getWidth(), initialSize.getWidth() );
	EXPECT_GT( calendar->getSize().getHeight(), initialSize.getHeight() );
	for ( auto* widget : calendar->querySelectorAll( "calendar::weekday" ) ) {
		auto* label = widget->asType<UITextView>();
		EXPECT_GE( label->getPixelsSize().getWidth(), label->getTextWidth() );
		EXPECT_GE( label->getPixelsSize().getHeight(), label->getTextHeight() );
	}
	calendar->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	calendar->setSize( 500, 500 );
	calendar->setDisplayedMonth( CalendarDate{ 2026, 9, 1 } );
	EXPECT_TRUE( calendar->getSize() == Sizef( 500, 500 ) );
	eeDelete( calendar );
}

UTEST( UICalendar, FractionalGridWidthsKeepSevenColumnsAndSixRows ) {
	auto app = testApp();
	auto* calendar = UICalendar::New();
	calendar->setParent( app.getUI()->getRoot() );
	calendar->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	for ( const Float width : { 240.f, 320.f, 480.f, 481.f } ) {
		calendar->setSize( width, 480 );
		app.getUI()->update( Milliseconds( 16 ) );
		auto cells = calendar->querySelectorAll( "calendar::day" );
		ASSERT_GE( cells.size(), 42u );
		const Float top = cells[0]->getPosition().y;
		for ( size_t i = 1; i < 7; ++i )
			EXPECT_EQ( cells[i]->getPosition().y, top );
		EXPECT_GT( cells[7]->getPosition().y, top );
		EXPECT_EQ( cells[35]->getPosition().y, cells[41]->getPosition().y );
		const auto* grid = cells[0]->getParent();
		EXPECT_LE( cells[41]->getPosition().y + cells[41]->getSize().getHeight(),
				   grid->getSize().getHeight() + 0.01f );
	}
	eeDelete( calendar );
}

UTEST( UIDateTimeEdit, ClickSelectsNearestSectionIndependentOfExistingSelection ) {
	auto app = testApp();
	auto* edit = TestableDateTimeEdit::New( DateTimeEditMode::Date );
	edit->setParent( app.getUI()->getRoot() );
	edit->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	edit->setSize( 400, 40 );
	edit->setPosition( 30, 50 );
	edit->setDateFormat( "dd/MM/yyyy" );
	edit->setDate( CalendarDate{ 2026, 9, 28 } );
	app.getUI()->update( Milliseconds( 16 ) );

	edit->setActiveSection( DateTimeSection::Day );
	edit->clickAt( { 200, 20 } );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Year );
	EXPECT_EQ( edit->getDocument().getSelection().start().column(), 6 );
	EXPECT_EQ( edit->getDocument().getSelection().end().column(), 10 );

	edit->clickAt( { 0, 20 } );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Day );
	edit->clickAt( edit->sectionClickPosition( 4 ) );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Month );
	edit->clickAt( edit->sectionClickPosition( 8 ) );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Year );
	edit->clickAt( edit->sectionClickPosition( 1 ) );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Day );

	edit->setEditMode( DateTimeEditMode::Time );
	edit->setTimeFormat( "HH:mm:ss" );
	edit->setTime( TimeOfDay{ 20, 14, 32 } );
	edit->setActiveSection( DateTimeSection::Hour );
	edit->clickAt( { 200, 20 } );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Second );
	edit->clickAt( { 0, 20 } );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Hour );

	edit->setEditMode( DateTimeEditMode::DateTime );
	edit->setTimeFormat( "HH:mm:ss.SSS" );
	edit->setShowMilliseconds( true );
	edit->setDateTime( LocalDateTime{ CalendarDate{ 2026, 9, 28 }, TimeOfDay{ 20, 14, 32, 125 } } );
	edit->setActiveSection( DateTimeSection::Day );
	edit->clickAt( { 380, 20 } );
	EXPECT_TRUE( edit->getActiveSection() == DateTimeSection::Millisecond );
	eeDelete( edit );
}

UTEST( UIDateTimePicker, LivePopupConfiguration ) {
	auto app = testApp();
	auto* picker = UIDateTimePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setDateTime( LocalDateTime{ { 2026, 9, 28 }, { 20, 14 } } );
	picker->showCalendar();
	auto* time = picker->getPopupTimePicker();
	ASSERT_TRUE( time );
	picker->setHourStep( 2 );
	picker->setMinuteStep( 15 );
	picker->setSecondStep( 10 );
	picker->setPendingDigitTimeout( Milliseconds( 250 ) );
	EXPECT_EQ( time->getHourStep(), 2u );
	EXPECT_EQ( time->getMinuteStep(), 15u );
	EXPECT_EQ( time->getSecondStep(), 10u );
	EXPECT_TRUE( time->getPendingDigitTimeout() == Milliseconds( 250 ) );
	picker->setShowSeconds( true );
	EXPECT_TRUE( time->getShowSeconds() );
	picker->setShowMilliseconds( true );
	EXPECT_TRUE( time->getShowMilliseconds() );
	auto locale = DateTimeLocale::fromLocaleName( "en_US" );
	locale.monthNames[8] = "Localized September";
	picker->setLocale( locale );
	EXPECT_TRUE( picker->getCalendar()->getLocale().monthNames[8] == "Localized September" );
	EXPECT_TRUE( time->getHourCycle() == picker->getHourCycle() );
	time->setActiveSection( DateTimeSection::Minute );
	time->stepActiveSection( 1 );
	EXPECT_EQ( picker->getDateTime()->time.minute, 29 );
	// Test the nonvirtual base API too: read-only changes cannot bypass popup synchronization.
	static_cast<UITextInput*>( picker )->setAllowEditing( false );
	EXPECT_FALSE( picker->isCalendarVisible() );
	EXPECT_FALSE( time->isEditingAllowed() );
	picker->setAllowEditing( true );
	picker->showCalendar();
	EXPECT_TRUE( picker->isCalendarVisible() );
	EXPECT_TRUE( time->isEditingAllowed() );
	eeDelete( picker );
}

UTEST( UIDateTimePicker, EmptyPopupNavigationUpdatesBoundaryTime ) {
	auto app = testApp();
	auto* picker = UIDateTimePicker::New();
	picker->setParent( app.getUI()->getRoot() );
	picker->setMinDateTime( LocalDateTime{ { 2026, 9, 28 }, { 10, 30 } } );
	picker->setMaxDateTime( LocalDateTime{ { 2026, 9, 30 }, { 18, 45 } } );
	picker->showCalendar();
	picker->getCalendar()->setFocusedDate( { 2026, 9, 28 } );
	EXPECT_TRUE( picker->getPopupTimePicker()->getMinTime() == ( TimeOfDay{ 10, 30 } ) );
	picker->getCalendar()->setFocusedDate( { 2026, 9, 29 } );
	EXPECT_FALSE( picker->getPopupTimePicker()->getMinTime() );
	picker->getCalendar()->setFocusedDate( { 2026, 9, 30 } );
	EXPECT_TRUE( picker->getPopupTimePicker()->getMaxTime() == ( TimeOfDay{ 18, 45 } ) );
	EXPECT_FALSE( picker->hasValue() );
	eeDelete( picker );
}

UTEST( UIDatePicker, PopupTracksAncestorMovementAndVisibleSceneBounds ) {
	UIApplication app(
		WindowSettings{ 1020,
						900,
						"Picker movement tests",
						WindowStyle::Default,
						WindowBackend::Default,
						32,
						{},
						1,
						false,
						true },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	for ( const float density : { 1.f, 1.5f, 2.f } ) {
		PixelDensity::setPixelDensity( density );
		auto* container = UIWidget::New();
		container->setParent( app.getUI()->getRoot() );
		container->setPosition( 20, 20 );
		auto* picker = UIDatePicker::New();
		picker->setParent( container );
		picker->setSize( 150, 24 );
		picker->setPosition( 10, 10 );
		picker->applyProperty( StyleSheetProperty( "popup-to-root", "true" ) );
		picker->showCalendar();
		const Vector2f before = picker->getCalendar()->getScreenRect().getPosition();
		container->setPosition( 40, 50 );
		picker
			->getScreenRect(); // Screen-position invalidation also covers scroll-container motion.
		const Vector2f after = picker->getCalendar()->getScreenRect().getPosition();
		EXPECT_NEAR( after.x - before.x, 20 * density, 0.1f );
		EXPECT_NEAR( after.y - before.y, 30 * density, 0.1f );
		picker->applyProperty( StyleSheetProperty( "popup-to-root", "false" ) );
		EXPECT_TRUE( picker->isCalendarVisible() );
		container->setPosition( 500 / density, 400 / density );
		picker->getScreenRect();
		EXPECT_TRUE( app.getUI()->getVisibleWorldBounds().contains(
			picker->getCalendar()->getScreenRect() ) );
		eeDelete( container );
	}
	PixelDensity::setPixelDensity( 1 );
}

UTEST( DateTimeWidgets, BuiltInPropertyAndShorthandInventoryIsComplete ) {
	auto* spec = StyleSheetSpecification::instance();
	for ( Uint16 id = 1; id < static_cast<Uint16>( PropertyId::NumDefinedIds ); ++id )
		EXPECT_TRUE( spec->getProperty( static_cast<PropertyId>( id ) ) != nullptr );
	for ( Uint8 id = 1; id < static_cast<Uint8>( ShorthandId::NumDefinedIds ); ++id )
		EXPECT_TRUE( spec->getShorthand( static_cast<ShorthandId>( id ) ) != nullptr );
}

UTEST( UIDatePicker, PopupTracksScrollingAndMovingWindows ) {
	UIApplication app(
		WindowSettings{ 1020,
						900,
						"Picker scrolling tests",
						WindowStyle::Default,
						WindowBackend::Default,
						32,
						{},
						1,
						false,
						true },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* ui = app.getUI();
	ui->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* window = UIWindow::New();
	window->setParent( ui->getRoot() );
	window->setSize( 400, 350 );
	window->setPosition( 50, 40 );
	auto* scroll = UIScrollView::New();
	scroll->setParent( window->getContainer() );
	scroll->setSize( 350, 250 );
	auto* content = UIWidget::New();
	content->setSize( 320, 800 );
	content->setParent( scroll );
	auto* picker = UIDatePicker::New();
	picker->setParent( content );
	picker->setSize( 200, 30 );
	picker->setPosition( 20, 120 );
	ui->flushDirtyStyleAndLayout();
	ASSERT_TRUE( scroll->getVerticalScrollBar()->isEnabled() );
	for ( bool toRoot : { false, true } ) {
		picker->applyProperty( StyleSheetProperty( "popup-to-root", toRoot ? "true" : "false" ) );
		picker->showCalendar();
		ui->flushDirtyStyleAndLayout();
		EXPECT_TRUE( picker->getCalendar()->getParent() ==
					 ( toRoot ? static_cast<Node*>( ui->getRoot() ) : window->getContainer() ) );
		const auto before = picker->getCalendar()->getScreenRect();
		const auto fieldBefore = picker->getScreenRect();
		scroll->getVerticalScrollBar()->setValue( toRoot ? 0.1f : 0.05f );
		const auto fieldAfter = picker->getScreenRect();
		const auto after = picker->getCalendar()->getScreenRect();
		EXPECT_LT( fieldAfter.Top, fieldBefore.Top );
		EXPECT_NEAR( after.Top - before.Top, fieldAfter.Top - fieldBefore.Top, 0.1f );
		window->setPosition( window->getPosition() + Vector2f( 30, 20 ) );
		picker->getScreenRect();
		EXPECT_NEAR( picker->getCalendar()->getScreenRect().Left - after.Left, 30.f, 0.1f );
		EXPECT_NEAR( picker->getCalendar()->getScreenRect().Top - after.Top, 20.f, 0.1f );
		picker->hideCalendar();
	}
	ui->setPixelsSize( 300, 250 );
	picker->showCalendar();
	EXPECT_TRUE( ui->getVisibleWorldBounds().contains( picker->getCalendar()->getScreenRect() ) );
}
