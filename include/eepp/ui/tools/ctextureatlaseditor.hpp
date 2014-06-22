#ifndef EE_UITOOLSCTEXTUREATLASEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuilistbox.hpp>
#include <eepp/ui/cuiwinmenu.hpp>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace UI { namespace Tools {

class TextureAtlasSubTextureEditor;

class EE_API TextureAtlasEditor {
	public:
		typedef cb::Callback0<void> TGEditorCloseCb;

		TextureAtlasEditor( cUIWindow * AttatchTo = NULL, const TGEditorCloseCb& callback = TGEditorCloseCb() );

		virtual ~TextureAtlasEditor();

		cUISpinBox *			SpinOffX() const { return mSpinOffX; }

		cUISpinBox *			SpinOffY() const { return mSpinOffY; }
	protected:
		class cUITGEUpdater : public cUIControl
		{
			public:
				cUITGEUpdater( const CreateParams& Params, TextureAtlasEditor * TGEditor ) : cUIControl( Params ), mTGEditor( TGEditor ) {}
				virtual void Update() { mTGEditor->Update(); }
			protected:
				TextureAtlasEditor * mTGEditor;
		};
		friend class cUITGEUpdater;

		cUIWindow *				mUIWindow;
		cUIControl *			mUIContainer;
		cUITheme *				mTheme;
		TGEditorCloseCb			mCloseCb;
		TexturePacker *		mTexturePacker;
		TextureAtlasLoader *	mTextureAtlasLoader;
		SubTexture *			mCurSubTexture;
		cUISpinBox *			mSpinOffX;
		cUISpinBox *			mSpinOffY;
		cUISpinBox *			mSpinDestW;
		cUISpinBox *			mSpinDestH;
		cUIListBox *			mSubTextureList;
		cUIWinMenu *			mWinMenu;
		TextureAtlasSubTextureEditor * mSubTextureEditor;
		cUITGEUpdater *			mTGEU;

		void WindowClose( const cUIEvent * Event );

		void CreateTGEditor();

		void CreateWinMenu();

		void FileMenuClick( const cUIEvent * Event );

		void OnTextureAtlasCreate( TexturePacker * TexPacker );

		void OpenTextureAtlas( const cUIEvent * Event );

		void SaveTextureAtlas( const cUIEvent * Event );

		void OnTextureAtlasClose( const cUIEvent * Event );

		void OnSubTextureChange( const cUIEvent * Event );

		cUITextBox * CreateTxtBox( Vector2i Pos, const String& Text );

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

		void OnTextureAtlasLoaded( TextureAtlasLoader * TGLoader );

		void Update();
};

}}}

#endif
