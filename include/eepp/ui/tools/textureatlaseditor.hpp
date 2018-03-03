#ifndef EE_UITOOLSCTEXTUREATLASEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uigridlayout.hpp>
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

		bool				isEdited() { return mEdited; }
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
		Node *				mUIContainer;
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
		UIGridLayout *		mTextureRegionGrid;
		UIWinMenu *			mWinMenu;
		UIDropDownList *	mTextureFilterList;
		TextureAtlasTextureRegionEditor * mTextureRegionEditor;
		UITGEUpdater *		mTGEU;
		bool mEdited;

		void windowClose( const Event * Event );

		void fileMenuClick( const Event * Event );

		void onTextureAtlasCreate( TexturePacker * TexPacker );

		void openTextureAtlas( const Event * Event );

		void saveTextureAtlas( const Event * Event );

		void onTextureAtlasClose( const Event * Event );

		void onTextureRegionChange( const Event * Event );

		void updateControls();

		void fillTextureRegionList();

		void onOffXChange( const Event * Event );

		void onOffYChange( const Event * Event );

		void onDestWChange( const Event * Event );

		void onDestHChange( const Event * Event );

		void onResetDestSize( const Event * Event );

		void onResetOffset( const Event * Event );

		void onCenterOffset( const Event * Event );

		void onHBOffset( const Event * Event );

		void onTextureFilterChange( const Event * Event );

		void onTextureAtlasLoaded( TextureAtlasLoader * TGLoader );

		void update();

		UIWidget * createTextureAtlasTextureRegionEditor( std::string name );
};

}}}

#endif
