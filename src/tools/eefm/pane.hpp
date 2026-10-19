#ifndef EEFM_PANE_HPP
#define EEFM_PANE_HPP

#include "fileview.hpp"

#include <eepp/system/clock.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/vfs/ui/vfsdirectorymodel.hpp>
#include <memory>
#include <string>
#include <vector>

namespace eefm {

struct Pane;

/** One tab: a directory view with its own history and filter. */
struct FileTab {
	Pane* pane{ nullptr };
	UITab* tab{ nullptr };
	FileTableView* view{ nullptr };
	std::shared_ptr<EE::VFS::VFSDirectoryModel> model;
	std::vector<URI> history;
	size_t historyIndex{ 0 };
	/** Filter text of this tab (the filter bar shows the current tab's). */
	std::string filter;
	/** Entry to select once it is listed (the folder we came from when going up, a created or
	 * renamed item). */
	std::string selectWhenListed;
	/** Since when selectWhenListed waits: changes can arrive in steps (a rename is a removal,
	 * then an addition), so it is dropped only after a while without appearing. */
	EE::System::Clock selectWhenListedSince;

	void selectLater( std::string name ) {
		selectWhenListed = std::move( name );
		selectWhenListedSince.restart();
	}

	/** @return The selected rows, sorted. */
	std::vector<size_t> selectedRows() const;

	std::vector<URI> selectedURIs() const;
};

/** Location bar of a pane (lives in the toolbar, aligned with its pane). */
struct LocationBar {
	UIWidget* box{ nullptr };
	UITextInput* input{ nullptr };
};

/** One side of the split view: a tab widget with its status and filter bars. */
struct Pane {
	int index{ 0 };
	UIWidget* root{ nullptr };
	UITabWidget* tabs{ nullptr };
	UIPushButton* newTabButton{ nullptr };
	UITextView* status{ nullptr };
	UIWidget* transferBox{ nullptr };
	UIProgressBar* transferBar{ nullptr };
	UITextView* transferText{ nullptr };
	UIWidget* filterBar{ nullptr };
	UITextInput* filterInput{ nullptr };
	UIPushButton* filterLock{ nullptr };
	UIPushButton* filterCase{ nullptr };
	UIDropDownList* filterMode{ nullptr };
	LocationBar location;
	std::vector<std::unique_ptr<FileTab>> tabList;
	bool filterLocked{ false };
};

} // namespace eefm

#endif
