#ifndef EE_UITOOLSCTEXTUREGROUPSHAPEEDITOR_HPP
#define EE_UITOOLSCTEXTUREGROUPSHAPEEDITOR_HPP

#include "../base.hpp"
#include "../cuicomplexcontrol.hpp"
#include "../../graphics/cshape.hpp"
#include "../cuigfx.hpp"

namespace EE { namespace UI { namespace Tools {

class cTextureGroupEditor;

class cTextureGroupShapeEditor : public cUIComplexControl {
	public:
		cTextureGroupShapeEditor( const cUIComplexControl::CreateParams& Params, cTextureGroupEditor * Editor );

		virtual ~cTextureGroupShapeEditor();

		virtual void Draw();

		virtual void Update();

		cShape * Shape() const;

		void Shape( cShape * shape );

		cUIGfx * Gfx() const;
	protected:
		cUITheme *				mTheme;
		cUIGfx *				mGfx;
		cUIDragable *			mDrag;
		eeVector2i				mUICenter;
		cTextureGroupEditor *	mEditor;

		virtual void OnSizeChange();

		void GetCenter();
};

}}}

#endif
