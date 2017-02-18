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

		UISpinBox *			getSpinOffX() const { return mSpinOffX; }

		UISpinBox *			getSpinOffY() const { return mSpinOffY; }
	protected:
		class UITGEUpdater : public UIControl
		{
			public:
				UITGEUpdater( const CreateParams& Params, TextureAtlasEditor * TGEditor ) : UIControl( Params ), mTGEditor( TGEditor ) {}
				virtual void update() { mTGEditor->update(); }
			protected:
				TextureAtlasEditor * mTGEditor;
		};
		friend class UITGEUpdater;

		UIWindow *			mUIWindow;
		UIControl *			mUIContainer;
		UITheme *			mTheme;
		TGEditorCloseCb		mCloseCb;
		TexturePacker *		mTexturePacker;
		TextureAtlasLoader *mTextureAtlasLoader;
		SubTexture *		mCurSubTexture;
		UISpinBox *			mSpinOffX;
		UISpinBox *			mSpinOffY;
		UISpinBox *			mSpinDestW;
		UISpinBox *			mSpinDestH;
		UIListBox *			mSubTextureList;
		UIWinMenu *			mWinMenu;
		TextureAtlasSubTextureEditor * mSubTextureEditor;
		UITGEUpdater *		mTGEU;

		void windowClose( const UIEvent * Event );

		void createTGEditor();

		void createWinMenu();

		void fileMenuClick( const UIEvent * Event );

		void onTextureAtlasCreate( TexturePacker * TexPacker );

		void openTextureAtlas( const UIEvent * Event );

		void saveTextureAtlas( const UIEvent * Event );

		void onTextureAtlasClose( const UIEvent * Event );

		void onSubTextureChange( const UIEvent * Event );

		UITextBox * createTextBox( Vector2i Pos, const String& Text );

		void updateControls();

		void fillSubTextureList();

		void onOffXChange( const UIEvent * Event );

		void onOffYChange( const UIEvent * Event );

		void onDestWChange( const UIEvent * Event );

		void onDestHChange( const UIEvent * Event );

		void onResetDestSize( const UIEvent * Event );

		void onResetOffset( const UIEvent * Event );

		void onCenterOffset( const UIEvent * Event );

		void onHBOffset( const UIEvent * Event );

		void onTextureAtlasLoaded( TextureAtlasLoader * TGLoader );

		void update();
};

}}}

#endif
