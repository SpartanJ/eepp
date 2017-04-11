#ifndef EE_UICUISPRITE_HPP
#define EE_UICUISPRITE_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace Graphics {
class Sprite;
class SubTexture;
}}

namespace EE { namespace UI {

class EE_API UISprite : public UIWidget {
	public:
		static UISprite * New();

		UISprite();

		virtual ~UISprite();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void update();

		virtual void setAlpha( const Float& alpha );

		Graphics::Sprite * getSprite() const;

		void setSprite( Graphics::Sprite * sprite );

		Color getColor() const;

		void setColor( const Color& color );

		const EE_RENDER_MODE& getRenderMode() const;

		void setRenderMode( const EE_RENDER_MODE& render );

		const Vector2i& getAlignOffset() const;

		void setDeallocSprite( const bool& dealloc );

		bool getDeallocSprite();

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		Graphics::Sprite * 	mSprite;
		EE_RENDER_MODE 		mRender;
		Vector2i			mAlignOffset;
		SubTexture *		mSubTextureLast;
		bool				mDealloc;

		void updateSize();

		void autoAlign();

		void checkSubTextureUpdate();

		virtual void onSizeChange();

		Uint32 deallocSprite();
};

}}

#endif
