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

		void windowClose( const UIEvent * Event );

		void cancelClick( const UIEvent * Event );

		void okClick( const UIEvent * Event );

		UITextView * createTxtBox( Vector2i Pos, const String& Text );

		void onDialogFolderSelect( const UIEvent * Event );

		void onSelectFolder( const UIEvent * Event );

		void textureAtlasSave( const UIEvent * Event );
};

}}}

#endif
