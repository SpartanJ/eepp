#ifndef EEFM_FILEVIEW_HPP
#define EEFM_FILEVIEW_HPP

#include <eepp/core/uri.hpp>
#include <eepp/ui/uitableview.hpp>
#include <functional>
#include <optional>
#include <vector>

namespace eefm {

using namespace EE;
using namespace EE::UI;

class FileTableView;

/** Files being dragged inside eefm (between views, tabs and places). */
struct FileDrag {
	std::vector<URI> uris;
	FileTableView* source{ nullptr };
	/** Every dragged item is a folder (they can be added as places). */
	bool folders{ false };
	/** A system drag started with it (other applications can take the files). */
	bool native{ false };
};

/** @brief Details view of one directory: draggable rows and a drop target. */
class FileTableView : public UITableView {
  public:
	/** Widget type of the draggable cells, checked by drop targets. */
	static constexpr Uint32 CellType = UI_TYPE_USER + 0x0EF1;

	static FileTableView* New();

	/** The drag in progress (or the last one; it is replaced when the next drag starts). */
	static FileDrag& currentDrag();

	/** Returns the URIs to drag when a drag starts (the selection). */
	std::function<std::vector<URI>()> dragPayload;

	/** Files were dropped on this view, on row (when over a row) or on the background. */
	std::function<void( std::optional<size_t> row )> onFilesDropped;

	/** @return True when row is a folder (drops onto it go inside). */
	std::function<bool( size_t row )> isFolderRow;

	/** A file drag started from this view (after the pointer moved). */
	std::function<void()> onDragStarted;

	/** A row was clicked with the middle button. */
	std::function<void( size_t row )> onRowMiddleClick;

	/** Context menu requested over the empty area. */
	std::function<void()> onBackgroundMenu;

	/** Called immediately before drawing the visible row range (end is exclusive). */
	std::function<void( size_t first, size_t end )> onVisibleRows;

	void drawChildren() override;

	/** Drops thumbnail drawables held by recycled cells. Ordinary file icons are unaffected. */
	void releaseThumbnailIcons( size_t firstPooledRow = 0 );

	/** Starts renaming row in place (an editor over its name; the model renames on Enter). */
	void beginRename( size_t row );

	/** @return The row of a widget inside this view (cells or rows), if any. */
	std::optional<size_t> rowOf( Node* node ) const;

	/** Reset type-to-select for a new location and retain input until its listing is ready. */
	void beginNavigation();

	void endNavigation();

  protected:
	FileTableView();

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index ) override;

	Uint32 onMessage( const NodeMessage* msg ) override;

	Uint32 onMouseClick( const Vector2i& position, const Uint32& flags ) override;

	Uint32 onKeyDown( const KeyEvent& event ) override;

	Uint32 onTextInput( const TextInputEvent& event ) override;

	void onWidgetFocusLoss() override;

  private:
	String mNavigationInput;
	bool mLoading{ false };
	bool mReplayingNavigation{ false };
};

} // namespace eefm

#endif
