#ifndef EE_UI_UICALENDAR_HPP
#define EE_UI_UICALENDAR_HPP

#include <array>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/system/datetime.hpp>
#include <eepp/system/datetimeformat.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <optional>

namespace EE { namespace UI {

using namespace EE::System;
using namespace EE::UI::CSS;

class UILinearLayout;
class UIGridLayout;
class UIPushButton;
class UITextView;

enum class CalendarView : Uint8 { Days, Months, Years };

/** Standalone Gregorian calendar widget used by the date picker family.
 *
 * Display navigation is intentionally independent from the selected value. Navigating months or
 * years never changes the selection until a date is explicitly selected.
 */
class EE_API UICalendar : public UIWidget {
  public:
	static UICalendar* New();

	virtual ~UICalendar() = default;

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	UICalendar* setSelectedDate( const CalendarDate& date );

	UICalendar* setSelectedDate( std::optional<CalendarDate> date );

	const std::optional<CalendarDate>& getSelectedDate() const;

	UICalendar* clearSelection();

	UICalendar* setDisplayedMonth( const CalendarDate& date );

	const CalendarDate& getDisplayedMonth() const;

	UICalendar* setFocusedDate( const CalendarDate& date );

	const CalendarDate& getFocusedDate() const;

	UICalendar* setMinDate( std::optional<CalendarDate> date );

	UICalendar* setMaxDate( std::optional<CalendarDate> date );

	const std::optional<CalendarDate>& getMinDate() const;

	const std::optional<CalendarDate>& getMaxDate() const;

	UICalendar* setView( CalendarView view );

	CalendarView getView() const;

	UICalendar* setLocale( const DateTimeLocale& locale );

	const DateTimeLocale& getLocale() const;

	UICalendar* setFirstDayOfWeek( Uint8 weekday );

	Uint8 getFirstDayOfWeek() const;

	UICalendar* previous();

	UICalendar* next();

	UICalendar* goToToday();

	bool selectFocusedDate();

	/** First date rendered by the fixed 42-cell day grid. Useful for tests/inspection. */
	CalendarDate getFirstVisibleDate() const;

	/** Returns the date represented by one day cell (0..41). */
	CalendarDate getDayCellDate( size_t index ) const;

	size_t getDayCellCount() const { return mDayCells.size(); }

	virtual bool applyProperty( const StyleSheetProperty& attribute ) override;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const override;

	virtual std::vector<PropertyId> getPropertiesImplemented() const override;

  protected:
	UICalendar();

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;

	virtual void onSizeChange() override;

	virtual void onThemeLoaded() override;

	virtual void onAutoSize() override;

	void buildUI();

	void refresh();

	void refreshHeader();

	void refreshDays();

	void refreshMonths();

	void refreshYears();

	void updateCellClasses( UIPushButton* button, bool selected, bool today, bool outside,
							bool focused );

	bool isDateEnabled( const CalendarDate& date ) const;

	CalendarDate clampDate( CalendarDate date ) const;

	void moveFocusedDate( Int64 days );

	void setFocusedAndDisplay( const CalendarDate& date );

	void handleDayClicked( size_t index );

	void handleOptionClicked( size_t index );

	void updateLayoutSize();

	static int dayOfWeek( const CalendarDate& date ); // Sunday=0

	static const char* viewToString( CalendarView view );

	DateTimeLocale mLocale;
	CalendarDate mDisplayedMonth;
	CalendarDate mFocusedDate;
	std::optional<CalendarDate> mSelectedDate;
	std::optional<CalendarDate> mMinDate;
	std::optional<CalendarDate> mMaxDate;
	Int64 mYearPageStart{ 0 };
	CalendarView mView{ CalendarView::Days };
	bool mUpdatingLayout{ false };

	UILinearLayout* mMainLayout{ nullptr };
	UILinearLayout* mHeader{ nullptr };
	UIPushButton* mPreviousButton{ nullptr };
	UIPushButton* mTitleButton{ nullptr };
	UIPushButton* mNextButton{ nullptr };
	UIGridLayout* mWeekdayRow{ nullptr };
	UIGridLayout* mDayGrid{ nullptr };
	UIGridLayout* mOptionGrid{ nullptr };
	UIPushButton* mTodayButton{ nullptr };
	std::array<UITextView*, 7> mWeekdayLabels{};
	std::array<UIPushButton*, 42> mDayCells{};
	std::array<CalendarDate, 42> mDayCellDates{};
	std::array<UIPushButton*, 12> mOptionCells{};
	EventConnectionList mConnections;
};

}} // namespace EE::UI

#endif
