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
					ShapeColor 	= eeColorA();
					ShapeRender = RN_NORMAL;
				}

				inline ~CreateParams() {}

				cShape * 		Shape;
				eeColorA 		ShapeColor;
				EE_RENDERTYPE 	ShapeRender;
		};

		cUIGfx( const cUIGfx::CreateParams& Params );

		~cUIGfx();

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cShape * Shape() const;

		void Shape( cShape * shape );

		const eeColorA& Color() const;

		void Color( const eeColorA& color );

		const EE_RENDERTYPE& RenderType() const;

		void RenderType( const EE_RENDERTYPE& render );
	protected:
		cShape * 		mShape;
		eeColorA 		mColor;
		EE_RENDERTYPE 	mRender;

		virtual void OnSizeChange();
};

}}

#endif
