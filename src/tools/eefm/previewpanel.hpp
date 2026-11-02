#ifndef EEFM_PREVIEWPANEL_HPP
#define EEFM_PREVIEWPANEL_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/uri.hpp>
#include <eepp/system/applications.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <optional>

namespace EE::UI {

class UICodeEditor;
class UILoader;
class UIPushButton;
class UITextView;
class UIWindow;
namespace Tools {

class UIAudioPlayer;
class UIImageViewer;

} // namespace Tools

} // namespace EE::UI

namespace eefm {

class App;

/** Selection preview in a sidebar or a floating Quick Look window. Widgets are created on first
 * use; reads are cancellable and completions always return through App's UI executor. */
class PreviewPanel {
  public:
	/** A null splitter creates a floating preview instead of the sidebar. */
	explicit PreviewPanel( App& app, EE::UI::UISplitter* splitter );

	~PreviewPanel();

	void show( bool visible );

	bool isVisible() const;

	/** Follows the selection in the active tab; unchanged revisions do not reload. */
	void syncSelection();

	/** Cancels pending work and stops playback before the application shuts down. */
	void cancel();

	void rememberSize();

	/** Moves the file selection while the floating preview owns keyboard focus. */
	void moveSelection( int direction );

	const std::string& partition() const { return mPartition; }

	void setPartition( std::string partition ) { mPartition = std::move( partition ); }

  private:
	void create();

	void clear();

	void load();

	void loadApplication( std::string mime );

	void showMessage( const EE::String& text );

	App& mApp;
	EE::UI::UISplitter* mSplitter;
	EE::UI::UIWindow* mWindow{ nullptr };
	EE::UI::UIWidget* mFallback{ nullptr };
	EE::UI::UIWidget* mRoot{ nullptr };
	EE::UI::UIWidget* mContent{ nullptr };
	EE::UI::UIPushButton* mOpen{ nullptr };
	EE::UI::UITextView* mName{ nullptr };
	EE::UI::UITextView* mMessage{ nullptr };
	EE::UI::UILoader* mLoader{ nullptr };
	EE::UI::UICodeEditor* mEditor{ nullptr };
	EE::UI::Tools::UIImageViewer* mImage{ nullptr };
	EE::UI::Tools::UIAudioPlayer* mAudio{ nullptr };
	std::optional<EE::System::Application> mDefaultApplication;
	std::optional<EE::URI> mURI;
	EE::VFS::FileRevision mRevision;
	std::optional<EE::CancellationSource> mCancellation;
	EE::String mDetails;
	std::string mPartition{ "72%" };
	EE::Uint64 mGeneration{ 0 };
	EE::Uint64 mSize{ 0 };
};

} // namespace eefm

#endif
