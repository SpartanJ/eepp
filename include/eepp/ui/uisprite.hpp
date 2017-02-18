#ifndef EE_UICUISPRITE_HPP
#define EE_UICUISPRITE_HPP

#include <eepp/ui/uicomplexcontrol.hpp>

namespace EE { namespace Graphics {
class Sprite;
}}

namespace EE { namespace UI {

class EE_API UISprite : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					Sprite( NULL ),
					SpriteRender( RN_NORMAL ),
					DealloSprite( true )
				{
				}

				inline ~CreateParams() {}

				Graphics::Sprite * 	Sprite;
				EE_RENDER_MODE		SpriteRender;
				bool				DealloSprite;
		};

		UISprite( const UISprite::CreateParams& Params );

		virtual ~UISprite();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Graphics::Sprite * getSprite() const;

		void setSprite( Graphics::Sprite * sprite );

		ColorA getColor() const;

		void setColor( const ColorA& color );

		const EE_RENDER_MODE& getRenderMode() const;

		void setRenderMode( const EE_RENDER_MODE& render );

		const Vector2i& getAlignOffset() const;
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
