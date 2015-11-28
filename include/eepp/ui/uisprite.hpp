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

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Draw();

		virtual void Alpha( const Float& alpha );

		Graphics::Sprite * Sprite() const;

		void Sprite( Graphics::Sprite * sprite );

		ColorA Color() const;

		void Color( const ColorA& color );

		const EE_RENDER_MODE& RenderMode() const;

		void RenderMode( const EE_RENDER_MODE& render );

		const Vector2i& AlignOffset() const;
	protected:
		Graphics::Sprite * 	mSprite;
		EE_RENDER_MODE 		mRender;
		Vector2i			mAlignOffset;
		SubTexture *		mSubTextureLast;
		bool				mDealloc;

		void UpdateSize();

		void AutoAlign();

		void CheckSubTextureUpdate();

		virtual void OnSizeChange();

		Uint32 DealloSprite();
};

}}

#endif
