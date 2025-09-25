#ifndef EE_UI_UIFILEDIALOG_HPP
#define EE_UI_UIFILEDIALOG_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uimultimodelview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {

struct NativeFileDialogHandler;

class EE_API UIFileDialog : public UIWindow {
  public:
	enum Flags {
		SaveDialog = ( 1 << 0 ),
		FoldersFirst = ( 1 << 1 ),
		SortAlphabetically = ( 1 << 2 ),
		AllowFolderSelect = ( 1 << 3 ),
		ShowOnlyFolders = ( 1 << 4 ),
		ShowHidden = ( 1 << 5 ),
		AllowMultiFileSelection = ( 1 << 6 ),
		UseNativeFileDialog =
			( 1 << 7 ), //< Uses the OS native file dialog if exists and it's available
	};

	static const Uint32 DefaultFlags = UIFileDialog::Flags::FoldersFirst |
									   UIFileDialog::Flags::SortAlphabetically |
									   UIFileDialog::Flags::ShowHidden;

	static UIFileDialog* New( Uint32 dialogFlags = UIFileDialog::DefaultFlags,
							  const std::string& defaultFilePattern = "*",
							  const std::string& defaultDirectory = Sys::getProcessPath() );

	virtual ~UIFileDialog();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	void refreshFolder( bool resetScroll = false );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	virtual void open();

	virtual void save();

	std::string getCurPath() const;

	std::string getCurFile( size_t index = 0 ) const;

	std::string getFullPath() const;

	std::vector<std::string> getFullPaths() const;

	UIPushButton* getButtonOpen() const;

	UIPushButton* getButtonCancel() const;

	UIPushButton* getButtonUp() const;

	UIMultiModelView* getMultiView() const;

	UITextInput* getPathInput() const;

	UITextInput* getFileInput() const;

	UIDropDownList* getFileTypeList() const;

	void addFilePattern( std::string pattern, bool select = false );

	bool isSaveDialog() const;

	bool getSortAlphabetically() const;

	bool getFoldersFirst() const;

	bool getShowOnlyFolders() const;

	bool getShowHidden() const;

	bool allowFolderSelect() const;

	bool allowMultiFileSelect() const;

	void setSortAlphabetically( const bool& sortAlphabetically );

	void setFoldersFirst( const bool& foldersFirst );

	void setAllowFolderSelect( const bool& allowFolderSelect );

	void setShowOnlyFolders( const bool& showOnlyFolders );

	void setShowHidden( const bool& showHidden );

	void setAllowsMultiFileSelect( bool allow );

	const KeyBindings::Shortcut& getCloseShortcut() const;

	void setFileName( const std::string& name );

	void setCloseShortcut( const KeyBindings::Shortcut& closeWithKey );

	void setViewMode( const UIMultiModelView::ViewMode& viewMode );

	const UIMultiModelView::ViewMode& getViewMode() const;

	const KeyBindings::Shortcut& openShortut() const;

	void setOpenShortut( const KeyBindings::Shortcut& newOpenShortut );

	void setSingleClickNavigation( bool singleClickNavigation );

	virtual bool show();

	virtual bool hide();

	bool usingNativeFileDialog() const;

	virtual void scheduledUpdate( const Time& time );

  protected:
	std::string mCurPath;
	std::string mFilePatterns;
	UIPushButton* mButtonOpen{ nullptr };
	UIPushButton* mButtonCancel{ nullptr };
	UIPushButton* mButtonUp{ nullptr };
	UIPushButton* mButtonNewFolder{ nullptr };
	UISelectButton* mButtonListView{ nullptr };
	UISelectButton* mButtonTableView{ nullptr };
	UIMultiModelView* mMultiView{ nullptr };
	UITextInput* mPath{ nullptr };
	UITextInput* mFile{ nullptr };
	std::string mFileName;
	UIDropDownList* mFiletype{ nullptr };
	Uint32 mDialogFlags;
	KeyBindings::Shortcut mCloseShortcut;
	KeyBindings::Shortcut mOpenShortut{ KEY_RETURN, KeyMod::getDefaultModifier() };
	std::shared_ptr<FileSystemModel> mModel;
	std::shared_ptr<DiskDrivesModel> mDiskDrivesModel;
	bool mDisplayingDrives{ false };
	NativeFileDialogHandler* mHandler{ nullptr };
	std::vector<std::string> mRes;

	UIFileDialog( Uint32 dialogFlags = UIFileDialog::DefaultFlags,
				  const std::string& defaultFilePattern = "*",
				  const std::string& defaultDirectory = Sys::getProcessPath() );

	virtual void onWindowReady();

	virtual Uint32 onKeyDown( const KeyEvent& event );

	void onPressEnter( const Event* Event );

	void onPressFileEnter( const Event* Event );

	void openSaveClick();

	void disableButtons();

	void openFileOrFolder( bool shouldOpenFolder );

	void goFolderUp();

	void updateClickStep();

	void setCurPath( const std::string& path );

	std::vector<const FileSystemModel::Node*> getSelectionNodes() const;

	std::vector<ModelIndex> getSelectionModelIndex() const;

	std::string getSelectedDrive() const;

	std::string getFullPath( size_t index ) const;
};

}} // namespace EE::UI

#endif
