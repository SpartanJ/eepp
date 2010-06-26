#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include "cuicontrolanim.hpp"

namespace EE { namespace UI {

class EE_API cUIGfx : public cUIControlAnim {
	public:
		class CreateParams : public cUIControl::CreateParams {
			public:
				inline CreateParams() : cUIControl::CreateParams() {
					Shape		= NULL;
					ShapeColor 	= eeRGBA( true );
					ShapeRender = RN_NORMAL;
				}
				
				inline ~CreateParams() {}
				
				cShape * 		Shape;
				eeRGBA 			ShapeColor;
				EE_RENDERTYPE 	ShapeRender;
		};
		
		cUIGfx( const cUIGfx::CreateParams& Params );
		
		~cUIGfx();
		
		virtual void Draw();
		
		virtual void Alpha( const eeFloat& alpha );
		
		cShape * Shape() const;
		
		const eeRGBA& Color() const;
		
		void Color( const eeRGBA& color );
		
		const EE_RENDERTYPE& RenderType() const;
		
		void RenderType( const EE_RENDERTYPE& render );
		
		virtual void OnSizeChange();
	protected:
		cShape * 		mShape;
		eeRGBA 			mColor;
		EE_RENDERTYPE 	mRender;
};

}}

#endif
