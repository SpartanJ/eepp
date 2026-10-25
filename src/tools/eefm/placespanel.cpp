#include "placespanel.hpp"
#include "fileview.hpp"
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/nodemessage.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextview.hpp>

using namespace EE::Scene;

namespace eefm {

namespace {

/** A place row. Saved places can be dragged (to reorder them); file drags highlight it unless
 * they point between rows. */
class PlaceRow : public UILinearLayout {
  public:
	static PlaceRow* New() { return eeNew( PlaceRow, () ); }

	PlaceRow() : UILinearLayout( "placerow", UIOrientation::Vertical ) {
		// Dragging only marks the insertion point: the row itself stays in place.
		unsetFlags( UI_DRAG_VERTICAL | UI_DRAG_HORIZONTAL );
	}

	PlacesPanel* panel() const { return getParent()->asType<PlacesPanel>(); }

	bool acceptsDropOfWidget( const UIWidget* widget ) override {
		return widget->isType( FileTableView::CellType ) && !panel()->isInserting();
	}

	Uint32 onDragStart( const Vector2i& position, const Uint32& flags ) override {
		// eepp starts dragging on press: the place drag begins once the pointer moves.
		mPressPosition = position.asFloat();
		mDragActive = false;
		return UILinearLayout::onDragStart( position, flags );
	}

	Uint32 onDrag( const Vector2f& position, const Uint32&, const Sizef& ) override {
		if ( !mDragActive ) {
			if ( std::abs( position.x - mPressPosition.x ) < PixelDensity::dpToPx( 4 ) &&
				 std::abs( position.y - mPressPosition.y ) < PixelDensity::dpToPx( 4 ) )
				return 0;
			mDragActive = true;
			addClass( "dragged" );
			panel()->beginPlaceDrag( this );
		}
		// The row itself never moves.
		return 1;
	}

	Uint32 onDragStop( const Vector2i& position, const Uint32& flags ) override {
		if ( mDragActive ) {
			mDragActive = false;
			removeClass( "dragged" );
			panel()->endPlaceDrag( this );
		}
		return UILinearLayout::onDragStop( position, flags );
	}

	Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags ) override {
		if ( isDragging() && !mDragActive && !( flags & getDragButton() ) ) {
			cancelPendingDrag();
			return 1;
		}
		return UILinearLayout::onCalculateDrag( position, flags );
	}

	Uint32 onMouseUp( const Vector2i& position, const Uint32& flags ) override {
		if ( isDragging() && !mDragActive && ( flags & getDragButton() ) )
			cancelPendingDrag();
		return UILinearLayout::onMouseUp( position, flags );
	}

  private:
	/** Ends a press that never became a drag, without emitting a drop. */
	void cancelPendingDrag() {
		setDragging( false, false );
		if ( getEventDispatcher() && getEventDispatcher()->getNodeDragging() == this )
			getEventDispatcher()->setNodeDragging( nullptr );
	}

	Vector2f mPressPosition;
	bool mDragActive{ false };
};

const char* sectionTitle( PlaceSection section ) {
	switch ( section ) {
		case PlaceSection::Places:
			return "Places";
		case PlaceSection::Remote:
			return "Remote";
		case PlaceSection::Devices:
			return "Devices";
	}
	return "";
}

String placeLabel( UISceneNode* ui, const Place& place ) {
	if ( place.automatic ) {
		struct Label {
			const char* id;
			const char* key;
			const char* text;
		};
		static constexpr Label labels[] = { { "home", "eefm_home", "Home" },
											{ "desktop", "eefm_desktop", "Desktop" },
											{ "documents", "eefm_documents", "Documents" },
											{ "downloads", "eefm_downloads", "Downloads" },
											{ "music", "eefm_music", "Music" },
											{ "pictures", "eefm_pictures", "Pictures" },
											{ "videos", "eefm_videos", "Videos" },
											{ "trash", "eefm_trash", "Trash" } };
		for ( const auto& label : labels ) {
			if ( place.id == label.id && place.name == label.text )
				return ui->i18n( label.key, label.text );
		}
		if ( place.id.empty() && place.icon == "device-root" && place.name == "Root" )
			return ui->i18n( "eefm_root", "Root" );
	}
	return String::fromUtf8( place.name );
}

bool isSameOrChild( const URI& uri, const URI& place ) {
	if ( uri.getScheme() != place.getScheme() || uri.getAuthority() != place.getAuthority() )
		return false;
	std::string path = uri.getPath(), root = place.getPath();
	if ( root.empty() || root == "/" )
		return true;
	if ( root.back() == '/' )
		root.pop_back();
	if ( path.size() > 1 && path.back() == '/' )
		path.pop_back();
	return path == root ||
		   ( path.size() > root.size() && path.compare( 0, root.size(), root ) == 0 &&
			 path[root.size()] == '/' );
}

} // namespace

PlacesPanel* PlacesPanel::New() {
	return eeNew( PlacesPanel, () );
}

PlacesPanel::PlacesPanel() : UILinearLayout( "placespanel", UIOrientation::Vertical ) {
	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
}

void PlacesPanel::setEntries( std::vector<Entry> entries ) {
	mEntries = std::move( entries );
	mRows.clear();
	mIcons.clear();
	mSlot.reset();
	mDraggedRow.reset();
	// Deferred close: this may run from the click handler of one of the rows.
	Node* child = getFirstChild();
	while ( child ) {
		Node* next = child->getNextNode();
		child->setVisible( false );
		child->close();
		child = next;
	}
	std::optional<PlaceSection> section;
	for ( size_t i = 0; i < mEntries.size(); ++i ) {
		const Place& place = mEntries[i].place;
		if ( !section || *section != place.section ) {
			section = place.section;
			UITextView* header = UITextView::New();
			header->setParent( this );
			header->addClass( "places_header" );
			header->setText( getUISceneNode()->i18n(
				String::toLower( std::string( "eefm_places_" ) + sectionTitle( place.section ) ),
				sectionTitle( place.section ) ) );
			header->setLayoutWidthPolicy( SizePolicy::MatchParent );
			if ( mEntries[i].sectionHidden )
				header->addClass( "hidden_place" );
			header->on( Event::MouseClick, [this, sectionId = place.section]( const Event* event ) {
				if ( ( static_cast<const MouseEvent*>( event )->getFlags() & EE_BUTTON_RMASK ) &&
					 onSectionMenu )
					onSectionMenu( sectionId );
			} );
		}
		PlaceRow* row = PlaceRow::New();
		row->setParent( this );
		row->addClass( "place" );
		if ( place.hidden || mEntries[i].sectionHidden )
			row->addClass( "hidden_place" );
		row->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		row->setTooltipText( place.uri.getScheme() == "file" ? place.uri.getFSPath()
															 : place.uri.toString() );
		auto* line = UILinearLayout::NewHorizontal();
		line->setParent( row );
		line->addClass( "place_line" );
		line->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
		UIImage* icon = UIImage::New();
		icon->setParent( line );
		icon->addClass( "place_icon" );
		UIIcon* placeIcon = iconOf( place );
		if ( placeIcon )
			icon->setDrawable( placeIcon->getSource( PixelDensity::dpToPxI( 16 ) ) );
		mIcons.emplace_back( icon, placeIcon );
		icon->setLayoutGravity( UI_VALIGN_CENTER );
		UITextView* label = UITextView::New();
		label->setParent( line );
		label->addClass( "place_name" );
		label->setText( placeLabel( getUISceneNode(), place ) );
		label->setLayoutGravity( UI_VALIGN_CENTER );
		std::vector<UIWidget*> parts{ row, line, icon, label };
		if ( place.section == PlaceSection::Devices ) {
			UIProgressBar* usage = UIProgressBar::New();
			usage->setParent( row );
			usage->addClass( "device_usage" );
			usage->setLayoutWidthPolicy( SizePolicy::MatchParent );
			usage->setDisplayPercent( false );
			usage->setTotalSteps( 100 );
			usage->setProgress( 0 );
			usage->setVisible( false );
			parts.push_back( usage );
		}
		// The row handles the mouse (clicks and drags): its parts are only drawn.
		for ( UIWidget* part : parts ) {
			if ( part != row )
				part->setEnabled( false );
		}
		row->setDragEnabled( mEntries[i].index >= 0 );
		// A drag that ends over a row may still produce its click; the next press re-arms.
		row->on( Event::MouseDown, [this]( const Event* ) { mSuppressClick = false; } );
		row->on( Event::MouseClick, [this, i]( const Event* event ) {
			if ( mSuppressClick ) {
				mSuppressClick = false;
				return;
			}
			if ( i >= mEntries.size() )
				return;
			Uint32 flags = static_cast<const MouseEvent*>( event )->getFlags();
			if ( ( flags & EE_BUTTON_LMASK ) && onActivate )
				onActivate( mEntries[i], false );
			else if ( ( flags & EE_BUTTON_MMASK ) && onActivate )
				onActivate( mEntries[i], true );
			else if ( ( flags & EE_BUTTON_RMASK ) && onContextMenu )
				onContextMenu( mEntries[i] );
		} );
		row->setData( i );
		mRows.push_back( row );
	}
}

UIIcon* PlacesPanel::iconOf( const Place& place ) const {
	return iconProvider ? iconProvider( place ) : getUISceneNode()->findIcon( place.icon );
}

void PlacesPanel::refreshIcons() {
	for ( auto& [image, icon] : mIcons ) {
		if ( !icon )
			continue;
		const DrawablePtr& source = icon->getSource( PixelDensity::dpToPxI( 16 ) );
		if ( source && image->getDrawable() != source )
			image->setDrawable( source );
	}
}

void PlacesPanel::setUsage( const URI& uri, float usedFraction ) {
	for ( size_t i = 0; i < mEntries.size() && i < mRows.size(); ++i ) {
		if ( mEntries[i].place.uri != uri )
			continue;
		auto* usage = mRows[i]->findByClass<UIProgressBar>( "device_usage" );
		if ( !usage )
			continue;
		usage->setVisible( true );
		usage->setProgress( std::clamp( usedFraction, 0.f, 1.f ) * 100.f );
		if ( usedFraction >= 0.9f )
			usage->addClass( "full" );
		else
			usage->removeClass( "full" );
	}
}

void PlacesPanel::setCurrent( const URI& uri ) {
	std::optional<size_t> best;
	size_t bestLength = 0;
	for ( size_t i = 0; i < mEntries.size(); ++i ) {
		const URI& place = mEntries[i].place.uri;
		if ( !isSameOrChild( uri, place ) )
			continue;
		size_t length = place.getPath().size() + 1;
		// Exact matches always win; otherwise the deepest containing place (but not "/" or a
		// device root for every folder: those only match exactly).
		bool exact = isSameOrChild( place, uri );
		if ( !exact && mEntries[i].place.section == PlaceSection::Devices )
			continue;
		if ( exact )
			length += 1u << 20;
		if ( length > bestLength ) {
			bestLength = length;
			best = i;
		}
	}
	for ( size_t i = 0; i < mRows.size(); ++i ) {
		if ( best && *best == i )
			mRows[i]->addClass( "selected" );
		else
			mRows[i]->removeClass( "selected" );
	}
}

std::optional<size_t> PlacesPanel::entryOf( Node* node ) const {
	while ( node && node != this ) {
		for ( size_t i = 0; i < mRows.size(); ++i ) {
			if ( mRows[i] == node )
				return i;
		}
		node = node->getParent();
	}
	return std::nullopt;
}

Uint32 PlacesPanel::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::Drop ) {
		const auto* drop = static_cast<const NodeDropMessage*>( msg );
		if ( drop->getDroppedNode() && drop->getDroppedNode()->isType( FileTableView::CellType ) ) {
			if ( mSlot && onFoldersAdded ) {
				// Deferred: adding places rebuilds the rows while the drop is being delivered.
				auto before = mSlot->before;
				setSlot( std::nullopt );
				runOnMainThread( [this, before] { onFoldersAdded( before ); } );
			} else if ( auto entry = entryOf( drop->getSender() ); entry && onFilesDropped ) {
				onFilesDropped( mEntries[*entry] );
			}
			return 1;
		}
	}
	return UILinearLayout::onMessage( msg );
}

void PlacesPanel::trackDrag() {
	if ( mTracking )
		return;
	mTracking = true;
	subscribeScheduledUpdate();
}

void PlacesPanel::beginPlaceDrag( UIWidget* row ) {
	for ( size_t i = 0; i < mRows.size(); ++i ) {
		if ( mRows[i] == row )
			mDraggedRow = i;
	}
	trackDrag();
}

void PlacesPanel::endPlaceDrag( UIWidget* row ) {
	// The drop may land outside the panel: a place drag is resolved here, from the last slot.
	if ( mDraggedRow && mRows[*mDraggedRow] == row && mSlot && onPlaceMoved ) {
		size_t index = static_cast<size_t>( mEntries[*mDraggedRow].index );
		auto before = mSlot->before;
		// Deferred: moving a place rebuilds the rows, including the one being dragged.
		runOnMainThread( [this, index, before] { onPlaceMoved( index, before ); } );
	}
	mSuppressClick = true;
	mDraggedRow.reset();
	setSlot( std::nullopt );
}

void PlacesPanel::scheduledUpdate( const Time& ) {
	EventDispatcher* dispatcher = getEventDispatcher();
	Node* dragging = dispatcher ? dispatcher->getNodeDragging() : nullptr;
	if ( !dragging ) {
		setSlot( std::nullopt );
		mDraggedRow.reset();
		mTracking = false;
		unsubscribeScheduledUpdate();
		return;
	}
	Vector2f position = dispatcher->getMousePosf();
	if ( mDraggedRow )
		setSlot( reorderSlot( position ) );
	else if ( dragging->isType( FileTableView::CellType ) )
		setSlot( folderSlot( position ) );
	else
		setSlot( std::nullopt );
}

std::vector<size_t> PlacesPanel::sectionRows( PlaceSection section ) const {
	std::vector<size_t> rows;
	for ( size_t i = 0; i < mEntries.size() && i < mRows.size(); ++i ) {
		if ( mEntries[i].index >= 0 && mEntries[i].place.section == section )
			rows.push_back( i );
	}
	return rows;
}

std::optional<PlacesPanel::Slot> PlacesPanel::reorderSlot( const Vector2f& position ) const {
	// Anywhere over the sidebar: the nearest position within the dragged place's section.
	Node* area = getParent() ? getParent() : const_cast<PlacesPanel*>( this );
	if ( !mDraggedRow || !area->getScreenRect().contains( position ) )
		return std::nullopt;
	auto rows = sectionRows( mEntries[*mDraggedRow].place.section );
	if ( rows.empty() )
		return std::nullopt;
	PlaceSection section = mEntries[*mDraggedRow].place.section;
	for ( size_t row : rows ) {
		Rectf rect = mRows[row]->getScreenRect();
		if ( position.y < rect.Top + rect.getHeight() * 0.5f )
			return Slot{ section, static_cast<size_t>( mEntries[row].index ), row, false };
	}
	return Slot{ section, std::nullopt, rows.back(), true };
}

std::optional<PlacesPanel::Slot> PlacesPanel::folderSlot( const Vector2f& position ) const {
	const FileDrag& drag = FileTableView::currentDrag();
	if ( !drag.folders || drag.uris.empty() )
		return std::nullopt;
	PlaceSection section = Places::sectionFor( drag.uris.front() );
	auto rows = sectionRows( section );
	for ( size_t k = 0; k < rows.size(); ++k ) {
		Rectf rect = mRows[rows[k]]->getScreenRect();
		if ( !rect.contains( position ) )
			continue;
		// The outer quarters insert; the middle drops into the place.
		Float edge = rect.getHeight() * 0.25f;
		if ( position.y < rect.Top + edge )
			return Slot{ section, static_cast<size_t>( mEntries[rows[k]].index ), rows[k], false };
		if ( position.y > rect.Bottom - edge ) {
			if ( k + 1 < rows.size() ) {
				return Slot{ section, static_cast<size_t>( mEntries[rows[k + 1]].index ),
							 rows[k + 1], false };
			}
			return Slot{ section, std::nullopt, rows[k], true };
		}
		return std::nullopt;
	}
	return std::nullopt;
}

void PlacesPanel::setSlot( std::optional<Slot> slot ) {
	if ( mSlot && slot && mSlot->row == slot->row && mSlot->after == slot->after &&
		 mSlot->before == slot->before )
		return;
	if ( mSlot && mSlot->row < mRows.size() ) {
		mRows[mSlot->row]->removeClass( "drop_before" );
		mRows[mSlot->row]->removeClass( "drop_after" );
	}
	mSlot = slot;
	if ( mSlot && mSlot->row < mRows.size() )
		mRows[mSlot->row]->addClass( mSlot->after ? "drop_after" : "drop_before" );
}

} // namespace eefm
