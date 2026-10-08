#include <algorithm>
#include <eepp/system/datetimeformat.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uicalendar.hpp>
#include <eepp/ui/uigridlayout.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextview.hpp>
#include <limits>

namespace EE { namespace UI {

using namespace System;
using namespace CSS;

namespace {

std::optional<CalendarDate> parseISODateProperty( const StyleSheetProperty& property ) {
	DateTimeFormat iso( "yyyy-MM-dd" );
	return DateTimeFormatter::parseDate( property.getValue(), iso );
}

std::string yearText( Int64 year ) {
	return String::toString( year );
}

} // namespace

UICalendar* UICalendar::New() {
	return eeNew( UICalendar, () );
}

UICalendar::UICalendar() : UIWidget( "calendar" ), mLocale( DateTimeLocale::system() ) {
	mDisplayedMonth = CalendarDate::today();
	if ( !mDisplayedMonth.isValid() )
		mDisplayedMonth = { 1970, 1, 1 };
	mDisplayedMonth.day = 1;
	mFocusedDate = CalendarDate::today();
	if ( !mFocusedDate.isValid() )
		mFocusedDate = mDisplayedMonth;
	mYearPageStart =
		static_cast<Int64>( mDisplayedMonth.year ) - ( ( mDisplayedMonth.year % 12 + 12 ) % 12 );
	setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
	setSize( Sizef( 280, 276 ) );
	buildUI();
	refresh();
}

Uint32 UICalendar::getType() const {
	return UI_TYPE_CALENDAR;
}

bool UICalendar::isType( const Uint32& type ) const {
	return getType() == type ? true : UIWidget::isType( type );
}

UICalendar* UICalendar::setSelectedDate( const CalendarDate& date ) {
	return date.isValid() ? setSelectedDate( std::optional<CalendarDate>{ date } ) : this;
}

UICalendar* UICalendar::setSelectedDate( std::optional<CalendarDate> date ) {
	if ( date && !date->isValid() )
		return this;
	if ( date )
		date = clampDate( *date );
	const bool changed = mSelectedDate != date;
	mSelectedDate = date;
	if ( date ) {
		mFocusedDate = *date;
		mDisplayedMonth = { date->year, date->month, 1 };
	}
	refresh();
	if ( changed )
		onValueChange();
	return this;
}

const std::optional<CalendarDate>& UICalendar::getSelectedDate() const {
	return mSelectedDate;
}

UICalendar* UICalendar::clearSelection() {
	return setSelectedDate( std::nullopt );
}

UICalendar* UICalendar::setDisplayedMonth( const CalendarDate& date ) {
	if ( !date.isValid() )
		return this;
	mDisplayedMonth = { date.year, date.month, 1 };
	mYearPageStart =
		static_cast<Int64>( mDisplayedMonth.year ) - ( ( mDisplayedMonth.year % 12 + 12 ) % 12 );
	refresh();
	return this;
}

const CalendarDate& UICalendar::getDisplayedMonth() const {
	return mDisplayedMonth;
}

UICalendar* UICalendar::setFocusedDate( const CalendarDate& date ) {
	if ( !date.isValid() )
		return this;
	setFocusedAndDisplay( clampDate( date ) );
	return this;
}

const CalendarDate& UICalendar::getFocusedDate() const {
	return mFocusedDate;
}

UICalendar* UICalendar::setMinDate( std::optional<CalendarDate> date ) {
	if ( date && !date->isValid() )
		return this;
	mMinDate = date;
	if ( mMaxDate && mMinDate && *mMinDate > *mMaxDate )
		mMaxDate = mMinDate;
	mFocusedDate = clampDate( mFocusedDate );
	if ( mSelectedDate )
		setSelectedDate( clampDate( *mSelectedDate ) );
	else
		setFocusedAndDisplay( mFocusedDate );
	return this;
}

UICalendar* UICalendar::setMaxDate( std::optional<CalendarDate> date ) {
	if ( date && !date->isValid() )
		return this;
	mMaxDate = date;
	if ( mMinDate && mMaxDate && *mMaxDate < *mMinDate )
		mMinDate = mMaxDate;
	mFocusedDate = clampDate( mFocusedDate );
	if ( mSelectedDate )
		setSelectedDate( clampDate( *mSelectedDate ) );
	else
		setFocusedAndDisplay( mFocusedDate );
	return this;
}

const std::optional<CalendarDate>& UICalendar::getMinDate() const {
	return mMinDate;
}

const std::optional<CalendarDate>& UICalendar::getMaxDate() const {
	return mMaxDate;
}

UICalendar* UICalendar::setView( CalendarView view ) {
	if ( mView == view )
		return this;
	mView = view;
	if ( view == CalendarView::Years )
		mYearPageStart = static_cast<Int64>( mDisplayedMonth.year ) -
						 ( ( mDisplayedMonth.year % 12 + 12 ) % 12 );
	refresh();
	return this;
}

CalendarView UICalendar::getView() const {
	return mView;
}

UICalendar* UICalendar::setLocale( const DateTimeLocale& locale ) {
	mLocale = locale;
	mLocale.firstDayOfWeek %= 7;
	refresh();
	return this;
}

const DateTimeLocale& UICalendar::getLocale() const {
	return mLocale;
}

UICalendar* UICalendar::setFirstDayOfWeek( Uint8 weekday ) {
	mLocale.firstDayOfWeek = weekday % 7;
	refresh();
	return this;
}

Uint8 UICalendar::getFirstDayOfWeek() const {
	return mLocale.firstDayOfWeek;
}

UICalendar* UICalendar::previous() {
	switch ( mView ) {
		case CalendarView::Days:
			mDisplayedMonth = mDisplayedMonth.addMonths( -1 );
			break;
		case CalendarView::Months:
			mDisplayedMonth = mDisplayedMonth.addYears( -1 );
			break;
		case CalendarView::Years:
			if ( mYearPageStart > std::numeric_limits<Int32>::min() )
				mYearPageStart -= 12;
			break;
	}
	refresh();
	return this;
}

UICalendar* UICalendar::next() {
	switch ( mView ) {
		case CalendarView::Days:
			mDisplayedMonth = mDisplayedMonth.addMonths( 1 );
			break;
		case CalendarView::Months:
			mDisplayedMonth = mDisplayedMonth.addYears( 1 );
			break;
		case CalendarView::Years:
			if ( mYearPageStart + 11 < std::numeric_limits<Int32>::max() )
				mYearPageStart += 12;
			break;
	}
	refresh();
	return this;
}

UICalendar* UICalendar::goToToday() {
	CalendarDate today = CalendarDate::today();
	if ( today.isValid() )
		setFocusedAndDisplay( clampDate( today ) );
	return this;
}

bool UICalendar::selectFocusedDate() {
	if ( !isDateEnabled( mFocusedDate ) )
		return false;
	setSelectedDate( mFocusedDate );
	// Explicit selection is distinct from a changed value (reselecting a day closes a picker).
	sendCommonEvent( Event::OnItemSelected );
	return true;
}

CalendarDate UICalendar::getFirstVisibleDate() const {
	CalendarDate first{ mDisplayedMonth.year, mDisplayedMonth.month, 1 };
	const int firstDow = dayOfWeek( first );
	const int offset = ( firstDow - static_cast<int>( mLocale.firstDayOfWeek ) + 7 ) % 7;
	return first.addDays( -offset );
}

CalendarDate UICalendar::getDayCellDate( size_t index ) const {
	return index < mDayCellDates.size() ? mDayCellDates[index] : CalendarDate{};
}

void UICalendar::buildUI() {
	mMainLayout = UILinearLayout::NewVertical();
	mMainLayout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent )
		->setParent( this );

	mHeader = UILinearLayout::NewWithTag( "calendar::header", UIOrientation::Horizontal );
	mHeader->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed )
		->setSize( Sizef( 0, 34 ) )
		->setParent( mMainLayout );

	mPreviousButton = UIPushButton::NewWithTag( "calendar::previous" );
	mPreviousButton->unsetFlags( UI_AUTO_SIZE );
	mPreviousButton->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::MatchParent )
		->setSize( Sizef( 34, 0 ) )
		->setParent( mHeader );
	mConnections += mPreviousButton->connect( Event::MouseClick, [this]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
			return;

		previous();
		setFocus();
	} );

	mTitleButton = UIPushButton::NewWithTag( "calendar::title" );
	mTitleButton->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::MatchParent )
		->setLayoutWeight( 1 )
		->setParent( mHeader );
	mConnections += mTitleButton->connect( Event::MouseClick, [this]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
			return;

		if ( mView == CalendarView::Days )
			setView( CalendarView::Months );
		else if ( mView == CalendarView::Months )
			setView( CalendarView::Years );
		setFocus();
	} );

	mNextButton = UIPushButton::NewWithTag( "calendar::next" );
	mNextButton->unsetFlags( UI_AUTO_SIZE );
	mNextButton->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::MatchParent )
		->setSize( Sizef( 34, 0 ) )
		->setParent( mHeader );
	mConnections += mNextButton->connect( Event::MouseClick, [this]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
			return;

		next();
		setFocus();
	} );

	mWeekdayRow = UIGridLayout::New();
	mWeekdayRow->setElementTag( "calendar::weekdays" );
	mWeekdayRow->setColumnWeight( 1.f / 7.f )->setRowMode( UIGridLayout::Size )->setRowHeight( 24 );
	mWeekdayRow->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed )
		->setSize( Sizef( 0, 24 ) )
		->setParent( mMainLayout );
	for ( size_t i = 0; i < mWeekdayLabels.size(); ++i ) {
		auto* label = UITextView::New();
		label->setElementTag( "calendar::weekday" );
		label->setTextAlign( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
		label->setEnabled( false );
		label->setParent( mWeekdayRow );
		mWeekdayLabels[i] = label;
	}

	mDayGrid = UIGridLayout::New();
	mDayGrid->setElementTag( "calendar::grid" );
	mDayGrid->setColumnWeight( 1.f / 7.f )->setRowWeight( 1.f / 6.f );
	mDayGrid->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed )
		->setSize( Sizef( 0, 180 ) )
		->setParent( mMainLayout );
	for ( size_t i = 0; i < mDayCells.size(); ++i ) {
		auto* button = UIPushButton::NewWithTag( "calendar::day" );
		button->setTextAlign( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
		button->setParent( mDayGrid );
		mConnections += button->connect( Event::KeyDown, [this]( const Event* event ) {
			const auto* key = event->asKeyEvent();
			if ( key->getKeyCode() != KEY_RETURN && key->getKeyCode() != KEY_KP_ENTER )
				onKeyDown( *key );
		} );
		mConnections += button->connect( Event::MouseClick, [this, i]( const Event* event ) {
			if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
				return;
			handleDayClicked( i );
		} );
		mDayCells[i] = button;
	}

	mOptionGrid = UIGridLayout::New();
	mOptionGrid->setElementTag( "calendar::grid" );
	mOptionGrid->setColumnWeight( 0.25f )->setRowWeight( 1.f / 3.f );
	mOptionGrid->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed )
		->setSize( Sizef( 0, 180 ) )
		->setParent( mMainLayout );
	for ( size_t i = 0; i < mOptionCells.size(); ++i ) {
		auto* button = UIPushButton::NewWithTag( "calendar::day" );
		button->setTextAlign( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
		button->setParent( mOptionGrid );
		mConnections += button->connect( Event::MouseClick, [this, i]( const Event* event ) {
			if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
				return;
			handleOptionClicked( i );
		} );
		mOptionCells[i] = button;
	}

	mTodayButton = UIPushButton::NewWithTag( "calendar::today" );
	mTodayButton->setText( "Today" )
		->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed )
		->setSize( Sizef( 0, 32 ) )
		->setParent( mMainLayout );
	for ( auto* button : { mPreviousButton, mTitleButton, mNextButton, mTodayButton } ) {
		mConnections += button->connect( Event::KeyDown, [this]( const Event* event ) {
			if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
				sendEvent( event );
		} );
	}
	for ( auto* button : mOptionCells ) {
		mConnections += button->connect( Event::KeyDown, [this]( const Event* event ) {
			if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
				sendEvent( event );
		} );
	}
	mConnections += mTodayButton->connect( Event::MouseClick, [this]( const Event* event ) {
		if ( !( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) )
			return;

		goToToday();
		setView( CalendarView::Days );
		setFocus();
	} );
	// Text metrics change with fonts, density, and styles; resize only on those notifications.
	auto watchFont = [this]( UITextView* text ) {
		mConnections +=
			text->connect( Event::OnFontChanged, [this]( const Event* ) { updateLayoutSize(); } );
		mConnections += text->connect( Event::OnFontStyleChanged,
									   [this]( const Event* ) { updateLayoutSize(); } );
	};
	for ( auto* label : mWeekdayLabels )
		watchFont( label );
	for ( auto* button : { mPreviousButton, mTitleButton, mNextButton, mTodayButton } )
		watchFont( button->getTextView() );
	for ( auto* button : mDayCells )
		watchFont( button->getTextView() );
	for ( auto* button : mOptionCells )
		watchFont( button->getTextView() );
}

void UICalendar::refresh() {
	refreshHeader();
	if ( mView == CalendarView::Days )
		refreshDays();
	else if ( mView == CalendarView::Months )
		refreshMonths();
	else
		refreshYears();
	updateLayoutSize();
}

void UICalendar::refreshHeader() {
	if ( !mTitleButton )
		return;
	if ( mView == CalendarView::Days ) {
		std::string title =
			mLocale.monthNames[mDisplayedMonth.month - 1] + " " + yearText( mDisplayedMonth.year );
		mTitleButton->setText( title );
	} else if ( mView == CalendarView::Months ) {
		mTitleButton->setText( yearText( mDisplayedMonth.year ) );
	} else {
		mTitleButton->setText( yearText( mYearPageStart ) + " - " +
							   yearText( mYearPageStart + 11 ) );
	}
}

void UICalendar::refreshDays() {
	mWeekdayRow->setVisible( true );
	mDayGrid->setVisible( true );
	mOptionGrid->setVisible( false );
	const CalendarDate today = CalendarDate::today();
	for ( size_t i = 0; i < 7; ++i ) {
		const size_t weekday = ( mLocale.firstDayOfWeek + i ) % 7;
		mWeekdayLabels[i]->setText( mLocale.shortWeekdayNames[weekday] );
	}
	CalendarDate date = getFirstVisibleDate();
	for ( size_t i = 0; i < mDayCells.size(); ++i, date = date.addDays( 1 ) ) {
		mDayCellDates[i] = date;
		auto* cell = mDayCells[i];
		cell->setText( String::toString( static_cast<int>( date.day ) ) );
		cell->setEnabled( isDateEnabled( date ) );
		updateCellClasses( cell, mSelectedDate && *mSelectedDate == date,
						   today.isValid() && today == date,
						   date.month != mDisplayedMonth.month || date.year != mDisplayedMonth.year,
						   mFocusedDate == date );
	}
}

void UICalendar::refreshMonths() {
	mWeekdayRow->setVisible( false );
	mDayGrid->setVisible( false );
	mOptionGrid->setVisible( true );
	for ( size_t i = 0; i < mOptionCells.size(); ++i ) {
		auto* cell = mOptionCells[i];
		cell->setText( mLocale.shortMonthNames[i] );
		const CalendarDate first{ mDisplayedMonth.year, static_cast<Uint8>( i + 1 ), 1 };
		const CalendarDate last{ mDisplayedMonth.year, static_cast<Uint8>( i + 1 ),
								 static_cast<Uint8>( CalendarDate::daysInMonth(
									 mDisplayedMonth.year, static_cast<Uint8>( i + 1 ) ) ) };
		const bool enabled =
			!( mMaxDate && first > *mMaxDate ) && !( mMinDate && last < *mMinDate );
		cell->setEnabled( enabled );
		updateCellClasses( cell, mDisplayedMonth.month == i + 1, false, false, false );
	}
}

void UICalendar::refreshYears() {
	mWeekdayRow->setVisible( false );
	mDayGrid->setVisible( false );
	mOptionGrid->setVisible( true );
	for ( size_t i = 0; i < mOptionCells.size(); ++i ) {
		const Int64 year = mYearPageStart + static_cast<Int64>( i );
		auto* cell = mOptionCells[i];
		cell->setText( yearText( year ) );
		if ( year < std::numeric_limits<Int32>::min() ||
			 year > std::numeric_limits<Int32>::max() ) {
			cell->setEnabled( false );
			updateCellClasses( cell, false, false, false, false );
			continue;
		}
		const CalendarDate first{ static_cast<Int32>( year ), 1, 1 };
		const CalendarDate last{ static_cast<Int32>( year ), 12, 31 };
		const bool enabled =
			!( mMaxDate && first > *mMaxDate ) && !( mMinDate && last < *mMinDate );
		cell->setEnabled( enabled );
		updateCellClasses( cell, mDisplayedMonth.year == year, false, false, false );
	}
}

void UICalendar::updateCellClasses( UIPushButton* button, bool selected, bool today, bool outside,
									bool focused ) {
	auto toggle = [button]( const char* name, bool active ) {
		if ( button->hasClass( name ) == active )
			return;
		if ( active )
			button->addClass( name );
		else
			button->removeClass( name );
	};
	toggle( "selected", selected );
	toggle( "today", today );
	toggle( "outside-month", outside );
	toggle( "focused", focused );
}

bool UICalendar::isDateEnabled( const CalendarDate& date ) const {
	return date.isValid() && !( mMinDate && date < *mMinDate ) && !( mMaxDate && date > *mMaxDate );
}

CalendarDate UICalendar::clampDate( CalendarDate date ) const {
	if ( mMinDate && date < *mMinDate )
		date = *mMinDate;
	if ( mMaxDate && date > *mMaxDate )
		date = *mMaxDate;
	return date;
}

void UICalendar::moveFocusedDate( Int64 days ) {
	setFocusedAndDisplay( clampDate( mFocusedDate.addDays( days ) ) );
}

void UICalendar::setFocusedAndDisplay( const CalendarDate& date ) {
	if ( !date.isValid() )
		return;
	const bool changed = mFocusedDate != date;
	mFocusedDate = date;
	mDisplayedMonth = { date.year, date.month, 1 };
	refresh();
	if ( changed )
		sendCommonEvent( Event::OnFocusedDateChange );
}

void UICalendar::handleDayClicked( size_t index ) {
	if ( index >= mDayCellDates.size() || !isDateEnabled( mDayCellDates[index] ) )
		return;
	mFocusedDate = mDayCellDates[index];
	setFocus();
	selectFocusedDate();
}

void UICalendar::handleOptionClicked( size_t index ) {
	if ( index >= mOptionCells.size() || !mOptionCells[index]->isEnabled() )
		return;
	setFocus();
	if ( mView == CalendarView::Months ) {
		mDisplayedMonth.month = static_cast<Uint8>( index + 1 );
		mDisplayedMonth.day = 1;
		mFocusedDate = clampDate( mDisplayedMonth );
		setView( CalendarView::Days );
	} else if ( mView == CalendarView::Years ) {
		mDisplayedMonth.year = static_cast<Int32>( mYearPageStart + static_cast<Int64>( index ) );
		mDisplayedMonth.day = 1;
		setView( CalendarView::Months );
	}
}

Uint32 UICalendar::onKeyDown( const KeyEvent& event ) {
	if ( mView == CalendarView::Days ) {
		switch ( event.getKeyCode() ) {
			case KEY_LEFT:
				moveFocusedDate( -1 );
				return 1;
			case KEY_RIGHT:
				moveFocusedDate( 1 );
				return 1;
			case KEY_UP:
				moveFocusedDate( -7 );
				return 1;
			case KEY_DOWN:
				moveFocusedDate( 7 );
				return 1;
			case KEY_HOME: {
				const int delta =
					( dayOfWeek( mFocusedDate ) - static_cast<int>( mLocale.firstDayOfWeek ) + 7 ) %
					7;
				moveFocusedDate( -delta );
				return 1;
			}
			case KEY_END: {
				const int delta =
					( dayOfWeek( mFocusedDate ) - static_cast<int>( mLocale.firstDayOfWeek ) + 7 ) %
					7;
				moveFocusedDate( 6 - delta );
				return 1;
			}
			case KEY_PAGEUP:
				if ( event.getMod() & KEYMOD_CTRL )
					setFocusedAndDisplay( clampDate( mFocusedDate.addYears( -1 ) ) );
				else
					setFocusedAndDisplay( clampDate( mFocusedDate.addMonths( -1 ) ) );
				return 1;
			case KEY_PAGEDOWN:
				if ( event.getMod() & KEYMOD_CTRL )
					setFocusedAndDisplay( clampDate( mFocusedDate.addYears( 1 ) ) );
				else
					setFocusedAndDisplay( clampDate( mFocusedDate.addMonths( 1 ) ) );
				return 1;
			case KEY_RETURN:
			case KEY_KP_ENTER:
				selectFocusedDate();
				return 1;
			default:
				break;
		}
	}
	return UIWidget::onKeyDown( event );
}

void UICalendar::onSizeChange() {
	UIWidget::onSizeChange();
	updateLayoutSize();
}

void UICalendar::onThemeLoaded() {
	UIWidget::onThemeLoaded();
	refresh();
}

void UICalendar::onAutoSize() {
	updateLayoutSize();
}

void UICalendar::updateLayoutSize() {
	if ( !mTodayButton || mUpdatingLayout )
		return;
	mUpdatingLayout = true;
	const Float headerHeight = std::max<Float>(
		34, PixelDensity::pxToDp( mTitleButton->getTextView()->getTextHeight() ) + 8 );
	const Float todayHeight = std::max<Float>(
		32, PixelDensity::pxToDp( mTodayButton->getTextView()->getTextHeight() ) + 8 );
	Float weekdayHeight = 24;
	Float weekdayWidth = 0;
	Float cellHeight = 0;
	Float cellWidth = 0;
	for ( auto* label : mWeekdayLabels ) {
		weekdayHeight =
			std::max( weekdayHeight, PixelDensity::pxToDp( label->getTextHeight() ) + 4 );
		weekdayWidth = std::max( weekdayWidth, PixelDensity::pxToDp( label->getTextWidth() ) + 8 );
	}
	for ( auto* cell : mDayCells ) {
		cellHeight = std::max( cellHeight,
							   PixelDensity::pxToDp( cell->getTextView()->getTextHeight() ) + 8 );
		cellWidth =
			std::max( cellWidth, PixelDensity::pxToDp( cell->getTextView()->getTextWidth() ) + 8 );
	}
	const Float fixed =
		headerHeight + todayHeight + ( mView == CalendarView::Days ? weekdayHeight : 0.f );
	// CSS sets preferred minimum dimensions; content measurement only prevents text clipping.
	Float width = std::max( std::max( weekdayWidth, cellWidth ) * 7,
							PixelDensity::pxToDp( mTitleButton->getTextView()->getTextWidth() ) +
								headerHeight * 2 + 16 );
	Float height = fixed + cellHeight * 6;
	if ( mView != CalendarView::Days ) {
		Float optionWidth = 0;
		Float optionHeight = 0;
		for ( auto* cell : mOptionCells ) {
			optionWidth = std::max(
				optionWidth, PixelDensity::pxToDp( cell->getTextView()->getTextWidth() ) + 8 );
			optionHeight = std::max(
				optionHeight, PixelDensity::pxToDp( cell->getTextView()->getTextHeight() ) + 8 );
		}
		width = std::max( width, optionWidth * 4 );
		height = std::max( height, fixed + optionHeight * 3 );
	}
	const Sizef size = fitMinMaxSizeDp( Sizef(
		getLayoutWidthPolicy() == SizePolicy::WrapContent ? width : getSize().getWidth(),
		getLayoutHeightPolicy() == SizePolicy::WrapContent ? height : getSize().getHeight() ) );
	if ( size != getSize() )
		setSize( size );
	mMainLayout->setSize( getSize() );
	mHeader->setSize( Sizef( getSize().getWidth(), headerHeight ) );
	mPreviousButton->setSize( headerHeight, headerHeight );
	mNextButton->setSize( headerHeight, headerHeight );
	mTodayButton->setSize( Sizef( getSize().getWidth(), todayHeight ) );
	mWeekdayRow->setRowHeight( weekdayHeight );
	mWeekdayRow->setSize( Sizef( getSize().getWidth(), weekdayHeight ) );
	const Float content = std::max<Float>( 0, getSize().getHeight() - fixed );
	mDayGrid->setSize( Sizef( getSize().getWidth(), content ) );
	mOptionGrid->setSize( Sizef( getSize().getWidth(), content ) );
	mUpdatingLayout = false;
}

int UICalendar::dayOfWeek( const CalendarDate& date ) {
	// Tomohiko Sakamoto's Gregorian algorithm. 0 = Sunday.
	static constexpr int offsets[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	Int64 year = date.year;
	if ( date.month < 3 )
		--year;
	Int64 value = year + ( year >= 0 ? year / 4 : ( year - 3 ) / 4 ) -
				  ( year >= 0 ? year / 100 : ( year - 99 ) / 100 ) +
				  ( year >= 0 ? year / 400 : ( year - 399 ) / 400 ) + offsets[date.month - 1] +
				  date.day;
	value %= 7;
	if ( value < 0 )
		value += 7;
	return static_cast<int>( value );
}

const char* UICalendar::viewToString( CalendarView view ) {
	switch ( view ) {
		case CalendarView::Days:
			return "days";
		case CalendarView::Months:
			return "months";
		case CalendarView::Years:
			return "years";
	}
	return "days";
}

bool UICalendar::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !attribute.getPropertyDefinition() )
		return false;
	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::FirstDayOfWeek:
			setFirstDayOfWeek( attribute.asUint() );
			return true;
		case PropertyId::CalendarView: {
			const auto& value = attribute.getValue();
			if ( value == "days" )
				setView( CalendarView::Days );
			else if ( value == "months" )
				setView( CalendarView::Months );
			else if ( value == "years" )
				setView( CalendarView::Years );
			else
				return false;
			return true;
		}
		case PropertyId::DisplayedMonth:
		case PropertyId::FocusedDate: {
			const auto date = parseISODateProperty( attribute );
			if ( !date )
				return false;
			if ( attribute.getPropertyDefinition()->getPropertyId() == PropertyId::DisplayedMonth )
				setDisplayedMonth( *date );
			else
				setFocusedDate( *date );
			return true;
		}
		case PropertyId::Value: {
			if ( attribute.getValue().empty() )
				clearSelection();
			else if ( auto value = parseISODateProperty( attribute ) )
				setSelectedDate( *value );
			return true;
		}
		case PropertyId::MinDate:
		case PropertyId::MaxDate: {
			const auto date =
				attribute.getValue().empty() ? std::nullopt : parseISODateProperty( attribute );
			if ( !attribute.getValue().empty() && !date )
				return false;
			if ( attribute.getPropertyDefinition()->getPropertyId() == PropertyId::MinDate )
				setMinDate( date );
			else
				setMaxDate( date );
			return true;
		}
		default:
			return UIWidget::applyProperty( attribute );
	}
}

std::string UICalendar::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return {};
	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::FirstDayOfWeek:
			return String::toString( mLocale.firstDayOfWeek );
		case PropertyId::CalendarView:
			return viewToString( mView );
		case PropertyId::DisplayedMonth:
			return DateTimeFormatter::toISODate( mDisplayedMonth );
		case PropertyId::FocusedDate:
			return DateTimeFormatter::toISODate( mFocusedDate );
		case PropertyId::Value:
			return mSelectedDate ? DateTimeFormatter::toISODate( *mSelectedDate ) : std::string{};
		case PropertyId::MinDate:
			return mMinDate ? DateTimeFormatter::toISODate( *mMinDate ) : std::string{};
		case PropertyId::MaxDate:
			return mMaxDate ? DateTimeFormatter::toISODate( *mMaxDate ) : std::string{};
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UICalendar::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	const std::initializer_list<PropertyId> local = {
		PropertyId::Value,			PropertyId::MinDate,	  PropertyId::MaxDate,
		PropertyId::FirstDayOfWeek, PropertyId::CalendarView, PropertyId::DisplayedMonth,
		PropertyId::FocusedDate };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

}} // namespace EE::UI
