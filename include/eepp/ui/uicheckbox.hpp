#ifndef EE_UICUICHECKBOX_H
#define EE_UICUICHECKBOX_H

#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextview.hpp>

namespace EE { namespace UI {

class EE_API UICheckBox : public UITextView {
  public:
	enum CheckMode { TextAndButton, Button };

	static UICheckBox* New();

	static UICheckBox* NewWithTag( const std::string& tag );

	virtual ~UICheckBox();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	const bool& isChecked() const;

	UICheckBox* setChecked( const bool& checked );

	UIWidget* getCheckedButton() const;

	UIWidget* getInactiveButton() const;

	UIWidget* getCurrentButton() const;

	Int32 getTextSeparation() const;

	void setTextSeparation( const Int32& textSeparation );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	bool getCheckMode() const;

	void setCheckMode( CheckMode mode );

  protected:
	UIWidget* mActiveButton;
	UIWidget* mInactiveButton;
	bool mChecked;
	CheckMode mCheckMode{ CheckMode::TextAndButton };
	Uint32 mLastTick;
	Int32 mTextSeparation;

	UICheckBox();

	UICheckBox( const std::string& tag );

	virtual void onSizeChange();

	void switchState();

	virtual void onAlphaChange();

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onThemeLoaded();

	virtual void onAutoSize();

	virtual void onPaddingChange();

	virtual void onTextChanged();

	virtual void alignFix();
};

}} // namespace EE::UI

#endif
