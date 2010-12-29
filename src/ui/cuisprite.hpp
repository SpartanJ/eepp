#ifndef EE_UICUISPRITE_HPP
#define EE_UICUISPRITE_HPP

#include "cuicomplexcontrol.hpp"

namespace EE { namespace UI {

class EE_API cUISprite : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					Sprite( NULL ),
					SpriteRender( RN_NORMAL )
				{
				}

				inline ~CreateParams() {}

				cSprite * 			Sprite;
				EE_RENDERTYPE		SpriteRender;
		};

		cUISprite( const cUISprite::CreateParams& Params );

		~cUISprite();

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cSprite * Sprite() const;

		void Sprite( cSprite * sprite );

		eeColorA Color() const;

		void Color( const eeColorA& color );

		const EE_RENDERTYPE& RenderType() const;

		void RenderType( const EE_RENDERTYPE& render );

		const eeVector2i& AlignOffset() const;
	protected:
		cSprite * 			mSprite;
		EE_RENDERTYPE 		mRender;
		eeVector2i			mAlignOffset;
		cShape *			mShapeLast;

		void UpdateSize();

		void AutoAlign();

		void CheckShapeUpdate();

		virtual void OnSizeChange();
};

}}

#endif
