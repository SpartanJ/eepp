#ifndef EE_UI_UIDATETIMEPICKER_HPP
#define EE_UI_UIDATETIMEPICKER_HPP

#include <eepp/ui/uidatepicker.hpp>

namespace EE { namespace UI {

class UITimePicker;

/** One segmented local date/time field with a calendar and time editor popup. Date selection
 * preserves the time and keeps the popup open. Changes commit immediately; Enter in the time
 * editor or Escape closes it. Opening an empty picker does not assign a value. */
class EE_API UIDateTimePicker : public UIDatePicker {
  public:
	static UIDateTimePicker* New();

	virtual ~UIDateTimePicker();

	virtual UIDateTimePicker* hideCalendar( bool restoreFocus = true ) override;

	/** Returns nullptr until the first popup opening. The popup owns the time editor. */
	UITimePicker* getPopupTimePicker() const;

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

  protected:
	UIDateTimePicker();

	virtual UIWidget* getCalendarPopup() const override;

	virtual void prepareCalendarPopup() override;

	virtual void selectCalendarDate() override;

	virtual void onConstraintsChange() override;

	virtual void onConfigurationChange() override;

	virtual void onCalendarFocusedDateChange() override;

	void synchronizeTimePopup();

	void selectPopupTime();

	UIWidget* mPopup{ nullptr };
	UITimePicker* mPopupTimePicker{ nullptr };
	bool mSynchronizingPopup{ false };
	std::array<EventConnection, 2> mConnections;
	std::array<EventConnection, 4> mPopupConnections;
	std::array<EventConnection, 2> mTimeConnections;
};

}} // namespace EE::UI

#endif
