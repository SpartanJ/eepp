#ifndef EE_UI_UILOADER_HPP
#define EE_UI_UILOADER_HPP

#include <eepp/graphics/arcdrawable.hpp>
#include <eepp/graphics/circledrawable.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UILoader : public UIWidget {
  public:
	static UILoader* New();

	UILoader();

	~UILoader();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual void scheduledUpdate( const Time& time );

	UILoader* setOutlineThickness( const Float& thickness );

	const Float& getOutlineThickness() const;

	UILoader* setRadius( const Float& radius );

	const Float& getRadius() const;

	UILoader* setFillColor( const Color& color );

	const Color& getFillColor() const;

	const bool& isIndeterminate() const;

	UILoader* setIndeterminate( const bool& indeterminate );

	UILoader* setProgress( const Float& progress );

	const Float& getProgress() const;

	const Float& getMaxProgress() const;

	UILoader* setMaxProgress( const Float& maxProgress );

	const Float& getAnimationSpeed() const;

	UILoader* setAnimationSpeed( const Float& animationSpeed );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	Float getArcStartAngle() const;

	UILoader* setArcStartAngle( const Float& arcStartAngle );

  protected:
	Float mRadius;
	Float mOutlineThickness;
	ArcDrawable mArc;
	CircleDrawable mCircle;
	Color mColor;
	Float mArcAngle;
	Float mArcStartAngle;
	Float mProgress;
	Float mMaxProgress;
	Float mAnimationSpeed;
	IntPtr mOp;
	bool mIndeterminate;

	virtual void onAutoSize();

	virtual void onSizeChange();

	virtual void onPaddingChange();

	void updateRadius();
};

}} // namespace EE::UI

#endif
