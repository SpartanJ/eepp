#ifndef EE_UICUIRADIOBUTTON_H
#define EE_UICUIRADIOBUTTON_H

#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextview.hpp>

namespace EE { namespace UI {

class EE_API UIRadioButton : public UITextView {
  public:
	static UIRadioButton* New();

	UIRadioButton();

	virtual ~UIRadioButton();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	const bool& isActive() const;

	void setActive( const bool& active );

	UIWidget* getActiveButton() const;

	UIWidget* getInactiveButton() const;

	Int32 getTextSeparation() const;

	void setTextSeparation( const Int32& textSeparation );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	UIWidget* mActiveButton;
	UIWidget* mInactiveButton;
	bool mActive;
	Uint32 mLastTick;
	Int32 mTextSeparation;

	virtual void onSizeChange();

	void switchState();

	void autoActivate();

	bool checkActives();

	virtual void onAlphaChange();

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onAutoSize();

	virtual void onThemeLoaded();

	virtual void onPaddingChange();

	virtual void alignFix();
};

}} // namespace EE::UI

#endif
