#ifndef EE_UI_UIHTMLIMAGE_HPP
#define EE_UI_UIHTMLIMAGE_HPP

#include <atomic>
#include <eepp/network/uri.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <memory>

namespace EE { namespace UI {

/** HTML replaced image element. CSS box/layout behavior comes from UIHTMLWidget. */
class EE_API UIHTMLImage : public UIHTMLWidget {
  public:
	static UIHTMLImage* New();

	virtual ~UIHTMLImage();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	virtual void draw();

	virtual void setAlpha( const Float& alpha );

	const DrawablePtr& getDrawable() const;

	UIHTMLImage* setDrawable( DrawablePtr drawable );

	UIHTMLImage* setDrawable( TexturePtr texture );

	const Color& getColor() const;

	UIHTMLImage* setColor( const Color& col );

	const Vector2f& getAlignOffset() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual void scheduledUpdate( const Time& time );

	virtual void updateLayout();

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const UIScaleType& getScaleType() const;

	UIHTMLImage* setScaleType( const UIScaleType& scaleType );

	const std::string& getAlt() const;

	UIHTMLImage* setAlt( const std::string& alt );

	virtual bool isInline() const;

  protected:
	UIHTMLImage();

	virtual void onSizeChange();

	virtual void onSizePolicyChange();

	virtual void onAlignChange();

	virtual void onParentSizeChange( const Vector2f& sizeChange );

	virtual void onDisplayChange();

	void autoSizeImage();

	void calcDestSize();

	void clearDrawable();

	void onDrawableResourceChange();

	bool loadFileDrawable( const Network::URI& uri );

	void loadRemoteDrawable( const Network::URI& uri );

	UIScaleType mScaleType{ UIScaleType::Expand };
	DrawablePtr mDrawable;
	Color mColor;
	Vector2f mAlignOffset;
	Vector2f mDestSize;
	DrawableResourceConnection mResourceChangeConnection;
	Uint32 mSpriteChangeCb{ 0 };
	bool mDeferLoad{ false };
	std::shared_ptr<std::atomic<bool>> mAsyncImageAlive;
	Uint64 mRemoteImageLoadId{ 0 };
	std::string mAlt;
};

}} // namespace EE::UI

#endif
