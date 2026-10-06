#ifndef EE_UI_UIDATEPICKER_HPP
#define EE_UI_UIDATEPICKER_HPP

#include <array>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/ui/uidatetimeedit.hpp>

namespace EE { namespace UI {

using namespace EE::System;
using namespace EE::UI::CSS;

class UICalendar;
class UIPushButton;

/** Segmented date editor with a lazily created calendar popup. Opening the popup never assigns
 * a value. Alt+Down or F4 opens it; Escape closes it and restores editor focus. */
class EE_API UIDatePicker : public UIDateTimeEdit {
  public:
	static UIDatePicker* New();

	virtual ~UIDatePicker();

	virtual Uint32 getType() const override;

	virtual bool isType( const Uint32& type ) const override;

	virtual Float getMinIntrinsicWidth() const override;

	virtual UIDatePicker* showCalendar();

	virtual UIDatePicker* hideCalendar( bool restoreFocus = true );

	bool isCalendarVisible() const;

	/** Returns nullptr until the first opening. The picker owns the calendar, even while it is
	 * parented to the window or scene root. */
	UICalendar* getCalendar() const;

	UIPushButton* getCalendarButton() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute ) override;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const override;

	virtual std::vector<PropertyId> getPropertiesImplemented() const override;

  protected:
	UIDatePicker();

	UIDatePicker( DateTimeEditMode mode, const std::string& tag );

	virtual Uint32 onKeyDown( const KeyEvent& event ) override;

	virtual void onSizeChange() override;

	virtual void onPaddingChange() override;

	virtual void onPositionChange() override;

	virtual void onVisibilityChange() override;

	virtual void onEnabledChange() override;

	virtual void draw() override;

	void updateButton();

	void closeOnFocusLoss();

	virtual void selectCalendarDate();

	virtual UIWidget* getCalendarPopup() const;

	virtual void prepareCalendarPopup();

	virtual void onConstraintsChange() override;

	virtual void onConfigurationChange() override;

	virtual void onCalendarFocusedDateChange();

	EventConnection mSceneSizeConnection;
	UICalendar* mCalendar{ nullptr };
	UIPushButton* mCalendarButton{ nullptr };
	Float mEditorRightPadding{ 0 };
	Float mReservedButtonWidth{ 0 };
	bool mUpdatingButton{ false };
	bool mCalendarVisible{ false };
	bool mPopUpToRoot{ false };
	std::array<EventConnection, 4> mConnections;
	std::array<EventConnection, 6> mCalendarConnections;
};

}} // namespace EE::UI

#endif
