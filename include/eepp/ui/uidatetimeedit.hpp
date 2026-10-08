#ifndef EE_UI_UIDATETIMEEDIT_HPP
#define EE_UI_UIDATETIMEEDIT_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/datetime.hpp>
#include <eepp/system/datetimeformat.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <optional>
#include <string>

namespace EE { namespace UI {

using namespace EE::System;
using namespace EE::UI::CSS;

enum class DateTimeEditMode : Uint8 { Date, Time, DateTime };

enum class DateTimeSection : Uint8 {
	None,
	Day,
	Month,
	Year,
	Hour,
	Minute,
	Second,
	Millisecond,
	AmPm
};

class EE_API UIDateTimeEdit : public UITextInput {
  public:
	static UIDateTimeEdit* New();

	static UIDateTimeEdit* New( DateTimeEditMode mode );

	virtual ~UIDateTimeEdit() = default;

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	virtual Float getMinIntrinsicWidth() const override;

	virtual Float getMaxIntrinsicWidth() const override;

	UIDateTimeEdit* setEditMode( DateTimeEditMode mode );

	DateTimeEditMode getEditMode() const;

	UIDateTimeEdit* setDate( const CalendarDate& date );

	UIDateTimeEdit* setDate( std::optional<CalendarDate> date );

	const std::optional<CalendarDate>& getDate() const;

	UIDateTimeEdit* setTime( const TimeOfDay& time );

	UIDateTimeEdit* setTime( std::optional<TimeOfDay> time );

	const std::optional<TimeOfDay>& getTime() const;

	UIDateTimeEdit* setDateTime( const LocalDateTime& dateTime );

	UIDateTimeEdit* setDateTime( std::optional<LocalDateTime> dateTime );

	const std::optional<LocalDateTime>& getDateTime() const;

	bool hasValue() const;

	UIDateTimeEdit* clear();

	UIDateTimeEdit* setAllowEmpty( bool allow );

	bool getAllowEmpty() const;

	UIDateTimeEdit* setLocale( const DateTimeLocale& locale );

	const DateTimeLocale& getLocale() const;

	UIDateTimeEdit* setDateFormat( const std::string& pattern );

	UIDateTimeEdit* setTimeFormat( const std::string& pattern );

	const std::string& getDateFormat() const;

	const std::string& getTimeFormat() const;

	const DateTimeFormat& getDisplayFormat() const;

	UIDateTimeEdit* setHourCycle( HourCycle cycle );

	HourCycle getHourCycle() const;

	/** Shows seconds in the locale format (off by default). Hiding also hides milliseconds. */
	UIDateTimeEdit* setShowSeconds( bool show );

	bool getShowSeconds() const;

	/** Shows milliseconds in the locale format (off by default). Enabling also shows seconds;
	 * disabling leaves seconds visible. Hidden precision is preserved in the value. */
	UIDateTimeEdit* setShowMilliseconds( bool show );

	bool getShowMilliseconds() const;

	UIDateTimeEdit* setDayStep( Uint32 step );

	Uint32 getDayStep() const;

	/** Native controls clamp to bounds by default. HTML adapters disable clamping so range
	 * constraints report invalid values instead of silently changing them. */
	UIDateTimeEdit* setClampToBounds( bool clamp );

	bool getClampToBounds() const;

	UIDateTimeEdit* setHourStep( Uint32 step );

	UIDateTimeEdit* setMinuteStep( Uint32 step );

	UIDateTimeEdit* setSecondStep( Uint32 step );

	Uint32 getHourStep() const;

	Uint32 getMinuteStep() const;

	Uint32 getSecondStep() const;

	UIDateTimeEdit* setMinDate( std::optional<CalendarDate> date );

	UIDateTimeEdit* setMaxDate( std::optional<CalendarDate> date );

	const std::optional<CalendarDate>& getMinDate() const;

	const std::optional<CalendarDate>& getMaxDate() const;

	UIDateTimeEdit* setMinTime( std::optional<TimeOfDay> time );

	UIDateTimeEdit* setMaxTime( std::optional<TimeOfDay> time );

	const std::optional<TimeOfDay>& getMinTime() const;

	const std::optional<TimeOfDay>& getMaxTime() const;

	UIDateTimeEdit* setMinDateTime( std::optional<LocalDateTime> dateTime );

	UIDateTimeEdit* setMaxDateTime( std::optional<LocalDateTime> dateTime );

	const std::optional<LocalDateTime>& getMinDateTime() const;

	const std::optional<LocalDateTime>& getMaxDateTime() const;

	UIDateTimeEdit* setWheelEditingEnabled( bool enabled );

	bool isWheelEditingEnabled() const;

	UIDateTimeEdit* setPendingDigitTimeout( const Time& timeout );

	const Time& getPendingDigitTimeout() const;

	DateTimeSection getActiveSection() const;

	UIDateTimeEdit* setActiveSection( DateTimeSection section );

	bool stepActiveSection( Int32 direction );

	std::string getSerializedValue() const;

	virtual UITextView* setText( const String& text ) override;

	virtual bool applyProperty( const StyleSheetProperty& attribute ) override;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const override;

	virtual std::vector<PropertyId> getPropertiesImplemented() const override;

	virtual void scheduledUpdate( const Time& time ) override;

  protected:
	virtual void onAutoSize() override;

	struct Section {
		size_t start{ 0 };
		size_t length{ 0 };
		Int32 minValue{ 0 };
		Int32 maxValue{ 0 };
		Uint32 displayDigits{ 0 };
		DateTimeSection type{ DateTimeSection::None };
	};

	UIDateTimeEdit();

	explicit UIDateTimeEdit( DateTimeEditMode mode, const std::string& tag = "datetimeedit" );

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;

	virtual Uint32 onTextInput( const TextInputEvent& event ) override;

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags ) override;

	virtual Uint32 onMouseWheel( const Vector2f& offset, bool flipped ) override;

	virtual void onTextChanged() override;

	virtual void onDocumentTextChanged( const DocumentContentChange& change ) override;

	virtual Uint32 onFocus( NodeFocusReason reason ) override;

	void pasteValue();

	virtual Uint32 onFocusLoss() override;

	void rebuildFormat();

	void updateTextFromValue( bool preserveSection = true );

	void rebuildSections( const FormattedDateTime& formatted );

	void updateAutoHint();

	void selectActiveSection();

	void selectSectionAtColumn( Int64 column );

	void moveActiveSection( Int32 direction );

	void updatePendingText();

	void resetPendingDigits();

	bool commitPendingDigits( bool advance );

	bool processDigit( char digit );

	bool processAmPm( char value );

	bool setSectionNumericValue( DateTimeSection section, Int32 value );

	std::optional<Section> findSection( DateTimeSection section ) const;

	SmallVector<DateTimeSection, 8> getSectionOrder() const;

	bool parseAndSetDisplayText( std::string_view text );

	bool parseAndSetSerializedValue( std::string_view text );

	void initializeValueForEditing();

	void clampCurrentValue();

	virtual void onConstraintsChange();

	virtual void onConfigurationChange();

	static DateTimeSection tokenToSection( DateTimeFormatTokenType type );

	static const char* modeToString( DateTimeEditMode mode );

	static std::optional<DateTimeEditMode> modeFromString( std::string_view mode );

	static const char* hourCycleToString( HourCycle cycle );

	DateTimeEditMode mEditMode{ DateTimeEditMode::Date };
	std::optional<CalendarDate> mDate;
	std::optional<TimeOfDay> mTime;
	std::optional<LocalDateTime> mDateTime;
	std::optional<CalendarDate> mMinDate;
	std::optional<CalendarDate> mMaxDate;
	std::optional<TimeOfDay> mMinTime;
	std::optional<TimeOfDay> mMaxTime;
	std::optional<LocalDateTime> mMinDateTime;
	std::optional<LocalDateTime> mMaxDateTime;
	DateTimeLocale mLocale;
	DateTimeFormat mDisplayFormat;
	std::string mDateFormat;
	std::string mTimeFormat;
	std::string mLastAutoHint;
	HourCycle mHourCycle{ HourCycle::Locale };
	bool mShowSeconds{ false };
	bool mShowMilliseconds{ false };
	bool mAllowEmpty{ true };
	bool mWheelEditing{ false };
	bool mUpdatingText{ false };
	bool mClampToBounds{ true };
	Uint32 mDayStep{ 1 };
	Uint32 mHourStep{ 1 };
	Uint32 mMinuteStep{ 1 };
	Uint32 mSecondStep{ 1 };
	DateTimeSection mActiveSection{ DateTimeSection::None };
	SmallVector<Section, 8> mSections;
	std::string mPendingDigits;
	Clock mPendingDigitClock;

	Time mPendingDigitTimeout{ Milliseconds( 1000 ) };
	EventConnection mAllowEditingConnection;
};

}} // namespace EE::UI

#endif
