#ifndef EEFM_PREVIEWPANEL_HPP
#define EEFM_PREVIEWPANEL_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/uri.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <optional>

namespace EE::UI {
class UICodeEditor;
class UILoader;
class UITextView;
namespace Tools {
class UIAudioPlayer;
class UIImageViewer;
} // namespace Tools
} // namespace EE::UI

namespace eefm {

class App;

/** Selection preview to the right of the files. Widgets are created on first use; reads are
 * cancellable and completions always return through App's UI executor. */
class PreviewPanel {
  public:
	explicit PreviewPanel( App& app, EE::UI::UISplitter* splitter );

	~PreviewPanel();

	void show( bool visible );

	bool isVisible() const;

	/** Follows the selection in the active tab; unchanged revisions do not reload. */
	void syncSelection();

	/** Cancels pending work and stops playback before the application shuts down. */
	void cancel();

	void rememberSize();

	const std::string& partition() const { return mPartition; }

	void setPartition( std::string partition ) { mPartition = std::move( partition ); }

  private:
	void create();

	void clear();

	void load();

	void showMessage( const EE::String& text );

	App& mApp;
	EE::UI::UISplitter* mSplitter;
	EE::UI::UIWidget* mRoot{ nullptr };
	EE::UI::UIWidget* mContent{ nullptr };
	EE::UI::UITextView* mName{ nullptr };
	EE::UI::UITextView* mMessage{ nullptr };
	EE::UI::UILoader* mLoader{ nullptr };
	EE::UI::UICodeEditor* mEditor{ nullptr };
	EE::UI::Tools::UIImageViewer* mImage{ nullptr };
	EE::UI::Tools::UIAudioPlayer* mAudio{ nullptr };
	std::optional<EE::URI> mURI;
	EE::VFS::FileRevision mRevision;
	std::optional<EE::CancellationSource> mCancellation;
	std::string mPartition{ "72%" };
	EE::Uint64 mGeneration{ 0 };
	EE::Uint64 mSize{ 0 };
};

} // namespace eefm

#endif
