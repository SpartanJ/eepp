#ifndef EE_UITOOLSCTEXTUREATLASEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace UI { namespace Tools {

class TextureAtlasTextureRegionEditor;

class EE_API TextureAtlasEditor {
	public:
		typedef cb::Callback0<void> TGEditorCloseCb;

		static TextureAtlasEditor * New( UIWindow * AttatchTo = NULL, const TGEditorCloseCb& callback = TGEditorCloseCb() );

		TextureAtlasEditor( UIWindow * AttatchTo = NULL, const TGEditorCloseCb& callback = TGEditorCloseCb() );

		virtual ~TextureAtlasEditor();

		UISpinBox *			getSpinOffX() const { return mSpinOffX; }

		UISpinBox *			getSpinOffY() const { return mSpinOffY; }
	protected:
		class UITGEUpdater : public UINode
		{
			public:
				UITGEUpdater( TextureAtlasEditor * TGEditor ) : UINode(), mTGEditor( TGEditor ) {}

				virtual void update( const Time& time ) { mTGEditor->update(); }
			protected:
				TextureAtlasEditor * mTGEditor;
		};
		friend class UITGEUpdater;

		UIWindow *			mUIWindow;
		UINode *			mUIContainer;
		UITheme *			mTheme;
		TGEditorCloseCb		mCloseCb;
		TexturePacker *		mTexturePacker;
		TextureAtlasLoader *mTextureAtlasLoader;
		TextureRegion *		mCurTextureRegion;
		UISpinBox *			mSpinOffX;
		UISpinBox *			mSpinOffY;
		UISpinBox *			mSpinDestW;
		UISpinBox *			mSpinDestH;
		UIListBox *			mTextureRegionList;
		UIWinMenu *			mWinMenu;
		UIDropDownList *	mTextureFilterList;
		TextureAtlasTextureRegionEditor * mTextureRegionEditor;
		UITGEUpdater *		mTGEU;

		void windowClose( const UIEvent * Event );

		void fileMenuClick( const UIEvent * Event );

		void onTextureAtlasCreate( TexturePacker * TexPacker );

		void openTextureAtlas( const UIEvent * Event );

		void saveTextureAtlas( const UIEvent * Event );

		void onTextureAtlasClose( const UIEvent * Event );

		void onTextureRegionChange( const UIEvent * Event );

		void updateControls();

		void fillTextureRegionList();

		void onOffXChange( const UIEvent * Event );

		void onOffYChange( const UIEvent * Event );

		void onDestWChange( const UIEvent * Event );

		void onDestHChange( const UIEvent * Event );

		void onResetDestSize( const UIEvent * Event );

		void onResetOffset( const UIEvent * Event );

		void onCenterOffset( const UIEvent * Event );

		void onHBOffset( const UIEvent * Event );

		void onTextureFilterChange( const UIEvent * Event );

		void onTextureAtlasLoaded( TextureAtlasLoader * TGLoader );

		void update();

		UIWidget * createTextureAtlasTextureRegionEditor( std::string name );
};

}}}

#endif
