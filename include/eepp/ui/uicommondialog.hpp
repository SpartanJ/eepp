#ifndef EE_UICUICOMMONDIALOG_HPP
#define EE_UICUICOMMONDIALOG_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {

class EE_API UICommonDialog : public UIWindow {
	public:
		class CreateParams : public UIWindow::CreateParams {
			public:
				inline CreateParams() :
					UIWindow::CreateParams(),
					DefaultDirectory( Sys::getProcessPath() ),
					DefaultFilePattern( "*" ),
					CDLFlags( UI_CDL_DEFAULT_FLAGS )
				{
				}

				inline ~CreateParams() {}

				std::string DefaultDirectory;
				std::string	DefaultFilePattern;
				Uint32		CDLFlags;
		};

		UICommonDialog( const UICommonDialog::CreateParams& Params );

		virtual ~UICommonDialog();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void		setTheme( UITheme * Theme );

		void				refreshFolder();

		virtual Uint32		onMessage( const UIMessage *Msg );

		virtual void		open();

		virtual void		save();

		std::string			getCurPath() const;

		std::string			getCurFile() const;

		std::string			getFullPath();

		UIPushButton *		getButtonOpen() const;

		UIPushButton *		getButtonCancel() const;

		UIPushButton *		getButtonUp() const;

		UIListBox *			getList() const;

		UITextInput *		getPathInput() const;

		UITextInput *		getFileInput() const;

		UIDropDownList *	getFiletypeList() const;

		void				addFilePattern( std::string pattern, bool select = false );

		bool				isSaveDialog();

		bool				sortAlphabetically();

		bool				foldersFirst();

		bool				allowFolderSelect();

		void				sortAlphabetically( const bool& sortAlphabetically );

		void				foldersFirst( const bool& foldersFirst );

		void				allowFolderSelect( const bool& allowFolderSelect );
	protected:
		std::string			mCurPath;
		UIPushButton *		mButtonOpen;
		UIPushButton *		mButtonCancel;
		UIPushButton *		mButtonUp;
		UIListBox *		mList;
		UITextInput *		mPath;
		UITextInput *		mFile;
		UIDropDownList *	mFiletype;
		Uint32				mCDLFlags;

		void onPressEnter( const UIEvent * Event );

		void onPressFileEnter( const UIEvent * Event );

		void openSaveClick();

		std::string			getTempFullPath();

		void disableButtons();
};

}}

#endif
