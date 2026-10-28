#ifndef EE_UI_TOOLS_UIKEYBINDINGSEDITOR_HPP
#define EE_UI_TOOLS_UIKEYBINDINGSEDITOR_HPP

#include <eepp/ui/tools/keybindingsconfig.hpp>
#include <eepp/ui/tools/keybindingsconfigdata.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <functional>
#include <span>

namespace EE::UI::Tools {

/** Standalone editor. The save callback must persist the complete proposal and update runtime
 * bindings before returning an empty error string. A nonempty result leaves the editor unchanged.
 * Children and outstanding confirmation dialogs are owned by the editor. Labels are translated
 * through the scene's translator. */
class EE_API UIKeybindingsEditor : public UILinearLayout {
  public:
	using Save = std::function<String( const KeybindingsConfigData& )>;

	/** A keybindings file managed by NewForFile(). Context names must outlive the widget. */
	struct FileSource {
		std::string path;
		std::span<const KeybindingsConfig::Context> contexts;
		/** Adds the application's commands; the catalog reads the current bindings itself. */
		std::function<void( KeybindingsConfig::Catalog& )> catalog;
		/** Installs the saved file at runtime. Returns an error to show, empty on success. */
		std::function<String()> apply;
		/** Optional check before writing, such as conflicts with shortcuts the file doesn't own. */
		std::function<String( const KeybindingsConfigData& )> validate = {};
		/** Optional manual-edit action, also offered when the file can't be edited here. */
		std::function<void()> manualEdit = {};
		/** Optional text shown below the editor. */
		String note = {};
	};

	static UIKeybindingsEditor* New( UIWidget* parent, KeybindingsConfigData configuration,
									 Save save );

	/** Edit a keybindings file. The file is strictly validated first; when it can't be edited,
	 * an error view with the manual-edit action is returned instead of an editor. Saves refuse
	 * to overwrite a file changed since it was read. */
	static UIWidget* NewForFile( UIWidget* parent, FileSource source );

	virtual ~UIKeybindingsEditor();

	const KeybindingsConfigData& getConfiguration() const;

	/** Confirm and restore every command's default shortcuts. Modifier preferences are retained. */
	void restoreDefaults();

	/** Show the manual-edit action when a callback is supplied. Empty hides and disables it.
	 * The consumer owns opening the file/editor and any surrounding window behavior. */
	void setManualEditCallback( std::function<void()> callback );

	/** Text shown below the editor. Empty hides it. */
	void setNote( const String& note );

  protected:
	struct Impl;
	std::unique_ptr<Impl> mImpl;

	UIKeybindingsEditor( UIWidget* parent, KeybindingsConfigData configuration, Save save );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onKeyUp( const KeyEvent& event );

	virtual Uint32 onFocusLoss();

	void refresh();

	void clearDetails();

	void edit( size_t command, size_t slot );

	void stopRecording();

	void closeDialog( bool immediately = false );

	void propose( KeybindingsConfigData configuration, bool confirmRestore = false );

	void commit( KeybindingsConfigData configuration );
};

} // namespace EE::UI::Tools

#endif
