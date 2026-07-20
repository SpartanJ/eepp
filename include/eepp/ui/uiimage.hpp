#ifndef EE_UI_UIIMAGE_HPP
#define EE_UI_UIIMAGE_HPP

#include <atomic>
#include <eepp/network/uri.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <memory>

namespace EE { namespace UI {

class EE_API UIImage : public UIWidget {
  public:
	static UIImage* New();

	static UIImage* NewWithTag( const std::string& tag );

	virtual ~UIImage();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual void setAlpha( const Float& alpha );

	Drawable* getDrawable() const;

	UIImage* setDrawable( Drawable* drawable, bool ownIt = false );

	UIImage* setDrawable( TexturePtr texture );

	const Color& getColor() const;

	UIImage* setColor( const Color& col );

	const Vector2f& getAlignOffset() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual void scheduledUpdate( const Time& time );

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const UIScaleType& getScaleType() const;

	UIImage* setScaleType( const UIScaleType& scaleType );

  protected:
	UIScaleType mScaleType;
	Drawable* mDrawable;
	TexturePtr mTexture;
	Color mColor;
	Vector2f mAlignOffset;
	Vector2f mDestSize;
	Uint32 mResourceChangeCb;
	bool mDrawableOwner;
	bool mDeferLoad{ false };
	std::shared_ptr<std::atomic<bool>> mAsyncImageAlive;
	Uint64 mRemoteImageLoadId{ 0 };

	UIImage();

	explicit UIImage( const std::string& tag );

	virtual void onSizeChange();

	virtual void onSizePolicyChange();

	virtual void onAlignChange();

	void onAutoSize();

	void calcDestSize();

	void autoAlign();

	void safeDeleteDrawable();

	void onDrawableResourceEvent( DrawableResource::Event event, DrawableResource* );

	bool loadFileDrawable( const Network::URI& uri );

	void loadRemoteDrawable( const Network::URI& uri );
};

}} // namespace EE::UI

#endif
