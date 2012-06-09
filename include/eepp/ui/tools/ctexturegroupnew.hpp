#ifndef EE_UITOOLSCTEXTUREGROUPNEW_HPP
#define EE_UITOOLSCTEXTUREGROUPNEW_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuicombobox.hpp>
#include <eepp/ui/cuispinbox.hpp>
#include <eepp/ui/cuipushbutton.hpp>
#include <eepp/graphics/ctexturepacker.hpp>

namespace EE { namespace UI { namespace Tools {

class EE_API cTextureGroupNew {
	public:
		typedef cb::Callback1<void, cTexturePacker *> TGCreateCb;

		cTextureGroupNew( TGCreateCb NewTGCb = TGCreateCb() );

		virtual ~cTextureGroupNew();
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

		cUITextBox * CreateTxtBox( eeVector2i Pos, const String& Text );

		void OnDialogFolderSelect( const cUIEvent * Event );

		void OnSelectFolder( const cUIEvent * Event );

		void TextureGroupSave( const cUIEvent * Event );
};

}}}

#endif
