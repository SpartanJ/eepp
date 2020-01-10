#ifndef EE_UITOOLSCTEXTUREATLASNEW_HPP
#define EE_UITOOLSCTEXTUREATLASNEW_HPP

#include <eepp/graphics/texturepacker.hpp>
#include <eepp/ui/base.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI { namespace Tools {

class EE_API TextureAtlasNew {
  public:
	typedef std::function<void( TexturePacker* )> TGCreateCb;

	TextureAtlasNew( TGCreateCb NewTGCb = TGCreateCb() );

	virtual ~TextureAtlasNew();

  protected:
	UIWindow* mUIWindow;
	TGCreateCb mNewTGCb;
	UIComboBox* mComboWidth;
	UIComboBox* mComboHeight;
	UISpinBox* mPixelSpace;
	UITextInput* mTGPath;
	UIPushButton* mSetPathButton;
	UIDropDownList* mSaveFileType;
	UIDropDownList* mPixelDensity;
	UIDropDownList* mTextureFilter;

	void windowClose( const Event* Event );

	void cancelClick( const Event* Event );

	void okClick( const Event* Event );

	void onDialogFolderSelect( const Event* Event );

	void onSelectFolder( const Event* Event );

	void textureAtlasSave( const Event* Event );
};

}}} // namespace EE::UI::Tools

#endif
