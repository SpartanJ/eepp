#ifndef EE_UI_UIHTMLINPUT_HPP
#define EE_UI_UIHTMLINPUT_HPP

#include <eepp/scene/eventconnection.hpp>
#include <eepp/system/datetime.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <optional>

namespace EE { namespace UI {

using namespace EE::System;

/** Constraint flags for the implemented HTML temporal input states. */
struct HTMLInputValidity {
	bool valueMissing{ false };
	bool rangeUnderflow{ false };
	bool rangeOverflow{ false };
	bool stepMismatch{ false };
	bool badInput{ false };

	bool valid() const {
		return !( valueMissing || rangeUnderflow || rangeOverflow || stepMismatch || badInput );
	}
};

class EE_API UIHTMLInput : public UIHTMLWidget {
  public:
	static UIHTMLInput* New();

	UIHTMLInput();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual void updateLayout();

	Float getReplacedElementBaseline() const;

	const std::string& getInputType() const;

	void setInputType( const std::string& type );

	UIWidget* getChildWidget() const;

	virtual String getFormValue() const;

	/** HTML constraint validation for date, time and datetime-local inputs. Read-only and
	 * disabled controls are barred from validation. */
	HTMLInputValidity getValidity() const;

	bool willValidate() const;

	bool checkValidity() const;

  protected:
	std::string mInputType{ "text" };
	UIWidget* mChildWidget{ nullptr };
	EventConnection mControlChildrenConnection;
	EventConnection mPopupTimeChildrenConnection;
	std::map<PropertyId, StyleSheetProperty> mImplementationProperties;
	String mValue;
	bool mChecked{ false };
	bool mSyncingGeometry{ false };
	bool mHasSpecifiedHeight{ false };
	bool mHiddenByType{ false };
	bool mVisibleBeforeHidden{ true };
	bool mEnabledBeforeHidden{ true };
	CSSDisplay mDisplayBeforeHidden{ CSSDisplay::InlineBlock };

	void createChildWidget();

	void configureChildWidget();

	void applyImplementationProperty( const StyleSheetProperty& property );

	void syncImplementationState();

	void syncStateFromImplementation();

	void updateHostGeometry();

	void updateChildGeometry();

	void syncCheckedState();

	bool isTemporalInput() const;

	void setTemporalValue( const String& value );

	void syncTemporalConfiguration();

	const std::string& temporalAttribute( PropertyId id ) const;

	std::optional<LocalDateTime> getTemporalValue() const;

	virtual void onSizeChange();

	virtual void onPaddingChange();
};

}} // namespace EE::UI

#endif
