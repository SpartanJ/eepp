#ifndef EEFM_PANEMANAGER_HPP
#define EEFM_PANEMANAGER_HPP

#include "pane.hpp"

#include <array>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace eefm {

class App;

/** @brief The two panes of the split view: their tabs, navigation, status and filter bars. */
class PaneManager {
  public:
	static constexpr int Count = 2;

	explicit PaneManager( App& app );

	~PaneManager();

	/** Builds pane index (its tab widget, status and filter bars) inside parent, and wires its
	 * location bar (already loaded in the toolbar). */
	void createPane( int index, UIWidget* parent );

	Pane& pane( int index ) { return *mPanes[index]; }

	Pane& activePane() { return pane( activeIndex() ); }

	/** @return The pane owning keyboard focus, or the last focused pane while other UI owns it. */
	int activeIndex() const;

	/** @return True once both panes exist. */
	bool ready() const { return mPanes[0] && mPanes[1]; }

	const std::array<std::unique_ptr<Pane>, Count>& all() const { return mPanes; }

	FileTab* currentTab( Pane& pane );

	FileTab* currentTab();

	FileTab* findTab( UITab* tab );

	/** @return The pane owning a node in its view or location bar, or nullptr for other UI. */
	Pane* paneForNode( const Node* node ) const;

	FileTab* openTab( Pane& pane, const URI& uri, bool select = true );

	void closeTab( FileTab* tab );

	/** Closes every tab (their models stop listing and watching). */
	void clearTabs();

	/** Gives a pane's files keyboard focus unless one of its controls already owns it. */
	void focusPane( int index );

	/** Updates pane decorations and remembers the pane owning the current keyboard focus. */
	void onFocusChange();

	void navigate( FileTab* tab, URI uri, bool recordHistory = true );

	void goBack();

	void goForward();

	void goUp();

	void goHome();

	/** Location bar, status, toolbar buttons and title for the current tab of pane. */
	void updateChrome( Pane& pane );

	void updateStatus( Pane& pane );

	void applyFilter( FileTab* tab );

	/** Opens the command line locations (or the saved tabs, or the home folder in each pane). */
	void restoreTabs( const nlohmann::json& state, const std::vector<std::string>& locations,
					  bool forceSplit );

	void loadState( const nlohmann::json& state );

	/** Writes the tabs, the filter options and the view state of the current tab. */
	void saveState( nlohmann::json& state );

  private:
	void updateTabTitle( FileTab* tab );

	void onTabStateChanged( FileTab* tab );

	void positionNewTabButton( Pane& pane );

	void showLocationHistory( Pane& pane );

	void applyViewState( FileTab* tab );

	void rememberViewState( FileTab* tab );

	App& mApp;
	UISceneNode* mUI;
	std::array<std::unique_ptr<Pane>, Count> mPanes;
	/** A tab being dragged to the other pane: out of its pane, not yet in the other one. */
	std::unique_ptr<FileTab> mMovingTab;
	/** Column widths / visibility / sort shared by new tabs. */
	nlohmann::json mViewState;
	/** Command context while a toolbar, popup, dialog, places or terminal owns keyboard focus.
	 * Only updated from actual pane focus; activeIndex() always prefers the current focus. */
	int mLastFocusedPane{ 0 };
	EE::VFS::VFSDirectoryModel::NameFilterMode mFilterMode{
		EE::VFS::VFSDirectoryModel::NameFilterMode::Glob };
	bool mFilterCaseSensitive{ false };
};

} // namespace eefm

#endif
