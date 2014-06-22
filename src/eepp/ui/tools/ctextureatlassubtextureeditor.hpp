#ifndef EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/ui/cuigfx.hpp>

namespace EE { namespace UI { namespace Tools {

class TextureAtlasEditor;

class EE_API TextureAtlasSubTextureEditor : public cUIComplexControl {
	public:
		TextureAtlasSubTextureEditor( const cUIComplexControl::CreateParams& Params, TextureAtlasEditor * Editor );

		virtual ~TextureAtlasSubTextureEditor();

		virtual void Draw();

		virtual void Update();

		Graphics::SubTexture * SubTexture() const;

		void SubTexture( Graphics::SubTexture * subTexture );

		cUIGfx * Gfx() const;
	protected:
		cUITheme *				mTheme;
		cUIGfx *				mGfx;
		cUIDragable *			mDrag;
		Vector2i				mUICenter;
		TextureAtlasEditor *	mEditor;

		virtual void OnSizeChange();

		void GetCenter();
};

}}}

#endif
