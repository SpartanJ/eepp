#ifndef EE_UICUISPRITE_HPP
#define EE_UICUISPRITE_HPP

#include <eepp/graphics/sprite.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UISprite : public UIWidget {
  public:
	static UISprite* New();

	virtual ~UISprite();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual void scheduledUpdate( const Time& time );

	virtual void setAlpha( const Float& alpha );

	const Graphics::SpritePtr& getSprite() const;

	UISprite* setSprite( Graphics::SpritePtr sprite );

	Color getColor() const;

	UISprite* setColor( const Color& color );

	const RenderMode& getRenderMode() const;

	UISprite* setRenderMode( const RenderMode& render );

	const Vector2f& getAlignOffset() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	Graphics::SpritePtr mSprite;
	RenderMode mRender;
	Vector2f mAlignOffset;
	TextureRegion* mTextureRegionLast;

	UISprite();

	void updateSize();

	void autoAlign();

	void checkTextureRegionUpdate();

	virtual void onSizeChange();

};

}} // namespace EE::UI

#endif
