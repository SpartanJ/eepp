#ifndef EE_UI_UIFILEDIALOG_HPP
#define EE_UI_UIFILEDIALOG_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {

class EE_API UIFileDialog : public UIWindow {
  public:
	enum Flags {
		SaveDialog = ( 1 << 0 ),
		FoldersFirst = ( 1 << 1 ),
		SortAlphabetically = ( 1 << 2 ),
		AllowFolderSelect = ( 1 << 3 ),
		ShowOnlyFolders = ( 1 << 4 ),
		ShowHidden = ( 1 << 5 )
	};

	static const Uint32 DefaultFlags = FoldersFirst | SortAlphabetically | UIFileDialog::ShowHidden;

	static UIFileDialog* New( Uint32 dialogFlags = DefaultFlags,
							  const std::string& defaultFilePattern = "*",
							  const std::string& defaultDirectory = Sys::getProcessPath() );

	UIFileDialog( Uint32 dialogFlags = DefaultFlags, const std::string& defaultFilePattern = "*",
				  const std::string& defaultDirectory = Sys::getProcessPath() );

	virtual ~UIFileDialog();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	void refreshFolder();

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void open();

	virtual void save();

	std::string getCurPath() const;

	std::string getCurFile() const;

	std::string getFullPath();

	UIPushButton* getButtonOpen() const;

	UIPushButton* getButtonCancel() const;

	UIPushButton* getButtonUp() const;

	UIListView* getList() const;

	UITextInput* getPathInput() const;

	UITextInput* getFileInput() const;

	UIDropDownList* getFiletypeList() const;

	void addFilePattern( std::string pattern, bool select = false );

	bool isSaveDialog();

	bool getSortAlphabetically();

	bool getFoldersFirst();

	bool getShowOnlyFolders();

	bool getShowHidden();

	bool getAllowFolderSelect();

	void setSortAlphabetically( const bool& sortAlphabetically );

	void setFoldersFirst( const bool& foldersFirst );

	void setAllowFolderSelect( const bool& allowFolderSelect );

	void setShowOnlyFolders( const bool& showOnlyFolders );

	void setShowHidden( const bool& showHidden );

	const KeyBindings::Shortcut& getCloseShortcut() const;

	void setFileName( const std::string& name );

	void setCloseShortcut( const KeyBindings::Shortcut& closeWithKey );

  protected:
	std::string mCurPath;
	UIPushButton* mButtonOpen;
	UIPushButton* mButtonCancel;
	UIPushButton* mButtonUp;
	UIListView* mList;
	UITextInput* mPath;
	UITextInput* mFile;
	UIDropDownList* mFiletype;
	Uint32 mDialogFlags;
	KeyBindings::Shortcut mCloseShortcut;

	virtual void onWindowReady();

	virtual Uint32 onKeyUp( const KeyEvent& Event );

	void onPressEnter( const Event* Event );

	void onPressFileEnter( const Event* Event );

	void openSaveClick();

	void disableButtons();

	void openFileOrFolder( bool shouldOpenFolder );

	void goFolderUp();

	void updateClickStep();

	void setCurPath( const std::string& path );
};

}} // namespace EE::UI

#endif
