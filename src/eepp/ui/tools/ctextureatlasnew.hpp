#ifndef EE_UITOOLSCTEXTUREATLASNEW_HPP
#define EE_UITOOLSCTEXTUREATLASNEW_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuicombobox.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuipushbutton.hpp>
#include <eepp/graphics/texturepacker.hpp>

namespace EE { namespace UI { namespace Tools {

class EE_API TextureAtlasNew {
	public:
		typedef cb::Callback1<void, TexturePacker *> TGCreateCb;

		TextureAtlasNew( TGCreateCb NewTGCb = TGCreateCb() );

		virtual ~TextureAtlasNew();
	protected:
		cUITheme *			mTheme;
		cUIWindow *			mUIWindow;
		TGCreateCb			mNewTGCb;
		cUIComboBox *		mComboWidth;
		cUIComboBox *		mComboHeight;
		cUISpinBox *		mPixelSpace;
		cUITextInput *		mTGPath;
		cUIPushButton *		mSetPathButton;
		cUIDropDownList *	mSaveFileType;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );

		cUITextBox * CreateTxtBox( Vector2i Pos, const String& Text );

		void OnDialogFolderSelect( const cUIEvent * Event );

		void OnSelectFolder( const cUIEvent * Event );

		void TextureAtlasSave( const cUIEvent * Event );
};

}}}

#endif
