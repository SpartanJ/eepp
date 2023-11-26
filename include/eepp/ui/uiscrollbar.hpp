#ifndef EE_UICUISCROLLBAR_HPP
#define EE_UICUISCROLLBAR_HPP

#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIScrollBar : public UIWidget {
  public:
	enum ScrollBarType { TwoButtons, NoButtons };

	static UIScrollBar* New();

	static UIScrollBar* NewHorizontal();

	static UIScrollBar* NewVertical();

	static UIScrollBar* NewWithTag( const std::string& tag );

	static UIScrollBar* NewHorizontalWithTag( const std::string& tag );

	static UIScrollBar* NewVerticalWithTag( const std::string& tag );

	explicit UIScrollBar( const std::string& tag = "scrollbar",
						  const UIOrientation& orientation = UIOrientation::Vertical );

	virtual ~UIScrollBar();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setValue( Float val, const bool& emmitEvent = true );

	const Float& getValue() const;

	virtual void setMinValue( const Float& MinVal );

	const Float& getMinValue() const;

	virtual void setMaxValue( const Float& MaxVal );

	const Float& getMaxValue() const;

	virtual void setClickStep( const Float& step );

	const Float& getClickStep() const;

	Float getPageStep() const;

	void setPageStep( const Float& pageStep );

	virtual void setTheme( UITheme* Theme );

	bool isVertical() const;

	UISlider* getSlider() const;

	UINode* getButtonUp() const;

	UINode* getButtonDown() const;

	UIOrientation getOrientation() const;

	UINode* setOrientation( const UIOrientation& orientation );

	ScrollBarType getScrollBarType() const;

	void setScrollBarStyle( const ScrollBarType& scrollBarType );

	bool getExpandBackground() const;

	void setExpandBackground( bool expandBackground );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	bool isDragging() const;

  protected:
	ScrollBarType mScrollBarStyle;
	UISlider* mSlider;
	UIWidget* mBtnUp;
	UIWidget* mBtnDown;

	virtual void onSizeChange();

	virtual void onAutoSize();

	void adjustChilds();

	void onValueChangeCb( const Event* Event );

	virtual void onAlphaChange();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void onPaddingChange();
};

}} // namespace EE::UI

#endif
