#ifndef EE_UICUISPRITE_HPP
#define EE_UICUISPRITE_HPP

#include <eepp/ui/cuicomplexcontrol.hpp>

namespace EE { namespace Graphics {
class cSprite;
}}

namespace EE { namespace UI {

class EE_API cUISprite : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					Sprite( NULL ),
					SpriteRender( RN_NORMAL ),
					DeallocSprite( true )
				{
				}

				inline ~CreateParams() {}

				cSprite * 			Sprite;
				EE_RENDER_MODE		SpriteRender;
				bool				DeallocSprite;
		};

		cUISprite( const cUISprite::CreateParams& Params );

		virtual ~cUISprite();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cSprite * Sprite() const;

		void Sprite( cSprite * sprite );

		eeColorA Color() const;

		void Color( const eeColorA& color );

		const EE_RENDER_MODE& RenderMode() const;

		void RenderMode( const EE_RENDER_MODE& render );

		const eeVector2i& AlignOffset() const;
	protected:
		cSprite * 			mSprite;
		EE_RENDER_MODE 		mRender;
		eeVector2i			mAlignOffset;
		cSubTexture *			mSubTextureLast;
		bool				mDealloc;

		void UpdateSize();

		void AutoAlign();

		void CheckSubTextureUpdate();

		virtual void OnSizeChange();

		Uint32 DeallocSprite();
};

}}

#endif
