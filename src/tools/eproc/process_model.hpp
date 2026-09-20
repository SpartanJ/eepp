#ifndef EPROC_PROCESS_MODEL_HPP
#define EPROC_PROCESS_MODEL_HPP

#include "process_collector.hpp"
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/ui/models/model.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace EE::UI {
class UISceneNode;
}

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace eproc {

/** Presents a snapshot of the process list to the views.
 *
 *  Collection happens off the UI thread (see App::collectAsync), so this model never reads
 *  /proc itself: it only owns the last snapshot handed to it by applySnapshot(). All methods are
 *  UI-thread only. */
class ProcessModel : public Model {
  public:
	enum Columns : size_t {
		ColIcon = 0,
		ColName,
		ColPid,
		ColUsername,
		ColCpu,
		ColMemory,
		ColSharedMem,
		ColGpuUsage,
		ColGpuMemory,
		ColDownload,
		ColUpload,
		ColTotalMemory,
		ColVirtualSize,
		ColCpuTime,
		ColNiceness,
		ColRelativeStartTime,
		ColTty,
		ColIoRead,
		ColIoWrite,
		ColCommand,
		ColCount
	};

	/** Mirrors ksysguard6's ProcessFilter::State, minus the two tree variants (this model is
	 *  flat). The numeric order matches the original enum so the dropdown maps directly. */
	enum FilterMode {
		AllProcesses = 0,
		SystemProcesses,
		UserProcesses,
		OwnProcesses,
		ProgramsOnly,
		FilterModeCount
	};

	static std::shared_ptr<ProcessModel> create( UISceneNode* ui ) {
		return std::shared_ptr<ProcessModel>( new ProcessModel( ui ) );
	}

	~ProcessModel();

	size_t rowCount( const ModelIndex& = ModelIndex() ) const override;
	size_t columnCount( const ModelIndex& = ModelIndex() ) const override;
	std::string columnName( const size_t& column ) const override;
	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override;
	ModelIndex index( int row, int column = 0,
					  const ModelIndex& parent = ModelIndex() ) const override;

	/** Replaces the snapshot and notifies the views. UI thread only. */
	void applySnapshot( std::vector<ProcessInfo>&& processes, const SystemInfo& sysInfo );

	void setFilter( FilterMode mode );

	/** Quick search: a case-insensitive regular expression matched against the name, command,
	 *  username and PID columns (the original also lets the user search by PID). */
	void setTextFilter( const std::string& text );

	/** PIDs owning a top-level window, used by the Programs Only filter. Set this before
	 *  applySnapshot so the filter sees current data. */
	void setGuiWindowPids( UnorderedSet<long>&& pids );

	FilterMode getFilter() const { return mFilterMode; }
	const SystemInfo& getSystemInfo() const { return mSystemInfo; }

	/** Number of rows currently visible after filtering. */
	size_t visibleCount() const { return mFilteredProcesses.size(); }

	const ProcessInfo* getProcessByRow( int row ) const;

	/** Row of the currently visible list holding @p pid, or -1 when it is filtered out or gone.
	 *  Used to re-select a process by identity across a snapshot, since rows reorder. */
	int rowForPid( long pid ) const;

	/** The icon column carries no data of its own, so sorting it is meaningless. */
	bool isColumnSortable( const size_t& columnIndex ) const override {
		return columnIndex != ColIcon;
	}

	/** Enables per-column CSS classes for process table cells. */
	bool classModelRoleEnabled() override { return true; }

  private:
	explicit ProcessModel( UISceneNode* ui );

	void applyFilters();

	/** Applies the active filter mode to a single process, mirroring the original's predicates. */
	bool accepts( const ProcessInfo& proc ) const;

	bool matchesText( const ProcessInfo& proc ) const;

	/** Loads (once) and caches the drawable for an icon file. */
	DrawablePtr iconFor( const std::string& path ) const;

	std::vector<ProcessInfo> mProcesses;
	std::vector<ProcessInfo*> mFilteredProcesses;
	UISceneNode* mUI{ nullptr };
	SystemInfo mSystemInfo;
	FilterMode mFilterMode{ AllProcesses };
	// Compiled once per filter change, never per row. Null means "no text filter".
	std::unique_ptr<RegEx> mTextRegex;
	// Set only when the typed text does not compile as a pattern (a group still open, a stray
	// quantifier): it is then searched literally, lowercased, so a half-typed pattern does not
	// blank the table. Empty when it is not in use.
	std::string mTextLiteral;
	UnorderedSet<long> mGuiPids;
	mutable UnorderedMap<std::string, DrawablePtr> mIconCache;
};

} // namespace eproc

#endif // EPROC_PROCESS_MODEL_HPP
