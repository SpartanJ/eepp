#ifndef EE_UITOOLSCTEXTUREATLASTEXTUREREGIONEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASTEXTUREREGIONEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/uitextureregion.hpp>

namespace EE { namespace UI { namespace Tools {

class TextureAtlasEditor;

class EE_API TextureAtlasTextureRegionEditor : public UIWidget {
	public:
		static TextureAtlasTextureRegionEditor * New( TextureAtlasEditor * Editor );

		TextureAtlasTextureRegionEditor( TextureAtlasEditor * Editor );

		virtual ~TextureAtlasTextureRegionEditor();

		virtual void draw();

		Graphics::TextureRegion * getTextureRegion() const;

		void setTextureRegion( Graphics::TextureRegion * TextureRegion );

		UITextureRegion * getGfx() const;
	protected:
		UITextureRegion * mGfx;
		UINode * mDrag;
		Vector2f mUICenter;
		TextureAtlasEditor * mEditor;
		Vector2f mDragPos;

		virtual void onSizeChange();

		void getCenter();
};

}}}

#endif
