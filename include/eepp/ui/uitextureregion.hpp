#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace Graphics {
class TextureRegion;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class EE_API UITextureRegion : public UIWidget {
  public:
	static UITextureRegion* New();

	UITextureRegion();

	virtual ~UITextureRegion();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual void setAlpha( const Float& alpha );

	Graphics::TextureRegion* getTextureRegion() const;

	UITextureRegion* setTextureRegion( Graphics::TextureRegion* TextureRegion );

	const Color& getColor() const;

	void setColor( const Color& col );

	const RenderMode& getRenderMode() const;

	void setRenderMode( const RenderMode& render );

	const Vector2f& getAlignOffset() const;

	const UIScaleType& getScaleType() const;

	UITextureRegion* setScaleType( const UIScaleType& scaleType );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	UIScaleType mScaleType;
	Graphics::TextureRegion* mTextureRegion;
	Color mColor;
	RenderMode mRender;
	Vector2f mAlignOffset;

	virtual void onSizeChange();

	virtual void onAlignChange();

	void onAutoSize();

	void autoAlign();

	void drawTextureRegion();
};

}} // namespace EE::UI

#endif
