#ifndef EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/graphics/csubtexture.hpp>
#include <eepp/ui/cuigfx.hpp>

namespace EE { namespace UI { namespace Tools {

class cTextureAtlasEditor;

class EE_API cTextureAtlasSubTextureEditor : public cUIComplexControl {
	public:
		cTextureAtlasSubTextureEditor( const cUIComplexControl::CreateParams& Params, cTextureAtlasEditor * Editor );

		virtual ~cTextureAtlasSubTextureEditor();

		virtual void Draw();

		virtual void Update();

		cSubTexture * SubTexture() const;

		void SubTexture( cSubTexture * subTexture );

		cUIGfx * Gfx() const;
	protected:
		cUITheme *				mTheme;
		cUIGfx *				mGfx;
		cUIDragable *			mDrag;
		Vector2i				mUICenter;
		cTextureAtlasEditor *	mEditor;

		virtual void OnSizeChange();

		void GetCenter();
};

}}}

#endif
