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

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void		SetTheme( UITheme * Theme );

		void				RefreshFolder();

		virtual Uint32		OnMessage( const UIMessage *Msg );

		virtual void		Open();

		virtual void		Save();

		std::string			GetCurPath() const;

		std::string			GetCurFile() const;

		std::string			GetFullPath();

		UIPushButton *		GetButtonOpen() const;

		UIPushButton *		GetButtonCancel() const;

		UIPushButton *		GetButtonUp() const;

		UIListBox *		GetList() const;

		UITextInput *		GetPathInput() const;

		UITextInput *		GetFileInput() const;

		UIDropDownList *	GetFiletypeList() const;

		void				AddFilePattern( std::string pattern, bool select = false );

		bool				IsSaveDialog();

		bool				SortAlphabetically();

		bool				FoldersFirst();

		bool				AllowFolderSelect();

		void				SortAlphabetically( const bool& sortAlphabetically );

		void				FoldersFirst( const bool& foldersFirst );

		void				AllowFolderSelect( const bool& allowFolderSelect );
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

		void OnPressEnter( const UIEvent * Event );

		void OnPressFileEnter( const UIEvent * Event );

		void OpenSaveClick();

		std::string			GetTempFullPath();

		void DisableButtons();
};

}}

#endif
