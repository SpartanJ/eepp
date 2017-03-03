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
		TextureAtlasSubTextureEditor( TextureAtlasEditor * Editor );

		virtual ~TextureAtlasSubTextureEditor();

		virtual void draw();

		virtual void update();

		Graphics::SubTexture * getSubTexture() const;

		void setSubTexture( Graphics::SubTexture * subTexture );

		UIGfx * getGfx() const;
	protected:
		UITheme *				mTheme;
		UIGfx *					mGfx;
		UIDragable *			mDrag;
		Vector2i				mUICenter;
		TextureAtlasEditor *	mEditor;

		virtual void onSizeChange();

		void getCenter();
};

}}}

#endif
