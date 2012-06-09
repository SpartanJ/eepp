#ifndef EE_UITOOLSCTEXTUREGROUPEDITOR_HPP
#define EE_UITOOLSCTEXTUREGROUPEDITOR_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/tools/ctexturegroupshapeeditor.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuilistbox.hpp>
#include <eepp/ui/cuiwinmenu.hpp>
#include <eepp/graphics/ctexturepacker.hpp>
#include <eepp/graphics/ctexturegrouploader.hpp>
#include <eepp/graphics/cshapegroupmanager.hpp>

namespace EE { namespace UI { namespace Tools {

class EE_API cTextureGroupEditor {
	public:
		typedef cb::Callback0<void> TGEditorCloseCb;

		cTextureGroupEditor( cUIWindow * AttatchTo = NULL, const TGEditorCloseCb& callback = TGEditorCloseCb() );

		virtual ~cTextureGroupEditor();

		cUISpinBox *			SpinOffX() const { return mSpinOffX; }

		cUISpinBox *			SpinOffY() const { return mSpinOffY; }
	protected:
		class cUITGEUpdater : public cUIControl
		{
			public:
				cUITGEUpdater( const CreateParams& Params, cTextureGroupEditor * TGEditor ) : cUIControl( Params ), mTGEditor( TGEditor ) {}
				virtual void Update() { mTGEditor->Update(); }
			protected:
				cTextureGroupEditor * mTGEditor;
		};
		friend class cUITGEUpdater;

		cUIWindow *				mUIWindow;
		cUIControl *			mUIContainer;
		cUITheme *				mTheme;
		TGEditorCloseCb			mCloseCb;
		cTexturePacker *		mTexturePacker;
		cTextureGroupLoader *	mTextureGroupLoader;
		cShape *				mCurShape;
		cUISpinBox *			mSpinOffX;
		cUISpinBox *			mSpinOffY;
		cUISpinBox *			mSpinDestW;
		cUISpinBox *			mSpinDestH;
		cUIListBox *			mShapeList;
		cUIWinMenu *			mWinMenu;
		cTextureGroupShapeEditor * mShapeEditor;
		cUITGEUpdater *			mTGEU;

		void WindowClose( const cUIEvent * Event );

		void CreateTGEditor();

		void CreateWinMenu();

		void FileMenuClick( const cUIEvent * Event );

		void OnTextureGroupCreate( cTexturePacker * TexPacker );

		void OpenTextureGroup( const cUIEvent * Event );

		void SaveTextureGroup( const cUIEvent * Event );

		void OnTextureGroupClose( const cUIEvent * Event );

		void OnShapeChange( const cUIEvent * Event );

		cUITextBox * CreateTxtBox( eeVector2i Pos, const String& Text );

		void UpdateControls();

		void FillShapeList();

		void OnOffXChange( const cUIEvent * Event );

		void OnOffYChange( const cUIEvent * Event );

		void OnDestWChange( const cUIEvent * Event );

		void OnDestHChange( const cUIEvent * Event );

		void OnResetDestSize( const cUIEvent * Event );

		void OnResetOffset( const cUIEvent * Event );

		void OnCenterOffset( const cUIEvent * Event );

		void OnHBOffset( const cUIEvent * Event );

		void OnTextureGroupLoaded( cTextureGroupLoader * TGLoader );

		void Update();
};

}}}

#endif
