#ifndef EE_UICUISPINBOX_HPP
#define EE_UICUISPINBOX_HPP

#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class EE_API UISpinBox : public UIWidget {
  public:
	static UISpinBox* New();

	UISpinBox();

	virtual ~UISpinBox();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void setPadding( const Rectf& padding );

	const Rectf& getPadding() const;

	virtual void setClickStep( const Float& step );

	const Float& getClickStep() const;

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void addValue( const Float& value );

	virtual UISpinBox* setMinValue( const Float& MinVal );

	const Float& getMinValue() const;

	virtual UISpinBox* setMaxValue( const Float& MaxVal );

	const Float& getMaxValue() const;

	virtual UISpinBox* setValue( const Float& Val );

	const Float& getValue() const;

	UINode* getButtonPushUp() const;

	UINode* getButtonPushDown() const;

	UITextInput* getTextInput() const;

	UISpinBox* setAllowOnlyNumbers( bool allow );

	bool dotsInNumbersAllowed();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef );

  protected:
	UITextInput* mInput;
	UINode* mPushUp;
	UINode* mPushDown;
	Float mMinValue;
	Float mMaxValue;
	Float mValue;
	Float mClickStep;

	void adjustChilds();

	void internalValue( const Float& Val, const bool& Force = false );

	virtual void onSizeChange();

	virtual void onPositionChange();

	virtual void onAlphaChange();

	virtual void onPaddingChange();

	virtual void onBufferChange( const Event* event );
};

}} // namespace EE::UI

#endif
