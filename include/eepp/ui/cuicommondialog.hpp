#ifndef EE_UICUICOMMONDIALOG_HPP
#define EE_UICUICOMMONDIALOG_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/ui/cuipushbutton.hpp>
#include <eepp/ui/cuilistbox.hpp>
#include <eepp/ui/cuitextinput.hpp>
#include <eepp/ui/cuicombobox.hpp>
#include <eepp/ui/cuiwindow.hpp>

namespace EE { namespace UI {

class EE_API cUICommonDialog : public cUIWindow {
	public:
		class CreateParams : public cUIWindow::CreateParams {
			public:
				inline CreateParams() :
					cUIWindow::CreateParams(),
					DefaultDirectory( GetProcessPath() ),
					DefaultFilePattern( "*" ),
					CDLFlags( UI_CDL_DEFAULT_FLAGS )
				{
				}

				inline ~CreateParams() {}

				std::string DefaultDirectory;
				std::string	DefaultFilePattern;
				Uint32		CDLFlags;
		};

		cUICommonDialog( const cUICommonDialog::CreateParams& Params );

		virtual ~cUICommonDialog();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void		SetTheme( cUITheme * Theme );

		void				RefreshFolder();

		virtual Uint32		OnMessage( const cUIMessage *Msg );

		virtual void		Open();

		virtual void		Save();

		std::string			GetCurPath() const;

		std::string			GetCurFile() const;

		std::string			GetFullPath();

		cUIPushButton *		GetButtonOpen() const;

		cUIPushButton *		GetButtonCancel() const;

		cUIPushButton *		GetButtonUp() const;

		cUIListBox *		GetList() const;

		cUITextInput *		GetPathInput() const;

		cUITextInput *		GetFileInput() const;

		cUIDropDownList *	GetFiletypeList() const;

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
		cUIPushButton *		mButtonOpen;
		cUIPushButton *		mButtonCancel;
		cUIPushButton *		mButtonUp;
		cUIListBox *		mList;
		cUITextInput *		mPath;
		cUITextInput *		mFile;
		cUIDropDownList *	mFiletype;
		Uint32				mCDLFlags;

		void OnPressEnter( const cUIEvent * Event );

		void OnPressFileEnter( const cUIEvent * Event );

		void OpenSaveClick();

		std::string			GetTempFullPath();
};

}}

#endif
