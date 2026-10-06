#include <algorithm>
#include <cctype>
#include <eepp/core/string.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uidatetimeedit.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>
#include <limits>

namespace EE { namespace UI {

using namespace System;
using namespace CSS;

namespace {

template <typename T>
std::optional<T> clampOptional( std::optional<T> value, const std::optional<T>& minValue,
								const std::optional<T>& maxValue ) {
	if ( !value )
		return value;
	if ( minValue && *value < *minValue )
		value = *minValue;
	if ( maxValue && *value > *maxValue )
		value = *maxValue;
	return value;
}

Int32 positiveMod( Int64 value, Int32 modulo ) {
	Int64 result = value % modulo;
	return static_cast<Int32>( result < 0 ? result + modulo : result );
}

bool validEditorFormat( const DateTimeFormat& format, bool date ) {
	if ( !format.isValid() )
		return false;
	std::array<Uint8, 10> counts{};
	for ( const auto& token : format.getTokens() ) {
		if ( token.type == DateTimeFormatTokenType::Literal )
			continue;
		const bool dateToken = token.type == DateTimeFormatTokenType::Day ||
							   token.type == DateTimeFormatTokenType::Month ||
							   token.type == DateTimeFormatTokenType::Year;
		if ( dateToken != date || ++counts[static_cast<Uint8>( token.type )] > 1 )
			return false;
	}
	if ( date )
		return counts[static_cast<Uint8>( DateTimeFormatTokenType::Day )] &&
			   counts[static_cast<Uint8>( DateTimeFormatTokenType::Month )] &&
			   counts[static_cast<Uint8>( DateTimeFormatTokenType::Year )];
	const auto hour12 = counts[static_cast<Uint8>( DateTimeFormatTokenType::Hour12 )];
	const auto hour24 = counts[static_cast<Uint8>( DateTimeFormatTokenType::Hour24 )];
	return hour12 + hour24 == 1 && counts[static_cast<Uint8>( DateTimeFormatTokenType::Minute )] &&
		   counts[static_cast<Uint8>( DateTimeFormatTokenType::AmPm )] == hour12;
}

constexpr const char* SectionNames[] = { "none",   "day",	 "month",		"year", "hour",
										 "minute", "second", "millisecond", "am-pm" };

std::string lowerAscii( std::string_view value ) {
	std::string out( value );
	std::transform( out.begin(), out.end(), out.begin(),
					[]( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
	return out;
}

} // namespace

UIDateTimeEdit* UIDateTimeEdit::New() {
	return eeNew( UIDateTimeEdit, () );
}

UIDateTimeEdit* UIDateTimeEdit::New( DateTimeEditMode mode ) {
	return eeNew( UIDateTimeEdit, ( mode ) );
}

UIDateTimeEdit::UIDateTimeEdit() : UIDateTimeEdit( DateTimeEditMode::Date ) {}

UIDateTimeEdit::UIDateTimeEdit( DateTimeEditMode mode, const std::string& tag ) :
	UITextInput( tag ), mEditMode( mode ), mLocale( DateTimeLocale::system() ) {
	mAllowEditingConnection =
		connect( Event::OnAllowEditingChange, [this]( const Event* ) { onConfigurationChange(); } );
	setSelectAllDocOnTabNavigate( false );
	mDoc.setCommand( "paste", [this] { pasteValue(); } );
	mDoc.setCommand( "cut", [this] {
		if ( !isEditingAllowed() )
			return;
		copy();
		clear();
	} );
	rebuildFormat();
}

Uint32 UIDateTimeEdit::getType() const {
	return UI_TYPE_DATETIMEEDIT;
}

bool UIDateTimeEdit::isType( const Uint32& type ) const {
	return type == UI_TYPE_DATETIMEEDIT || UITextInput::isType( type );
}

Float UIDateTimeEdit::getMinIntrinsicWidth() const {
	// Text metrics lazily refresh their existing cache.
	return eemax( const_cast<UIDateTimeEdit*>( this )->getTextWidth(),
				  mHintCache ? mHintCache->getTextWidth() : 0.f ) +
		   mPaddingPx.getWidth();
}

Float UIDateTimeEdit::getMaxIntrinsicWidth() const {
	return getMinIntrinsicWidth();
}

void UIDateTimeEdit::onAutoSize() {
	UITextInput::onAutoSize();
	if ( mWidthPolicy == SizePolicy::WrapContent && getFont() )
		setInternalPixelsWidth( getMinIntrinsicWidth() );
}

UIDateTimeEdit* UIDateTimeEdit::setEditMode( DateTimeEditMode mode ) {
	if ( mEditMode == mode )
		return this;
	mEditMode = mode;
	mActiveSection = DateTimeSection::None;
	resetPendingDigits();
	rebuildFormat();
	if ( !mAllowEmpty )
		initializeValueForEditing();
	return this;
}

DateTimeEditMode UIDateTimeEdit::getEditMode() const {
	return mEditMode;
}

UIDateTimeEdit* UIDateTimeEdit::setDate( const CalendarDate& date ) {
	return date.isValid() ? setDate( std::optional<CalendarDate>{ date } ) : this;
}

UIDateTimeEdit* UIDateTimeEdit::setDate( std::optional<CalendarDate> date ) {
	if ( date && !date->isValid() )
		return this;
	if ( !date && !mAllowEmpty )
		return this;
	resetPendingDigits();
	if ( mClampToBounds )
		date = clampOptional( date, mMinDate, mMaxDate );
	const bool changed = mDate != date;
	mDate = date;
	if ( mEditMode == DateTimeEditMode::Date )
		updateTextFromValue();
	if ( changed && mEditMode == DateTimeEditMode::Date )
		onValueChange();
	return this;
}

const std::optional<CalendarDate>& UIDateTimeEdit::getDate() const {
	return mDate;
}

UIDateTimeEdit* UIDateTimeEdit::setTime( const TimeOfDay& time ) {
	return time.isValid() ? setTime( std::optional<TimeOfDay>{ time } ) : this;
}

UIDateTimeEdit* UIDateTimeEdit::setTime( std::optional<TimeOfDay> time ) {
	if ( time && !time->isValid() )
		return this;
	if ( !time && !mAllowEmpty )
		return this;
	resetPendingDigits();
	if ( mClampToBounds )
		time = clampOptional( time, mMinTime, mMaxTime );
	const bool changed = mTime != time;
	mTime = time;
	if ( mEditMode == DateTimeEditMode::Time )
		updateTextFromValue();
	if ( changed && mEditMode == DateTimeEditMode::Time )
		onValueChange();
	return this;
}

const std::optional<TimeOfDay>& UIDateTimeEdit::getTime() const {
	return mTime;
}

UIDateTimeEdit* UIDateTimeEdit::setDateTime( const LocalDateTime& dateTime ) {
	return dateTime.isValid() ? setDateTime( std::optional<LocalDateTime>{ dateTime } ) : this;
}

UIDateTimeEdit* UIDateTimeEdit::setDateTime( std::optional<LocalDateTime> dateTime ) {
	if ( dateTime && !dateTime->isValid() )
		return this;
	if ( !dateTime && !mAllowEmpty )
		return this;
	resetPendingDigits();
	if ( mClampToBounds )
		dateTime = clampOptional( dateTime, mMinDateTime, mMaxDateTime );
	const bool changed = mDateTime != dateTime;
	mDateTime = dateTime;
	if ( mEditMode == DateTimeEditMode::DateTime )
		updateTextFromValue();
	if ( changed && mEditMode == DateTimeEditMode::DateTime )
		onValueChange();
	return this;
}

const std::optional<LocalDateTime>& UIDateTimeEdit::getDateTime() const {
	return mDateTime;
}

bool UIDateTimeEdit::hasValue() const {
	switch ( mEditMode ) {
		case DateTimeEditMode::Date:
			return mDate.has_value();
		case DateTimeEditMode::Time:
			return mTime.has_value();
		case DateTimeEditMode::DateTime:
			return mDateTime.has_value();
	}
	return false;
}

UIDateTimeEdit* UIDateTimeEdit::clear() {
	if ( !mAllowEmpty )
		return this;
	switch ( mEditMode ) {
		case DateTimeEditMode::Date:
			setDate( std::nullopt );
			break;
		case DateTimeEditMode::Time:
			setTime( std::nullopt );
			break;
		case DateTimeEditMode::DateTime:
			setDateTime( std::nullopt );
			break;
	}
	return this;
}

UIDateTimeEdit* UIDateTimeEdit::setAllowEmpty( bool allow ) {
	if ( mAllowEmpty == allow )
		return this;
	mAllowEmpty = allow;
	if ( !allow && !hasValue() ) {
		switch ( mEditMode ) {
			case DateTimeEditMode::Date:
				setDate( CalendarDate::today() );
				break;
			case DateTimeEditMode::Time:
				setTime( TimeOfDay::now() );
				break;
			case DateTimeEditMode::DateTime:
				setDateTime( LocalDateTime::now() );
				break;
		}
	}
	return this;
}

bool UIDateTimeEdit::getAllowEmpty() const {
	return mAllowEmpty;
}

UIDateTimeEdit* UIDateTimeEdit::setLocale( const DateTimeLocale& locale ) {
	mLocale = locale;
	mLocale.firstDayOfWeek %= 7;
	rebuildFormat();
	return this;
}

const DateTimeLocale& UIDateTimeEdit::getLocale() const {
	return mLocale;
}

UIDateTimeEdit* UIDateTimeEdit::setDateFormat( const std::string& pattern ) {
	const std::string value = String::iequals( pattern, "locale" ) ? "" : pattern;
	if ( mDateFormat != value ) {
		DateTimeFormat candidate( value.empty() ? mLocale.datePattern() : value );
		if ( !validEditorFormat( candidate, true ) )
			return this;
		mDateFormat = value;
		rebuildFormat();
	}
	return this;
}

UIDateTimeEdit* UIDateTimeEdit::setTimeFormat( const std::string& pattern ) {
	const std::string value = String::iequals( pattern, "locale" ) ? "" : pattern;
	if ( mTimeFormat != value ) {
		DateTimeFormat candidate(
			value.empty() ? mLocale.timePattern( mShowSeconds, mShowMilliseconds, mHourCycle )
						  : value );
		if ( !validEditorFormat( candidate, false ) )
			return this;
		mTimeFormat = value;
		rebuildFormat();
	}
	return this;
}

const std::string& UIDateTimeEdit::getDateFormat() const {
	return mDateFormat;
}

const std::string& UIDateTimeEdit::getTimeFormat() const {
	return mTimeFormat;
}

const DateTimeFormat& UIDateTimeEdit::getDisplayFormat() const {
	return mDisplayFormat;
}

UIDateTimeEdit* UIDateTimeEdit::setHourCycle( HourCycle cycle ) {
	if ( mHourCycle != cycle ) {
		mHourCycle = cycle;
		rebuildFormat();
	}
	return this;
}

HourCycle UIDateTimeEdit::getHourCycle() const {
	return mHourCycle;
}

UIDateTimeEdit* UIDateTimeEdit::setShowSeconds( bool show ) {
	if ( mShowSeconds != show ) {
		mShowSeconds = show;
		if ( !show )
			mShowMilliseconds = false;
		rebuildFormat();
	}
	return this;
}

bool UIDateTimeEdit::getShowSeconds() const {
	return mShowSeconds;
}

UIDateTimeEdit* UIDateTimeEdit::setShowMilliseconds( bool show ) {
	if ( mShowMilliseconds != show ) {
		mShowMilliseconds = show;
		if ( show )
			mShowSeconds = true;
		rebuildFormat();
	}
	return this;
}

bool UIDateTimeEdit::getShowMilliseconds() const {
	return mShowMilliseconds;
}

UIDateTimeEdit* UIDateTimeEdit::setDayStep( Uint32 step ) {
	step = std::max<Uint32>( 1, step );
	if ( mDayStep != step ) {
		mDayStep = step;
		onConfigurationChange();
	}
	return this;
}

Uint32 UIDateTimeEdit::getDayStep() const {
	return mDayStep;
}

UIDateTimeEdit* UIDateTimeEdit::setClampToBounds( bool clamp ) {
	if ( mClampToBounds != clamp ) {
		mClampToBounds = clamp;
		clampCurrentValue();
		onConstraintsChange();
	}
	return this;
}

bool UIDateTimeEdit::getClampToBounds() const {
	return mClampToBounds;
}

UIDateTimeEdit* UIDateTimeEdit::setHourStep( Uint32 step ) {
	step = std::max<Uint32>( 1, step );
	if ( mHourStep != step ) {
		mHourStep = step;
		onConfigurationChange();
	}
	return this;
}

UIDateTimeEdit* UIDateTimeEdit::setMinuteStep( Uint32 step ) {
	step = std::max<Uint32>( 1, step );
	if ( mMinuteStep != step ) {
		mMinuteStep = step;
		onConfigurationChange();
	}
	return this;
}

UIDateTimeEdit* UIDateTimeEdit::setSecondStep( Uint32 step ) {
	step = std::max<Uint32>( 1, step );
	if ( mSecondStep != step ) {
		mSecondStep = step;
		onConfigurationChange();
	}
	return this;
}

Uint32 UIDateTimeEdit::getHourStep() const {
	return mHourStep;
}

Uint32 UIDateTimeEdit::getMinuteStep() const {
	return mMinuteStep;
}

Uint32 UIDateTimeEdit::getSecondStep() const {
	return mSecondStep;
}

void UIDateTimeEdit::onConstraintsChange() {}

void UIDateTimeEdit::onConfigurationChange() {}

UIDateTimeEdit* UIDateTimeEdit::setMinDate( std::optional<CalendarDate> date ) {
	if ( date && !date->isValid() )
		return this;
	mMinDate = date;
	if ( mMinDate && mMaxDate && *mMinDate > *mMaxDate )
		mMaxDate = mMinDate;
	clampCurrentValue();
	onConstraintsChange();
	return this;
}

UIDateTimeEdit* UIDateTimeEdit::setMaxDate( std::optional<CalendarDate> date ) {
	if ( date && !date->isValid() )
		return this;
	mMaxDate = date;
	if ( mMinDate && mMaxDate && *mMaxDate < *mMinDate )
		mMinDate = mMaxDate;
	clampCurrentValue();
	onConstraintsChange();
	return this;
}

const std::optional<CalendarDate>& UIDateTimeEdit::getMinDate() const {
	return mMinDate;
}

const std::optional<CalendarDate>& UIDateTimeEdit::getMaxDate() const {
	return mMaxDate;
}

UIDateTimeEdit* UIDateTimeEdit::setMinTime( std::optional<TimeOfDay> time ) {
	if ( time && !time->isValid() )
		return this;
	mMinTime = time;
	if ( mMinTime && mMaxTime && *mMinTime > *mMaxTime )
		mMaxTime = mMinTime;
	clampCurrentValue();
	onConstraintsChange();
	return this;
}

UIDateTimeEdit* UIDateTimeEdit::setMaxTime( std::optional<TimeOfDay> time ) {
	if ( time && !time->isValid() )
		return this;
	mMaxTime = time;
	if ( mMinTime && mMaxTime && *mMaxTime < *mMinTime )
		mMinTime = mMaxTime;
	clampCurrentValue();
	onConstraintsChange();
	return this;
}

const std::optional<TimeOfDay>& UIDateTimeEdit::getMinTime() const {
	return mMinTime;
}

const std::optional<TimeOfDay>& UIDateTimeEdit::getMaxTime() const {
	return mMaxTime;
}

UIDateTimeEdit* UIDateTimeEdit::setMinDateTime( std::optional<LocalDateTime> dateTime ) {
	if ( dateTime && !dateTime->isValid() )
		return this;
	mMinDateTime = dateTime;
	if ( mMinDateTime && mMaxDateTime && *mMinDateTime > *mMaxDateTime )
		mMaxDateTime = mMinDateTime;
	clampCurrentValue();
	onConstraintsChange();
	return this;
}

UIDateTimeEdit* UIDateTimeEdit::setMaxDateTime( std::optional<LocalDateTime> dateTime ) {
	if ( dateTime && !dateTime->isValid() )
		return this;
	mMaxDateTime = dateTime;
	if ( mMinDateTime && mMaxDateTime && *mMaxDateTime < *mMinDateTime )
		mMinDateTime = mMaxDateTime;
	clampCurrentValue();
	onConstraintsChange();
	return this;
}

const std::optional<LocalDateTime>& UIDateTimeEdit::getMinDateTime() const {
	return mMinDateTime;
}

const std::optional<LocalDateTime>& UIDateTimeEdit::getMaxDateTime() const {
	return mMaxDateTime;
}

UIDateTimeEdit* UIDateTimeEdit::setWheelEditingEnabled( bool enabled ) {
	mWheelEditing = enabled;
	return this;
}

bool UIDateTimeEdit::isWheelEditingEnabled() const {
	return mWheelEditing;
}

UIDateTimeEdit* UIDateTimeEdit::setPendingDigitTimeout( const Time& timeout ) {
	const Time value = timeout < Time::Zero ? Time::Zero : timeout;
	if ( mPendingDigitTimeout != value ) {
		mPendingDigitTimeout = value;
		onConfigurationChange();
	}
	return this;
}

const Time& UIDateTimeEdit::getPendingDigitTimeout() const {
	return mPendingDigitTimeout;
}

DateTimeSection UIDateTimeEdit::getActiveSection() const {
	return mActiveSection;
}

UIDateTimeEdit* UIDateTimeEdit::setActiveSection( DateTimeSection section ) {
	if ( section != DateTimeSection::None && !findSection( section ) )
		return this;
	commitPendingDigits( false );
	mActiveSection = section;
	resetPendingDigits();
	selectActiveSection();
	return this;
}

bool UIDateTimeEdit::stepActiveSection( Int32 direction ) {
	if ( direction == 0 || !isEditingAllowed() )
		return false;
	if ( mActiveSection == DateTimeSection::None ) {
		const auto order = getSectionOrder();
		if ( order.empty() )
			return false;
		mActiveSection = order.front();
	}
	initializeValueForEditing();
	bool changed = false;
	const Int64 sign = direction > 0 ? 1 : -1;
	if ( mEditMode == DateTimeEditMode::Date && mDate ) {
		CalendarDate value = *mDate;
		switch ( mActiveSection ) {
			case DateTimeSection::Day:
				value = value.addDays( sign * mDayStep );
				break;
			case DateTimeSection::Month:
				value = value.addMonths( sign );
				break;
			case DateTimeSection::Year:
				value = value.addYears( sign );
				break;
			default:
				return false;
		}
		auto old = mDate;
		setDate( value );
		changed = old != mDate;
	} else if ( mEditMode == DateTimeEditMode::Time && mTime ) {
		TimeOfDay value = *mTime;
		switch ( mActiveSection ) {
			case DateTimeSection::Hour:
				value = value.addHours( sign * mHourStep );
				break;
			case DateTimeSection::Minute:
				value = value.addMinutes( sign * mMinuteStep );
				break;
			case DateTimeSection::Second:
				value = value.addSeconds( sign * mSecondStep );
				break;
			case DateTimeSection::Millisecond: {
				const Int64 ms = value.millisecond + sign;
				value = value.addSeconds( ms < 0 ? -1 : ms >= 1000 ? 1 : 0 );
				value.millisecond = static_cast<Uint16>( positiveMod( ms, 1000 ) );
				break;
			}
			case DateTimeSection::AmPm:
				value = value.addHours( 12 );
				break;
			default:
				return false;
		}
		auto old = mTime;
		setTime( value );
		changed = old != mTime;
	} else if ( mEditMode == DateTimeEditMode::DateTime && mDateTime ) {
		LocalDateTime value = *mDateTime;
		switch ( mActiveSection ) {
			case DateTimeSection::Day:
				value.date = value.date.addDays( sign * mDayStep );
				break;
			case DateTimeSection::Month:
				value.date = value.date.addMonths( sign );
				break;
			case DateTimeSection::Year:
				value.date = value.date.addYears( sign );
				break;
			case DateTimeSection::Hour:
				value = value.addHours( sign * mHourStep );
				break;
			case DateTimeSection::Minute:
				value = value.addMinutes( sign * mMinuteStep );
				break;
			case DateTimeSection::Second:
				value = value.addSeconds( sign * mSecondStep );
				break;
			case DateTimeSection::Millisecond: {
				Int32 ms =
					static_cast<Int32>( value.time.millisecond ) + static_cast<Int32>( sign );
				if ( ms < 0 || ms >= 1000 ) {
					const auto stepped = value.addSeconds( sign );
					if ( stepped != value ) {
						value = stepped;
						value.time.millisecond = static_cast<Uint16>( positiveMod( ms, 1000 ) );
					}
				} else {
					value.time.millisecond = static_cast<Uint16>( ms );
				}
				break;
			}
			case DateTimeSection::AmPm:
				value.time = value.time.addHours( 12 );
				break;
			default:
				return false;
		}
		auto old = mDateTime;
		setDateTime( value );
		changed = old != mDateTime;
	}
	resetPendingDigits();
	selectActiveSection();
	return changed;
}

std::string UIDateTimeEdit::getSerializedValue() const {
	switch ( mEditMode ) {
		case DateTimeEditMode::Date:
			return mDate ? DateTimeFormatter::toISODate( *mDate ) : "";
		case DateTimeEditMode::Time:
			return mTime ? DateTimeFormatter::toISOTime( *mTime, mTime->millisecond != 0 ) : "";
		case DateTimeEditMode::DateTime:
			return mDateTime ? DateTimeFormatter::toISOLocalDateTime(
								   *mDateTime, mDateTime->time.millisecond != 0 )
							 : "";
	}
	return "";
}

UITextView* UIDateTimeEdit::setText( const String& text ) {
	if ( mUpdatingText )
		return UITextInput::setText( text );
	if ( text.empty() )
		clear();
	else
		parseAndSetDisplayText( text.toUtf8() );
	return this;
}

void UIDateTimeEdit::rebuildFormat() {
	const std::string datePattern = mDateFormat.empty() ? mLocale.datePattern() : mDateFormat;
	const std::string timePattern =
		mTimeFormat.empty() ? mLocale.timePattern( mShowSeconds, mShowMilliseconds, mHourCycle )
							: mTimeFormat;
	std::string pattern;
	switch ( mEditMode ) {
		case DateTimeEditMode::Date:
			pattern = datePattern;
			break;
		case DateTimeEditMode::Time:
			pattern = timePattern;
			break;
		case DateTimeEditMode::DateTime:
			pattern = datePattern + " " + timePattern;
			break;
	}
	DateTimeFormat candidate( pattern );
	if ( candidate.isValid() )
		mDisplayFormat = std::move( candidate );
	updateAutoHint();
	updateTextFromValue();
	onConfigurationChange();
}

void UIDateTimeEdit::updateTextFromValue( bool preserveSection ) {
	const DateTimeSection previous = preserveSection ? mActiveSection : DateTimeSection::None;
	FormattedDateTime formatted;
	bool hasFormatted = false;
	switch ( mEditMode ) {
		case DateTimeEditMode::Date:
			if ( mDate ) {
				formatted =
					DateTimeFormatter::formatDateDetailed( *mDate, mDisplayFormat, mLocale );
				hasFormatted = true;
			}
			break;
		case DateTimeEditMode::Time:
			if ( mTime ) {
				formatted =
					DateTimeFormatter::formatTimeDetailed( *mTime, mDisplayFormat, mLocale );
				hasFormatted = true;
			}
			break;
		case DateTimeEditMode::DateTime:
			if ( mDateTime ) {
				formatted = DateTimeFormatter::formatDateTimeDetailed( *mDateTime, mDisplayFormat,
																	   mLocale );
				hasFormatted = true;
			}
			break;
	}
	mUpdatingText = true;
	UITextInput::setText( hasFormatted ? String::fromUtf8( formatted.text ) : String() );
	mUpdatingText = false;
	mSections.clear();
	if ( hasFormatted )
		rebuildSections( formatted );
	else {
		const LocalDateTime sample{ CalendarDate{ 2000, 12, 28 }, TimeOfDay{ 20, 14, 32, 123 } };
		FormattedDateTime skeleton;
		switch ( mEditMode ) {
			case DateTimeEditMode::Date:
				skeleton =
					DateTimeFormatter::formatDateDetailed( sample.date, mDisplayFormat, mLocale );
				break;
			case DateTimeEditMode::Time:
				skeleton =
					DateTimeFormatter::formatTimeDetailed( sample.time, mDisplayFormat, mLocale );
				break;
			case DateTimeEditMode::DateTime:
				skeleton =
					DateTimeFormatter::formatDateTimeDetailed( sample, mDisplayFormat, mLocale );
				break;
		}
		rebuildSections( skeleton );
	}
	mActiveSection = previous;
	if ( mActiveSection != DateTimeSection::None && !findSection( mActiveSection ) )
		mActiveSection = DateTimeSection::None;
	selectActiveSection();
}

void UIDateTimeEdit::rebuildSections( const FormattedDateTime& formatted ) {
	mSections.clear();
	for ( const auto& range : formatted.ranges ) {
		const DateTimeSection type = tokenToSection( range.type );
		if ( type == DateTimeSection::None )
			continue;
		Section section;
		section.type = type;
		section.start =
			String::utf8Length( std::string_view( formatted.text ).substr( 0, range.start ) );
		section.length = String::utf8Length(
			std::string_view( formatted.text ).substr( range.start, range.length ) );
		section.displayDigits = mDisplayFormat.getTokens()[range.tokenIndex].width;
		if ( type != DateTimeSection::Year && type != DateTimeSection::Millisecond &&
			 type != DateTimeSection::AmPm )
			section.displayDigits = 2;
		switch ( type ) {
			case DateTimeSection::Day:
				section.minValue = 1;
				section.maxValue = 31;
				break;
			case DateTimeSection::Month:
				section.minValue = 1;
				section.maxValue = 12;
				break;
			case DateTimeSection::Year:
				section.minValue = 1;
				section.maxValue = 9999;
				break;
			case DateTimeSection::Hour:
				section.minValue = range.type == DateTimeFormatTokenType::Hour12 ? 1 : 0;
				section.maxValue = range.type == DateTimeFormatTokenType::Hour12 ? 12 : 23;
				break;
			case DateTimeSection::Minute:
			case DateTimeSection::Second:
				section.minValue = 0;
				section.maxValue = 59;
				break;
			case DateTimeSection::Millisecond:
				section.minValue = 0;
				section.maxValue = section.displayDigits == 1	? 9
								   : section.displayDigits == 2 ? 99
																: 999;
				break;
			case DateTimeSection::AmPm:
				section.minValue = 0;
				section.maxValue = 1;
				break;
			case DateTimeSection::None:
				break;
		}
		mSections.push_back( section );
	}
}

void UIDateTimeEdit::updateAutoHint() {
	std::string hint;
	for ( const auto& token : mDisplayFormat.getTokens() ) {
		switch ( token.type ) {
			case DateTimeFormatTokenType::Literal:
				hint += token.literal;
				break;
			case DateTimeFormatTokenType::Day:
				hint += std::string( token.width, 'd' );
				break;
			case DateTimeFormatTokenType::Month:
				hint += std::string( token.width, 'm' );
				break;
			case DateTimeFormatTokenType::Year:
				hint += std::string( token.width, 'y' );
				break;
			case DateTimeFormatTokenType::Hour24:
			case DateTimeFormatTokenType::Hour12:
				hint += std::string( token.width, 'h' );
				break;
			case DateTimeFormatTokenType::Minute:
				hint += std::string( token.width, 'm' );
				break;
			case DateTimeFormatTokenType::Second:
				hint += std::string( token.width, 's' );
				break;
			case DateTimeFormatTokenType::Millisecond:
				hint += std::string( token.width, 's' );
				break;
			case DateTimeFormatTokenType::AmPm:
				hint += "am/pm";
				break;
		}
	}
	if ( getHint().empty() || getHint().toUtf8() == mLastAutoHint )
		UITextInput::setHint( String::fromUtf8( hint ) );
	mLastAutoHint = std::move( hint );
}

void UIDateTimeEdit::selectActiveSection() {
	if ( !hasValue() || mActiveSection == DateTimeSection::None )
		return;
	auto section = findSection( mActiveSection );
	if ( !section )
		return;
	mDoc.setSelection( { { 0, static_cast<Int64>( section->start ) },
						 { 0, static_cast<Int64>( section->start + section->length ) } } );
}

void UIDateTimeEdit::selectSectionAtColumn( Int64 column ) {
	if ( mSections.empty() )
		return;
	const Section* best = &mSections.front();
	Int64 bestDistance = std::numeric_limits<Int64>::max();
	for ( const auto& section : mSections ) {
		const Int64 start = static_cast<Int64>( section.start );
		const Int64 end = static_cast<Int64>( section.start + section.length );
		if ( column >= start && column <= end ) {
			best = &section;
			bestDistance = 0;
			break;
		}
		const Int64 distance = column < start ? start - column : column - end;
		if ( distance < bestDistance ) {
			bestDistance = distance;
			best = &section;
		}
	}
	mActiveSection = best->type;
	resetPendingDigits();
	selectActiveSection();
}

void UIDateTimeEdit::moveActiveSection( Int32 direction ) {
	commitPendingDigits( false );
	const auto order = getSectionOrder();
	if ( order.empty() )
		return;
	auto it = std::find( order.begin(), order.end(), mActiveSection );
	Int32 index = it == order.end() ? 0 : static_cast<Int32>( std::distance( order.begin(), it ) );
	index = std::clamp<Int32>( index + direction, 0, static_cast<Int32>( order.size() - 1 ) );
	mActiveSection = order[index];
	resetPendingDigits();
	selectActiveSection();
}

void UIDateTimeEdit::updatePendingText() {
	const auto section = findSection( mActiveSection );
	if ( !section || mPendingDigits.empty() || !hasValue() )
		return;
	String text = getText();
	String pending( mPendingDigits );
	while ( pending.size() < section->length )
		pending += '_';
	text = text.substr( 0, section->start ) + pending +
		   text.substr( section->start + section->length );
	mUpdatingText = true;
	UITextInput::setText( text );
	mUpdatingText = false;
	selectActiveSection();
}

void UIDateTimeEdit::resetPendingDigits() {
	const bool pending = !mPendingDigits.empty();
	mPendingDigits.clear();
	if ( pending )
		updateTextFromValue();
	mPendingDigitClock.restart();
}

bool UIDateTimeEdit::commitPendingDigits( bool advance ) {
	if ( mPendingDigits.empty() || mActiveSection == DateTimeSection::None )
		return false;
	Int32 value = 0;
	if ( !String::fromString( value, mPendingDigits ) ) {
		resetPendingDigits();
		return false;
	}
	if ( mActiveSection == DateTimeSection::Millisecond ) {
		const auto section = findSection( mActiveSection );
		if ( section && section->displayDigits < 3 )
			value *= section->displayDigits == 1 ? 100 : 10;
	}
	mPendingDigits.clear();
	const bool changed = setSectionNumericValue( mActiveSection, value );
	if ( advance )
		moveActiveSection( 1 );
	else
		selectActiveSection();
	return changed;
}

bool UIDateTimeEdit::processDigit( char digit ) {
	if ( mActiveSection == DateTimeSection::None ) {
		const auto order = getSectionOrder();
		if ( order.empty() )
			return false;
		mActiveSection = order.front();
	}
	if ( mActiveSection == DateTimeSection::AmPm )
		return false;
	initializeValueForEditing();
	auto section = findSection( mActiveSection );
	if ( !section )
		return false;
	if ( mPendingDigitClock.getElapsedTime() >= mPendingDigitTimeout )
		mPendingDigits.clear();
	mPendingDigitClock.restart();
	mPendingDigits.push_back( digit );
	Int32 value = 0;
	String::fromString( value, mPendingDigits );
	const Uint32 targetDigits = section->type == DateTimeSection::Year ? 4 : section->displayDigits;
	const bool full = mPendingDigits.size() >= targetDigits;
	const bool canContinue = mPendingDigits.size() < targetDigits &&
							 static_cast<Int64>( value ) * 10 <= section->maxValue;
	if ( full ) {
		if ( value >= section->minValue && value <= section->maxValue )
			return commitPendingDigits( true );
		const char retry = mPendingDigits.back();
		mPendingDigits.pop_back();
		if ( mPendingDigits.empty() ) {
			resetPendingDigits();
			return true;
		}
		commitPendingDigits( true );
		return processDigit( retry );
	}
	if ( !canContinue )
		return commitPendingDigits( true );
	updatePendingText();
	return true;
}

bool UIDateTimeEdit::processAmPm( char value ) {
	if ( mActiveSection != DateTimeSection::AmPm )
		return false;
	const char lower = static_cast<char>( std::tolower( static_cast<unsigned char>( value ) ) );
	if ( lower != 'a' && lower != 'p' )
		return false;
	initializeValueForEditing();
	const bool wantPm = lower == 'p';
	if ( mEditMode == DateTimeEditMode::Time && mTime ) {
		TimeOfDay valueTime = *mTime;
		const bool isPm = valueTime.hour >= 12;
		if ( isPm != wantPm )
			valueTime = valueTime.addHours( 12 );
		setTime( valueTime );
	} else if ( mEditMode == DateTimeEditMode::DateTime && mDateTime ) {
		LocalDateTime valueDateTime = *mDateTime;
		const bool isPm = valueDateTime.time.hour >= 12;
		if ( isPm != wantPm )
			valueDateTime.time = valueDateTime.time.addHours( 12 );
		setDateTime( valueDateTime );
	}
	moveActiveSection( 1 );
	return true;
}

bool UIDateTimeEdit::setSectionNumericValue( DateTimeSection section, Int32 value ) {
	initializeValueForEditing();
	auto setDateSection = [&]( CalendarDate& date ) -> bool {
		switch ( section ) {
			case DateTimeSection::Day:
				date.day = static_cast<Uint8>( std::clamp<Int32>( value, 1, date.daysInMonth() ) );
				break;
			case DateTimeSection::Month: {
				date.month = static_cast<Uint8>( std::clamp<Int32>( value, 1, 12 ) );
				date.day = static_cast<Uint8>( std::min<int>( date.day, date.daysInMonth() ) );
				break;
			}
			case DateTimeSection::Year:
				date.year = std::clamp<Int32>( value, 1, 9999 );
				date.day = static_cast<Uint8>( std::min<int>( date.day, date.daysInMonth() ) );
				break;
			default:
				return false;
		}
		return true;
	};
	if ( mEditMode == DateTimeEditMode::Date && mDate ) {
		CalendarDate date = *mDate;
		if ( !setDateSection( date ) )
			return false;
		auto old = mDate;
		setDate( date );
		return old != mDate;
	}

	auto setTimeSection = [&]( TimeOfDay& time ) -> bool {
		switch ( section ) {
			case DateTimeSection::Hour: {
				const bool hour12 = std::any_of(
					mDisplayFormat.getTokens().begin(), mDisplayFormat.getTokens().end(),
					[]( const DateTimeFormatToken& token ) {
						return token.type == DateTimeFormatTokenType::Hour12;
					} );
				if ( hour12 ) {
					const bool pm = time.hour >= 12;
					Int32 h = std::clamp<Int32>( value, 1, 12 );
					time.hour = static_cast<Uint8>( ( h % 12 ) + ( pm ? 12 : 0 ) );
				} else {
					time.hour = static_cast<Uint8>( std::clamp<Int32>( value, 0, 23 ) );
				}
				return true;
			}
			case DateTimeSection::Minute:
				time.minute = static_cast<Uint8>( std::clamp<Int32>( value, 0, 59 ) );
				return true;
			case DateTimeSection::Second:
				time.second = static_cast<Uint8>( std::clamp<Int32>( value, 0, 59 ) );
				return true;
			case DateTimeSection::Millisecond:
				time.millisecond = static_cast<Uint16>( std::clamp<Int32>( value, 0, 999 ) );
				return true;
			default:
				return false;
		}
	};

	if ( mEditMode == DateTimeEditMode::Time && mTime ) {
		TimeOfDay time = *mTime;
		if ( !setTimeSection( time ) )
			return false;
		auto old = mTime;
		setTime( time );
		return old != mTime;
	}
	if ( mEditMode == DateTimeEditMode::DateTime && mDateTime ) {
		LocalDateTime dateTime = *mDateTime;
		if ( !setDateSection( dateTime.date ) && !setTimeSection( dateTime.time ) )
			return false;
		auto old = mDateTime;
		setDateTime( dateTime );
		return old != mDateTime;
	}
	return false;
}

std::optional<UIDateTimeEdit::Section>
UIDateTimeEdit::findSection( DateTimeSection section ) const {
	auto it = std::find_if( mSections.begin(), mSections.end(),
							[section]( const Section& value ) { return value.type == section; } );
	return it == mSections.end() ? std::nullopt : std::optional<Section>{ *it };
}

SmallVector<DateTimeSection, 8> UIDateTimeEdit::getSectionOrder() const {
	SmallVector<DateTimeSection, 8> result;
	for ( const auto& section : mSections )
		result.push_back( section.type );
	return result;
}

bool UIDateTimeEdit::parseAndSetDisplayText( std::string_view text ) {
	switch ( mEditMode ) {
		case DateTimeEditMode::Date: {
			auto value = DateTimeFormatter::parseDateTolerant( text, mDisplayFormat, mLocale );
			if ( !value )
				return false;
			setDate( *value );
			return true;
		}
		case DateTimeEditMode::Time: {
			auto value = DateTimeFormatter::parseTimeTolerant( text, mDisplayFormat, mLocale );
			if ( !value )
				return false;
			setTime( *value );
			return true;
		}
		case DateTimeEditMode::DateTime: {
			auto value = DateTimeFormatter::parseDateTimeTolerant( text, mDisplayFormat, mLocale );
			if ( !value )
				return false;
			setDateTime( *value );
			return true;
		}
	}
	return false;
}

bool UIDateTimeEdit::parseAndSetSerializedValue( std::string_view text ) {
	if ( text.empty() ) {
		clear();
		return true;
	}
	return parseAndSetDisplayText( text );
}

void UIDateTimeEdit::initializeValueForEditing() {
	if ( hasValue() )
		return;
	switch ( mEditMode ) {
		case DateTimeEditMode::Date:
			setDate( CalendarDate::today() );
			break;
		case DateTimeEditMode::Time: {
			TimeOfDay value = TimeOfDay::now();
			value.second = 0;
			value.millisecond = 0;
			setTime( value );
			break;
		}
		case DateTimeEditMode::DateTime: {
			LocalDateTime value = LocalDateTime::now();
			value.time.second = 0;
			value.time.millisecond = 0;
			setDateTime( value );
			break;
		}
	}
}

void UIDateTimeEdit::clampCurrentValue() {
	if ( !mClampToBounds )
		return;
	switch ( mEditMode ) {
		case DateTimeEditMode::Date:
			if ( mDate )
				setDate( mDate );
			break;
		case DateTimeEditMode::Time:
			if ( mTime )
				setTime( mTime );
			break;
		case DateTimeEditMode::DateTime:
			if ( mDateTime )
				setDateTime( mDateTime );
			break;
	}
}

DateTimeSection UIDateTimeEdit::tokenToSection( DateTimeFormatTokenType type ) {
	switch ( type ) {
		case DateTimeFormatTokenType::Day:
			return DateTimeSection::Day;
		case DateTimeFormatTokenType::Month:
			return DateTimeSection::Month;
		case DateTimeFormatTokenType::Year:
			return DateTimeSection::Year;
		case DateTimeFormatTokenType::Hour24:
		case DateTimeFormatTokenType::Hour12:
			return DateTimeSection::Hour;
		case DateTimeFormatTokenType::Minute:
			return DateTimeSection::Minute;
		case DateTimeFormatTokenType::Second:
			return DateTimeSection::Second;
		case DateTimeFormatTokenType::Millisecond:
			return DateTimeSection::Millisecond;
		case DateTimeFormatTokenType::AmPm:
			return DateTimeSection::AmPm;
		case DateTimeFormatTokenType::Literal:
			return DateTimeSection::None;
	}
	return DateTimeSection::None;
}

const char* UIDateTimeEdit::modeToString( DateTimeEditMode mode ) {
	switch ( mode ) {
		case DateTimeEditMode::Date:
			return "date";
		case DateTimeEditMode::Time:
			return "time";
		case DateTimeEditMode::DateTime:
			return "datetime";
	}
	return "date";
}

std::optional<DateTimeEditMode> UIDateTimeEdit::modeFromString( std::string_view mode ) {
	const auto value = lowerAscii( mode );
	if ( value == "date" )
		return DateTimeEditMode::Date;
	if ( value == "time" )
		return DateTimeEditMode::Time;
	if ( value == "datetime" || value == "date-time" || value == "datetime-local" )
		return DateTimeEditMode::DateTime;
	return std::nullopt;
}

const char* UIDateTimeEdit::hourCycleToString( HourCycle cycle ) {
	switch ( cycle ) {
		case HourCycle::Locale:
			return "locale";
		case HourCycle::H12:
			return "12";
		case HourCycle::H24:
			return "24";
	}
	return "locale";
}

Uint32 UIDateTimeEdit::onKeyDown( const KeyEvent& event ) {
	if ( !isEditingAllowed() )
		return UITextInput::onKeyDown( event );
	if ( event.getSanitizedMod() == 0 ) {
		switch ( event.getKeyCode() ) {
			case KEY_LEFT:
				moveActiveSection( -1 );
				return 1;
			case KEY_RIGHT:
				moveActiveSection( 1 );
				return 1;
			case KEY_HOME: {
				const auto order = getSectionOrder();
				if ( !order.empty() )
					setActiveSection( order.front() );
				return 1;
			}
			case KEY_END: {
				const auto order = getSectionOrder();
				if ( !order.empty() )
					setActiveSection( order.back() );
				return 1;
			}
			case KEY_DELETE:
			case KEY_BACKSPACE:
				resetPendingDigits();
				clear();
				return 1;
			case KEY_UP:
				stepActiveSection( 1 );
				return 1;
			case KEY_DOWN:
				stepActiveSection( -1 );
				return 1;
			case KEY_ESCAPE:
				if ( !mPendingDigits.empty() ) {
					resetPendingDigits();
					selectActiveSection();
					return 1;
				}
				break;
			case KEY_RETURN:
			case KEY_KP_ENTER:
				commitPendingDigits( false );
				break;
			default:
				break;
		}
	}
	return UITextInput::onKeyDown( event );
}

Uint32 UIDateTimeEdit::onTextInput( const TextInputEvent& event ) {
	if ( !isEditingAllowed() )
		return 0;
	if ( !event.isValid( getInput() ) )
		return 0;
	const String& text = event.getText();
	if ( text.size() == 1 ) {
		const auto chr = text[0];
		if ( chr >= '0' && chr <= '9' )
			return processDigit( static_cast<char>( chr ) ) ? 1 : 0;
		if ( ( chr == 'a' || chr == 'A' || chr == 'p' || chr == 'P' ) &&
			 processAmPm( static_cast<char>( chr ) ) )
			return 1;
	}
	if ( text.size() > 1 )
		return parseAndSetDisplayText( text.toUtf8() ) ? 1 : 0;
	return 0;
}

Uint32 UIDateTimeEdit::onMouseClick( const Vector2i& position, const Uint32& flags ) {
	const Uint32 result = UITextInput::onMouseClick( position, flags );
	if ( hasValue() && ( flags & EE_BUTTON_LMASK ) &&
		 !( getInput()->getModState() & KEYMOD_SHIFT ) ) {
		// Focus and mouse-down handling can replace the document selection before this event.
		// Hit-test the click itself, including the blank space after the formatted value.
		Vector2f textPosition( position.asFloat() );
		worldToNode( textPosition );
		textPosition = PixelDensity::dpToPx( textPosition ) - mRealAlignOffset -
					   Vector2f( mPaddingPx.Left, mPaddingPx.Top );
		const Int32 column = getVisibleTextCache().findCharacterFromPos(
			Vector2i( std::max( 0.f, textPosition.x ), 0 ) );
		if ( column >= 0 )
			selectSectionAtColumn( column );
	}
	return result;
}

Uint32 UIDateTimeEdit::onMouseWheel( const Vector2f& offset, bool flipped ) {
	if ( !mWheelEditing || !hasFocus() || offset.y == 0.f )
		return UITextInput::onMouseWheel( offset, flipped );
	const Float value = flipped ? -offset.y : offset.y;
	return stepActiveSection( value > 0 ? 1 : -1 ) ? 1 : 0;
}

void UIDateTimeEdit::onTextChanged() {
	// UITextView treats text itself as the value and therefore emits OnValueChange here.
	// For UIDateTimeEdit the canonical typed date/time value is authoritative, so formatting
	// changes must only emit OnTextChanged; typed setters emit OnValueChange exactly once.
	sendCommonEvent( Event::OnTextChanged );
	invalidateDraw();
}

void UIDateTimeEdit::onDocumentTextChanged( const DocumentContentChange& change ) {
	UITextInput::onDocumentTextChanged( change );
	if ( mUpdatingText )
		return;
	const std::string attempted = getText().toUtf8();
	if ( attempted.empty() ) {
		clear();
		updateTextFromValue();
		return;
	}
	if ( !parseAndSetDisplayText( attempted ) )
		updateTextFromValue();
}

Uint32 UIDateTimeEdit::onFocus( NodeFocusReason reason ) {
	const Uint32 result = UITextInput::onFocus( reason );
	if ( mActiveSection == DateTimeSection::None ) {
		const auto order = getSectionOrder();
		if ( !order.empty() )
			mActiveSection = order.front();
	}
	selectActiveSection();
	return result;
}

void UIDateTimeEdit::scheduledUpdate( const Time& time ) {
	UITextInput::scheduledUpdate( time );
	if ( !mPendingDigits.empty() && mPendingDigitClock.getElapsedTime() >= mPendingDigitTimeout )
		commitPendingDigits( true );
}

bool UIDateTimeEdit::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;
	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ActiveSection: {
			for ( Uint8 i = 0; i < sizeof( SectionNames ) / sizeof( SectionNames[0] ); ++i ) {
				if ( attribute.getValue() == SectionNames[i] ) {
					setActiveSection( static_cast<DateTimeSection>( i ) );
					return true;
				}
			}
			return false;
		}
		case PropertyId::Value:
			return parseAndSetSerializedValue( attribute.asString() );
		case PropertyId::DateTimeMode: {
			auto mode = modeFromString( attribute.asString() );
			if ( mode )
				setEditMode( *mode );
			return mode.has_value();
		}
		case PropertyId::AllowEmpty:
			setAllowEmpty( attribute.asBool() );
			return true;
		case PropertyId::DateFormat:
			setDateFormat( attribute.asString() );
			return true;
		case PropertyId::TimeFormat:
			setTimeFormat( attribute.asString() );
			return true;
		case PropertyId::HourCycle: {
			const auto value = lowerAscii( attribute.asString() );
			setHourCycle( value == "12" || value == "h12"	? HourCycle::H12
						  : value == "24" || value == "h24" ? HourCycle::H24
															: HourCycle::Locale );
			return true;
		}
		case PropertyId::ShowSeconds:
			setShowSeconds( attribute.asBool() );
			return true;
		case PropertyId::ShowMilliseconds:
			setShowMilliseconds( attribute.asBool() );
			return true;
		case PropertyId::WheelEditing:
			setWheelEditingEnabled( attribute.asBool() );
			return true;
		case PropertyId::HourStep:
			setHourStep( attribute.asUint() );
			return true;
		case PropertyId::MinuteStep:
			setMinuteStep( attribute.asUint() );
			return true;
		case PropertyId::SecondStep:
			setSecondStep( attribute.asUint() );
			return true;
		case PropertyId::MinDate: {
			if ( attribute.getValue().empty() ) {
				setMinDate( std::nullopt );
				return true;
			}
			auto value = DateTimeFormatter::parseDateTolerant(
				attribute.asString(), DateTimeFormat( "yyyy-MM-dd" ), mLocale );
			if ( value )
				setMinDate( *value );
			return value.has_value();
		}
		case PropertyId::MaxDate: {
			if ( attribute.getValue().empty() ) {
				setMaxDate( std::nullopt );
				return true;
			}
			auto value = DateTimeFormatter::parseDateTolerant(
				attribute.asString(), DateTimeFormat( "yyyy-MM-dd" ), mLocale );
			if ( value )
				setMaxDate( *value );
			return value.has_value();
		}
		case PropertyId::MinTime: {
			if ( attribute.getValue().empty() ) {
				setMinTime( std::nullopt );
				return true;
			}
			auto value = DateTimeFormatter::parseTimeTolerant(
				attribute.asString(), DateTimeFormat( "HH:mm:ss" ), mLocale );
			if ( value )
				setMinTime( *value );
			return value.has_value();
		}
		case PropertyId::MaxTime: {
			if ( attribute.getValue().empty() ) {
				setMaxTime( std::nullopt );
				return true;
			}
			auto value = DateTimeFormatter::parseTimeTolerant(
				attribute.asString(), DateTimeFormat( "HH:mm:ss" ), mLocale );
			if ( value )
				setMaxTime( *value );
			return value.has_value();
		}
		case PropertyId::MinDateTime: {
			if ( attribute.getValue().empty() ) {
				setMinDateTime( std::nullopt );
				return true;
			}
			auto value = DateTimeFormatter::parseDateTimeTolerant(
				attribute.asString(), DateTimeFormat( "yyyy-MM-dd'T'HH:mm:ss" ), mLocale );
			if ( value )
				setMinDateTime( *value );
			return value.has_value();
		}
		case PropertyId::MaxDateTime: {
			if ( attribute.getValue().empty() ) {
				setMaxDateTime( std::nullopt );
				return true;
			}
			auto value = DateTimeFormatter::parseDateTimeTolerant(
				attribute.asString(), DateTimeFormat( "yyyy-MM-dd'T'HH:mm:ss" ), mLocale );
			if ( value )
				setMaxDateTime( *value );
			return value.has_value();
		}
		default:
			return UITextInput::applyProperty( attribute );
	}
}

std::string UIDateTimeEdit::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return "";
	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::ActiveSection:
			return SectionNames[static_cast<Uint8>( mActiveSection )];
		case PropertyId::Value:
			return getSerializedValue();
		case PropertyId::DateTimeMode:
			return modeToString( mEditMode );
		case PropertyId::AllowEmpty:
			return mAllowEmpty ? "true" : "false";
		case PropertyId::DateFormat:
			return mDateFormat.empty() ? "locale" : mDateFormat;
		case PropertyId::TimeFormat:
			return mTimeFormat.empty() ? "locale" : mTimeFormat;
		case PropertyId::HourCycle:
			return hourCycleToString( mHourCycle );
		case PropertyId::ShowSeconds:
			return mShowSeconds ? "true" : "false";
		case PropertyId::ShowMilliseconds:
			return mShowMilliseconds ? "true" : "false";
		case PropertyId::WheelEditing:
			return mWheelEditing ? "true" : "false";
		case PropertyId::HourStep:
			return String::toString( mHourStep );
		case PropertyId::MinuteStep:
			return String::toString( mMinuteStep );
		case PropertyId::SecondStep:
			return String::toString( mSecondStep );
		case PropertyId::MinDate:
			return mMinDate ? DateTimeFormatter::toISODate( *mMinDate ) : "";
		case PropertyId::MaxDate:
			return mMaxDate ? DateTimeFormatter::toISODate( *mMaxDate ) : "";
		case PropertyId::MinTime:
			return mMinTime ? DateTimeFormatter::toISOTime( *mMinTime, mMinTime->millisecond != 0 )
							: "";
		case PropertyId::MaxTime:
			return mMaxTime ? DateTimeFormatter::toISOTime( *mMaxTime, mMaxTime->millisecond != 0 )
							: "";
		case PropertyId::MinDateTime:
			return mMinDateTime ? DateTimeFormatter::toISOLocalDateTime(
									  *mMinDateTime, mMinDateTime->time.millisecond != 0 )
								: "";
		case PropertyId::MaxDateTime:
			return mMaxDateTime ? DateTimeFormatter::toISOLocalDateTime(
									  *mMaxDateTime, mMaxDateTime->time.millisecond != 0 )
								: "";
		default:
			return UITextInput::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIDateTimeEdit::getPropertiesImplemented() const {
	auto props = UITextInput::getPropertiesImplemented();
	const std::initializer_list<PropertyId> local = {
		PropertyId::ActiveSection, PropertyId::DateTimeMode,	 PropertyId::AllowEmpty,
		PropertyId::DateFormat,	   PropertyId::TimeFormat,		 PropertyId::HourCycle,
		PropertyId::ShowSeconds,   PropertyId::ShowMilliseconds, PropertyId::WheelEditing,
		PropertyId::HourStep,	   PropertyId::MinuteStep,		 PropertyId::SecondStep,
		PropertyId::MinDate,	   PropertyId::MaxDate,			 PropertyId::MinTime,
		PropertyId::MaxTime,	   PropertyId::MinDateTime,		 PropertyId::MaxDateTime };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UIDateTimeEdit::pasteValue() {
	if ( !isEditingAllowed() || !getUISceneNode() )
		return;
	resetPendingDigits();
	const std::string text = getUISceneNode()->getWindow()->getClipboard()->getText();
	if ( text.empty() )
		clear();
	else
		parseAndSetDisplayText( text );
	sendCommonEvent( Event::OnTextPasted );
}

Uint32 UIDateTimeEdit::onFocusLoss() {
	commitPendingDigits( false );
	return UITextInput::onFocusLoss();
}

}} // namespace EE::UI
