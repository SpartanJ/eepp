#ifndef EE_UITOOLSCTEXTUREATLASEDITOR_HPP
#define EE_UITOOLSCTEXTUREATLASEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/graphics/texturepacker.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>

namespace EE { namespace UI { namespace Tools {

class TextureAtlasSubTextureEditor;

class EE_API TextureAtlasEditor {
	public:
		typedef cb::Callback0<void> TGEditorCloseCb;

		TextureAtlasEditor( UIWindow * AttatchTo = NULL, const TGEditorCloseCb& callback = TGEditorCloseCb() );

		virtual ~TextureAtlasEditor();

		UISpinBox *			SpinOffX() const { return mSpinOffX; }

		UISpinBox *			SpinOffY() const { return mSpinOffY; }
	protected:
		class UITGEUpdater : public UIControl
		{
			public:
				UITGEUpdater( const CreateParams& Params, TextureAtlasEditor * TGEditor ) : UIControl( Params ), mTGEditor( TGEditor ) {}
				virtual void Update() { mTGEditor->Update(); }
			protected:
				TextureAtlasEditor * mTGEditor;
		};
		friend class UITGEUpdater;

		UIWindow *				mUIWindow;
		UIControl *			mUIContainer;
		UITheme *				mTheme;
		TGEditorCloseCb			mCloseCb;
		TexturePacker *		mTexturePacker;
		TextureAtlasLoader *	mTextureAtlasLoader;
		SubTexture *			mCurSubTexture;
		UISpinBox *			mSpinOffX;
		UISpinBox *			mSpinOffY;
		UISpinBox *			mSpinDestW;
		UISpinBox *			mSpinDestH;
		UIListBox *			mSubTextureList;
		UIWinMenu *			mWinMenu;
		TextureAtlasSubTextureEditor * mSubTextureEditor;
		UITGEUpdater *			mTGEU;

		void WindowClose( const UIEvent * Event );

		void CreateTGEditor();

		void CreateWinMenu();

		void FileMenuClick( const UIEvent * Event );

		void OnTextureAtlasCreate( TexturePacker * TexPacker );

		void OpenTextureAtlas( const UIEvent * Event );

		void SaveTextureAtlas( const UIEvent * Event );

		void OnTextureAtlasClose( const UIEvent * Event );

		void OnSubTextureChange( const UIEvent * Event );

		UITextBox * CreateTxtBox( Vector2i Pos, const String& Text );

		void UpdateControls();

		void FillSubTextureList();

		void OnOffXChange( const UIEvent * Event );

		void OnOffYChange( const UIEvent * Event );

		void OnDestWChange( const UIEvent * Event );

		void OnDestHChange( const UIEvent * Event );

		void OnResetDestSize( const UIEvent * Event );

		void OnResetOffset( const UIEvent * Event );

		void OnCenterOffset( const UIEvent * Event );

		void OnHBOffset( const UIEvent * Event );

		void OnTextureAtlasLoaded( TextureAtlasLoader * TGLoader );

		void Update();
};

}}}

#endif
