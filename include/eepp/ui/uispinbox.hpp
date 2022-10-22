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

	virtual void setClickStep( const double& step );

	const double& getClickStep() const;

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void addValue( const double& value );

	virtual UISpinBox* setMinValue( const double& minVal );

	const double& getMinValue() const;

	virtual UISpinBox* setMaxValue( const double& maxVal );

	const double& getMaxValue() const;

	virtual UISpinBox* setValue( const double& val );

	const double& getValue() const;

	UINode* getButtonPushUp() const;

	UINode* getButtonPushDown() const;

	UITextInput* getTextInput() const;

	UISpinBox* allowFloatingPoint( bool allow );

	bool dotsInNumbersAllowed();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	UITextInput* mInput;
	UIWidget* mPushUp;
	UIWidget* mPushDown;
	double mMinValue;
	double mMaxValue;
	double mValue;
	double mClickStep;
	bool mModifyingVal;

	void adjustChilds();

	virtual void onSizeChange();

	virtual void onPositionChange();

	virtual void onAlphaChange();

	virtual void onPaddingChange();

	virtual void onBufferChange( const Event* event );
};

}} // namespace EE::UI

#endif
