#ifndef EE_UICUICOMMONDIALOG_HPP
#define EE_UICUICOMMONDIALOG_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {

enum UICommonDialogFlags {
	CDL_FLAG_SAVE_DIALOG			= ( 1 << 0 ),
	CDL_FLAG_FOLDERS_FISRT			= ( 1 << 1 ),
	CDL_FLAG_SORT_ALPHABETICALLY	= ( 1 << 2 ),
	CDL_FLAG_ALLOW_FOLDER_SELECT	= ( 1 << 3 )
};

static const Uint32 UI_CDL_DEFAULT_FLAGS = CDL_FLAG_FOLDERS_FISRT | CDL_FLAG_SORT_ALPHABETICALLY;

class EE_API UICommonDialog : public UIWindow {
	public:
		static UICommonDialog * New( Uint32 CDLFlags = UI_CDL_DEFAULT_FLAGS, std::string DefaultFilePattern = "*", std::string DefaultDirectory = Sys::getProcessPath() );

		UICommonDialog( Uint32 CDLFlags = UI_CDL_DEFAULT_FLAGS, std::string DefaultFilePattern = "*", std::string DefaultDirectory = Sys::getProcessPath() );

		virtual ~UICommonDialog();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void		setTheme( UITheme * Theme );

		void				refreshFolder();

		virtual Uint32		onMessage( const NodeMessage *Msg );

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

		bool				getSortAlphabetically();

		bool				getFoldersFirst();

		bool				getAllowFolderSelect();

		void				setSortAlphabetically( const bool& sortAlphabetically );

		void				setFoldersFirst( const bool& foldersFirst );

		void				setAllowFolderSelect( const bool& allowFolderSelect );
	protected:
		std::string			mCurPath;
		UIPushButton *		mButtonOpen;
		UIPushButton *		mButtonCancel;
		UIPushButton *		mButtonUp;
		UIListBox *			mList;
		UITextInput *		mPath;
		UITextInput *		mFile;
		UIDropDownList *	mFiletype;
		Uint32				mCDLFlags;

		void onPressEnter( const Event * Event );

		void onPressFileEnter( const Event * Event );

		void openSaveClick();

		std::string			getTempFullPath();

		void disableButtons();
};

}}

#endif
