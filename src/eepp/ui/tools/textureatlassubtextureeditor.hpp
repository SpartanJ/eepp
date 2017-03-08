#ifndef EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASSUBTEXTUREEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/ui/uiimage.hpp>

namespace EE { namespace UI { namespace Tools {

class TextureAtlasEditor;

class EE_API TextureAtlasSubTextureEditor : public UIWidget {
	public:
		TextureAtlasSubTextureEditor( TextureAtlasEditor * Editor );

		virtual ~TextureAtlasSubTextureEditor();

		virtual void draw();

		virtual void update();

		Graphics::SubTexture * getSubTexture() const;

		void setSubTexture( Graphics::SubTexture * subTexture );

		UIImage * getGfx() const;
	protected:
		UITheme *				mTheme;
		UIImage *					mGfx;
		UIDragableControl *			mDrag;
		Vector2i				mUICenter;
		TextureAtlasEditor *	mEditor;

		virtual void onSizeChange();

		void getCenter();
};

}}}

#endif
