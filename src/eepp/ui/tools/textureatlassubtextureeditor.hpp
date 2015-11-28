#ifndef EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/ui/uigfx.hpp>

namespace EE { namespace UI { namespace Tools {

class TextureAtlasEditor;

class EE_API TextureAtlasSubTextureEditor : public UIComplexControl {
	public:
		TextureAtlasSubTextureEditor( const UIComplexControl::CreateParams& Params, TextureAtlasEditor * Editor );

		virtual ~TextureAtlasSubTextureEditor();

		virtual void Draw();

		virtual void Update();

		Graphics::SubTexture * SubTexture() const;

		void SubTexture( Graphics::SubTexture * subTexture );

		UIGfx * Gfx() const;
	protected:
		UITheme *				mTheme;
		UIGfx *				mGfx;
		UIDragable *			mDrag;
		Vector2i				mUICenter;
		TextureAtlasEditor *	mEditor;

		virtual void OnSizeChange();

		void GetCenter();
};

}}}

#endif
