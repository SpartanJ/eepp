#ifndef EE_UIUISlider_HPP
#define EE_UIUISlider_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UISlider : public UIWidget {
  public:
	static UISlider* New();

	static UISlider* NewWithTag( const std::string& tag, const UIOrientation& orientation );

	static UISlider* NewVertical();

	static UISlider* NewHorizontal();

	static UISlider* NewVerticalWithTag( const std::string& tag );

	static UISlider* NewHorizontalWithTag( const std::string& tag );

	UISlider( const std::string& tag,
			  const UIOrientation& orientation = UIOrientation::Horizontal );

	virtual ~UISlider();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	virtual void setValue( Float val, bool emmitEvent = true );

	const Float& getValue() const;

	virtual void setMinValue( const Float& MinVal );

	const Float& getMinValue() const;

	virtual void setMaxValue( const Float& MaxVal );

	const Float& getMaxValue() const;

	virtual void setClickStep( const Float& step );

	const Float& getClickStep() const;

	bool isVertical() const;

	UIWidget* getBackSlider() const;

	UIWidget* getSliderButton() const;

	void adjustChilds();

	void manageClick( const Uint32& flags );

	UIOrientation getOrientation() const;

	UISlider* setOrientation( const UIOrientation& orientation, std::string childsBaseTag = "" );

	bool getAllowHalfSliderOut() const;

	void setAllowHalfSliderOut( bool allowHalfSliderOut );

	bool getExpandBackground() const;

	void setExpandBackground( bool expandBackground );

	Float getPageStep() const;

	void setPageStep( const Float& pageStep );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	Sizef getMinimumSize();

	bool isDragging() const;

  protected:
	UIOrientation mOrientation;
	bool mAllowHalfSliderOut;
	bool mExpandBackground;
	bool mUpdating;
	UIWidget* mBackSlider;
	UIWidget* mSlider;
	Float mMinValue;
	Float mMaxValue;
	Float mValue;
	Float mClickStep;
	Float mPageStep;

	bool mOnPosChange;

	Uint32 mLastTickMove;

	virtual void onAutoSize();

	virtual void onSizeChange();

	virtual void onPaddingChange();

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	void fixSliderPos();

	void adjustSliderPos();

	virtual Uint32 onKeyDown( const KeyEvent& Event );

	virtual void onAlphaChange();

	virtual Uint32 onMessage( const NodeMessage* Msg );
};

}} // namespace EE::UI

#endif
