#ifndef EEFM_PLACESPANEL_HPP
#define EEFM_PLACESPANEL_HPP

#include "places.hpp"
#include <eepp/ui/uilinearlayout.hpp>
#include <functional>
#include <optional>

namespace eefm {

using namespace EE;
using namespace EE::UI;

/** @brief Dolphin-like Places sidebar: sections of places, remote hosts and devices.
 *
 * Rows are activated with the left button (middle button: in a new tab), offer a context menu and
 * accept files dragged from file views. Saved places can be dragged to reorder them within their
 * section, and folders dropped between places are added there, like Dolphin. Hidden places and
 * sections are drawn dimmed when listed.
 */
class PlacesPanel : public UILinearLayout {
  public:
	struct Entry {
		Place place;
		/** Index in Places::places(), or -1 for devices. */
		int index{ -1 };
		/** Listed only because hidden places are being shown: drawn dimmed. */
		bool sectionHidden{ false };
	};

	static PlacesPanel* New();

	/** Rebuilds the rows. */
	void setEntries( std::vector<Entry> entries );

	const std::vector<Entry>& entries() const { return mEntries; }

	/** Usage bar of a device (fraction of the capacity used). */
	void setUsage( const URI& uri, float usedFraction );

	/** Highlights the entry that best matches uri (the longest containing place). */
	void setCurrent( const URI& uri );

	std::function<void( const Entry&, bool newTab )> onActivate;

	std::function<void( const Entry& )> onContextMenu;

	std::function<void( const Entry& )> onFilesDropped;

	/** Right click on a section header. */
	std::function<void( PlaceSection )> onSectionMenu;

	/** A place was dragged to a new position: before the place at index before, or to the end of
	 * its section (nullopt). Indexes are Places::places() indexes. */
	std::function<void( size_t index, std::optional<size_t> before )> onPlaceMoved;

	/** Dragged folders were dropped between places: add them there (see onPlaceMoved). */
	std::function<void( std::optional<size_t> before )> onFoldersAdded;

	/** Follows the drag in progress, showing where it would insert, until it ends. */
	void trackDrag();

	/** True while a drag points between places (an insertion, not a drop into a place). */
	bool isInserting() const { return mSlot.has_value(); }

	/** Called by the rows: a place drag became active / ended. */
	void beginPlaceDrag( UIWidget* row );

	void endPlaceDrag( UIWidget* row );

  protected:
	PlacesPanel();

	Uint32 onMessage( const NodeMessage* msg ) override;

	void scheduledUpdate( const Time& time ) override;

  private:
	/** Insertion point: before a place (Places index) or at the end of section. */
	struct Slot {
		PlaceSection section;
		std::optional<size_t> before;
		/** Row that draws the indicator, on its top or bottom edge. */
		size_t row;
		bool after;
	};

	std::optional<size_t> entryOf( Node* node ) const;

	/** Rows of saved places of a section, in display order. */
	std::vector<size_t> sectionRows( PlaceSection section ) const;

	std::optional<Slot> reorderSlot( const Vector2f& position ) const;

	std::optional<Slot> folderSlot( const Vector2f& position ) const;

	void setSlot( std::optional<Slot> slot );

	std::vector<Entry> mEntries;
	std::vector<UIWidget*> mRows;
	std::optional<Slot> mSlot;
	/** Row of the place being dragged. */
	std::optional<size_t> mDraggedRow;
	bool mTracking{ false };
	/** The click that ends a drag must not activate the place. */
	bool mSuppressClick{ false };
};

} // namespace eefm

#endif
