#ifndef EE_UI_UIIMAGE_HPP
#define EE_UI_UIIMAGE_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UIImage : public UIWidget {
  public:
	static UIImage* New();

	static UIImage* NewWithTag( const std::string& tag );

	UIImage();

	explicit UIImage( const std::string& tag );

	virtual ~UIImage();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual void setAlpha( const Float& alpha );

	Drawable* getDrawable() const;

	UIImage* setDrawable( Drawable* drawable, bool ownIt = false );

	const Color& getColor() const;

	UIImage* setColor( const Color& col );

	const Vector2f& getAlignOffset() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const UIScaleType& getScaleType() const;

	UIImage* setScaleType( const UIScaleType& scaleType );

  protected:
	UIScaleType mScaleType;
	Drawable* mDrawable;
	Color mColor;
	Vector2f mAlignOffset;
	Vector2f mDestSize;
	Uint32 mResourceChangeCb;
	bool mDrawableOwner;

	virtual void onSizeChange();

	virtual void onAlignChange();

	void onAutoSize();

	void calcDestSize();

	void autoAlign();

	void safeDeleteDrawable();

	void onDrawableResourceEvent( DrawableResource::Event event, DrawableResource* );
};

}} // namespace EE::UI

#endif
