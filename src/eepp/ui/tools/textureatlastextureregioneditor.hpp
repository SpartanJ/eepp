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

		virtual void scheduledUpdate( const Time& time );

		Graphics::TextureRegion * getTextureRegion() const;

		void setTextureRegion( Graphics::TextureRegion * TextureRegion );

		UITextureRegion * getGfx() const;
	protected:
		UITheme *				mTheme;
		UITextureRegion *					mGfx;
		UINode *			mDrag;
		Vector2f				mUICenter;
		TextureAtlasEditor *	mEditor;

		virtual void onSizeChange();

		void getCenter();
};

}}}

#endif
