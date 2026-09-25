#ifndef EPROC_HPP
#define EPROC_HPP

#include "appconfig.hpp"
#include "gui_window_tracker.hpp"
#include "process_model.hpp"
#include <array>
#include <atomic>
#include <eepp/ee.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/charts/uichart.hpp>
#include <eepp/ui/models/sortingproxymodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <memory>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::UI::Charts;

namespace eproc {

class App {
  public:
	App();
	~App();

	int run( int argc, char* argv[] );

  private:
	bool init();
	void setupUI();
	void setupProcessTable();

	/** Restores the saved display, position, size, and maximized state. */
	void restoreWindowState();

	/** Captures and writes the current window state. */
	void saveWindowState();

	/** Restores the process table's columns, widths, and sorting state. */
	void restoreProcessTableState();

	/** Serializes the process table's columns, widths, and sorting state. */
	std::string serializeProcessTableState() const;

	/** Saves the state before the primary window is closed. */
	bool closeWindow( EE::Window::Window* window );

	/** Creates the collector and worker, and dispatches the first sample before window creation. */
	void startCollection();

	/** Starts the poll/dispatch timer. Requires the UI to exist. */
	void setupRefreshTimer();

	/** Blocks up to @p timeoutMs until the first snapshot is staged. Called before the render
	 *  loop, where blocking cannot stall a frame. */
	bool waitForFirstSnapshot( Uint32 timeoutMs );

	/** Collects the current snapshot and publishes it to the UI thread through the staging
	 *  buffer. Runs on a worker thread; never touches the model or any Node. */
	void collectAsync();

	/** UI-thread tick: publishes any staged snapshot, then schedules the next collection. */
	void onRefreshTick();

	/** Moves a staged snapshot into the model, preserving the selected process by PID. UI thread
	 *  only. */
	void publishStagedSnapshot();

	/** Re-selects surviving processes after a snapshot, when row indexes may have changed. */
	void restoreSelection( const std::vector<Int64>& pids );

	UIAbstractTableView* activeProcessView() const;

	void captureTreeExpansion();

	void restoreTreeExpansion();

	void updateStatusBar();
	void setupPerformance();
	void updatePerformance( const SystemInfo& sysInfo, const std::vector<ProcessInfo>& processes );
	void selectPerformanceMetric( size_t metric );
	void setPerCoreView( bool perCore );
	void rebuildCoreCharts( size_t count );
	String formatPerformanceTime( double value ) const;
	void onEndProcess();
	void onSearchChanged();
	void onFilterChanged();
	void onSelectionChange();

	/** Builds and shows the process context menu for a right-clicked row. */
	void showProcessContextMenu( const ModelIndex& proxyIndex );

	/** PIDs of the currently selected rows. */
	std::vector<Int64> selectedPids() const;

	/** Maps a proxy index to the process behind it, or nullptr. */
	const ProcessInfo* processForIndex( const ModelIndex& index ) const;

	/** Sends @p signal to every pid in @p pids, asking for confirmation first when @p confirm. */
	void requestSignal( std::vector<Int64> pids, int signal, const std::string& actionLabel,
						bool confirm );

	/** Selects the row holding @p pid and scrolls it into view. */
	void selectProcess( Int64 pid );

	std::unique_ptr<AppConfig> mConfig;
	std::optional<Float> mPixelDensity;
	std::unique_ptr<UIApplication> mApp;
	UIWidget* mRoot{ nullptr };
	UITabWidget* mTabWidget{ nullptr };

	UIWidget* mProcessTableLayout{ nullptr };
	UIPushButton* mEndProcessBtn{ nullptr };
	UITextInput* mSearchInput{ nullptr };
	UIDropDownList* mFilterDropdown{ nullptr };
	UITableView* mTableView{ nullptr };
	UITreeView* mTreeView{ nullptr };
	UITextView* mStatusText{ nullptr };
	UITextView* mCpuText{ nullptr };
	UITextView* mMemText{ nullptr };
	UITextView* mSwapText{ nullptr };
	UIChart* mPerformanceDetailChart{ nullptr };
	UIWidget* mCoreChartScroll{ nullptr };
	UIWidget* mCoreChartStack{ nullptr };
	UITextView* mPerformanceTitle{ nullptr };
	UITextView* mPerformanceSummary{ nullptr };
	UITextView* mPerformanceDetail{ nullptr };
	UISelectButton* mOverallButton{ nullptr };
	UISelectButton* mPerCoreButton{ nullptr };
	UIWidget* mCpuModeControls{ nullptr };
	struct PerformanceMetric {
		UIWidget* card{ nullptr };
		UIChart* preview{ nullptr };
		UITextView* value{ nullptr };
		std::shared_ptr<RingXYDataSource> primary;
		std::shared_ptr<RingXYDataSource> secondary;
	};
	std::array<PerformanceMetric, 4> mPerformanceMetrics;
	std::array<String, 4> mPerformanceSummaries;
	std::array<String, 4> mPerformanceDetails;
	std::vector<UIChart*> mCoreCharts;
	std::vector<UITextView*> mCoreLabels;
	std::vector<std::shared_ptr<RingXYDataSource>> mCoreHistory;
	Clock mPerformanceClock;
	size_t mSelectedPerformanceMetric{ 0 };
	bool mPerCoreView{ false };

	std::shared_ptr<ProcessModel> mProcessModel;
	std::shared_ptr<ProcessTreeModel> mTreeModel;
	std::shared_ptr<SortingProxyModel> mSortProxy;
	std::vector<Int64> mExpandedTreePids;
	bool mTreeMode{ false };
	bool mTreeExpansionInitialized{ false };
	bool mTreeSearchActive{ false };
	bool mProcessTableStateRestored{ false };
	bool mProcessTableStateRestoreScheduled{ false };
	bool mCollectFamilyMemoryAfterRestore{ false };
	bool mWindowStateSaved{ false };

	// Window ownership backs the Programs Only filter. Refreshed on the UI thread, since Xlib is
	// not safe to drive from the collection worker.
	GuiWindowTracker mGuiWindows;

	// Collection runs on a worker thread so the render loop never blocks on /proc. The worker
	// owns mCollector exclusively; the UI thread only ever reads staged snapshots.
	std::unique_ptr<ProcessCollector> mCollector;
	std::shared_ptr<ThreadPool> mThreadPool;
	std::atomic<bool> mCollectInFlight{ false };
	Mutex mStagingMutex;
	std::vector<ProcessInfo> mStagedProcesses;
	SystemInfo mStagedSystemInfo;
	bool mStagedReady{ false };

	Uint32 mUpdateIntervalMs{ 1000 };

	// The publish poll runs much faster than the sampling cadence so a finished snapshot reaches
	// the table immediately instead of waiting for the next sample tick.
	Uint32 mTickIntervalMs{ 100 };
	Clock mDispatchClock;
};

} // namespace eproc

#endif // EPROC_HPP
