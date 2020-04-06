#ifndef EE_UICUICHECKBOX_H
#define EE_UICUICHECKBOX_H

#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextview.hpp>

namespace EE { namespace UI {

class EE_API UICheckBox : public UITextView {
  public:
	static UICheckBox* New();

	UICheckBox();

	virtual ~UICheckBox();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	const bool& isChecked() const;

	void setChecked( const bool& checked );

	UIWidget* getCheckedButton() const;

	UIWidget* getInactiveButton() const;

	Int32 getTextSeparation() const;

	void setTextSeparation( const Int32& textSeparation );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	std::string getPropertyString( const PropertyDefinition* propertyDef,
								   const Uint32& propertyIndex = 0 );

  protected:
	UIWidget* mActiveButton;
	UIWidget* mInactiveButton;
	bool mChecked;
	Uint32 mLastTick;
	Int32 mTextSeparation;

	virtual void onSizeChange();

	void switchState();

	virtual void onAlphaChange();

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onThemeLoaded();

	virtual void onAutoSize();

	virtual void onPaddingChange();

	virtual void alignFix();
};

}} // namespace EE::UI

#endif
