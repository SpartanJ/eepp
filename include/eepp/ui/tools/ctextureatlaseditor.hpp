#ifndef EE_UITOOLSCTEXTUREATLASEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuilistbox.hpp>
#include <eepp/ui/cuiwinmenu.hpp>
#include <eepp/graphics/ctexturepacker.hpp>
#include <eepp/graphics/ctextureatlasloader.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>

namespace EE { namespace UI { namespace Tools {

class cTextureAtlasSubTextureEditor;

class EE_API cTextureAtlasEditor {
	public:
		typedef cb::Callback0<void> TGEditorCloseCb;

		cTextureAtlasEditor( cUIWindow * AttatchTo = NULL, const TGEditorCloseCb& callback = TGEditorCloseCb() );

		virtual ~cTextureAtlasEditor();

		cUISpinBox *			SpinOffX() const { return mSpinOffX; }

		cUISpinBox *			SpinOffY() const { return mSpinOffY; }
	protected:
		class cUITGEUpdater : public cUIControl
		{
			public:
				cUITGEUpdater( const CreateParams& Params, cTextureAtlasEditor * TGEditor ) : cUIControl( Params ), mTGEditor( TGEditor ) {}
				virtual void Update() { mTGEditor->Update(); }
			protected:
				cTextureAtlasEditor * mTGEditor;
		};
		friend class cUITGEUpdater;

		cUIWindow *				mUIWindow;
		cUIControl *			mUIContainer;
		cUITheme *				mTheme;
		TGEditorCloseCb			mCloseCb;
		cTexturePacker *		mTexturePacker;
		cTextureAtlasLoader *	mTextureAtlasLoader;
		cSubTexture *			mCurSubTexture;
		cUISpinBox *			mSpinOffX;
		cUISpinBox *			mSpinOffY;
		cUISpinBox *			mSpinDestW;
		cUISpinBox *			mSpinDestH;
		cUIListBox *			mSubTextureList;
		cUIWinMenu *			mWinMenu;
		cTextureAtlasSubTextureEditor * mSubTextureEditor;
		cUITGEUpdater *			mTGEU;

		void WindowClose( const cUIEvent * Event );

		void CreateTGEditor();

		void CreateWinMenu();

		void FileMenuClick( const cUIEvent * Event );

		void OnTextureAtlasCreate( cTexturePacker * TexPacker );

		void OpenTextureAtlas( const cUIEvent * Event );

		void SaveTextureAtlas( const cUIEvent * Event );

		void OnTextureAtlasClose( const cUIEvent * Event );

		void OnSubTextureChange( const cUIEvent * Event );

		cUITextBox * CreateTxtBox( eeVector2i Pos, const String& Text );

		void UpdateControls();

		void FillSubTextureList();

		void OnOffXChange( const cUIEvent * Event );

		void OnOffYChange( const cUIEvent * Event );

		void OnDestWChange( const cUIEvent * Event );

		void OnDestHChange( const cUIEvent * Event );

		void OnResetDestSize( const cUIEvent * Event );

		void OnResetOffset( const cUIEvent * Event );

		void OnCenterOffset( const cUIEvent * Event );

		void OnHBOffset( const cUIEvent * Event );

		void OnTextureAtlasLoaded( cTextureAtlasLoader * TGLoader );

		void Update();
};

}}}

#endif
