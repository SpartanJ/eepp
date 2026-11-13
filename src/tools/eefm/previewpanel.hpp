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
class UIIcon;
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

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::VFS;

class App;

/** Selection preview in a sidebar or a floating Quick Look window. Widgets are created on first
 * use; reads are cancellable and completions always return through App's UI executor. */
class PreviewPanel {
  public:
	/** A null splitter creates a floating preview instead of the sidebar. */
	explicit PreviewPanel( App& app, UISplitter* splitter );

	~PreviewPanel();

	void show( bool visible );

	bool isVisible() const;

	/** Follows the selection in the active tab; unchanged revisions do not reload. */
	void syncSelection();

	/** Cancels pending work and stops playback before the application shuts down. */
	void cancel();

	/** Rebinds the selected file icon after the system theme loads a sharper source. */
	void refreshIcon();

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

	/** Measures the selected folder, updating its size line as the worker reports progress. */
	void measureFolder();

	void hideLoader();

	void showMessage( const String& text );

	void updateMessage();

	App& mApp;
	UISplitter* mSplitter;
	/** Borrowed from App's icon themes; cleared when the selection is discarded. */
	UIIcon* mFileIcon{ nullptr };
	UIWindow* mWindow{ nullptr };
	UIWidget* mFallback{ nullptr };
	UIWidget* mRoot{ nullptr };
	UIWidget* mContent{ nullptr };
	UIPushButton* mOpen{ nullptr };
	UITextView* mName{ nullptr };
	UITextView* mMessage{ nullptr };
	UILoader* mLoader{ nullptr };
	UICodeEditor* mEditor{ nullptr };
	UIImageViewer* mImage{ nullptr };
	UIAudioPlayer* mAudio{ nullptr };
	std::optional<Application> mDefaultApplication;
	std::optional<URI> mURI;
	FileRevision mRevision;
	std::optional<CancellationSource> mCancellation;
	/** The fallback text, from top to bottom: mType, mSizeText, mDetails, mNotice. */
	String mType;
	String mSizeText;
	String mDetails;
	String mNotice;
	std::string mPartition{ "72%" };
	Uint64 mGeneration{ 0 };
	Uint64 mSize{ 0 };
};

} // namespace eefm

#endif
