#ifndef EE_UITOOLSCTEXTUREATLASNEW_HPP
#define EE_UITOOLSCTEXTUREATLASNEW_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/graphics/texturepacker.hpp>

namespace EE { namespace UI { namespace Tools {

class EE_API TextureAtlasNew {
	public:
		typedef cb::Callback1<void, TexturePacker *> TGCreateCb;

		TextureAtlasNew( TGCreateCb NewTGCb = TGCreateCb() );

		virtual ~TextureAtlasNew();
	protected:
		UITheme *			mTheme;
		UIWindow *			mUIWindow;
		TGCreateCb			mNewTGCb;
		UIComboBox *		mComboWidth;
		UIComboBox *		mComboHeight;
		UISpinBox *		mPixelSpace;
		UITextInput *		mTGPath;
		UIPushButton *		mSetPathButton;
		UIDropDownList *	mSaveFileType;

		void WindowClose( const UIEvent * Event );

		void CancelClick( const UIEvent * Event );

		void OKClick( const UIEvent * Event );

		UITextBox * CreateTxtBox( Vector2i Pos, const String& Text );

		void OnDialogFolderSelect( const UIEvent * Event );

		void OnSelectFolder( const UIEvent * Event );

		void TextureAtlasSave( const UIEvent * Event );
};

}}}

#endif
