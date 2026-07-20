#ifndef EE_UI_UINODEDRAWABLE_HPP
#define EE_UI_UINODEDRAWABLE_HPP

#include <atomic>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/scene/action.hpp>
#include <eepp/ui/uibackgrounddrawable.hpp>
#include <map>
#include <memory>

using namespace EE::Graphics;
using namespace EE::Scene;

namespace EE { namespace UI {

class UINode;

enum class BackgroundMode { Native, Html };

class EE_API UINodeDrawable : public Drawable {
  public:
	enum class RepeatX { NoRepeat, Repeat, Space, Round };
	enum class RepeatY { NoRepeat, Repeat, Space, Round };

	static void repeatFromText( const std::string& text, RepeatX& repeatX, RepeatY& repeatY );

	class EE_API LayerDrawable : public Drawable {
	  public:
		enum class Origin { PaddingBox, BorderBox, ContentBox };
		enum class Clip { BorderBox, PaddingBox, ContentBox };
		enum class Attachment { Scroll, Fixed, Local };

		static Origin originFromText( const std::string& text );
		static Clip clipFromText( const std::string& text );
		static Attachment attachmentFromText( const std::string& text );

		static LayerDrawable* New( UINodeDrawable* container );

		LayerDrawable( UINodeDrawable* container );

		virtual ~LayerDrawable();

		bool isStateful() { return false; }

		virtual void draw();

		virtual void draw( const Vector2f& position );

		virtual void draw( const Vector2f& position, const Sizef& size );

		virtual Sizef getSize();

		virtual Sizef getPixelsSize();

		virtual void setSize( const Sizef& size );

		Drawable* getDrawable() const;

		const std::string& getDrawableRef() const;

		void setDrawable( Drawable* drawable, const bool& ownIt );

		void setDrawable( TexturePtr texture );

		void setDrawable( const std::string& drawableRef );

		void setOffset( const Vector2f& offset );

		const Vector2f& getOffset() const;

		std::string getOffsetEq();

		void setPositionEq( const std::string& offset );

		void setSizeEq( const std::string& size );

		const std::string& getSizeEq() const;

		RepeatX getRepeatX() const;

		RepeatY getRepeatY() const;

		void setRepeatX( RepeatX repeatX );

		void setRepeatY( RepeatY repeatY );

		void invalidate();

		const Sizef& getDrawableSize() const;

		void setDrawableSize( const Sizef& drawableSize );

		Sizef calcDrawableSize( const std::string& drawableSizeEq );

		Vector2f calcPosition( std::string positionXEq, std::string positionYEq );

		const std::string& getPositionX() const;

		void setPositionX( const std::string& positionX );

		const std::string& getPositionY() const;

		void setPositionY( const std::string& positionY );

		void setOrigin( const std::string& origin );

		void setClip( const std::string& clip );

		void setAttachment( const std::string& attachment );

		Origin getOrigin() const;

		Clip getClip() const;

		Attachment getAttachment() const;

		const std::string& getOriginEq() const;

		const std::string& getClipEq() const;

		const std::string& getAttachmentEq() const;

	  protected:
		UINodeDrawable* mContainer;
		Sizef mSize;
		Sizef mDrawableSize;
		Vector2f mOffset;
		std::string mPositionX;
		std::string mPositionY;
		std::string mSizeEq;
		bool mNeedsUpdate{ false };
		bool mOwnsDrawable{ false };
		bool mColorWasSet{ false };
		Drawable* mDrawable;
		TexturePtr mTexture;
		std::string mDrawableRef;
		Uint32 mResourceChangeCbId;
		RepeatX mRepeatX{ RepeatX::NoRepeat };
		RepeatY mRepeatY{ RepeatY::NoRepeat };
		std::string mOriginEq{ "padding-box" };
		std::string mClipEq{ "border-box" };
		std::string mAttachmentEq{ "scroll" };
		Origin mOrigin{ Origin::PaddingBox };
		Clip mClip{ Clip::BorderBox };
		Attachment mAttachment{ Attachment::Scroll };
		std::shared_ptr<std::atomic<bool>> mAsyncDrawableAlive;
		Uint64 mRemoteDrawableLoadId{ 0 };

		virtual void onPositionChange();

		virtual void onColorFilterChange();

		void update();

		Drawable* createDrawable( const std::string& value, const Sizef& size, bool& ownIt );

		bool loadRemoteDrawable( const std::string& value );
	};

	static UINodeDrawable* New( UINode* owner );

	UINodeDrawable( UINode* owner );

	virtual ~UINodeDrawable();

	virtual Sizef getSize();

	virtual Sizef getPixelsSize();

	virtual void setSize( const Sizef& size );

	virtual void draw();

	virtual void draw( const Vector2f& position );

	virtual void draw( const Vector2f& position, const Sizef& size );

	void draw( const Vector2f& position, const Sizef& size, const Uint32& alpha );

	virtual bool isStateful() { return false; }

	void clearDrawables();

	void setBorderRadius( const Uint32& radius );

	Uint32 getBorderRadius() const;

	bool layerExists( int index );

	LayerDrawable* getLayer( int index );

	void setDrawable( int index, Drawable* drawable, bool ownIt );

	void setDrawable( int index, const std::string& drawable );

	void setDrawablePositionX( int index, const std::string& positionX );

	void setDrawablePositionY( int index, const std::string& positionY );

	void setDrawableRepeat( int index, const std::string& repeatRule );

	void setDrawableSize( int index, const std::string& sizeEq );

	void setDrawableOrigin( int index, const std::string& origin );

	void setDrawableClip( int index, const std::string& clip );

	void setDrawableAttachment( int index, const std::string& attachment );

	void setDrawableColor( int index, const Color& color );

	void setBackgroundColor( const Color& color );

	Color getBackgroundColor() const;

	bool getClipEnabled() const;

	void setClipEnabled( bool clipEnabled );

	void invalidate();

	UINode* getOwner() const;

	UIBackgroundDrawable& getBackgroundDrawable();

	bool hasDrawableLayers() const;

	bool isSmooth() const;

	void setSmooth( bool smooth );

	void setBackgroundMode( BackgroundMode mode );

	BackgroundMode getBackgroundMode() const;

  protected:
	UINode* mOwner;
	UIBackgroundDrawable mBackgroundColor;
	std::map<int, LayerDrawable*> mGroup;
	Sizef mSize;
	bool mNeedsUpdate{ true };
	bool mClipEnabled{ false };
	bool mSmooth{ false };
	BackgroundMode mBackgroundMode{ BackgroundMode::Native };

	virtual void onPositionChange();

	virtual void onSizeChange();

	void update();
};

}} // namespace EE::UI

#endif
