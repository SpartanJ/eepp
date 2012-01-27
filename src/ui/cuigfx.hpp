#ifndef EE_UICUIGFX_H
#define EE_UICUIGFX_H

#include "cuicomplexcontrol.hpp"

namespace EE { namespace UI {

class EE_API cUIGfx : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					Shape( NULL ),
					ShapeColor(),
					ShapeRender( RN_NORMAL )
				{
				}

				inline ~CreateParams() {}

				cShape * 		Shape;
				eeColorA 		ShapeColor;
				EE_RENDERTYPE 	ShapeRender;
		};

		cUIGfx( const cUIGfx::CreateParams& Params );

		virtual ~cUIGfx();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cShape * Shape() const;

		void Shape( cShape * shape );

		const eeColorA& Color() const;

		void Color( const eeColorA& color );

		const EE_RENDERTYPE& RenderType() const;

		void RenderType( const EE_RENDERTYPE& render );

		const eeVector2i& AlignOffset() const;
	protected:
		cShape * 		mShape;
		eeColorA 		mColor;
		EE_RENDERTYPE 	mRender;
		eeVector2i		mAlignOffset;

		virtual void OnSizeChange();

		void AutoSize();

		void AutoAlign();

		void DrawShape();
};

}}

#endif
